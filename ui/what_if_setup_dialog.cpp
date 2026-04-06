/*
 Copyright 2026 Google LLC
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

#include "what_if_setup_dialog.h"

#include <qboxlayout.h>

#include <QButtonGroup>
#include <QCloseEvent>
#include <QComboBox>
#include <QCompleter>
#include <QDebug>
#include <QFrame>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSortFilterProxyModel>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <string>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "application_controller.h"
#include "capture_service/device_mgr.h"
#include "device_dialog.h"
#include "dive/common/app_types.h"
#include "network/drawcall_filter_config.h"

namespace
{

constexpr size_t kNumWhatIfAppTypes =
    std::count_if(Dive::kAppTypeInfos.begin(), Dive::kAppTypeInfos.end(),
                  [](const Dive::AppTypeInfo& info) { return info.is_what_if_supported; });
constexpr int kRuntimeWhatIfButtonId = 1;
constexpr int kReplayWhatIfButtonId = 2;

constexpr std::string_view kStartApplication = "&Start Application";
constexpr std::string_view kStopApplication = "&Stop Application";
constexpr std::string_view kEndSession = "&End Optimization Session";
constexpr std::string_view kAddModifications = "&Add Modifications";
constexpr std::string_view kDeleteModifications = "&Delete Modifications";
constexpr std::string_view kTestModifications = "&Test Modifications";
}  // namespace

// =================================================================================================
// WhatIfAppTypeFilterModel
// =================================================================================================

void WhatIfAppTypeFilterModel::setFilterActive(bool active)
{
    m_filter_active = active;
    invalidateFilter();
}

//--------------------------------------------------------------------------------------------------
bool WhatIfAppTypeFilterModel::filterAcceptsRow(int sourceRow,
                                                const QModelIndex& sourceParent) const
{
    if (m_filter_active)
    {
        return sourceRow < static_cast<int>(kNumWhatIfAppTypes);
    }
    return true;
}

// =================================================================================================
// WhatIfSetupDialog
// =================================================================================================
WhatIfSetupDialog::WhatIfSetupDialog(QWidget* parent) : DeviceDialog(parent)
{
    qDebug() << "WhatIfSetupDialog created.";

    setWindowTitle("What-Ifs");

    QHBoxLayout* root_layout = new QHBoxLayout(this);

    QVBoxLayout* left_layout = new QVBoxLayout();

    // --- Header Section ---
    left_layout->addLayout(CreateHeaderLayout());
    left_layout->addSpacing(15);

    // --- Radio Button Group Section ---
    left_layout->addLayout(CreateRadioButtonLayout());
    left_layout->addSpacing(15);

    // --- Options Section ---
    InitializeRuntimeOptions();
    left_layout->addWidget(m_runtime_options_widget);
    InitializeReplayOptions();
    left_layout->addWidget(m_replay_options_widget);

    // --- Button Section ---
    left_layout->addStretch();
    left_layout->addLayout(CreateButtonLayout());

    // --- Modification List Section ---
    QVBoxLayout* right_layout = InitializeModificationListOptions();

    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::VLine);
    separator->setFrameShadow(QFrame::Sunken);

    // Add both sections to the root layout
    root_layout->addLayout(left_layout, 1);
    root_layout->addWidget(separator);
    root_layout->addLayout(right_layout, 1);

    SetupConnections();
}

void WhatIfSetupDialog::SetupConnections()
{
    QObject::connect(m_what_if_type_button_group, QOverload<int>::of(&QButtonGroup::buttonClicked),
                     this, &WhatIfSetupDialog::OnWhatIfTypeChanged);
    QObject::connect(m_device_box, SIGNAL(currentIndexChanged(const QString&)), this,
                     SLOT(OnDeviceSelectionChanged(const QString&)));
    QObject::connect(m_device_refresh_button, &QPushButton::clicked, this,
                     &WhatIfSetupDialog::OnDevListRefresh);
    QObject::connect(m_pkg_box, SIGNAL(currentIndexChanged(const QString&)), this,
                     SLOT(OnPackageSelected(const QString&)));
    QObject::connect(m_pkg_box->lineEdit(), &QLineEdit::textEdited, m_filter_model,
                     &QSortFilterProxyModel::setFilterFixedString);
    QObject::connect(m_args_input_box, &QLineEdit::textEdited, this,
                     &WhatIfSetupDialog::OnInputArgs);
    QObject::connect(m_start_application_button, &QPushButton::clicked, this,
                     &WhatIfSetupDialog::OnStartClicked);
    QObject::connect(m_end_session_button, &QPushButton::clicked, this, &QWidget::close);
    QObject::connect(m_add_modification_button, &QPushButton::clicked, this,
                     &WhatIfSetupDialog::AddModification);
    QObject::connect(m_test_modification_button, &QPushButton::clicked, this,
                     &WhatIfSetupDialog::OnTestModifications);
    QObject::connect(m_delete_modification_button, &QPushButton::clicked, this,
                     &WhatIfSetupDialog::OnDeleteModifications);
}

QVBoxLayout* WhatIfSetupDialog::InitializeModificationListOptions()
{
    QVBoxLayout* layout = new QVBoxLayout();

    QHBoxLayout* header_layout = new QHBoxLayout();
    QLabel* mod_list_title = new QLabel(tr("Modification List"));
    QFont mod_title_font = mod_list_title->font();
    mod_title_font.setBold(true);
    mod_title_font.setPointSize(mod_title_font.pointSize() + 2);
    mod_list_title->setFont(mod_title_font);

    header_layout->addWidget(mod_list_title);
    header_layout->addStretch();

    QLabel* mod_list_subtitle =
        new QLabel(tr("List of currently modifications that may impact the rendering of the "
                      "currently running application"));
    mod_list_subtitle->setWordWrap(true);

    m_add_modification_button = new QPushButton(tr(kAddModifications.data()));
    m_add_modification_button->setEnabled(false);

    m_modification_list_view = new QListView();
    m_modification_list_model = new QStandardItemModel(this);
    m_modification_list_view->setModel(m_modification_list_model);

    QHBoxLayout* bottom_layout = new QHBoxLayout();
    bottom_layout->addStretch();
    m_delete_modification_button = new QPushButton(tr(kDeleteModifications.data()));
    m_delete_modification_button->setEnabled(false);
    m_test_modification_button = new QPushButton(tr(kTestModifications.data()));
    m_test_modification_button->setEnabled(false);
    bottom_layout->addWidget(m_delete_modification_button);
    bottom_layout->addWidget(m_test_modification_button);
    bottom_layout->addStretch();

    layout->addLayout(header_layout);
    layout->addWidget(mod_list_subtitle);
    layout->addWidget(m_add_modification_button, 0, Qt::AlignRight);
    layout->addWidget(m_modification_list_view);
    layout->addLayout(bottom_layout);

    return layout;
}

QVBoxLayout* WhatIfSetupDialog::CreateHeaderLayout()
{
    QVBoxLayout* layout = new QVBoxLayout();

    // --- Font Definition ---
    QFont title_font = this->font();
    title_font.setBold(true);
    title_font.setPointSize(title_font.pointSize() + 2);

    QLabel* title_label = new QLabel(tr("Explore What-If Scenarios"));
    title_label->setFont(title_font);
    QLabel* info_label =
        new QLabel(tr("Modify vulkan calls, shaders, etc. to explore the potential impact "
                      "rendering the application."));
    info_label->setWordWrap(true);

    layout->addWidget(title_label);
    layout->addWidget(info_label);
    return layout;
}

QVBoxLayout* WhatIfSetupDialog::CreateRadioButtonLayout()
{
    QVBoxLayout* layout = new QVBoxLayout();
    // --- Radio Button Group Section ---
    m_what_if_type_button_group = new QButtonGroup(this);

    m_runtime_what_if_type_button = new QRadioButton(tr("In a Running Application"));
    m_runtime_what_if_type_button->setChecked(true);
    QLabel* runtime_what_if_type_label =
        new QLabel(tr("This provides fewer options, but allows the running application to be "
                      "controlled interactively to get an overall feeling for performance."));
    runtime_what_if_type_label->setWordWrap(true);
    runtime_what_if_type_label->setContentsMargins(25, 0, 0, 10);

    m_replay_what_if_type_button = new QRadioButton(tr("For an existing Capture"));
    m_replay_what_if_type_button->setEnabled(false);
    QLabel* replay_what_if_type_label = new QLabel(
        tr("This provides more options, but analyzes the effects on a single frame capture."));
    replay_what_if_type_label->setWordWrap(true);
    replay_what_if_type_label->setContentsMargins(25, 0, 0, 10);

    m_what_if_type_button_group->addButton(m_runtime_what_if_type_button, kRuntimeWhatIfButtonId);
    m_what_if_type_button_group->addButton(m_replay_what_if_type_button, kReplayWhatIfButtonId);

    layout->addWidget(m_runtime_what_if_type_button);
    layout->addWidget(runtime_what_if_type_label);
    layout->addWidget(m_replay_what_if_type_button);
    layout->addWidget(replay_what_if_type_label);
    return layout;
}

void WhatIfSetupDialog::InitializeRuntimeOptions()
{
    m_runtime_options_widget = new QWidget(this);
    // --- Grid Section ---
    QGridLayout* settings_grid = new QGridLayout(m_runtime_options_widget);
    settings_grid->setContentsMargins(0, 0, 0, 0);
    settings_grid->setColumnStretch(1, 1);  // Allows the input boxes to expand

    // Device Selection
    QLabel* device_label = new QLabel(tr("Device:"));
    m_device_box = new QComboBox();
    m_device_box->setModel(m_device_model);
    m_device_refresh_button = new QPushButton(tr("&Refresh"));
    settings_grid->addWidget(device_label, 0, 0, Qt::AlignRight);
    settings_grid->addWidget(m_device_box, 0, 1);
    settings_grid->addWidget(m_device_refresh_button, 0, 2);

    // Package Selection
    m_pkg_model = new QStandardItemModel();
    m_runtime_data.pkg_list_options = Dive::AndroidDevice::PackageListOptions::kDebuggableOnly;
    QLabel* pkg_label = new QLabel(tr("Packages:"));
    m_pkg_box = new QComboBox();
    m_pkg_box->setModel(m_pkg_model);
    m_pkg_box->setEditable(true);
    m_filter_model = new QSortFilterProxyModel(m_pkg_box);
    m_filter_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_filter_model->setSourceModel(m_pkg_box->model());
    QCompleter* completer = new QCompleter(m_filter_model, m_pkg_box);
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    m_pkg_box->setCompleter(completer);
    m_pkg_refresh_button = new QPushButton(tr("&Refresh"));
    settings_grid->addWidget(pkg_label, 1, 0, Qt::AlignRight);
    settings_grid->addWidget(m_pkg_box, 1, 1);
    settings_grid->addWidget(m_pkg_refresh_button, 1, 2);

    // Additional Args
    QLabel* args_label = new QLabel(tr("Additional Args:"));
    m_args_input_box = new QLineEdit();
    settings_grid->addWidget(args_label, 2, 0, Qt::AlignRight);
    settings_grid->addWidget(m_args_input_box, 2, 1, 1, 2);

    // App Type Selection
    QStandardItemModel* m_app_type_model = new QStandardItemModel();
    for (const auto& ty : Dive::kAppTypeInfos)
    {
        QStandardItem* item = new QStandardItem(ty.ui_name.data());
        m_app_type_model->appendRow(item);
    }
    m_app_type_filter_model = new WhatIfAppTypeFilterModel(this);
    m_app_type_filter_model->setSourceModel(m_app_type_model);
    m_app_type_filter_model->setFilterActive(true);
    QLabel* app_type_label = new QLabel(tr("Application Type:"));
    m_app_type_box = new QComboBox();
    m_app_type_box->setModel(m_app_type_filter_model);
    settings_grid->addWidget(app_type_label, 3, 0, Qt::AlignRight);
    settings_grid->addWidget(m_app_type_box, 3, 1, 1, 2);
}

void WhatIfSetupDialog::InitializeReplayOptions() { m_replay_options_widget = new QWidget(this); }

QHBoxLayout* WhatIfSetupDialog::CreateButtonLayout()
{
    QHBoxLayout* button_layout = new QHBoxLayout();
    m_end_session_button = new QPushButton(tr(kEndSession.data()), this);
    m_start_application_button = new QPushButton(kStartApplication.data(), this);
    m_start_application_button->setEnabled(false);
    button_layout->addWidget(m_end_session_button);
    button_layout->addWidget(m_start_application_button);

    return button_layout;
}

WhatIfSetupDialog::~WhatIfSetupDialog()
{
    qDebug() << "WhatIfSetupDialog destroyed.";
    Dive::GetDeviceManager().RemoveDevice();
}

void WhatIfSetupDialog::EnableWhatIfTypeButtons(bool enable)
{
    m_runtime_what_if_type_button->setEnabled(enable);
}

void WhatIfSetupDialog::EnableModificationOptions(bool enable)
{
    m_add_modification_button->setEnabled(enable);
    m_delete_modification_button->setEnabled(enable);
    m_test_modification_button->setEnabled(enable);
}

void WhatIfSetupDialog::UpdatePackageList()
{
    Dive::AndroidDevice* device = Dive::GetDeviceManager().GetDevice();
    if (device == nullptr)
    {
        return;
    }

    absl::StatusOr<std::vector<std::string>> package_list =
        device->ListPackage(m_runtime_data.pkg_list_options);

    if (!package_list.ok())
    {
        std::string device_serial = GetCurrentDeviceSerial();
        std::string err_msg = absl::StrFormat("Failed to list package for device %s, error: %s",
                                              device_serial, package_list.status().message());
        qDebug() << err_msg.c_str();
        ShowMessage(QString::fromStdString(err_msg));
        return;
    }

    m_runtime_data.pkg_list.clear();
    for (const auto& pkg : *package_list)
    {
        m_runtime_data.pkg_list.append(QString::fromStdString(pkg));
    }

    const QSignalBlocker blocker(
        m_pkg_box);  // Do not emit index changed event when update the model
    m_pkg_model->clear();
    for (const QString& pkg : m_runtime_data.pkg_list)
    {
        QStandardItem* item = new QStandardItem(pkg);
        m_pkg_model->appendRow(item);
    }
    m_pkg_box->setCurrentIndex(-1);
    m_pkg_refresh_button->setDisabled(false);
}

void WhatIfSetupDialog::ShowMessage(const QString& message)
{
    QMessageBox* message_box = new QMessageBox(this);
    message_box->setAttribute(Qt::WA_DeleteOnClose, true);
    message_box->setText(message);
    message_box->open();
}

void WhatIfSetupDialog::OnDeviceSelected() { UpdatePackageList(); }

void WhatIfSetupDialog::OnDeviceSelectionCleared()
{
    m_start_application_button->setEnabled(false);
}

void WhatIfSetupDialog::ResetDialog()
{
    // Reset the app type to the default Vulkan (OpenXR)
    m_app_type_box->setCurrentIndex(0);
    m_args_input_box->clear();
    m_runtime_data.cur_pkg.clear();
    m_pkg_box->setCurrentIndex(-1);
    m_pkg_model->clear();
    m_start_application_button->setEnabled(false);
    m_start_application_button->setText(kStartApplication.data());
    EnableModificationOptions(false);

    EnableWhatIfTypeButtons(true);
    m_tcp_client.reset();
}

absl::Status WhatIfSetupDialog::StartPackage(Dive::AndroidDevice* device,
                                             const std::string& app_type)
{
    if (device == nullptr)
    {
        return absl::InternalError("Device is null");
    }

    device->CleanupApp().IgnoreError();
    m_start_application_button->setText("&Starting..");
    m_start_application_button->setDisabled(true);
    EnableWhatIfTypeButtons(false);

    absl::Status setup_app_res;
    std::string device_serial = GetCurrentDeviceSerial();
    qDebug() << "Start app on dev: " << device_serial.c_str()
             << ", package: " << m_runtime_data.cur_pkg.toStdString().c_str()
             << ", type: " << app_type.c_str() << ", args: " << m_runtime_data.command_args.c_str();

    if (app_type ==
        Dive::kAppTypeInfos[static_cast<size_t>(Dive::AppType::kVulkan_OpenXR)].ui_name.data())
    {
        setup_app_res =
            device->SetupApp(m_runtime_data.cur_pkg.toStdString(),
                             Dive::ApplicationType::OPENXR_APK, m_runtime_data.command_args,
                             /*gfxr_capture_directory*/ "");
    }
    else if (app_type == Dive::kAppTypeInfos[static_cast<size_t>(Dive::AppType::kVulkan_Non_OpenXR)]
                             .ui_name.data())
    {
        setup_app_res =
            device->SetupApp(m_runtime_data.cur_pkg.toStdString(),
                             Dive::ApplicationType::VULKAN_APK, m_runtime_data.command_args,
                             /*gfxr_capture_directory*/ "");
    }
    else if (app_type ==
             Dive::kAppTypeInfos[static_cast<size_t>(Dive::AppType::kVulkanCLI_Non_OpenXR)]
                 .ui_name.data())
    {
        if (m_runtime_data.cur_pkg.isEmpty())
        {
            return absl::InvalidArgumentError("Please input a valid package to run");
        }
        qDebug() << "exe: " << m_runtime_data.cur_pkg.toStdString().c_str()
                 << " args: " << m_runtime_data.command_args.c_str();
        setup_app_res =
            device->SetupApp(m_runtime_data.cur_pkg.toStdString(), m_runtime_data.command_args,
                             Dive::ApplicationType::VULKAN_CLI,
                             /*gfxr_capture_directory*/ "");
    }
    if (!setup_app_res.ok())
    {
        return absl::Status(
            setup_app_res.code(),
            absl::StrFormat("Fail to setup for package %s, error: %s",
                            m_runtime_data.cur_pkg.toStdString(), setup_app_res.message()));
    }
    if (absl::Status start_app_res = device->StartApp(); !start_app_res.ok())
    {
        return absl::Status(
            start_app_res.code(),
            absl::StrFormat("Fail to start package %s, error: %s",
                            m_runtime_data.cur_pkg.toStdString(), start_app_res.message()));
    }

    Dive::AndroidApplication* cur_app = device->GetCurrentApplication();

    if (!cur_app)
    {
        return absl::InternalError(
            absl::StrFormat("Failed to get current application for package %s",
                            m_runtime_data.cur_pkg.toStdString()));
    }

    if (!cur_app->IsRunning())
    {
        return absl::InternalError(
            absl::StrFormat("Process for package %s not found, possibly crashed.",
                            m_runtime_data.cur_pkg.toStdString()));
    }

    m_start_application_button->setDisabled(false);
    m_start_application_button->setText(kStopApplication.data());

    return absl::OkStatus();
}

