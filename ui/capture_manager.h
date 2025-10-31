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

#include <filesystem>
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
}  // namespace Dive

class CaptureManager : public QObject
{
    Q_OBJECT
public:
    enum class LoadedFileType
    {
        kUnknown,  // Load failure
        kDiveFile,
        kRdFile,
        kGfxrFile,
    };
    Q_ENUM(CaptureManager::LoadedFileType)

    enum class LoadFailureReason
    {
        kUnknown,
        kFileIoError,
        kCorruptData,
        kVersionError,
        kParseFailure,
        kUnsupportedFile,
        kGfxaAssetMissing,
    };
    Q_ENUM(CaptureManager::LoadFailureReason)

    static void RegisterCustomMetaType();

    explicit CaptureManager(QObject *parent);
    ~CaptureManager();

    CaptureManager(const CaptureManager &) = delete;
    CaptureManager(CaptureManager &&) = delete;
    CaptureManager &operator=(const CaptureManager &) = delete;
    CaptureManager &operator=(CaptureManager &&) = delete;

    void Start(Dive::DataCore &);

    // Locking:
    QReadWriteLock &GetDataCoreLock() { return m_data_core_lock; }

    void LoadFile(const std::filesystem::path &capture_file);

    void GatherTraceStats();
    void FillCaptureStatsResult(Dive::CaptureStats &out);

signals:
    void LoadingFailure(CaptureManager::LoadFailureReason reason,
                        const Dive::FilePath             &reference_path,
                        const QString                    &aux = QString());
    void FileLoaded(CaptureManager::LoadedFileType);
    void TraceStatsUpdated();

    // private:
    void GatherTraceStatsDone();
    void LoadFileDone(CaptureManager::LoadedFileType);

private slots:
    void OnGatherTraceStatsDone();
    void OnLoadFileDone(CaptureManager::LoadedFileType);

private:
    Dive::DataCore *m_data_core = nullptr;
    QReadWriteLock  m_data_core_lock;

    bool m_loading_in_progress = false;
    bool m_working = false;

    std::optional<std::filesystem::path> m_pending_capture_file;

    Dive::SimpleContext m_capture_stats_context;

    std::unique_ptr<Dive::CaptureStats> m_capture_stats;

    QThread *m_thread = nullptr;
    QObject *m_worker = nullptr;

    void           GatherTraceStatsImpl(const Dive::Context &context);
    LoadedFileType LoadFileImpl(const Dive::Context         &context,
                                const std::filesystem::path &capture_file);
};

Q_DECLARE_METATYPE(CaptureManager::LoadedFileType)
Q_DECLARE_METATYPE(CaptureManager::LoadFailureReason)
