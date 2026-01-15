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

#include <QMetaType>
#include <QObject>
#include <QReadWriteLock>
#include <QTemporaryDir>
#include <memory>
#include <optional>

#include "dive/ui/types/context.h"
#include "dive/ui/types/file_path.h"

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
        kFileNotFound,
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

    FileType file_type = FileType::kUnknown;
    Dive::FilePath reference = {};
    Dive::ComponentFilePaths components = {};
};

class CaptureFileManager : public QObject
{
    Q_OBJECT
 public:
    static void RegisterCustomMetaType();

    explicit CaptureFileManager(QObject* parent);
    ~CaptureFileManager();

    CaptureFileManager(const CaptureFileManager&) = delete;
    CaptureFileManager(CaptureFileManager&&) = delete;
    CaptureFileManager& operator=(const CaptureFileManager&) = delete;
    CaptureFileManager& operator=(CaptureFileManager&&) = delete;

    // Note: CaptureFileManager's worker thread requires a valid DataCore,
    // so keep a reference to it in case we close window before loading finishes.
    void Start(const std::shared_ptr<Dive::DataCore>& data_core);

    // Locking:
    QReadWriteLock& GetDataCoreLock() { return m_data_core_lock; }

    void LoadFile(const Dive::FilePath& reference);

    void GatherTraceStats();
    void FillCaptureStatsResult(Dive::CaptureStats& out);

 signals:
    void FileLoadingFinished(const LoadFileResult&);
    void TraceStatsUpdated();

    // private:
    void GatherTraceStatsDone();
    void LoadFileDone(const LoadFileResult&);

 private slots:
    void OnGatherTraceStatsDone();
    void OnLoadFileDone(const LoadFileResult&);

 private:
    std::shared_ptr<Dive::DataCore> m_data_core = nullptr;
    QReadWriteLock m_data_core_lock;

    bool m_loading_in_progress = false;
    bool m_working = false;

    std::optional<QTemporaryDir> m_temp_dir;

    std::optional<Dive::FilePath> m_pending_request;

    Dive::SimpleContext m_capture_file_context;

    std::unique_ptr<Dive::CaptureStats> m_capture_stats;

    QThread* m_thread = nullptr;
    QObject* m_worker = nullptr;

    Dive::ComponentFilePaths ResolveComponents(std::filesystem::path reference);
    LoadFileResult LoadFileFailed(LoadFileResult::Status status, const LoadFileResult& partial);

    void GatherTraceStatsImpl(const Dive::Context& context);

    void StartLoadFile();

    LoadFileResult LoadFileImpl(const Dive::Context& context, const Dive::FilePath& request);
};

Q_DECLARE_METATYPE(LoadFileResult)