absl::Status WhatIfSetupDialog::StopPackage()
{
    Dive::AndroidDevice* device = Dive::GetDeviceManager().GetDevice();
    Dive::AndroidApplication* cur_app = device ? device->GetCurrentApplication() : nullptr;

    if (!device || !cur_app || !cur_app->IsRunning())
    {
        ResetDialog();
        return absl::OkStatus();
    }

    device->StopApp().IgnoreError();

    absl::Status status = cur_app->Cleanup();
    if (!status.ok())
    {
        qDebug() << "Failed to cleanup application: " << status.ToString().c_str();
        ShowMessage(QString::fromStdString(
            absl::StrFormat("Failed to cleanup application: %s", status.message())));

        // Exit without resetting UI if it's a specific precondition failure
        if (status.code() == absl::StatusCode::kFailedPrecondition)
        {
            return status;
        }
    }

    ResetDialog();
    return status;
}

void WhatIfSetupDialog::OnAppListRefresh() { UpdatePackageList(); }

void WhatIfSetupDialog::OnDevListRefresh() { UpdateDeviceList(); }

void WhatIfSetupDialog::OnInputArgs(const QString& text)
{
    qDebug() << "Args changed to " << text;
    m_runtime_data.command_args = text.toStdString();
}

void WhatIfSetupDialog::OnPackageSelected(const QString& s)
{
    int cur_index = m_pkg_box->currentIndex();
    qDebug() << "Package selected: " << s << ", index: " << cur_index;
    if ((s.isEmpty() || cur_index == -1))
    {
        return;
    }
    if (m_runtime_data.cur_pkg != m_runtime_data.pkg_list[cur_index])
    {
        m_runtime_data.cur_pkg = m_runtime_data.pkg_list[cur_index];
        qDebug() << "Current package set to: " << m_runtime_data.cur_pkg.toStdString().c_str();
    }
    m_start_application_button->setEnabled(true);
}

