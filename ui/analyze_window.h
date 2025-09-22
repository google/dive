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
#include "capture_service/device_mgr.h"
#include "package_filter.h"
#include "dive_core/available_metrics.h"

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
class MainWindow;

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

    Q_OBJECT

public:
    AnalyzeDialog(
    std::optional<std::reference_wrapper<const Dive::AvailableMetrics>> available_metrics,
    QWidget                                                            *parent = nullptr);
    ~AnalyzeDialog();
    void UpdateDeviceList(bool isInitialized);
    void SetSelectedCaptureFile(const QString &filePath);
private slots:
    void OnDeviceSelected(const QString &);
    void OnDeviceListRefresh();
    void OnOpenFile();
    void OnReplay();
signals:
    void OnNewFileOpened(const QString &file_path);
    void OnDisplayPerfCounterResults(
    const std::filesystem::path                                        &file_path,
    std::optional<std::reference_wrapper<const Dive::AvailableMetrics>> available_metrics);
    void OnDisplayGpuTimingResults(const QString &file_path);
    void ReloadCapture(const QString &file_path);

private:
    void                        ShowErrorMessage(const std::string &message);
    void                        SetReplayButton(const std::string &message, bool is_enabled);
    void                        PopulateMetrics();
    void                        UpdateSelectedMetricsList();
    std::filesystem::path       GetFullLocalPath(const std::string &gfxr_stem,
                                                 const std::string &suffix) const;
    void                        WaitForReplay(Dive::AndroidDevice &device);
    absl::StatusOr<std::string> GetCaptureFileDirectory();
    absl::StatusOr<std::string> GetAssetFile();
    absl::StatusOr<std::string> PushFilesToDevice(Dive::AndroidDevice *device,
                                                  const std::string   &local_asset_file_path);
    std::string                 GetReplayArgs();
    absl::Status                Pm4Replay(Dive::DeviceManager &device_manager,
                                          const std::string   &remote_gfxr_file);
    absl::Status                PerfCounterReplay(Dive::DeviceManager &device_manager,
                                                  const std::string   &remote_gfxr_file);
    absl::Status                GpuTimeReplay(Dive::DeviceManager &device_manager,
                                              const std::string   &remote_gfxr_file);

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

    QHBoxLayout *m_selected_file_layout;
    QLabel      *m_selected_file_label;
    QLineEdit   *m_selected_file_input_box;
    QPushButton *m_open_files_button;

    QHBoxLayout        *m_dump_pm4_layout;
    QLabel             *m_dump_pm4_label;
    QStandardItemModel *m_dump_pm4_model;
    QComboBox          *m_dump_pm4_box;

    QHBoxLayout        *m_gpu_time_layout;
    QLabel             *m_gpu_time_label;
    QStandardItemModel *m_gpu_time_model;
    QComboBox          *m_gpu_time_box;

    QHBoxLayout *m_frame_count_layout;
    QLabel      *m_frame_count_label;
    QSpinBox    *m_frame_count_box;

    QHBoxLayout *m_replay_warning_layout;
    QLabel      *m_replay_warning_label;

    QHBoxLayout *m_button_layout;
    QPushButton *m_replay_button;

    QHBoxLayout                  *m_main_layout;
    QVBoxLayout                  *m_left_panel_layout;
    QVBoxLayout                  *m_right_panel_layout;
    std::vector<Dive::DeviceInfo> m_devices;
    std::string                   m_cur_device;
    QString                       m_selected_capture_file_string;
    QVector<CsvItem>             *m_csv_items;
    std::vector<std::string>     *m_enabled_metrics_vector;
    std::optional<std::reference_wrapper<const Dive::AvailableMetrics>> m_available_metrics;
    // Used to store a csv item's key in the enabled metrics vector.
    const int             kDataRole = Qt::UserRole + 1;
    const int             kDefaultFrameCount = 3;
    const std::string     kDefaultReplayButtonText = "Replay";
    std::filesystem::path m_local_capture_file_directory = "";

    bool m_dump_pm4_enabled;
    bool m_gpu_time_enabled;
};
