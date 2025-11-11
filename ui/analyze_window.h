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

#include <QDialog>
#include <future>
#include <optional>
#include "capture_service/device_mgr.h"
#include "package_filter.h"
#include "dive_core/available_metrics.h"
#include "utils/component_files.h"

#pragma once

// Forward declarations
class QLabel;
class QHBoxLayout;
class QPushButton;
class QVBoxLayout;
class QComboBox;
class QStandardItemModel;
class QLineEdit;
class QListWidget;
class QSpinBox;
class QTextEdit;
class OverlayHelper;
class MainWindow;
class QCheckBox;
class QGroupBox;

class ApplicationController;

namespace Dive
{
class AvailableMetrics;
}  // namespace Dive

class AnalyzeDialog : public QDialog
{
    // Data structure to hold a single item from the CSV
    struct CsvItem
    {
        QString          id;
        Dive::MetricType type;
        QString          key;
        QString          name;
        QString          description;
    };

    // Describes all files associated with a GFXR file, does not guarantee existence
    struct ReplayArtifactsPaths
    {
        std::filesystem::path gfxr;
        // TODO: std::filesystem::path gfxa;
        std::filesystem::path perf_counter_csv;
        std::filesystem::path gpu_timing_csv;
        std::filesystem::path pm4_rd;
        std::filesystem::path renderdoc_rdc;
    };

    Q_OBJECT

    enum class ReplayStatusUpdateCode : int
    {
        // This signals the async process is finished:
        kDone,

        // End of task status:
        kSuccess,
        kFailure,
        kSetupDeviceFailure,  // Special case where we disable replay button.

        // Individual step of replay:
        kSetup,
        kStartNormalReplay,
        kStartPm4Replay,
        kStartGpuTimeReplay,
        kStartPerfCounterReplay,

        // Deleting replay artifacts associated with currently opened GFXR file
        kDeletingReplayArtifacts,
    };

public:
    AnalyzeDialog(ApplicationController        &controller,
                  const Dive::AvailableMetrics *available_metrics,
                  QWidget                      *parent = nullptr);
    ~AnalyzeDialog();
    void UpdateDeviceList(bool isInitialized);
private slots:
    void OnReplayStatusUpdate(int status_code, const QString &error_message);
    void OnDeviceSelected(const QString &);
    void OnDeviceListRefresh();
    void OnReplay();
    void OnOverlayMessage(const QString &message);
    void OnDisableOverlay();
    void OnDeleteReplayArtifacts();

public slots:
    void OnAnalyzeCaptureStarted(const QString &file_path);

signals:
    void ReplayStatusUpdated(int status_code, const QString &error_message);
    void DisplayPerfCounterResults(const QString &file_path);
    void DisplayGpuTimingResults(const QString &file_path);
    void CaptureUpdated(const QString &file_path);
    void OverlayMessage(const QString &message);
    void DisableOverlay();

private:
    struct ReplayConfig
    {
        bool replay_dump_pm4 = false;
        bool replay_gpu_time = false;
        bool replay_renderdoc = false;
        bool replay_perf_counter = false;
        bool replay_custom = false;
    };
    void                        ShowMessage(const std::string &message);
    void                        SetReplayButton(const std::string &message, bool is_enabled);
    void                        PopulateMetrics();
    void                        UpdateSelectedMetricsList();
    void                        UpdatePerfCounterElements(bool show);
    absl::StatusOr<std::string> PushFilesToDevice(Dive::AndroidDevice *device,
                                                  const std::string   &local_asset_file_path);
    absl::Status                NormalReplay(Dive::DeviceManager &device_manager,
                                             const std::string   &remote_gfxr_file);
    absl::Status                Pm4Replay(Dive::DeviceManager &device_manager,
                                          const std::string   &remote_gfxr_file);
    absl::Status                PerfCounterReplay(Dive::DeviceManager &device_manager,
                                                  const std::string   &remote_gfxr_file);
    absl::Status                GpuTimeReplay(Dive::DeviceManager &device_manager,
                                              const std::string   &remote_gfxr_file);
    absl::Status                RenderDocReplay(Dive::DeviceManager &device_manager,
                                                const std::string   &remote_gfxr_file);

    void UpdateReplayStatus(ReplayStatusUpdateCode status, const std::string &messge = "");
    void ExecuteStatusUpdate();

    void ReplayImpl(const ReplayConfig &);
    void DeleteReplayArtifactsImpl();
    void OnAnalyzeCaptureEnded();

    ApplicationController &m_controller;

    QLabel      *m_metrics_list_label;
    QListWidget *m_metrics_list;

    QLabel    *m_selected_metrics_description_label;
    QTextEdit *m_selected_metrics_description;

    QLabel      *m_enabled_metrics_list_label;
    QListWidget *m_enabled_metrics_list;

    QHBoxLayout        *m_device_layout;
    QLabel             *m_device_label;
    QStandardItemModel *m_device_model;
    QComboBox          *m_device_box;
    QPushButton        *m_device_refresh_button;

    // Provides a description of which capture file is open, but immutable from
    // AnalyzeDialog. User would need to close the dialog and use MainWindow toolbar to change the
    // loaded capture
    QHBoxLayout *m_selected_file_layout;
    QLabel      *m_selected_file_label;
    QLineEdit   *m_selected_file_input_box;

    QGroupBox *m_custom_replay_box = nullptr;
    QCheckBox *m_dump_pm4_box = nullptr;
    QCheckBox *m_perf_counter_box = nullptr;
    QGroupBox *m_gpu_time_replay_box = nullptr;
    QCheckBox *m_renderdoc_capture_box = nullptr;

    QSpinBox *m_gpu_time_replay_frame_count = nullptr;
    QSpinBox *m_custom_replay_frame_count = nullptr;

    QHBoxLayout *m_replay_warning_layout;
    QLabel      *m_replay_warning_label;

    QHBoxLayout *m_delete_replay_artifacts_layout;
    QPushButton *m_delete_replay_artifacts_button;

    QHBoxLayout *m_button_layout;
    QPushButton *m_replay_button;

    QHBoxLayout                  *m_main_layout;
    QVBoxLayout                  *m_left_panel_layout;
    QVBoxLayout                  *m_right_panel_layout;
    std::vector<Dive::DeviceInfo> m_devices;
    std::string                   m_cur_device;

    // Representing a session with a specific GFXR capture file opened
    //
    // Set only in OnOpenFile(), this is the filename of the GFXR capture that will be replayed
    QString m_selected_capture_file_string = "";
    // The dir that contains m_selected_capture_file_string
    std::filesystem::path m_local_capture_file_directory = "";
    // Other artifacts
    Dive::ComponentFilePaths m_local_capture_files = {};

    QVector<CsvItem>             *m_csv_items;
    std::vector<std::string>     *m_enabled_metrics_vector;
    const Dive::AvailableMetrics *m_available_metrics = nullptr;
    // Used to store a csv item's key in the enabled metrics vector.
    const int         kDataRole = Qt::UserRole + 1;
    const int         kDefaultFrameCount = 3;
    const std::string kDefaultReplayButtonText = "Replay";
    std::future<void> m_replay_active;
    OverlayHelper    *m_overlay;

    struct StatusUpdateQueueItem
    {
        ReplayStatusUpdateCode status;
        QString                message;
    };
    std::vector<StatusUpdateQueueItem> m_status_update_queue;
};