void WhatIfSetupDialog::OnStartClicked()
{
    Dive::AndroidDevice* device = Dive::GetDeviceManager().GetDevice();
    if (!device)
    {
        std::string err_msg =
            "No device/application selected. Please select a device and application and "
            "then try again.";
        ShowMessage(QString::fromStdString(err_msg));
        return;
    }

    device->EnableRuntimeWhatIf(/*enable_runtime_what_if=*/true);
    if (absl::Status ret = device->SetupDevice(); !ret.ok())
    {
        std::string err_msg = absl::StrFormat("Fail to setup device: %s", ret.message());
        qDebug() << err_msg.c_str();
        ShowMessage(QString::fromStdString(err_msg));
        return;
    }

    int proxy_index = m_app_type_box->currentIndex();
    if (proxy_index == -1)
    {
        ShowMessage(QString("Please select application type"));
        return;
    }

    QModelIndex proxyModelIndex = m_app_type_filter_model->index(proxy_index, 0);
    QModelIndex sourceModelIndex = m_app_type_filter_model->mapToSource(proxyModelIndex);
    int source_row = sourceModelIndex.row();

    std::string ty_str = Dive::kAppTypeInfos[source_row].ui_name.data();
    if (m_start_application_button->text() != kStartApplication.data())
    {
        qDebug() << "Stop Application clicked for package: "
                 << m_runtime_data.cur_pkg.toStdString().c_str();
        absl::Status status = StopPackage();
        if (!status.ok())
        {
            qDebug() << std::string(status.message()).c_str();
        }
        return;
    }

    qDebug() << "Start Application clicked with package: "
             << m_runtime_data.cur_pkg.toStdString().c_str() << ", type: " << ty_str.c_str();
    absl::Status status = StartPackage(device, ty_str);
    if (!status.ok())
    {
        qDebug() << std::string(status.message()).c_str();
        ShowMessage(QString::fromStdString(std::string(status.message())));
        m_start_application_button->setDisabled(false);
        m_start_application_button->setText(kStartApplication.data());
        EnableWhatIfTypeButtons(true);
        return;
    }

    EnableModificationOptions(true);
}

