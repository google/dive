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

#pragma once

#include <memory>

#include <QMetaType>
#include <QObject>
#include <QReadWriteLock>

#include "context.h"
#include "file_path.h"

class QThread;
namespace Dive
{
class DataCore;
struct CaptureStats;
struct ComponentFilePaths;
}  // namespace Dive

struct LoadFileResult
{

    enum class Status
    {
        kSuccess,
        kUnknown,
        kFileIoError,
        kCorruptData,
        kVersionError,
        kParseFailure,
        kUnsupportedFile,
        kGfxaAssetMissing,
    };

    enum class FileType
    {
        kUnknown,  // Load failure
        kCorrelatedFiles,
        kRdFile,
        kGfxrFile,
    };

    Status status = Status::kUnknown;

    FileType                 file_type = FileType::kUnknown;
    Dive::FilePath           reference = {};
    Dive::ComponentFilePaths components = {};
};

class CaptureFileManager : public QObject
{
    Q_OBJECT
public:
    static void RegisterCustomMetaType();

    explicit CaptureFileManager(QObject *parent);
    ~CaptureFileManager();

    CaptureFileManager(const CaptureFileManager &) = delete;
    CaptureFileManager(CaptureFileManager &&) = delete;
    CaptureFileManager &operator=(const CaptureFileManager &) = delete;
    CaptureFileManager &operator=(CaptureFileManager &&) = delete;

    void Start(Dive::DataCore &);

    // Locking:
    QReadWriteLock &GetDataCoreLock() { return m_data_core_lock; }

    Dive::ComponentFilePaths ResolveComponents(const Dive::FilePath &reference);
    void LoadFile(const Dive::FilePath &reference, const Dive::ComponentFilePaths &components);

    void GatherTraceStats();
    void FillCaptureStatsResult(Dive::CaptureStats &out);

signals:
    void FileLoadingFinished(const LoadFileResult &);
    void TraceStatsUpdated();

    // private:
    void GatherTraceStatsDone();
    void LoadFileDone(const LoadFileResult &);

private slots:
    void OnGatherTraceStatsDone();
    void OnLoadFileDone(const LoadFileResult &);

private:
    struct LoadFileRequest
    {
        Dive::FilePath           reference;
        Dive::ComponentFilePaths components;
    };
    Dive::DataCore *m_data_core = nullptr;
    QReadWriteLock  m_data_core_lock;

    bool m_loading_in_progress = false;
    bool m_working = false;

    std::optional<LoadFileRequest> m_pending_request;

    Dive::SimpleContext m_capture_file_context;

    std::unique_ptr<Dive::CaptureStats> m_capture_stats;

    QThread *m_thread = nullptr;
    QObject *m_worker = nullptr;

    static LoadFileResult LoadFileFailed(LoadFileResult::Status                     status,
                                         const CaptureFileManager::LoadFileRequest &request);

    void GatherTraceStatsImpl(const Dive::Context &context);

    void StartLoadFile();

    LoadFileResult LoadFileImpl(const Dive::Context &context, const LoadFileRequest &request);
};

Q_DECLARE_METATYPE(LoadFileResult)
