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
#include <qapplication.h>
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
    m_enabled_settings_vector = new std::vector<std::string>();
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
    QStandardItem *enable_gpu_time = new QStandardItem("Enabled");
    QStandardItem *disable_gpu_time = new QStandardItem("Disabled");
    m_gpu_time_model->appendRow(enable_gpu_time);
    m_gpu_time_model->appendRow(disable_gpu_time);
    m_gpu_time_box->setModel(m_gpu_time_model);
    m_gpu_time_box->setCurrentIndex(1);
    m_gpu_time_layout->addWidget(m_gpu_time_box);
    m_gpu_time_enabled = m_gpu_time_box->currentIndex() == 0;

    // Enable Dump Pm4
    m_dump_pm4_layout = new QHBoxLayout();
    m_dump_pm4_label = new QLabel(tr("Dump Pm4:"));
    m_dump_pm4_layout->addWidget(m_dump_pm4_label);
    m_dump_pm4_box = new QComboBox();
    m_dump_pm4_model = new QStandardItemModel();
    QStandardItem *enable_dump_pm4 = new QStandardItem("Enabled");
    QStandardItem *disable_dump_pm4 = new QStandardItem("Disabled");
    m_dump_pm4_model->appendRow(enable_dump_pm4);
    m_dump_pm4_model->appendRow(disable_dump_pm4);
    m_dump_pm4_box->setModel(m_dump_pm4_model);
    m_dump_pm4_box->setCurrentIndex(1);
    m_dump_pm4_layout->addWidget(m_dump_pm4_box);
    m_dump_pm4_enabled = m_dump_pm4_box->currentIndex() == 0;

    // Capture Download Directory
    m_download_directory_layout = new QHBoxLayout();
    m_download_directory_label = new QLabel(tr("Download Directory:"));
    m_download_directory_input_box = new QLineEdit();
    m_download_directory_input_box->setPlaceholderText(
    "Directory path on the host to download files generated by replay");
    m_download_directory_layout->addWidget(m_download_directory_label);
    m_download_directory_layout->addWidget(m_download_directory_input_box);
    m_download_directory_label->hide();
    m_download_directory_input_box->hide();

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
    m_right_panel_layout->addLayout(m_dump_pm4_layout);
    m_right_panel_layout->addLayout(m_download_directory_layout);
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

    QObject::connect(m_dump_pm4_box,
                     SIGNAL(currentIndexChanged(const QString &)),
                     this,
                     SLOT(OnSettingChanged()));
    QObject::connect(m_gpu_time_box,
                     SIGNAL(currentIndexChanged(const QString &)),
                     this,
                     SLOT(OnSettingChanged()));
}

//--------------------------------------------------------------------------------------------------
AnalyzeDialog::~AnalyzeDialog()
{
    qDebug() << "AnalyzeDialog destroyed.";
    Dive::GetDeviceManager().RemoveDevice();
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::SetSelectedCaptureFile(const QString &filePath)
{
    m_selected_capture_file_string = filePath;
    m_selected_file_input_box->setText(m_selected_capture_file_string);
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::ShowErrorMessage(const std::string &err_msg)
{
    QMessageBox msgBox;
    msgBox.setText(err_msg.c_str());
    msgBox.exec();
    return;
}

//--------------------------------------------------------------------------------------------------
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
        csv_item->setData(kDataRole, item.key);
        csv_item->setFlags(csv_item->flags() | Qt::ItemIsUserCheckable);
        csv_item->setCheckState(Qt::Unchecked);
        m_settings_list->addItem(csv_item);
    }
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::UpdateSelectedSettingsList()
{
    // Clear the existing items in the target list
    m_enabled_settings_list->clear();
    m_enabled_settings_vector->clear();

    // Iterate through the source list to find checked items
    for (int i = 0; i < m_settings_list->count(); ++i)
    {
        QListWidgetItem *item = m_settings_list->item(i);

        // If the item is checked, add it to the target list
        if (item->checkState() == Qt::Checked)
        {
            m_enabled_settings_list->addItem(item->text());
            m_enabled_settings_vector->push_back(item->data(kDataRole).toString().toStdString());
        }
    }

    OnSettingChanged();
}

//--------------------------------------------------------------------------------------------------
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

//--------------------------------------------------------------------------------------------------
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

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::OnDeviceListRefresh()
{
    UpdateDeviceList(true);
}

//--------------------------------------------------------------------------------------------------
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
            SetSelectedCaptureFile(file_name);
            QString last_file_path = file_name.left(file_name.lastIndexOf('/'));
            Settings::Get()->WriteLastFilePath(last_file_path);
            emit OnNewFileOpened(file_name);
        }
    }
}