void WhatIfSetupDialog::OnWhatIfTypeChanged(int button_id)
{
    if (button_id == kRuntimeWhatIfButtonId)
    {
        m_runtime_options_widget->setVisible(true);
        m_replay_options_widget->setVisible(false);
    }
    else
    {
        m_runtime_options_widget->setVisible(false);
        m_replay_options_widget->setVisible(true);
    }
}

void WhatIfSetupDialog::closeEvent(QCloseEvent* event)
{
    absl::Status status = StopPackage();
    if (status.code() == absl::StatusCode::kFailedPrecondition)
    {
        // Prevent the dialog from closing so the user can read the error message
        event->ignore();
        return;
    }

    if (m_modification_list_model)
    {
        m_modification_list_model->clear();
    }

    m_runtime_what_if_type_button->setChecked(true);
    OnWhatIfTypeChanged(kRuntimeWhatIfButtonId);

    QDialog::closeEvent(event);
}

void WhatIfSetupDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);

    if (std::string current_device_serial = GetCurrentDeviceSerial(); current_device_serial.empty())
    {
        ResetDialog();
        return;
    }

    m_start_application_button->setEnabled(!m_runtime_data.cur_pkg.isEmpty());
}

void WhatIfSetupDialog::OnAddModificationToList(const Dive::WhatIfModification& modification)
{
    if (m_modification_list_model)
    {
        QStandardItem* item = new QStandardItem(modification.ui_text);
        item->setCheckable(true);
        item->setCheckState(Qt::Checked);
        item->setData(QVariant::fromValue(modification), Qt::UserRole + 1);
        m_modification_list_model->insertRow(0, item);
    }
}

