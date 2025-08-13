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

#include "analyze_window.h"

#include <QComboBox>
#include <QDebug>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QTextEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <filesystem>
#include <string>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "capture_service/device_mgr.h"
#include "settings.h"
#include "common/macros.h"

// =================================================================================================
// AnalyzeDialog
// =================================================================================================
AnalyzeDialog::AnalyzeDialog(QWidget *parent)
{
    qDebug() << "AnalyzeDialog created.";

    // Settings List
    m_settings_list_label = new QLabel(tr("Available Settings:"));
    m_settings_list = new QListWidget();
    m_csv_items = new QVector<CsvItem>();
    PopulateSettings();

    // Settings Description
    selected_setting_description_label = new QLabel(tr("Description:"));
    selected_setting_description = new QTextEdit();
    selected_setting_description->setReadOnly(true);
    selected_setting_description->setPlaceholderText("Select a setting to see its description...");

    // Enabled Settings
    m_enabled_settings_list_label = new QLabel(tr("Enabled Settings:"));
    m_enabled_settings_list = new QListWidget();

    // Device Selector
    m_device_layout = new QHBoxLayout();
    m_device_label = new QLabel(tr("Devices:"));
    m_device_model = new QStandardItemModel();
    m_device_box = new QComboBox();
    m_device_refresh_button = new QPushButton("&Refresh", this);
    m_devices = Dive::GetDeviceManager().ListDevice();
    UpdateDeviceList(false);
    m_device_box->setCurrentText("Please select a device");
    m_device_box->setModel(m_device_model);
    m_device_box->setCurrentIndex(0);
    m_device_layout->addWidget(m_device_label);
    m_device_layout->addWidget(m_device_box);
    m_device_layout->addWidget(m_device_refresh_button);

    // Selected File
    m_selected_file_layout = new QHBoxLayout();
    m_selected_file_label = new QLabel("Selected Capture file:");
    m_selected_file_input_box = new QLineEdit();
    m_selected_capture_file_string = "";
    m_selected_file_input_box->setText(m_selected_capture_file_string);
    m_selected_file_input_box->setReadOnly(true);
    m_open_files_button = new QPushButton(this);
    QIcon open_files_icon(":/images/open.png");
    m_open_files_button->setIcon(open_files_icon);
    m_selected_file_layout->addWidget(m_selected_file_label);
    m_selected_file_layout->addWidget(m_selected_file_input_box);
    m_selected_file_layout->addWidget(m_open_files_button);

    // Enable GPU Time
    m_gpu_time_layout = new QHBoxLayout();
    m_gpu_time_label = new QLabel(tr("GPU Time:"));
    m_gpu_time_layout->addWidget(m_gpu_time_label);
    m_gpu_time_box = new QComboBox();
    m_gpu_time_model = new QStandardItemModel();
    QStandardItem *enabled_item = new QStandardItem("Enabled");
    QStandardItem *disabled_item = new QStandardItem("Disabled");
    m_gpu_time_model->appendRow(enabled_item);
    m_gpu_time_model->appendRow(disabled_item);
    m_gpu_time_box->setModel(m_gpu_time_model);
    m_gpu_time_box->setCurrentIndex(1);
    m_gpu_time_layout->addWidget(m_gpu_time_box);

    // Single Frame Loop Count
    m_frame_count_layout = new QHBoxLayout();
    m_frame_count_label = new QLabel(tr("Loop Single Frame Count:"));
    m_frame_count_box = new QSpinBox(this);
    m_frame_count_box->setRange(0, std::numeric_limits<int>::max());
    m_frame_count_box->setSpecialValueText("Infinite");
    m_frame_count_box->setMinimum(-1);
    m_frame_count_box->setValue(-1);
    m_frame_count_layout->addWidget(m_frame_count_label);
    m_frame_count_layout->addWidget(m_frame_count_box);

    // Replay Button
    m_button_layout = new QHBoxLayout();
    m_replay_button = new QPushButton("&Replay", this);
    m_replay_button->setEnabled(true);
    m_button_layout->addWidget(m_replay_button);

    // Left Panel Layout
    m_left_panel_layout = new QVBoxLayout();
    m_left_panel_layout->addWidget(m_settings_list_label);
    m_left_panel_layout->addWidget(m_settings_list);

    // Right Panel Layout
    m_right_panel_layout = new QVBoxLayout();
    m_right_panel_layout->addWidget(selected_setting_description_label);
    m_right_panel_layout->addWidget(selected_setting_description);
    m_right_panel_layout->addWidget(m_enabled_settings_list_label);
    m_right_panel_layout->addWidget(m_enabled_settings_list);
    m_right_panel_layout->addLayout(m_device_layout);
    m_right_panel_layout->addLayout(m_selected_file_layout);
    m_right_panel_layout->addLayout(m_gpu_time_layout);
    m_right_panel_layout->addLayout(m_frame_count_layout);
    m_right_panel_layout->addLayout(m_button_layout);

    // Main Layout
    m_main_layout = new QHBoxLayout(this);
    m_main_layout->addLayout(m_left_panel_layout);
    m_main_layout->addLayout(m_right_panel_layout);

    setLayout(m_main_layout);

    // Connect the name list's selection change to a lambda
    QObject::connect(m_settings_list,
                     &QListWidget::currentItemChanged,
                     [&](QListWidgetItem *current, QListWidgetItem *previous) {
                         if (current)
                         {
                             int index = m_settings_list->row(current);
                             if (index >= 0 && index < m_csv_items->size())
                             {
                                 selected_setting_description->setText(
                                 m_csv_items->at(index).description);
                             }
                         }
                     });

    QObject::connect(m_settings_list, &QListWidget::itemChanged, [&](QListWidgetItem *item) {
        // This code will execute whenever an item's state changes
        // It will refresh the second list of selected items
        UpdateSelectedSettingsList();
    });

    QObject::connect(m_device_box,
                     SIGNAL(currentIndexChanged(const QString &)),
                     this,
                     SLOT(OnDeviceSelected(const QString &)));
    QObject::connect(m_device_refresh_button,
                     &QPushButton::clicked,
                     this,
                     &AnalyzeDialog::OnDeviceListRefresh);
    QObject::connect(m_open_files_button, &QPushButton::clicked, this, &AnalyzeDialog::OnOpenFile);
    QObject::connect(m_replay_button, &QPushButton::clicked, this, &AnalyzeDialog::OnReplay);
}

