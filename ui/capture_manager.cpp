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

#include "capture_manager.h"

#include <memory>

#include <QObject>
#include <QThread>
#include <QReadLocker>
#include <QMessageBox>
#include <optional>

#include "capture_service/constants.h"
#include "dive_core/context.h"
#include "dive_core/data_core.h"
#include "file_path.h"
#include "trace_stats/trace_stats.h"

namespace
{

//--------------------------------------------------------------------------------------------------
CaptureManager::LoadFailureReason ToLoadFailureReason(Dive::CaptureData::LoadResult result)
{
    switch (result)
    {
    case Dive::CaptureData::LoadResult::kSuccess:
        return CaptureManager::LoadFailureReason::kUnknown;
    case Dive::CaptureData::LoadResult::kFileIoError:
        return CaptureManager::LoadFailureReason::kFileIoError;
    case Dive::CaptureData::LoadResult::kCorruptData:
        return CaptureManager::LoadFailureReason::kCorruptData;
    case Dive::CaptureData::LoadResult::kVersionError:
        return CaptureManager::LoadFailureReason::kVersionError;
    }
    return CaptureManager::LoadFailureReason::kUnknown;
}

}  // namespace

void CaptureManager::RegisterCustomMetaType()
{
    qRegisterMetaType<LoadedFileType>();
    qRegisterMetaType<LoadFailureReason>();
}

CaptureManager::CaptureManager(QObject *parent) :
    QObject(parent)
{
    QObject::connect(this, &CaptureManager::LoadFileDone, this, &CaptureManager::OnLoadFileDone);
    QObject::connect(this,
                     &CaptureManager::GatherTraceStatsDone,
                     this,
                     &CaptureManager::OnGatherTraceStatsDone);
}

CaptureManager::~CaptureManager()
{
    if (m_thread == nullptr)
    {
        return;
    }
    m_thread->quit();
    m_thread->wait();
}

void CaptureManager::Start(Dive::DataCore &data_core)
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

void CaptureManager::OnGatherTraceStatsDone()
{
    m_working = false;
    if (!m_pending_capture_file)
    {
        TraceStatsUpdated();
    }
}

void CaptureManager::OnLoadFileDone(LoadedFileType file_type)
{
    m_working = false;
    m_loading_in_progress = false;

    if (m_pending_capture_file)
    {
        std::filesystem::path capture_file = m_pending_capture_file.value();
        m_pending_capture_file = std::nullopt;
        LoadFile(capture_file);
    }

    emit FileLoaded(file_type);
}

void CaptureManager::LoadFile(const std::filesystem::path &capture_file)
{
    if (!m_capture_stats_context.IsNull())
    {
        // Cancel trace stats gathering.
        m_capture_stats_context->Cancel();
    }

    m_loading_in_progress = true;
    if (m_working)
    {
        m_pending_capture_file = capture_file;
        return;
    }
    m_working = true;
    QMetaObject::invokeMethod(m_worker, [this, capture_file = capture_file]() {
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

        auto file_type = LoadFileImpl(Dive::Context::Background(), capture_file);

        [[maybe_unused]] int64_t
        time_used_to_load_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - begin)
                               .count();

        DIVE_DEBUG_LOG("Time used to load the capture is %f seconds.\n",
                       (time_used_to_load_ms / 1000.0));
        LoadFileDone(file_type);
    });
}