Network::TcpClient* WhatIfSetupDialog::GetConnectedTcpClient()
{
    Dive::AndroidDevice* device = Dive::GetDeviceManager().GetDevice();
    if (!device)
    {
        qDebug() << "GetConnectedTcpClient: No device connected.";
        ShowMessage(tr("Cannot connect to TCP server: No device connected."));
        return nullptr;
    }

    if (!m_tcp_client)
    {
        qDebug() << "GetConnectedTcpClient: TCP client not initialized. Creating new instance.";
        m_tcp_client = std::make_unique<Network::TcpClient>();
    }

    if (!m_tcp_client->IsConnected())
    {
        const std::string host = "127.0.0.1";
        int port = device->Port();
        qDebug() << "GetConnectedTcpClient: Client not connected. Attempting to connect to"
                 << host.c_str() << ":" << port;
        auto status = m_tcp_client->Connect(host, port);
        if (!status.ok())
        {
            std::string err_msg = absl::StrFormat(
                "Failed to connect to the application's TCP server at %s:%d.Error: %s", host, port,
                status.message());
            qDebug() << "GetConnectedTcpClient:" << err_msg.c_str();
            ShowMessage(QString::fromStdString(err_msg));
            m_tcp_client.reset();
            return nullptr;
        }
        qDebug() << "GetConnectedTcpClient: Successfully connected to" << host.c_str() << ":"
                 << port;
    }
    else
    {
        qDebug() << "GetConnectedTcpClient: TCP client is already connected.";
    }
    return m_tcp_client.get();
}

