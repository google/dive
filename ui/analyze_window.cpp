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

    m_overlay = new OverlayHelper(this);

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
    m_selected_file_layout->addWidget(m_selected_file_label);
    m_selected_file_layout->addWidget(m_selected_file_input_box);

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
    m_main_layout = new QHBoxLayout();
    m_main_layout->addLayout(m_left_panel_layout);
    m_main_layout->addLayout(m_right_panel_layout);

    m_overlay->Initialize(m_main_layout);
    setLayout(m_overlay->GetLayout());

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
void AnalyzeDialog::OnOverlayMessage(const QString &message)
{
    m_overlay->SetMessage(message);
    m_overlay->SetMessageIsTimed();
}

void AnalyzeDialog::OnDisableOverlay()
{
    m_overlay->Clear();
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::ShowMessage(const std::string &message)
{
    auto message_box = new QMessageBox(this);
    message_box->setAttribute(Qt::WA_DeleteOnClose, true);
    message_box->setText(message.c_str());
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
        ShowMessage(err_msg);
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
void AnalyzeDialog::OnAnalyzeCaptureStarted(const QString &file_path)
{
    // Clear members for previous session
    OnAnalyzeCaptureEnded();

    // Validate capture
    if (file_path.isEmpty())
    {
        qDebug() << "OnAnalyzeCaptureStarted(): empty filename";
        return;
    }
    std::filesystem::path    local_gfxr_parse = file_path.toStdString();
    Dive::ComponentFilePaths component_paths = {};
    {
        absl::StatusOr<Dive::ComponentFilePaths>
        ret = Dive::GetComponentFilesHostPaths(local_gfxr_parse.parent_path(),
                                               local_gfxr_parse.stem().string());
        if (!ret.ok())
        {
            std::string err_msg = absl::
            StrFormat("OnAnalyzeCaptureStarted(): could not get component files: %s",
                      ret.status().message());
            qDebug() << err_msg.c_str();
            return;
        }
        component_paths = *ret;
    }
    if (!std::filesystem::exists(component_paths.gfxr))
    {
        qDebug() << "OnAnalyzeCaptureStarted(): gfxr trace does not exist: "
                 << component_paths.gfxr.string().c_str();
        return;
    }
    if (!std::filesystem::exists(component_paths.gfxa))
    {
        qDebug() << "OnAnalyzeCaptureStarted(): gfxa trace does not exist: "
                 << component_paths.gfxa.string().c_str();
        QString title = QString("Unable to open file: %1").arg(file_path);
        QString description = QString("Required .gfxa file: %1 not found!")
                              .arg(QString::fromStdString(component_paths.gfxa.string()));
        QMessageBox::critical(this, title, description);
        return;
    }

    // Set members for current replay session
    assert(local_gfxr_parse == component_paths.gfxr);
    m_selected_capture_file_string = QString::fromStdString(component_paths.gfxr.generic_string());
    m_local_capture_file_directory = local_gfxr_parse.parent_path();
    m_local_capture_files = component_paths;

    // Update display and settings
    m_selected_file_input_box->setText(m_selected_capture_file_string);
    QString last_file_path = QString::fromStdString(
    local_gfxr_parse.parent_path().generic_string());
    Settings::Get()->WriteLastFilePath(last_file_path);

    // Open the dialog for users to initiate analysis
    open();
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::OnAnalyzeCaptureEnded()
{
    // Clear members for current analyze session
    m_selected_capture_file_string = "";
    m_local_capture_file_directory = "";
    m_local_capture_files = {};
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
    device->Adb().Run(absl::StrFormat(R"(push "%s" "%s")", local_gfxr_path, remote_gfxr_path)));

    // Push the .gfxa file.
    std::filesystem::path asset_file_path(local_asset_file_path);
    std::string           asset_file_name = asset_file_path.filename().string();
    RETURN_IF_ERROR(device->Adb().Run(
    absl::StrFormat(R"(push "%s" "%s/%s")", local_asset_file_path, remote_dir, asset_file_name)));

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

    m_replay_active = std::async([=, this]() {
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

    m_replay_active = std::async([=, this]() {
        DeleteReplayArtifactsImpl();
        UpdateReplayStatus(ReplayStatusUpdateCode::kDone);
    });
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::OnReplayStatusUpdate(int status_code_int, const QString &message)
{
    // Cast from qt known type.
    auto status_code = static_cast<ReplayStatusUpdateCode>(status_code_int);
    bool execute_update = m_status_update_queue.empty();
    m_status_update_queue.push_back({ status_code, message });
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
            ShowMessage(item.message.toStdString());
            SetReplayButton(kDefaultReplayButtonText, true);
            OverlayMessage("Replay done.");
            break;
        case ReplayStatusUpdateCode::kFailure:
            ShowMessage(item.message.toStdString());
            SetReplayButton(kDefaultReplayButtonText, true);
            OverlayMessage("Replay failed.");
            break;
        case ReplayStatusUpdateCode::kSetup:
            SetReplayButton("Setting up replay...", false);
            OverlayMessage("Setting up replay...");
            break;
        case ReplayStatusUpdateCode::kSetupDeviceFailure:
            ShowMessage(item.message.toStdString());
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

void AnalyzeDialog::UpdateReplayStatus(ReplayStatusUpdateCode status, const std::string &message)
{
    qDebug() << message.c_str();
    ReplayStatusUpdated(static_cast<int>(status), QString::fromStdString(message));
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
    absl::StatusOr<std::string>
    remote_file = PushFilesToDevice(device, m_local_capture_files.gfxa.generic_string());
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
        // MainWindow needs to reload the capture so the correct PM4 data (or absence thereof) is
        // displayed
        emit CaptureUpdated(m_selected_capture_file_string);
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
        // MainWindow needs to reload the capture so the correct PM4 data (or absence thereof) is
        // displayed
        emit CaptureUpdated(m_selected_capture_file_string);
    }

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
    if (std::filesystem::exists(m_local_capture_files.perf_counter_csv))
    {
        qDebug() << "Loading perf counter data: "
                 << m_local_capture_files.perf_counter_csv.string().c_str();
        emit DisplayPerfCounterResults(
        QString::fromStdString(m_local_capture_files.perf_counter_csv.string()));
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
    if (std::filesystem::exists(m_local_capture_files.gpu_timing_csv))
    {
        qDebug() << "Loading gpu timing data: "
                 << m_local_capture_files.gpu_timing_csv.string().c_str();
        emit DisplayGpuTimingResults(
        QString::fromStdString(m_local_capture_files.gpu_timing_csv.string()));
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
                 << m_local_capture_files.renderdoc_rdc.string().c_str();
    }

    UpdateReplayStatus(ReplayStatusUpdateCode::kSuccess, "Replay completed successfully.");
}

//--------------------------------------------------------------------------------------------------
void AnalyzeDialog::DeleteReplayArtifactsImpl()
{
    qDebug() << "Attempting to delete replay artifacts from previous runs...";
    UpdateReplayStatus(ReplayStatusUpdateCode::kDeletingReplayArtifacts);

    AttemptDeletingTemporaryLocalFile(m_local_capture_files.perf_counter_csv);
    AttemptDeletingTemporaryLocalFile(m_local_capture_files.gpu_timing_csv);
    AttemptDeletingTemporaryLocalFile(m_local_capture_files.pm4_rd);
}