CaptureManager::LoadedFileType CaptureManager::LoadFileImpl(
const Dive::Context         &context,
const std::filesystem::path &capture_file)
{
    auto file_name = capture_file;

    QWriteLocker locker(&m_data_core_lock);
    // Note: this function might not run on UI thread, thus can't do any UI modification.

    // Check the file type to determine what is loaded.
    std::string file_extension = std::filesystem::path(file_name).extension().generic_string();

    // Check if the file loaded is a .gfxr file.
    auto found_gfxr_file = (file_extension.compare(Dive::kGfxrSuffix) == 0);
    auto found_rd_file = (file_extension.compare(".dive") == 0) ||
                         (file_extension.compare(".rd") == 0);

    if (found_gfxr_file)
    {
        // Convert the filename to a string to perform a replacement.
        std::string potential_asset_name(file_name);

        const std::string trim_str = "_trim_trigger";
        const std::string asset_str = "_asset_file";

        // Find and replace the last occurrence of "trim_trigger" part of the filename.
        size_t pos = potential_asset_name.rfind(trim_str);
        if (pos != std::string::npos)
        {
            potential_asset_name.replace(pos, trim_str.length(), asset_str);
        }

        // Create a path object to the asset file.
        std::filesystem::path asset_file_path(potential_asset_name);
        asset_file_path.replace_extension(".gfxa");

        // Check if the required asset file exists.
        bool asset_file_exists = std::filesystem::exists(asset_file_path);

        if (!asset_file_exists)
        {
            emit LoadingFailure(LoadFailureReason::kGfxaAssetMissing,
                                Dive::FilePath{ file_name },
                                QString::fromStdString(asset_file_path.string()));
            return LoadedFileType::kUnknown;
        }

        // Paths of associated files produced by Dive's GFXR replay
        std::filesystem::path capture_file_path = file_name;
        std::filesystem::path rd_file_path = capture_file_path.replace_extension(".rd");
        // Check if there is a corresponding .rd file
        if (std::filesystem::exists(rd_file_path))
        {
            found_rd_file = true;
        }
    }

    LoadedFileType file_type = LoadedFileType::kUnknown;
    if (found_gfxr_file && found_rd_file)
    {
        file_type = LoadedFileType::kDiveFile;
    }
    else if (found_gfxr_file)
    {
        file_type = LoadedFileType::kGfxrFile;
    }
    else if (found_rd_file)
    {
        file_type = LoadedFileType::kRdFile;
    }
    else
    {
        file_type = LoadedFileType::kUnknown;
    }

    switch (file_type)
    {
    case LoadedFileType::kUnknown:
    {

        emit LoadingFailure(LoadFailureReason::kUnsupportedFile, Dive::FilePath{ file_name });
        return LoadedFileType::kUnknown;
    }
    case LoadedFileType::kDiveFile:
    {
        if (Dive::CaptureData::LoadResult load_res = m_data_core->LoadDiveCaptureData(file_name);
            load_res != Dive::CaptureData::LoadResult::kSuccess)
        {
            emit LoadingFailure(ToLoadFailureReason(load_res), Dive::FilePath{ file_name });
            return LoadedFileType::kUnknown;
        }

        if (!m_data_core->ParseDiveCaptureData())
        {
            emit LoadingFailure(LoadFailureReason::kParseFailure, Dive::FilePath{ file_name });
            return LoadedFileType::kUnknown;
        }
    }
    break;
    case LoadedFileType::kRdFile:
    {
        if (Dive::CaptureData::LoadResult load_res = m_data_core->LoadPm4CaptureData(file_name);
            load_res != Dive::CaptureData::LoadResult::kSuccess)
        {
            emit LoadingFailure(ToLoadFailureReason(load_res), Dive::FilePath{ file_name });
            return LoadedFileType::kUnknown;
        }

        if (!m_data_core->ParsePm4CaptureData())
        {
            emit LoadingFailure(LoadFailureReason::kParseFailure, Dive::FilePath{ file_name });
            return LoadedFileType::kUnknown;
        }
    }
    break;
    case LoadedFileType::kGfxrFile:
    {
        if (Dive::CaptureData::LoadResult load_res = m_data_core->LoadGfxrCaptureData(file_name);
            load_res != Dive::CaptureData::LoadResult::kSuccess)
        {
            emit LoadingFailure(ToLoadFailureReason(load_res), Dive::FilePath{ file_name });
            return LoadedFileType::kUnknown;
        }

        if (!m_data_core->ParseGfxrCaptureData())
        {
            emit LoadingFailure(LoadFailureReason::kParseFailure, Dive::FilePath{ file_name });
            return LoadedFileType::kUnknown;
        }
    }
    break;
    }
    return file_type;
}

void CaptureManager::GatherTraceStats()
{
    if (m_working)
    {
        // Ignore this request.
        return;
    }
    m_working = true;
    if (!m_capture_stats_context.IsNull())
    {
        m_capture_stats_context->Cancel();
    }
    m_capture_stats_context = Dive::SimpleContext::Create();
    QMetaObject::invokeMethod(m_worker, [this, context = m_capture_stats_context]() {
        GatherTraceStatsImpl(context);
        GatherTraceStatsDone();
    });
}

void CaptureManager::FillCaptureStatsResult(Dive::CaptureStats &out)
{
    if (m_working)
    {
        out = {};
        return;
    }
    out = *m_capture_stats;
}

void CaptureManager::GatherTraceStatsImpl(const Dive::Context &context)
{

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    QReadLocker locker(&m_data_core_lock);

    // Gather the trace stats and display in the overview tab
    Dive::TraceStats{}.GatherTraceStats(context,
                                        m_data_core->GetCaptureMetadata(),
                                        *m_capture_stats);

    [[maybe_unused]] int64_t
    time_used_to_load_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - begin)
                           .count();
    if (context.Cancelled())
    {
        DIVE_DEBUG_LOG("Trace stats cancelled after %f seconds.\n",
                       (time_used_to_load_ms / 1000.0));
    }
    else
    {
        DIVE_DEBUG_LOG("Time used to load the trace stats is %f seconds.\n",
                       (time_used_to_load_ms / 1000.0));
    }
}