absl::Status WhatIfSetupDialog::SyncActiveModifications()
{
    Dive::AndroidDevice* device = Dive::GetDeviceManager().GetDevice();
    if (!device)
    {
        return absl::FailedPreconditionError("No device connected.");
    }

    Dive::AndroidApplication* cur_app = device->GetCurrentApplication();
    if (!cur_app || !cur_app->IsRunning())
    {
        return absl::FailedPreconditionError("Application is not running.");
    }

    Network::TcpClient* tcp_client = GetConnectedTcpClient();
    if (!tcp_client)
    {
        // Return an empty CancelledError to indicate the error was already handled/logged
        return absl::CancelledError("");
    }

    Network::DrawcallFilterConfig drawcall_config;

    if (m_modification_list_model)
    {
        for (int i = 0; i < m_modification_list_model->rowCount(); i++)
        {
            QStandardItem* item =
                m_modification_list_model->itemFromIndex(m_modification_list_model->index(i, 0));
            if (item && item->checkState() == Qt::Checked)
            {
                Dive::WhatIfModification modification =
                    item->data(Qt::UserRole + 1).value<Dive::WhatIfModification>();
                switch (modification.type)
                {
                    case Dive::WhatIfType::kDrawCallDisabled:
                    {
                        if (modification.draw_call_filters.has_value())
                        {
                            if (modification.draw_call_filters->vertex_count > 0)
                            {
                                drawcall_config.filter_by_vertex_count = true;
                                drawcall_config.target_vertex_count =
                                    modification.draw_call_filters->vertex_count;
                            }
                            if (modification.draw_call_filters->index_count > 0)
                            {
                                drawcall_config.filter_by_index_count = true;
                                drawcall_config.target_index_count =
                                    modification.draw_call_filters->index_count;
                            }
                            if (modification.draw_call_filters->instance_count > 0)
                            {
                                drawcall_config.filter_by_instance_count = true;
                                drawcall_config.target_instance_count =
                                    modification.draw_call_filters->instance_count;
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }

    absl::Status send_draw_call_filtering_status =
        tcp_client->SendDrawcallFilterConfig(drawcall_config);
    if (!send_draw_call_filtering_status.ok())
    {
        std::string err_msg(send_draw_call_filtering_status.message());
        qDebug() << "Failed to send draw call filters: " << err_msg.c_str();
        return send_draw_call_filtering_status;
    }

    return absl::OkStatus();
}

void WhatIfSetupDialog::OnTestModifications()
{
    Dive::AndroidDevice* device = Dive::GetDeviceManager().GetDevice();
    if (!device)
    {
        ShowMessage(QString("No device connected."));
        ResetDialog();
        return;
    }

    Dive::AndroidApplication* cur_app = device->GetCurrentApplication();
    if (!cur_app || !cur_app->IsRunning())
    {
        ShowMessage(
            QString("Application is not running. Please start the application before testing "
                    "modifications."));
        ResetDialog();
        return;
    }

    bool any_item_selected = false;
    if (m_modification_list_model)
    {
        for (int i = 0; i < m_modification_list_model->rowCount(); ++i)
        {
            QModelIndex index = m_modification_list_model->index(i, 0);
            QStandardItem* item = m_modification_list_model->itemFromIndex(index);
            if (item && item->checkState() == Qt::Checked)
            {
                any_item_selected = true;
                break;
            }
        }
    }

    if (!any_item_selected)
    {
        ShowMessage("Please check the modification(s) you wish to test.");
        return;
    }

    absl::Status status = SyncActiveModifications();
    if (!status.ok())
    {
        if (!status.message().empty())
        {
            ShowMessage(QString::fromStdString(std::string(status.message())));
        }
        return;
    }

    ShowMessage(QString("Modification(s) successfully applied."));
}

void WhatIfSetupDialog::OnDeleteModifications()
{
    Dive::AndroidDevice* device = Dive::GetDeviceManager().GetDevice();
    if (!device)
    {
        ShowMessage(QString("No device connected."));
        ResetDialog();
        return;
    }

    Dive::AndroidApplication* cur_app = device->GetCurrentApplication();
    if (!cur_app || !cur_app->IsRunning())
    {
        ShowMessage(
            QString("Application is not running. Please start the application before deleting "
                    "modifications."));
        ResetDialog();
        return;
    }

    if (!m_modification_list_model || !m_modification_list_view)
    {
        return;
    }

    std::vector<int> rows_to_delete;

    for (int i = m_modification_list_model->rowCount() - 1; i >= 0; --i)
    {
        QModelIndex index = m_modification_list_model->index(i, 0);
        QStandardItem* item = m_modification_list_model->itemFromIndex(index);
        if (item && item->checkState() == Qt::Checked)
        {
            rows_to_delete.push_back(i);
            // Temporarily uncheck so they aren't included in the sync
            item->setCheckState(Qt::Unchecked);
        }
    }

    if (rows_to_delete.empty())
    {
        ShowMessage("Please check the modification(s) you wish to delete.");
        return;
    }

    absl::Status status = SyncActiveModifications();
    if (!status.ok())
    {
        // Revert check state if the sync failed
        for (int i : rows_to_delete)
        {
            QModelIndex index = m_modification_list_model->index(i, 0);
            QStandardItem* item = m_modification_list_model->itemFromIndex(index);
            if (item)
            {
                item->setCheckState(Qt::Checked);
            }
        }
        if (!status.message().empty())
        {
            ShowMessage(QString::fromStdString(std::string(status.message())));
        }
        return;
    }

    // Now safely remove the rows since the sync was successful
    for (int i : rows_to_delete)
    {
        m_modification_list_model->removeRow(i);
    }

    ShowMessage(QString("Modification(s) successfully deleted."));
}