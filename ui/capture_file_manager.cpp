/*
 Copyright 2025 Google LLC

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#include "capture_file_manager.h"

#include <filesystem>
#include <memory>

#include <QObject>
#include <QThread>
#include <QReadLocker>
#include <QMessageBox>
#include <optional>

#include "debug_utils.h"
#include "dive_core/context.h"
#include "dive_core/data_core.h"
#include "file_path.h"
#include "trace_stats/trace_stats.h"

namespace
{

//--------------------------------------------------------------------------------------------------
LoadFileResult::Status ToLoadFileStatus(Dive::CaptureData::LoadResult result)
{
    switch (result)
    {
    case Dive::CaptureData::LoadResult::kSuccess:
        return LoadFileResult::Status::kSuccess;
    case Dive::CaptureData::LoadResult::kFileIoError:
        return LoadFileResult::Status::kFileIoError;
    case Dive::CaptureData::LoadResult::kCorruptData:
        return LoadFileResult::Status::kCorruptData;
    case Dive::CaptureData::LoadResult::kVersionError:
        return LoadFileResult::Status::kVersionError;
    }
    return LoadFileResult::Status::kUnknown;
}
}  // namespace

void CaptureFileManager::RegisterCustomMetaType()
{
    qRegisterMetaType<LoadFileResult>();
}

CaptureFileManager::CaptureFileManager(QObject *parent) :
    QObject(parent)
{
    QObject::connect(this,
                     &CaptureFileManager::LoadFileDone,
                     this,
                     &CaptureFileManager::OnLoadFileDone);
    QObject::connect(this,
                     &CaptureFileManager::GatherTraceStatsDone,
                     this,
                     &CaptureFileManager::OnGatherTraceStatsDone);
}

CaptureFileManager::~CaptureFileManager()
{
    if (m_thread == nullptr)
    {
        return;
    }
    m_thread->quit();
    m_thread->wait();
}

void CaptureFileManager::Start(Dive::DataCore &data_core)
{
    if (m_worker != nullptr)
    {
        return;
    }
    m_data_core = &data_core;
    m_capture_stats = std::make_unique<Dive::CaptureStats>();

    m_thread = new QThread(parent());
    m_worker = new QObject;
    m_worker->moveToThread(m_thread);
    QObject::connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    m_thread->start();
}

void CaptureFileManager::OnGatherTraceStatsDone()
{
    m_working = false;
    if (!m_pending_request)
    {
        TraceStatsUpdated();
    }
}

void CaptureFileManager::OnLoadFileDone(const LoadFileResult &loaded_file)
{
    m_working = false;
    if (m_pending_request)
    {
        // Discard current result since we have a new file to load.
        StartLoadFile();
        return;
    }

    m_loading_in_progress = false;
    emit FileLoadingFinished(loaded_file);
}

Dive::ComponentFilePaths CaptureFileManager::ResolveComponents(const Dive::FilePath &reference)
{

    if (Dive::IsGfxrFile(reference.value))
    {
        auto ret = Dive::GetComponentFilesHostPaths(reference.value.parent_path(),
                                                    reference.value.stem().string());
        if (ret.ok())
        {
            return *ret;
        }
        return Dive::ComponentFilePaths{};
    }

    if (Dive::IsRdFile(reference.value))
    {
        std::filesystem::path gfxr_file_path = reference.value;
        gfxr_file_path.replace_extension("gfxr");
        if (std::filesystem::exists(gfxr_file_path))
        {
            // Try load it as gfxr file:
            auto ret = Dive::GetComponentFilesHostPaths(reference.value.parent_path(),
                                                        reference.value.stem().string());
            if (ret.ok())
            {
                return *ret;
            }
        }
        // Single component pm4 rd file.
        return Dive::ComponentFilePaths{
            .gfxr = {},
            .gfxa = {},
            .perf_counter_csv = {},
            .gpu_timing_csv = {},
            .pm4_rd = reference.value,
            .screenshot_png = {},
            .renderdoc_rdc = {},
        };
    }

    return Dive::ComponentFilePaths{};
}

void CaptureFileManager::LoadFile(const Dive::FilePath           &reference,
                                  const Dive::ComponentFilePaths &components)
{
    m_loading_in_progress = true;
    m_pending_request = LoadFileRequest{ .reference = reference, .components = components };
    // Cancel anything that depend on current capture file.
    if (!m_capture_file_context.IsNull())
    {
        m_capture_file_context->Cancel();
    }
    m_capture_file_context = Dive::SimpleContext::Create();
    StartLoadFile();
}

void CaptureFileManager::StartLoadFile()
{
    if (m_working || !m_pending_request)
    {
        return;
    }
    auto request = m_pending_request.value();
    m_pending_request = std::nullopt;
    m_working = true;
    QMetaObject::invokeMethod(m_worker, [this, request = request]() {
        auto debug_timer = DebugScopedStopwatch([](double duration) {
            DIVE_DEBUG_LOG("Time used to load the capture is %f seconds.\n", duration);
        });
        auto result = LoadFileImpl(Dive::Context::Background(), request);
        emit LoadFileDone(result);
    });
}

LoadFileResult CaptureFileManager::LoadFileFailed(
LoadFileResult::Status                     status,
const CaptureFileManager::LoadFileRequest &request)
{
    return LoadFileResult{ .status = status,
                           .reference = request.reference,
                           .components = request.components };
}

LoadFileResult CaptureFileManager::LoadFileImpl(const Dive::Context   &context,
                                                const LoadFileRequest &request)
{
    const auto &components = request.components;

    QWriteLocker locker(&m_data_core_lock);
    // Note: this function might not run on UI thread, thus can't do any UI modification.

    auto found_gfxr_file = (!components.gfxr.empty() && std::filesystem::exists(components.gfxr));
    auto found_rd_file = (!components.pm4_rd.empty() && std::filesystem::exists(components.pm4_rd));

    if (found_gfxr_file)
    {
        // Check if the required asset file exists.
        bool asset_file_exists = (!components.gfxa.empty() &&
                                  std::filesystem::exists(components.gfxa));

        if (!asset_file_exists)
        {
            return LoadFileFailed(LoadFileResult::Status::kGfxaAssetMissing, request);
        }
    }

    LoadFileResult::FileType file_type = LoadFileResult::FileType::kUnknown;
    if (found_gfxr_file && found_rd_file)
    {
        file_type = LoadFileResult::FileType::kCorrelatedFiles;
    }
    else if (found_gfxr_file)
    {
        file_type = LoadFileResult::FileType::kGfxrFile;
    }
    else if (found_rd_file)
    {
        file_type = LoadFileResult::FileType::kRdFile;
    }
    else
    {
        file_type = LoadFileResult::FileType::kUnknown;
    }

    switch (file_type)
    {
    case LoadFileResult::FileType::kUnknown:
    {
        return LoadFileFailed(LoadFileResult::Status::kUnsupportedFile, request);
    }
    case LoadFileResult::FileType::kCorrelatedFiles:
    {
        if (Dive::CaptureData::LoadResult load_res = m_data_core->LoadDiveCaptureData(
            components.gfxr.string());
            load_res != Dive::CaptureData::LoadResult::kSuccess)
        {
            return LoadFileFailed(ToLoadFileStatus(load_res), request);
        }

        if (!m_data_core->ParseDiveCaptureData())
        {
            return LoadFileFailed(LoadFileResult::Status::kParseFailure, request);
        }
    }
    break;
    case LoadFileResult::FileType::kRdFile:
    {
        if (Dive::CaptureData::LoadResult load_res = m_data_core->LoadPm4CaptureData(
            components.pm4_rd.string());
            load_res != Dive::CaptureData::LoadResult::kSuccess)
        {
            return LoadFileFailed(ToLoadFileStatus(load_res), request);
        }

        if (!m_data_core->ParsePm4CaptureData())
        {
            return LoadFileFailed(LoadFileResult::Status::kParseFailure, request);
        }
    }
    break;
    case LoadFileResult::FileType::kGfxrFile:
    {
        if (Dive::CaptureData::LoadResult load_res = m_data_core->LoadGfxrCaptureData(
            components.gfxr.string());
            load_res != Dive::CaptureData::LoadResult::kSuccess)
        {
            return LoadFileFailed(ToLoadFileStatus(load_res), request);
        }

        if (!m_data_core->ParseGfxrCaptureData())
        {
            return LoadFileFailed(LoadFileResult::Status::kParseFailure, request);
        }
    }
    break;
    }
    return LoadFileResult{
        .status = LoadFileResult::Status::kSuccess,
        .file_type = file_type,
        .reference = request.reference,
        .components = request.components,
    };
}

void CaptureFileManager::GatherTraceStats()
{
    if (m_working)
    {
        // Ignore this request.
        return;
    }
    m_working = true;
    QMetaObject::invokeMethod(m_worker, [this, context = m_capture_file_context]() {
        GatherTraceStatsImpl(context);
        GatherTraceStatsDone();
    });
}

void CaptureFileManager::FillCaptureStatsResult(Dive::CaptureStats &out)
{
    if (m_working)
    {
        out = {};
        return;
    }
    out = *m_capture_stats;
}

void CaptureFileManager::GatherTraceStatsImpl(const Dive::Context &context)
{
    auto debug_timer = DebugScopedStopwatch([&context](double duration) {
        if (context.Cancelled())
        {
            DIVE_DEBUG_LOG("Trace stats cancelled after %f seconds.\n", duration);
        }
        else
        {
            DIVE_DEBUG_LOG("Time used to load the trace stats is %f seconds.\n", duration);
        }
    });

    QReadLocker locker(&m_data_core_lock);

    // Gather the trace stats and display in the overview tab
    Dive::TraceStats{}.GatherTraceStats(context,
                                        m_data_core->GetCaptureMetadata(),
                                        *m_capture_stats);
}
