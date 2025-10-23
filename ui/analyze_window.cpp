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
#include <QCheckBox>
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
#include <future>
#include <optional>
#include <qapplication.h>
#include <qtemporarydir.h>
#include <string>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "capture_service/constants.h"
#include "capture_service/device_mgr.h"
#include "settings.h"
#include "overlay.h"
#include "common/macros.h"

//--------------------------------------------------------------------------------------------------
void AttemptDeletingTemporaryLocalFile(const std::filesystem::path &file_path)
{
    if (std::filesystem::remove(file_path))
    {
        qDebug() << "Successfully deleted: " << file_path.string().c_str();
    }
    else
    {
        qDebug() << "Was not present: " << file_path.string().c_str();
    }
}

// =================================================================================================
// AnalyzeDialog
// =================================================================================================
AnalyzeDialog::AnalyzeDialog(
std::optional<std::reference_wrapper<const Dive::AvailableMetrics>> available_metrics,
QWidget                                                            *parent) :
    QDialog(parent),
    m_available_metrics(available_metrics)
{
    qDebug() << "AnalyzeDialog created.";

    m_overlay = new Overlay(this);

    // Metrics List
    m_metrics_list_label = new QLabel(tr("Available Metrics:"));
    m_metrics_list = new QListWidget();
    m_csv_items = new QVector<CsvItem>();
    m_enabled_metrics_vector = new std::vector<std::string>();
    PopulateMetrics();

    // Metrics Description
    m_selected_metrics_description_label = new QLabel(tr("Description:"));
    m_selected_metrics_description = new QTextEdit();
    m_selected_metrics_description->setReadOnly(true);
    m_selected_metrics_description->setPlaceholderText("Select a metric to see its description...");

    // Enabled Metrics
    m_enabled_metrics_list_label = new QLabel(tr("Enabled Metrics:"));
    m_enabled_metrics_list = new QListWidget();

    // Replay Button
    m_button_layout = new QHBoxLayout();
    m_replay_button = new QPushButton("&Replay", this);
    m_replay_button->setEnabled(false);
    m_button_layout->addWidget(m_replay_button);

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
    m_gpu_time_label = new QLabel(tr("Enable GPU Time:"));
    m_gpu_time_layout->addWidget(m_gpu_time_label);
    m_gpu_time_box = new QCheckBox();
    m_gpu_time_box->setCheckState(Qt::Unchecked);
    m_gpu_time_layout->addWidget(m_gpu_time_box);
    m_gpu_time_layout->addStretch();

    // Enable Dump Pm4
    m_dump_pm4_layout = new QHBoxLayout();
    m_dump_pm4_label = new QLabel(tr("Enable Dump Pm4:"));
    m_dump_pm4_layout->addWidget(m_dump_pm4_label);
    m_dump_pm4_box = new QCheckBox();
    m_dump_pm4_box->setCheckState(Qt::Unchecked);
    m_dump_pm4_layout->addWidget(m_dump_pm4_box);
    m_dump_pm4_layout->addStretch();

    // Enable RenderDoc capture
    m_renderdoc_capture_layout = new QHBoxLayout();
    m_renderdoc_capture_label = new QLabel(tr("Enable RenderDoc capture:"));
    m_renderdoc_capture_layout->addWidget(m_renderdoc_capture_label);
    m_renderdoc_capture_box = new QCheckBox();
    m_renderdoc_capture_box->setCheckState(Qt::Unchecked);
    m_renderdoc_capture_layout->addWidget(m_renderdoc_capture_box);
    m_renderdoc_capture_layout->addStretch();

    // Single Frame Loop Count
    m_frame_count_layout = new QHBoxLayout();
    m_frame_count_label = new QLabel(tr("Loop Single Frame Count:"));
    m_frame_count_box = new QSpinBox(this);
    m_frame_count_box->setRange(1, std::numeric_limits<int>::max());
    m_frame_count_box->setValue(kDefaultFrameCount);
    m_frame_count_layout->addWidget(m_frame_count_label);
    m_frame_count_layout->addWidget(m_frame_count_box);

    // Replay Warning
    m_replay_warning_layout = new QHBoxLayout();
    m_replay_warning_label = new QLabel(
    tr("⚠ Initiating replay will use and potentially overwrite temporary artifacts from previous "
       "replays. Save any desired artifacts manually in a separate folder before proceeding."));
    m_replay_warning_label->setWordWrap(true);
    m_replay_warning_layout->addWidget(m_replay_warning_label);

    // Delete replay artifacts
    m_delete_replay_artifacts_layout = new QHBoxLayout();
    m_delete_replay_artifacts_button = new QPushButton("&Delete Previous Replay Artifacts", this);
    m_delete_replay_artifacts_layout->addWidget(m_delete_replay_artifacts_button);

    // Left Panel Layout
    m_left_panel_layout = new QVBoxLayout();
    m_left_panel_layout->addWidget(m_metrics_list_label);
    m_left_panel_layout->addWidget(m_metrics_list);

    // Right Panel Layout
    m_right_panel_layout = new QVBoxLayout();
    m_right_panel_layout->addWidget(m_selected_metrics_description_label);
    m_right_panel_layout->addWidget(m_selected_metrics_description);
    m_right_panel_layout->addWidget(m_enabled_metrics_list_label);
    m_right_panel_layout->addWidget(m_enabled_metrics_list);
    m_right_panel_layout->addLayout(m_device_layout);
    m_right_panel_layout->addLayout(m_selected_file_layout);
    m_right_panel_layout->addLayout(m_dump_pm4_layout);
    m_right_panel_layout->addLayout(m_renderdoc_capture_layout);
    m_right_panel_layout->addLayout(m_gpu_time_layout);
    m_right_panel_layout->addLayout(m_frame_count_layout);
    m_right_panel_layout->addLayout(m_replay_warning_layout);
    m_right_panel_layout->addLayout(m_delete_replay_artifacts_layout);
    m_right_panel_layout->addLayout(m_button_layout);

    // Main Layout
    m_main_layout = new QHBoxLayout(this);
    m_main_layout->addLayout(m_left_panel_layout);
    m_main_layout->addLayout(m_right_panel_layout);

    setLayout(m_main_layout);

    // Connect the name list's selection change to a lambda
    QObject::connect(m_metrics_list,
                     &QListWidget::currentItemChanged,
                     [&](QListWidgetItem *current, QListWidgetItem *previous) {
                         if (current)
                         {
                             int index = m_metrics_list->row(current);
                             if (index >= 0 && index < m_csv_items->size())
                             {
                                 m_selected_metrics_description->setText(
                                 m_csv_items->at(index).description);
                             }
                         }
                     });

    QObject::connect(m_metrics_list, &QListWidget::itemChanged, [&](QListWidgetItem *item) {
        // This code will execute whenever an item's state changes
        // It will refresh the second list of selected items
        UpdateSelectedMetricsList();
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
    QObject::connect(m_delete_replay_artifacts_button,
                     &QPushButton::clicked,
                     this,
                     &AnalyzeDialog::OnDeleteReplayArtifacts);

    QObject::connect(this,
                     &AnalyzeDialog::ReplayStatusUpdated,
                     this,
                     &AnalyzeDialog::OnReplayStatusUpdate);

    QObject::connect(this, &AnalyzeDialog::DisableOverlay, this, &AnalyzeDialog::OnDisableOverlay);
    QObject::connect(this, &AnalyzeDialog::OverlayMessage, this, &AnalyzeDialog::OnOverlayMessage);
}

//--------------------------------------------------------------------------------------------------
AnalyzeDialog::~AnalyzeDialog()
{
    qDebug() << "AnalyzeDialog destroyed.";
    Dive::GetDeviceManager().RemoveDevice();
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::resizeEvent(QResizeEvent *event)
{
    QDialog::resizeEvent(event);
    m_overlay->UpdateSize(rect());
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::OnOverlayMessage(const QString &message)
{
    setDisabled(true);
    m_overlay->SetMessage(message, /*timed*/ true);
    if (m_overlay->isHidden())
        m_overlay->show();
}

void AnalyzeDialog::OnDisableOverlay()
{
    setDisabled(false);
    m_overlay->SetMessage(QString());
    m_overlay->hide();
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
    auto message_box = new QMessageBox(this);
    message_box->setAttribute(Qt::WA_DeleteOnClose, true);
    message_box->setText(err_msg.c_str());
    message_box->open();
    return;
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::PopulateMetrics()
{
    if (m_available_metrics.has_value())
    {
        // Get the list of all available metrics
        std::vector<std::string> all_keys = m_available_metrics->get().GetAllMetricKeys();

        for (const auto &key : all_keys)
        {
            const Dive::MetricInfo *info = m_available_metrics->get().GetMetricInfo(key);
            if (info)
            {
                CsvItem item;
                item.id = info->m_metric_id;
                item.type = info->m_metric_type;
                item.key = QString::fromStdString(key);
                item.name = QString::fromStdString(info->m_name);
                item.description = QString::fromStdString(info->m_description);
                m_csv_items->append(item);
            }
        }

        // Populate the metrics list
        for (const auto &item : *m_csv_items)
        {
            QListWidgetItem *csv_item = new QListWidgetItem(item.name);
            csv_item->setData(kDataRole, item.key);
            csv_item->setFlags(csv_item->flags() | Qt::ItemIsUserCheckable);
            csv_item->setCheckState(Qt::Unchecked);
            m_metrics_list->addItem(csv_item);
        }

        // Add spacer so that all metrics are visible at the end of the list.
        QListWidgetItem *spacer = new QListWidgetItem();
        spacer->setFlags(spacer->flags() & ~Qt::ItemIsSelectable);
        m_metrics_list->addItem(spacer);
    }
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::UpdateSelectedMetricsList()
{
    // Clear the existing items in the target list
    m_enabled_metrics_list->clear();
    m_enabled_metrics_vector->clear();

    // Iterate through the source list to find checked items
    for (int i = 0; i < m_metrics_list->count(); ++i)
    {
        QListWidgetItem *item = m_metrics_list->item(i);

        // If the item is checked, add it to the target list
        if (item->checkState() == Qt::Checked)
        {
            m_enabled_metrics_list->addItem(item->text());
            m_enabled_metrics_vector->push_back(item->data(kDataRole).toString().toStdString());
        }
    }
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
    // Replay button should only be enabled when a device is selected.
    m_replay_button->setEnabled(false);

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
        // Enable the replay button as soon as a device is selected.
        m_replay_button->setEnabled(true);
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
        OnDeviceListRefresh();
        return;
    }

    m_replay_button->setEnabled(true);
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

    if (file_name.isEmpty())
    {
        qDebug() << "AnalyzeDialog::OnOpenFile(): empty file name";
        return;
    }

    std::filesystem::path gfxr_parse = file_name.toStdString();
    m_local_capture_file_directory = gfxr_parse.parent_path();
    absl::StatusOr<ReplayArtifactsPaths> ret = GetReplayFilesLocalPaths(gfxr_parse.stem().string());
    if (!ret.ok())
    {
        std::string err_msg = absl::StrFormat("AnalyzeDialog::OnOpenFile(): %s",
                                              ret.status().message());
        qDebug() << err_msg.c_str();
        return;
    }
    ReplayArtifactsPaths local_artifacts = *ret;

    if (!std::filesystem::exists(local_artifacts.gfxa))
    {
        QString title = QString("Unable to open file: %1").arg(file_name);
        QString description = QString("Required .gfxa file: %1 not found!")
                              .arg(QString::fromStdString(local_artifacts.gfxa.string()));
        QMessageBox::critical(this, title, description);
        return;
    }

    SetSelectedCaptureFile(file_name);
    QString last_file_path = file_name.left(file_name.lastIndexOf('/'));
    Settings::Get()->WriteLastFilePath(last_file_path);
    emit LoadCapture(file_name);
}

//--------------------------------------------------------------------------------------------------
absl::StatusOr<std::string> AnalyzeDialog::GetCaptureFileDirectory()
{
    std::filesystem::path capture_file_path(m_selected_capture_file_string.toStdString());

    // Get the parent directory of the capture file.
    std::filesystem::path directory_path = capture_file_path.parent_path();

    if (!std::filesystem::exists(directory_path))
    {
        return absl::NotFoundError("Failed to find directory for the selected capture file.");
    }

    return directory_path.string();
}

//--------------------------------------------------------------------------------------------------
absl::StatusOr<std::string> AnalyzeDialog::GetAssetFile()
{
    // Get the capture file's directory.
    absl::StatusOr<std::string> capture_directory = GetCaptureFileDirectory();
    if (!capture_directory.ok())
    {
        return capture_directory.status();
    }
    std::filesystem::path asset_directory = *capture_directory;

    // Convert the filename to a string to perform a replacement.
    std::string potential_asset_name(m_selected_capture_file_string.toStdString());

    const std::string trim_str = "_trim_trigger";
    const std::string asset_str = "_asset_file";

    // Find and replace the last occurrence of "trim_trigger" part of the filename.
    size_t pos = potential_asset_name.rfind(trim_str);
    if (pos != std::string::npos)
    {
        potential_asset_name.replace(pos, trim_str.length(), asset_str);
    }

    // Create a path object to the asset file.
    std::filesystem::path asset_file_name = std::filesystem::path(potential_asset_name).filename();
    std::filesystem::path asset_file_path = asset_directory / asset_file_name;
    asset_file_path.replace_extension(".gfxa");

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
void AnalyzeDialog::SetReplayButton(const std::string &message, bool is_enabled)
{
    m_replay_button->setEnabled(is_enabled);
    m_replay_button->setText(message.c_str());
    QApplication::processEvents();
}

//--------------------------------------------------------------------------------------------------
std::filesystem::path AnalyzeDialog::GetFullLocalPath(const std::string &gfxr_stem,
                                                      const std::string &suffix) const
{
    if (m_local_capture_file_directory.empty())
    {
        return "";
    }

    std::filesystem::path full_path = m_local_capture_file_directory / (gfxr_stem + suffix);
    return full_path;
}

//--------------------------------------------------------------------------------------------------
absl::StatusOr<AnalyzeDialog::ReplayArtifactsPaths> AnalyzeDialog::GetReplayFilesLocalPaths(
const std::string &gfxr_stem) const
{
    ReplayArtifactsPaths artifacts = {};

    assert(!gfxr_stem.empty());

    if (m_local_capture_file_directory.empty())
    {
        return absl::FailedPreconditionError("m_local_capture_file_directory empty");
    }

    std::string gfxa_stem = gfxr_stem;
    size_t      rpos = gfxa_stem.find(Dive::kGfxrFileNameSubstr);
    if (pos == std::string::npos)
    {
        return absl::FailedPreconditionError(
        absl::StrFormat("unexpected name for gfxr file: %s, expecting name containing: %s",
                        gfxr_stem,
                        Dive::kGfxrFileNameSubstr));
    }
    int gfxr_string_length = sizeof(Dive::kGfxrFileNameSubstr) / sizeof(char);
    gfxa_stem.replace(pos, gfxr_string_length, Dive::kGfxaFileNameSubstr);

    artifacts.gfxr = GetFullLocalPath(gfxr_stem, Dive::kGfxrSuffix);
    artifacts.gfxa = GetFullLocalPath(gfxa_stem, Dive::kGfxaSuffix);
    artifacts.perf_counter_csv = GetFullLocalPath(gfxr_stem, Dive::kProfilingMetricsCsvSuffix);
    artifacts.gpu_timing_csv = GetFullLocalPath(gfxr_stem, Dive::kGpuTimingCsvSuffix);
    artifacts.pm4_rd = GetFullLocalPath(gfxr_stem, Dive::kPm4RdSuffix);
    artifacts.renderdoc_rdc = GetFullLocalPath(gfxr_stem, Dive::kRenderDocRdcSuffix);
    return artifacts;
}

//--------------------------------------------------------------------------------------------------
absl::Status AnalyzeDialog::NormalReplay(Dive::DeviceManager &device_manager,
                                         const std::string   &remote_gfxr_file)
{
    UpdateReplayStatus(ReplayStatusUpdateCode::kStartNormalReplay);
    Dive::GfxrReplaySettings replay_settings;
    replay_settings.remote_capture_path = remote_gfxr_file;
    replay_settings.local_download_dir = m_local_capture_file_directory.string();
    replay_settings.run_type = Dive::GfxrReplayOptions::kNormal;

    // Variant-specific config
    replay_settings.loop_single_frame_count = m_frame_count_box->value();

    return device_manager.RunReplayApk(replay_settings);
}

//--------------------------------------------------------------------------------------------------
absl::Status AnalyzeDialog::Pm4Replay(Dive::DeviceManager &device_manager,
                                      const std::string   &remote_gfxr_file)
{
    UpdateReplayStatus(ReplayStatusUpdateCode::kStartPm4Replay);
    Dive::GfxrReplaySettings replay_settings;
    replay_settings.remote_capture_path = remote_gfxr_file;
    replay_settings.local_download_dir = m_local_capture_file_directory.string();
    replay_settings.run_type = Dive::GfxrReplayOptions::kPm4Dump;

    return device_manager.RunReplayApk(replay_settings);
}

//--------------------------------------------------------------------------------------------------
absl::Status AnalyzeDialog::PerfCounterReplay(Dive::DeviceManager &device_manager,
                                              const std::string   &remote_gfxr_file)
{
    UpdateReplayStatus(ReplayStatusUpdateCode::kStartPerfCounterReplay);
    Dive::GfxrReplaySettings replay_settings;
    replay_settings.remote_capture_path = remote_gfxr_file;
    replay_settings.local_download_dir = m_local_capture_file_directory.string();
    replay_settings.run_type = Dive::GfxrReplayOptions::kPerfCounters;

    // Variant-specific config
    replay_settings.metrics = *m_enabled_metrics_vector;

    return device_manager.RunReplayApk(replay_settings);
}

//--------------------------------------------------------------------------------------------------
absl::Status AnalyzeDialog::GpuTimeReplay(Dive::DeviceManager &device_manager,
                                          const std::string   &remote_gfxr_file)
{
    UpdateReplayStatus(ReplayStatusUpdateCode::kStartGpuTimeReplay);
    Dive::GfxrReplaySettings replay_settings;
    replay_settings.remote_capture_path = remote_gfxr_file;
    replay_settings.local_download_dir = m_local_capture_file_directory.string();
    replay_settings.run_type = Dive::GfxrReplayOptions::kGpuTiming;

    // Variant-specific config
    replay_settings.loop_single_frame_count = m_frame_count_box->value();

    return device_manager.RunReplayApk(replay_settings);
}

//--------------------------------------------------------------------------------------------------
absl::Status AnalyzeDialog::RenderDocReplay(Dive::DeviceManager &device_manager,
                                            const std::string   &remote_gfxr_file)
{
    SetReplayButton("Replaying with RenderDoc...", false);
    Dive::GfxrReplaySettings replay_settings;
    replay_settings.remote_capture_path = remote_gfxr_file;
    replay_settings.local_download_dir = m_local_capture_file_directory.string();
    replay_settings.run_type = Dive::GfxrReplayOptions::kRenderDoc;

    // Variant-specific config
    // loop count will be set appropriately by ValidateGfxrReplaySettings

    return device_manager.RunReplayApk(replay_settings);
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::OnReplay()
{
    if (m_replay_active.valid())
    {
        return;
    }

    OverlayMessage("Replaying...");

    m_replay_active = std::async([=]() {
        ReplayImpl();
        UpdateReplayStatus(ReplayStatusUpdateCode::kDone);
    });
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::OnDeleteReplayArtifacts()
{
    if (m_replay_active.valid())
    {
        qDebug() << "Cannot call OnDeleteReplayArtifacts() during active replay";
        return;
    }

    m_replay_active = std::async([=]() {
        DeleteReplayArtifactsImpl();
        UpdateReplayStatus(ReplayStatusUpdateCode::kDone);
    });
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::OnReplayStatusUpdate(int status_code_int, const QString &error_message)
{
    // Cast from qt known type.
    auto status_code = static_cast<ReplayStatusUpdateCode>(status_code_int);
    bool execute_update = m_status_update_queue.empty();
    m_status_update_queue.push_back({ status_code, error_message });
    if (!execute_update)
    {
        // Only execute update if it's first call on the stack.
        // Qt does recursive signal processing sometime.
        return;
    }
    ExecuteStatusUpdate();
}
void AnalyzeDialog::ExecuteStatusUpdate()
{
    for (size_t index = 0; index < m_status_update_queue.size(); ++index)
    {
        // Note: copy the item to prevent iterator invalidation.
        StatusUpdateQueueItem item = m_status_update_queue[index];
        switch (item.status)
        {
        case ReplayStatusUpdateCode::kDone:
            if (m_replay_active.valid())
            {
                m_replay_active.get();
            }
            DisableOverlay();
            break;
        case ReplayStatusUpdateCode::kSuccess:
            SetReplayButton(kDefaultReplayButtonText, true);
            OverlayMessage("Replay done.");
            break;
        case ReplayStatusUpdateCode::kFailure:
            ShowErrorMessage(item.error_message.toStdString());
            SetReplayButton(kDefaultReplayButtonText, true);
            OverlayMessage("Replay failed.");
            break;
        case ReplayStatusUpdateCode::kSetup:
            SetReplayButton("Setting up replay...", false);
            OverlayMessage("Setting up replay...");
            break;
        case ReplayStatusUpdateCode::kSetupDeviceFailure:
            ShowErrorMessage(item.error_message.toStdString());
            SetReplayButton(kDefaultReplayButtonText, false);
            OnDeviceListRefresh();
            break;
        case ReplayStatusUpdateCode::kStartNormalReplay:
            SetReplayButton("Replaying...", false);
            OverlayMessage("Replaying...");
            break;
        case ReplayStatusUpdateCode::kStartPm4Replay:
            SetReplayButton("Replaying with PM4 dump enabled...", false);
            OverlayMessage("Replaying with PM4 dump enabled...");
            break;
        case ReplayStatusUpdateCode::kStartGpuTimeReplay:
            SetReplayButton("Replaying with GPU timing enabled...", false);
            OverlayMessage("Replaying with GPU timing enabled...");
            break;
        case ReplayStatusUpdateCode::kStartPerfCounterReplay:
            SetReplayButton("Replaying with perf counter settings...", false);
            OverlayMessage("Replaying with perf counter settings...");
            break;
        case ReplayStatusUpdateCode::kDeletingReplayArtifacts:
            OverlayMessage("Deleting temporary artifacts...");
            break;
        }
    }
    m_status_update_queue.clear();
}

void AnalyzeDialog::UpdateReplayStatus(ReplayStatusUpdateCode status,
                                       const std::string     &error_message)
{
    qDebug() << error_message.c_str();
    ReplayStatusUpdated(static_cast<int>(status), QString::fromStdString(error_message));
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::ReplayImpl()
{
    Dive::DeviceManager &device_manager = Dive::GetDeviceManager();
    auto                 device = device_manager.GetDevice();

    UpdateReplayStatus(ReplayStatusUpdateCode::kSetup);

    // Setup the device
    absl::Status ret = device->SetupDevice();
    if (!ret.ok())
    {
        std::string err_msg = absl::StrCat("Fail to setup device: ", ret.message());
        UpdateReplayStatus(ReplayStatusUpdateCode::kSetupDeviceFailure, err_msg);
        return;
    }

    // Get the asset file name
    absl::StatusOr<std::string> asset_file = GetAssetFile();
    if (!asset_file.ok())
    {
        std::string err_msg = absl::StrCat(asset_file.status().message());
        UpdateReplayStatus(ReplayStatusUpdateCode::kFailure, err_msg);
        return;
    }

    absl::StatusOr<std::string> remote_file = PushFilesToDevice(device, asset_file.value());
    if (!remote_file.ok())
    {
        std::string err_msg = absl::StrCat("Failed to deploy replay apk: ",
                                           remote_file.status().message());
        UpdateReplayStatus(ReplayStatusUpdateCode::kFailure, err_msg);
        return;
    }

    // Deploying install/gfxr-replay.apk
    ret = device_manager.DeployReplayApk(m_cur_device);
    if (!ret.ok())
    {
        std::string err_msg = absl::StrCat("Failed to push files to device: ", ret.message());
        UpdateReplayStatus(ReplayStatusUpdateCode::kFailure, err_msg);
        return;
    }

    // Set the download directory to the directory of the current capture file
    auto ret2 = GetCaptureFileDirectory();
    if (!ret2.ok())
    {
        std::string err_msg = absl::StrCat("Failed to set download directory: ",
                                           ret2.status().message());
        UpdateReplayStatus(ReplayStatusUpdateCode::kFailure, err_msg);
        return;
    }
    m_local_capture_file_directory = ret2.value();

    // Get names of temporary artifacts
    absl::StatusOr<ReplayArtifactsPaths> ret3 = GetReplayFilesLocalPaths(
    std::filesystem::path(remote_file.value()).stem().string());
    if (!ret3.ok())
    {
        std::string err_msg = absl::StrFormat("ReplayImpl() error: %s", ret3.status().message());
        qDebug() << err_msg.c_str();
        return;
    }
    ReplayArtifactsPaths local_artifacts = *ret3;

    // Get info on which variants of replay to initiate runs for
    bool dump_pm4_run_enabled = m_dump_pm4_box->isChecked();
    bool gpu_time_run_enabled = m_gpu_time_box->isChecked();
    bool renderdoc_run_enabled = m_renderdoc_capture_box->isChecked();
    bool perf_counter_run_enabled = !m_enabled_metrics_vector->empty();
    bool normal_run_enabled = (!dump_pm4_run_enabled) && (!gpu_time_run_enabled) &&
                              (!perf_counter_run_enabled) && (!renderdoc_run_enabled);

    // Run only replay with default settings
    if (normal_run_enabled)
    {
        ret = NormalReplay(device_manager, remote_file.value());
        if (!ret.ok())
        {
            std::string err_msg = absl::StrCat("Failed to run normal replay: ", ret.message());
            UpdateReplayStatus(ReplayStatusUpdateCode::kFailure, err_msg);
            return;
        }

        UpdateReplayStatus(ReplayStatusUpdateCode::kSuccess);
        // Reload the capture so the correct PM4 data (or absence thereof) is displayed
        emit LoadCapture(m_selected_capture_file_string);
        return;
    }

    // Run the pm4 replay
    if (dump_pm4_run_enabled)
    {
        ret = Pm4Replay(device_manager, remote_file.value());
        if (!ret.ok())
        {
            std::string err_msg = absl::StrCat("Failed to run pm4 replay: ", ret.message());
            UpdateReplayStatus(ReplayStatusUpdateCode::kFailure, err_msg);
            return;
        }
    }

    // Reload the capture so the correct PM4 data (or absence thereof) is displayed
    emit LoadCapture(m_selected_capture_file_string);

    // Run the perf counter replay
    if (perf_counter_run_enabled)
    {
        ret = PerfCounterReplay(device_manager, remote_file.value());
        if (!ret.ok())
        {
            std::string err_msg = absl::StrCat("Failed to run perf counter replay: ",
                                               ret.message());
            UpdateReplayStatus(ReplayStatusUpdateCode::kFailure, err_msg);
            return;
        }
    }

    // File could exist from previous runs
    if (std::filesystem::exists(local_artifacts.perf_counter_csv))
    {
        qDebug() << "Loading perf counter data: "
                 << local_artifacts.perf_counter_csv.string().c_str();
        emit DisplayPerfCounterResults(
        QString::fromStdString(local_artifacts.perf_counter_csv.string()));
    }
    else
    {
        qDebug() << "Cleared perf counter data";
        emit DisplayPerfCounterResults("");
    }

    // Run the gpu_time replay
    if (gpu_time_run_enabled)
    {
        ret = GpuTimeReplay(device_manager, remote_file.value());
        if (!ret.ok())
        {
            std::string err_msg = absl::StrCat("Failed to run gpu_time replay: ", ret.message());
            UpdateReplayStatus(ReplayStatusUpdateCode::kFailure, err_msg);
            return;
        }
    }

    // File could exist from previous runs
    if (std::filesystem::exists(local_artifacts.gpu_timing_csv))
    {
        qDebug() << "Loading gpu timing data: " << local_artifacts.gpu_timing_csv.string().c_str();
        emit DisplayGpuTimingResults(
        QString::fromStdString(local_artifacts.gpu_timing_csv.string()));
    }
    else
    {
        qDebug() << "Cleared gpu timing data";
        emit DisplayGpuTimingResults("");
    }

    if (renderdoc_run_enabled)
    {
        ret = RenderDocReplay(device_manager, remote_file.value());
        if (!ret.ok())
        {
            std::string err_msg = absl::StrCat("Failed to run replay with RenderDoc Capture: ",
                                               ret.message());
            UpdateReplayStatus(ReplayStatusUpdateCode::kFailure, err_msg);
            return;
        }
        qDebug() << "RenderDoc capture saved to: "
                 << local_artifacts.renderdoc_rdc.string().c_str();
    }

    UpdateReplayStatus(ReplayStatusUpdateCode::kSuccess);
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::DeleteReplayArtifactsImpl()
{
    qDebug() << "Attempting to delete replay artifacts from previous runs...";
    UpdateReplayStatus(ReplayStatusUpdateCode::kDeletingReplayArtifacts);

    absl::StatusOr<std::string> ret = GetCaptureFileDirectory();
    if (!ret.ok())
    {
        std::string err_msg = absl::StrCat("Failed to get local capture directory: ",
                                           ret.status().message());
        UpdateReplayStatus(ReplayStatusUpdateCode::kFailure, err_msg);
        qDebug() << "Failed to get local capture directory";
        return;
    }
    m_local_capture_file_directory = ret.value();

    std::filesystem::path                gfxr_parse = m_selected_capture_file_string.toStdString();
    absl::StatusOr<ReplayArtifactsPaths> ret2 = GetReplayFilesLocalPaths(
    gfxr_parse.stem().string());
    if (!ret2.ok())
    {
        std::string err_msg = absl::
        StrFormat("AnalyzeDialog::DeleteReplayArtifactsImpl(): cannot get local paths: %s",
                  ret2.status().message());
        qDebug() << err_msg.c_str();
        return;
    }
    ReplayArtifactsPaths local_artifacts = *ret2;

    if (local_artifacts.gfxr.empty())
    {
        qDebug() << "Could not obtain names for replay artifacts";
        return;
    }

    AttemptDeletingTemporaryLocalFile(local_artifacts.perf_counter_csv);
    AttemptDeletingTemporaryLocalFile(local_artifacts.gpu_timing_csv);
    AttemptDeletingTemporaryLocalFile(local_artifacts.pm4_rd);
}