AnalyzeDialog::~AnalyzeDialog()
{
    qDebug() << "AnalyzeDialog destroyed.";
    Dive::GetDeviceManager().RemoveDevice();
}

void AnalyzeDialog::setSelectedCaptureFile(const QString &filePath)
{
    m_selected_capture_file_string = filePath;
    m_selected_file_input_box->setText(m_selected_capture_file_string);
}

void AnalyzeDialog::ShowErrorMessage(const std::string &err_msg)
{
    QMessageBox msgBox;
    msgBox.setText(err_msg.c_str());
    msgBox.exec();
    return;
}

void AnalyzeDialog::PopulateSettings()
{
    QFile file(":/resources/available_settings.csv");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        std::cout << "Could not open file:" << file.errorString().toStdString() << std::endl;
        qDebug() << "Could not open file:" << file.errorString();
        return;
    }

    QTextStream in(&file);

    // Read and discard the first line (headers)
    if (!in.atEnd())
    {
        in.readLine();
    }

    while (!in.atEnd())
    {
        QString     line = in.readLine();
        QStringList fields = line.split(',');

        if (fields.size() == 5)
        {
            CsvItem item;
            item.id = fields[0].replace('"', "");
            ;
            item.type = fields[1].replace('"', "");
            ;
            item.key = fields[2].replace('"', "");
            ;
            item.name = fields[3].replace('"', "");
            ;
            item.description = fields[4].replace('"', "");
            ;
            m_csv_items->append(item);
        }
    }
    file.close();

    // Populate the settings list
    for (const auto &item : *m_csv_items)
    {
        QListWidgetItem *csv_item = new QListWidgetItem(item.name);
        csv_item->setFlags(csv_item->flags() | Qt::ItemIsUserCheckable);
        csv_item->setCheckState(Qt::Unchecked);
        m_settings_list->addItem(csv_item);
    }
}

