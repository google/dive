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
class SelectedCaptureFiles;
}  // namespace Dive

class AnalyzeDialog : public QDialog
{
    // Data structure to hold a single item from the CSV
    struct CsvItem
    {
        QString id;
        QString type;
        QString key;
        QString name;
        QString description;
    };

    Q_OBJECT

public:
    AnalyzeDialog(QWidget *parent = 0);
    ~AnalyzeDialog();
    void UpdateDeviceList(bool isInitialized);
    void setSelectedCaptureFile(const QString &filePath);
private slots:
    void OnDeviceSelected(const QString &);
    void OnDeviceListRefresh();
    void OnOpenFile();
    void OnReplay();
signals:
    void OnNewFileOpened(const QString &file_path);

private:
    void                        ShowErrorMessage(const std::string &message);
    void                        PopulateSettings();
    void                        UpdateSelectedSettingsList();
    absl::StatusOr<std::string> GetAssetFile();
    absl::Status                PushFilesToDevice(Dive::AndroidDevice *device,
                                                  const std::string   &local_asset_file_path);

    QLabel      *m_settings_list_label;
    QListWidget *m_settings_list;

    QLabel    *selected_setting_description_label;
    QTextEdit *selected_setting_description;

    QLabel      *m_enabled_settings_list_label;
    QListWidget *m_enabled_settings_list;

    QHBoxLayout        *m_device_layout;
    QLabel             *m_device_label;
    QStandardItemModel *m_device_model;
    QComboBox          *m_device_box;
    QPushButton        *m_device_refresh_button;

    QHBoxLayout *m_selected_file_layout;
    QLabel      *m_selected_file_label;
    QLineEdit   *m_selected_file_input_box;
    QPushButton *m_open_files_button;

    QHBoxLayout        *m_gpu_time_layout;
    QLabel             *m_gpu_time_label;
    QStandardItemModel *m_gpu_time_model;
    QComboBox          *m_gpu_time_box;

    QHBoxLayout *m_frame_count_layout;
    QLabel      *m_frame_count_label;
    QSpinBox    *m_frame_count_box;

    QHBoxLayout *m_button_layout;
    QPushButton *m_load_settings_button;
    QPushButton *m_replay_button;

    QHBoxLayout                  *m_main_layout;
    QVBoxLayout                  *m_left_panel_layout;
    QVBoxLayout                  *m_right_panel_layout;
    std::vector<Dive::DeviceInfo> m_devices;
    std::string                   m_cur_device;
    QString                       m_selected_capture_file_string;
    QVector<CsvItem>             *m_csv_items;
};