//--------------------------------------------------------------------------------------------------
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

//--------------------------------------------------------------------------------------------------
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

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::OnSettingChanged()
{
    m_dump_pm4_enabled = m_dump_pm4_box->currentIndex() == 0;
    m_gpu_time_enabled = m_gpu_time_box->currentIndex() == 0;

    if (m_enabled_settings_vector->size() > 0 || m_dump_pm4_enabled)
    {
        m_download_directory_label->show();
        m_download_directory_input_box->show();
    }
    else
    {
        m_download_directory_label->hide();
        m_download_directory_input_box->hide();
    }
    QApplication::processEvents();
}

//--------------------------------------------------------------------------------------------------
std::string AnalyzeDialog::GetReplayArgs()
{
    int         frame_count = m_frame_count_box->value();
    std::string args = "--loop-single-frame";
    if (frame_count > 0)
    {

        args += " --loop-single-frame-count " + std::to_string(frame_count);
    }

    if (m_dump_pm4_enabled && m_gpu_time_enabled)
    {
        args = "--enable-gpu-time";
    }
    else if (!m_dump_pm4_enabled && m_gpu_time_enabled)
    {
        args += " --enable-gpu-time";
    }
    else if (m_dump_pm4_enabled)
    {
        // Dump Pm4 does not support the loop-single-frame and loop-single-frame-count arguments
        args = "";
    }

    return args;
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::SetReplayButton(const std::string &message, bool is_enabled)
{
    m_replay_button->setEnabled(is_enabled);
    m_replay_button->setText(message.c_str());
    QApplication::processEvents();
}

void AnalyzeDialog::SetReplayDownloadDir()
{
    if (m_download_directory_input_box->text() == "")
    {
        m_download_directory_input_box->setText("./" +
                                                QString::fromUtf8(Dive::kDefaultReplayFolderName));
    }

    if (!std::filesystem::exists(m_download_directory_input_box->text().toStdString()))
    {

        std::error_code ec;
        if (!std::filesystem::create_directories(m_download_directory_input_box->text()
                                                 .toStdString(),
                                                 ec))
        {
            std::string err_msg = absl::StrCat("Error creating directory: ", ec.message());
            qDebug() << err_msg.c_str();
            ShowErrorMessage(err_msg);
            return;
        }
    }
    else
    {
        // If the target directory already exists on the local machine, append a number to it to
        // differentiate.
        int                   counter = 1;
        std::filesystem::path newDirPath;
        while (true)
        {
            newDirPath = std::filesystem::path(
            m_download_directory_input_box->text().toStdString() + "_" + std::to_string(counter));
            if (!std::filesystem::exists(newDirPath))
            {
                std::error_code ec;

                if (!std::filesystem::create_directories(newDirPath, ec))
                {
                    std::string err_msg = absl::StrCat("Error creating directory: ", ec.message());
                    qDebug() << err_msg.c_str();
                    ShowErrorMessage(err_msg);
                    return;
                }
                m_download_directory_input_box->setText(newDirPath.string().c_str());
                break;
            }
            counter++;
        }
    }

    QApplication::processEvents();
}

void AnalyzeDialog::UpdatePerfTabView(const std::string remote_file_name)
{
    QString directory = m_download_directory_input_box->text();

    // Get the original filename from the remote path
    QFileInfo original_file_info(QString::fromStdString(remote_file_name));
    QString   file_name = original_file_info.completeBaseName() + ".csv";

    // Construct the new full path.
    QString     full_path = QDir(directory).filePath(file_name);
    std::string output_path = full_path.toStdString();

    emit OnDisplayPerfCounterResults(output_path.c_str());
}

//--------------------------------------------------------------------------------------------------
absl::Status AnalyzeDialog::Pm4Replay(Dive::DeviceManager &device_manager,
                                      const std::string   &remote_gfxr_file)
{
    std::string replay_args = GetReplayArgs();
    if (m_dump_pm4_enabled && m_gpu_time_enabled)
    {
        SetReplayButton("Replaying with dump_pm4 and gpu_time enabled...", false);
        SetReplayDownloadDir();
    }
    else if (m_dump_pm4_enabled && !m_gpu_time_enabled)
    {
        SetReplayButton("Replaying with dump_pm4 enabled...", false);
        SetReplayDownloadDir();
    }
    else if (!m_dump_pm4_enabled && m_gpu_time_enabled)
    {
        SetReplayButton("Replaying with gpu_time enabled...", false);
    }
    else
    {
        SetReplayButton("Replaying...", false);
    }

    return device_manager.RunReplayApk(remote_gfxr_file,
                                       replay_args,
                                       m_dump_pm4_enabled,
                                       m_download_directory_input_box->text().toStdString());
}

//--------------------------------------------------------------------------------------------------
absl::Status AnalyzeDialog::PerfCounterReplay(Dive::DeviceManager &device_manager,
                                              const std::string   &remote_gfxr_file)
{
    SetReplayButton("Replaying with perf counter settings...", false);
    SetReplayDownloadDir();
    return device_manager
    .RunProfilingOnReplay(remote_gfxr_file,
                          *m_enabled_settings_vector,
                          m_download_directory_input_box->text().toStdString());
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::WaitForReplay(Dive::AndroidDevice &device)
{
    while (device.IsProcessRunning(Dive::kGfxrReplayAppName))
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::OnReplay()
{
    Dive::DeviceManager &device_manager = Dive::GetDeviceManager();
    auto                 device = device_manager.GetDevice();

    SetReplayButton("Setting up replay...", false);

    // Setup the device
    absl::Status ret = device->SetupDevice();
    if (!ret.ok())
    {
        std::string err_msg = absl::StrCat("Fail to setup device: ", ret.message());
        qDebug() << err_msg.c_str();
        ShowErrorMessage(err_msg);
        SetReplayButton(kDefaultReplayButtonText, true);
        return;
    }

    // Get the asset file name
    absl::StatusOr<std::string> asset_file = GetAssetFile();
    if (!asset_file.ok())
    {
        std::string err_msg = absl::StrCat(asset_file.status().message());
        qDebug() << err_msg.c_str();
        ShowErrorMessage(err_msg);
        SetReplayButton(kDefaultReplayButtonText, true);
        return;
    }

    absl::StatusOr<std::string> remote_file = PushFilesToDevice(device, asset_file.value());
    if (!remote_file.ok())
    {
        std::string err_msg = absl::StrCat("Failed to deploy replay apk: ", ret.message());
        qDebug() << err_msg.c_str();
        ShowErrorMessage(err_msg);
        SetReplayButton(kDefaultReplayButtonText, true);
        return;
    }

    // Deploying install/gfxr-replay.apk
    ret = device_manager.DeployReplayApk(m_cur_device);
    if (!ret.ok())
    {
        std::string err_msg = absl::StrCat("Failed to push files to device: ", ret.message());
        qDebug() << err_msg.c_str();
        ShowErrorMessage(err_msg);
        SetReplayButton(kDefaultReplayButtonText, true);
        return;
    }

    // Run the pm4 replay
    if (m_dump_pm4_enabled || m_gpu_time_enabled || m_enabled_settings_vector->empty())
    {
        ret = Pm4Replay(device_manager, remote_file.value());
        if (!ret.ok())
        {
            std::string err_msg = absl::StrCat("Failed to run pm4 replay: ", ret.message());
            qDebug() << err_msg.c_str();
            ShowErrorMessage(err_msg);
            SetReplayButton(kDefaultReplayButtonText, true);
            return;
        }

        WaitForReplay(*device);
    }

    // Run the perf counter replay
    if (!m_enabled_settings_vector->empty())
    {
        ret = PerfCounterReplay(device_manager, remote_file.value());
        if (!ret.ok())
        {
            std::string err_msg = absl::StrCat("Failed to run perf counter replay: ",
                                               ret.message());
            qDebug() << err_msg.c_str();
            ShowErrorMessage(err_msg);
            SetReplayButton(kDefaultReplayButtonText, true);
            return;
        }

        UpdatePerfTabView(remote_file.value());
        WaitForReplay(*device);
    }

    SetReplayButton(kDefaultReplayButtonText, true);
}