void AnalyzeDialog::UpdateSelectedSettingsList()
{
    // Clear the existing items in the target list
    m_enabled_settings_list->clear();

    // Iterate through the source list to find checked items
    for (int i = 0; i < m_settings_list->count(); ++i)
    {
        QListWidgetItem *item = m_settings_list->item(i);

        // If the item is checked, add it to the target list
        if (item->checkState() == Qt::Checked)
        {
            m_enabled_settings_list->addItem(item->text());
        }
    }
}

void AnalyzeDialog::UpdateDeviceList(bool isInitialized)
{
    auto cur_list = Dive::GetDeviceManager().ListDevice();
    qDebug() << "m_device_box->currentIndex() " << m_device_box->currentIndex();
    if (cur_list == m_devices && isInitialized)
    {
        qDebug() << "No changes from the list of the connected devices. ";
        return;
    }

    m_devices = cur_list;
    m_device_model->clear();

    if (m_devices.empty())
    {
        QStandardItem *item = new QStandardItem("No devices found");
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        m_device_model->appendRow(item);
        m_device_box->setCurrentIndex(0);
    }
    else
    {
        for (size_t i = 0; i < m_devices.size(); i++)
        {
            if (i == 0)
            {
                QStandardItem *item = new QStandardItem("Please select a device");
                item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
                m_device_model->appendRow(item);
                m_device_box->setCurrentIndex(0);
            }

            QStandardItem *item = new QStandardItem(m_devices[i].GetDisplayName().c_str());
            m_device_model->appendRow(item);
            // Keep the original selected devices as selected.
            if (m_cur_device == m_devices[i].m_serial)
            {
                m_device_box->setCurrentIndex(static_cast<int>(i));
            }
        }
    }
}

void AnalyzeDialog::OnDeviceSelected(const QString &s)
{
    if (s.isEmpty() || m_device_box->currentIndex() == 0)
    {
        qDebug() << "No devices selected";
        return;
    }
    int device_index = m_device_box->currentIndex() - 1;
    assert(static_cast<size_t>(device_index) < m_devices.size());

    qDebug() << "Device selected: " << m_cur_device.c_str() << ", index " << device_index
             << ", m_devices[device_index].m_serial " << m_devices[device_index].m_serial.c_str();
    if (m_cur_device == m_devices[device_index].m_serial)
    {
        return;
    }

    m_cur_device = m_devices[device_index].m_serial;
    auto dev_ret = Dive::GetDeviceManager().SelectDevice(m_cur_device);
    if (!dev_ret.ok())
    {
        std::string err_msg = absl::StrCat("Failed to select device ",
                                           m_cur_device.c_str(),
                                           ", error: ",
                                           dev_ret.status().message());
        qDebug() << err_msg.c_str();
        ShowErrorMessage(err_msg);
        return;
    }
}

void AnalyzeDialog::OnDeviceListRefresh()
{
    UpdateDeviceList(true);
}

void AnalyzeDialog::OnOpenFile()
{
    QString supported_files = QStringLiteral("GFXR files (*.gfxr)");
    QString file_name = QFileDialog::getOpenFileName(this,
                                                     "Open Capture File",
                                                     Settings::Get()->ReadLastFilePath(),
                                                     supported_files);

    if (!file_name.isEmpty())
    {
        // Convert the filename to a string to perform a replacement.
        std::string potential_asset_name(file_name.toStdString());

        const std::string trim_str = "_trim_trigger";
        const std::string asset_str = "_asset_file";

        // Find and replace the "trim_trigger" part of the filename.
        size_t pos = potential_asset_name.find(trim_str);
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
            QString title = QString("Unable to open file: %1").arg(file_name);
            QString description = QString("Required .gfxa file: %1 not found!")
                                  .arg(QString::fromStdString(asset_file_path.string()));
            QMessageBox::critical(this, title, description);
            return;
        }
        else
        {
            QString last_file_path = file_name.left(file_name.lastIndexOf('/'));
            Settings::Get()->WriteLastFilePath(last_file_path);
        }
    }
}

absl::StatusOr<std::string> AnalyzeDialog::GetAssetFile()
{
    // Convert the filename to a string to perform a replacement.
    std::string potential_asset_name(m_selected_capture_file_string.toStdString());

    const std::string trim_str = "_trim_trigger";
    const std::string asset_str = "_asset_file";

    // Find and replace the "trim_trigger" part of the filename.
    size_t pos = potential_asset_name.find(trim_str);
    if (pos != std::string::npos)
    {
        potential_asset_name.replace(pos, trim_str.length(), asset_str);
    }

    // Create a path object to the asset file.
    std::filesystem::path asset_file_path(potential_asset_name);
    asset_file_path.replace_extension(".gfxa");

    std::cout << "Asset file path: " << asset_file_path << std::endl;
    // Check if the required asset file exists.
    bool asset_file_exists = std::filesystem::exists(asset_file_path);

    if (!asset_file_exists)
    {
        return absl::NotFoundError("Failed to find corresponding .gfxa asset file.");
    }

    return asset_file_path.string();
}

absl::StatusOr<std::string> AnalyzeDialog::PushFilesToDevice(
Dive::AndroidDevice *device,
const std::string   &local_asset_file_path)
{
    const std::string remote_dir = "/sdcard/gfxr_captures_for_replay";

    // Create the remote directory on the device.
    RETURN_IF_ERROR(device->Adb().Run(absl::StrFormat("shell mkdir -p %s", remote_dir)));

    // Push the .gfxr file.
    std::string           local_gfxr_path = m_selected_capture_file_string.toStdString();
    std::filesystem::path gfxr_path(local_gfxr_path);
    std::string           gfxr_filename = gfxr_path.filename().string();
    std::string           remote_gfxr_path = absl::StrFormat("%s/%s", remote_dir, gfxr_filename);
    RETURN_IF_ERROR(
    device->Adb().Run(absl::StrFormat("push %s %s", local_gfxr_path, remote_gfxr_path)));

    // Push the .gfxa file.
    std::filesystem::path asset_file_path(local_asset_file_path);
    std::string           asset_file_name = asset_file_path.filename().string();
    RETURN_IF_ERROR(device->Adb().Run(
    absl::StrFormat("push %s %s/%s", local_asset_file_path, remote_dir, asset_file_name)));

    return remote_gfxr_path;
}

void AnalyzeDialog::OnReplay()
{
    Dive::DeviceManager &device_manager = Dive::GetDeviceManager();
    auto                 device = device_manager.GetDevice();

    // Setup the device
    absl::Status ret = device->SetupDevice();
    if (!ret.ok())
    {
        std::string err_msg = absl::StrCat("Fail to setup device: ", ret.message());
        qDebug() << err_msg.c_str();
        ShowErrorMessage(err_msg);
        return;
    }

    // Get the asset file name
    absl::StatusOr<std::string> asset_file = GetAssetFile();
    if (!asset_file.ok())
    {
        std::string err_msg = absl::StrCat(asset_file.status().message());
        qDebug() << err_msg.c_str();
        ShowErrorMessage(err_msg);
        return;
    }

    absl::StatusOr<std::string> remote_file = PushFilesToDevice(device, asset_file.value());
    if (!remote_file.ok())
    {
        std::string err_msg = absl::StrCat("Failed to deploy replay apk: ", ret.message());
        qDebug() << err_msg.c_str();
        ShowErrorMessage(err_msg);
        return;
    }

    // Deploying install/gfxr-replay.apk
    ret = device_manager.DeployReplayApk(m_cur_device);
    if (!ret.ok())
    {
        std::string err_msg = absl::StrCat("Failed to push files to device: ", ret.message());
        qDebug() << err_msg.c_str();
        ShowErrorMessage(err_msg);
        return;
    }

    // Running replay
    std::string replay_args = "";
    int         frame_count = m_frame_count_box->value();
    if (frame_count > 0)
    {
        replay_args = "--loop-single-frame --loop-single-frame-count " +
                      std::to_string(frame_count);
    }
    ret = device_manager.RunReplayApk(remote_file.value(), replay_args, false, "");
    if (!ret.ok())
    {
        std::string err_msg = absl::StrCat("Failed to run replay apk: ", ret.message());
        qDebug() << err_msg.c_str();
        ShowErrorMessage(err_msg);
        return;
    }
}
