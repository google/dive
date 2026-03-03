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
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
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
#include "application_controller.h"
#include "capture_service/device_mgr.h"
#include "dive/common/app_types.h"

namespace
{

constexpr size_t kNumWhatIfAppTypes =
    std::count_if(Dive::kAppTypeInfos.begin(), Dive::kAppTypeInfos.end(),
                  [](const Dive::AppTypeInfo& info) { return info.is_what_if_supported; });
constexpr int kRuntimeWhatIfButtonId = 1;
constexpr int kReplayWhatIfButtonId = 2;

constexpr std::string_view kStartApplication = "&Start Application";
constexpr std::string_view kStopApplication = "&Stop Application";
constexpr std::string_view kDismiss = "&Dismiss";
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

    // --- Font Definitions ---
    QFont boldFont = this->font();
    boldFont.setBold(true);
    QFont titleFont = boldFont;
    titleFont.setPointSize(titleFont.pointSize() + 2);

    // --- Header Section ---
    QLabel* what_if_title_label = new QLabel(tr("Explore What-If Scenarios"));
    what_if_title_label->setFont(titleFont);
    QLabel* what_if_info_label =
        new QLabel(tr("Modify vulkan calls, shaders, etc. to explore the potential impact "
                      "rendering the application."));
    what_if_info_label->setWordWrap(true);

    // --- Radio Button Group Section ---
    QButtonGroup* what_if_type_button_group = new QButtonGroup(this);

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

    what_if_type_button_group->addButton(m_runtime_what_if_type_button, kRuntimeWhatIfButtonId);
    what_if_type_button_group->addButton(m_replay_what_if_type_button, kReplayWhatIfButtonId);

    m_runtime_options_widget = new QWidget(this);
    m_replay_options_widget = new QWidget(this);

    // --- Grid Section ---
    QGridLayout* settings_grid = new QGridLayout(m_runtime_options_widget);
    settings_grid->setContentsMargins(0, 0, 0, 0);
    settings_grid->setColumnStretch(1, 1);  // Allows the input boxes to expand

    // Device Selection
    QLabel* device_label = new QLabel(tr("Device:"));
    m_device_box = new QComboBox();
    m_device_box->setModel(m_device_model);
    QPushButton* m_device_refresh_button = new QPushButton(tr("&Refresh"));
    settings_grid->addWidget(device_label, 0, 0, Qt::AlignRight);
    settings_grid->addWidget(m_device_box, 0, 1);
    settings_grid->addWidget(m_device_refresh_button, 0, 2);

    // Package Selection
    m_pkg_model = new QStandardItemModel();
    m_runtime_data.m_pkg_list_options = Dive::AndroidDevice::PackageListOptions::kDebuggableOnly;
    QLabel* pkg_label = new QLabel(tr("Packages:"));
    m_pkg_box = new QComboBox();
    m_pkg_box->setModel(m_pkg_model);
    m_pkg_box->setEditable(true);
    QSortFilterProxyModel* filter_model = new QSortFilterProxyModel(m_pkg_box);
    filter_model->setFilterCaseSensitivity(Qt::CaseInsensitive);
    filter_model->setSourceModel(m_pkg_box->model());
    QCompleter* completer = new QCompleter(filter_model, m_pkg_box);
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    m_pkg_box->setCompleter(completer);
    m_pkg_refresh_button = new QPushButton(tr("&Refresh"));
    settings_grid->addWidget(pkg_label, 1, 0, Qt::AlignRight);
    settings_grid->addWidget(m_pkg_box, 1, 1);
    settings_grid->addWidget(m_pkg_refresh_button, 1, 2);

    // Additional Args
    QLabel* m_args_label = new QLabel(tr("Additional Args:"));
    m_args_input_box = new QLineEdit();
    settings_grid->addWidget(m_args_label, 2, 0, Qt::AlignRight);
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

    // --- Buttons ---
    QHBoxLayout* button_layout = new QHBoxLayout();
    QPushButton* dismiss_button = new QPushButton(tr(kDismiss.data()), this);
    m_start_application_button = new QPushButton(kStartApplication.data(), this);
    m_start_application_button->setEnabled(false);
    button_layout->addWidget(dismiss_button);
    button_layout->addWidget(m_start_application_button);
    // --- Main Layout ---
    QVBoxLayout* main_layout = new QVBoxLayout(this);
    main_layout->addWidget(what_if_title_label);
    main_layout->addWidget(what_if_info_label);
    main_layout->addSpacing(15);

    main_layout->addWidget(m_runtime_what_if_type_button);
    main_layout->addWidget(runtime_what_if_type_label);
    main_layout->addWidget(m_replay_what_if_type_button);
    main_layout->addWidget(replay_what_if_type_label);

    main_layout->addSpacing(15);
    main_layout->addWidget(m_runtime_options_widget);
    main_layout->addWidget(m_replay_options_widget);
    main_layout->addStretch();
    main_layout->addLayout(button_layout);
    setLayout(main_layout);

    QObject::connect(m_device_box, SIGNAL(currentIndexChanged(const QString&)), this,
                     SLOT(OnDeviceSelectionChanged(const QString&)));
    QObject::connect(m_device_refresh_button, &QPushButton::clicked, this,
                     &WhatIfSetupDialog::OnDevListRefresh);
    QObject::connect(m_pkg_box, SIGNAL(currentIndexChanged(const QString&)), this,
                     SLOT(OnPackageSelected(const QString&)));
    QObject::connect(m_pkg_box->lineEdit(), &QLineEdit::textEdited, filter_model,
                     &QSortFilterProxyModel::setFilterFixedString);
    QObject::connect(m_args_input_box, &QLineEdit::textEdited, this,
                     &WhatIfSetupDialog::OnInputArgs);
    QObject::connect(m_start_application_button, &QPushButton::clicked, this,
                     &WhatIfSetupDialog::OnStartClicked);

    QObject::connect(dismiss_button, &QPushButton::clicked, this, &QDialog::reject);
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

void WhatIfSetupDialog::UpdatePackageList()
{
    auto device = Dive::GetDeviceManager().GetDevice();
    if (device == nullptr)
    {
        return;
    }

    if (auto ret = device->ListPackage(m_runtime_data.m_pkg_list_options); !ret.ok())
    {
        std::string device_serial = GetCurrentDeviceSerial();
        std::string err_msg = absl::StrFormat("Failed to list package for device %s, error: %s",
                                               device_serial, ret.status().message());
        qDebug() << err_msg.c_str();
        ShowMessage(QString::fromStdString(err_msg));
        return;
    }
    else
    {
        m_runtime_data.m_pkg_list = *ret;
    }

    const QSignalBlocker blocker(
        m_pkg_box);  // Do not emit index changed event when update the model
    m_pkg_model->clear();
    for (size_t i = 0; i < m_runtime_data.m_pkg_list.size(); i++)
    {
        QStandardItem* item = new QStandardItem(m_runtime_data.m_pkg_list[i].c_str());
        m_pkg_model->appendRow(item);
    }
    m_pkg_box->setCurrentIndex(-1);
    m_pkg_refresh_button->setDisabled(false);
}

void WhatIfSetupDialog::ShowMessage(const QString& message)
{
    auto message_box = new QMessageBox(this);
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
    m_runtime_data.m_cur_pkg.clear();
    m_pkg_box->setCurrentIndex(-1);
    m_pkg_model->clear();
    m_start_application_button->setEnabled(false);
    m_start_application_button->setText(kStartApplication.data());

    EnableWhatIfTypeButtons(true);
}

bool WhatIfSetupDialog::StartPackage(Dive::AndroidDevice* device, const std::string& app_type)
{
    if (device == nullptr)
    {
        return false;
    }

    device->CleanupApp().IgnoreError();
    m_start_application_button->setText("&Starting..");
    m_start_application_button->setDisabled(true);
    EnableWhatIfTypeButtons(false);

    absl::Status ret;
    std::string device_serial = GetCurrentDeviceSerial();
    qDebug() << "Start app on dev: " << device_serial.c_str()
             << ", package: " << m_runtime_data.m_cur_pkg.toStdString().c_str()
             << ", type: " << app_type.c_str()
             << ", args: " << m_runtime_data.m_command_args.c_str();

    if (app_type ==
        Dive::kAppTypeInfos[static_cast<size_t>(Dive::AppType::kVulkan_OpenXR)].ui_name.data())
    {
        ret = device->SetupApp(m_runtime_data.m_cur_pkg.toStdString(),
                               Dive::ApplicationType::OPENXR_APK, m_runtime_data.m_command_args,
                               /*gfxr_capture_directory*/ "");
    }
    else if (app_type == Dive::kAppTypeInfos[static_cast<size_t>(Dive::AppType::kVulkan_Non_OpenXR)]
                             .ui_name.data())
    {
        ret = device->SetupApp(m_runtime_data.m_cur_pkg.toStdString(),
                               Dive::ApplicationType::VULKAN_APK, m_runtime_data.m_command_args,
                               /*gfxr_capture_directory*/ "");
    }
    else if (app_type ==
             Dive::kAppTypeInfos[static_cast<size_t>(Dive::AppType::kVulkanCLI_Non_OpenXR)]
                 .ui_name.data())
    {
        if (m_runtime_data.m_cur_pkg.isEmpty())
        {
            std::string err_msg = "Please input a valid command to execute";
            qDebug() << err_msg.c_str();
            ShowMessage(QString::fromStdString(err_msg));
            return false;
        }
        qDebug() << "exe: " << m_runtime_data.m_cur_pkg.toStdString().c_str()
                 << " args: " << m_runtime_data.m_command_args.c_str();
        ret = device->SetupApp(m_runtime_data.m_cur_pkg.toStdString(),
                               m_runtime_data.m_command_args, Dive::ApplicationType::VULKAN_CLI,
                               /*gfxr_capture_directory*/ "");
    }
    if (!ret.ok())
    {
        std::string err_msg =
            absl::StrFormat("Fail to setup for package %s, error: %s",
                            m_runtime_data.m_cur_pkg.toStdString(),
                            ret.message());
        qDebug() << err_msg.c_str();
        ShowMessage(QString::fromStdString(err_msg));
        return false;
    }
    ret = device->StartApp();
    if (!ret.ok())
    {
        std::string err_msg =
            absl::StrFormat("Fail to start package %s, error: %s",
                            m_runtime_data.m_cur_pkg.toStdString(),
                            ret.message());
        qDebug() << err_msg.c_str();
        ShowMessage(QString::fromStdString(err_msg));
        return false;
    }
    auto cur_app = device->GetCurrentApplication();

    if (!cur_app->IsRunning())
    {
        std::string err_msg =
            absl::StrFormat("Process for package %s not found, possibly crashed.",
                            m_runtime_data.m_cur_pkg.toStdString());
        qDebug() << err_msg.c_str();
        ShowMessage(QString::fromStdString(err_msg));
        return false;
    }

    if (cur_app)
    {
        m_start_application_button->setDisabled(false);
        m_start_application_button->setText(kStopApplication.data());
    }

    emit RuntimeWhatIfEnabled(m_runtime_data.m_cur_pkg, true);
    return true;
}

void WhatIfSetupDialog::StopPackage()
{
    auto device = Dive::GetDeviceManager().GetDevice();
    auto cur_app = device ? device->GetCurrentApplication() : nullptr;

    if (!device || !cur_app || !cur_app->IsRunning())
    {
        emit RuntimeWhatIfEnabled("None", false);
        ResetDialog();
        return;
    }

    device->StopApp().IgnoreError();

    if (absl::Status status = cur_app->Cleanup(); !status.ok())
    {
        qDebug() << "Failed to cleanup application: " << status.ToString().c_str();
        ShowMessage(QString::fromStdString(
            absl::StrFormat("Failed to cleanup application: %s", status.message())));

        // Exit without resetting UI if it's a specific precondition failure
        if (status.code() == absl::StatusCode::kFailedPrecondition)
        {
            return;
        }
    }

    emit RuntimeWhatIfEnabled("None", false);
    ResetDialog();
}

void WhatIfSetupDialog::OnStopRuntimeWhatIf() { StopPackage(); }

void WhatIfSetupDialog::OnAppListRefresh() { UpdatePackageList(); }

void WhatIfSetupDialog::OnDevListRefresh() { UpdateDeviceList(); }

void WhatIfSetupDialog::OnInputArgs(const QString& text)
{
    qDebug() << "Args changed to " << text;
    m_runtime_data.m_command_args = text.toStdString();
}

void WhatIfSetupDialog::OnPackageSelected(const QString& s)
{
    int cur_index = m_pkg_box->currentIndex();
    qDebug() << "Package selected: " << s << ", index: " << cur_index;
    if ((s.isEmpty() || cur_index == -1))
    {
        return;
    }
    if (m_runtime_data.m_cur_pkg.toStdString() != m_runtime_data.m_pkg_list[cur_index])
    {
        m_runtime_data.m_cur_pkg = m_runtime_data.m_pkg_list[cur_index].c_str();
        qDebug() << "Current package set to: " << m_runtime_data.m_cur_pkg.toStdString().c_str();
    }
    m_start_application_button->setEnabled(true);
}

void WhatIfSetupDialog::OnStartClicked()
{
    auto device = Dive::GetDeviceManager().GetDevice();
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
        std::string err_msg = absl::StrFormat("Fail to setup device: ", ret.message());
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
                 << m_runtime_data.m_cur_pkg.toStdString().c_str();
        StopPackage();
        return;
    }

    qDebug() << "Start Application clicked with package: "
             << m_runtime_data.m_cur_pkg.toStdString().c_str() << ", type: " << ty_str.c_str();
    if (!StartPackage(device, ty_str))
    {
        m_start_application_button->setDisabled(false);
        m_start_application_button->setText(kStartApplication.data());
        EnableWhatIfTypeButtons(true);
    }
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
    auto device = Dive::GetDeviceManager().GetDevice();
    auto cur_app = device ? device->GetCurrentApplication() : nullptr;

    bool no_application_currently_running_on_device =
        (device == nullptr) || (cur_app == nullptr) || !cur_app->IsRunning();

    if (no_application_currently_running_on_device)
    {
        EnableWhatIfTypeButtons(true);
        m_runtime_what_if_type_button->setChecked(true);
        OnWhatIfTypeChanged(kRuntimeWhatIfButtonId);
        m_start_application_button->setText(kStartApplication.data());
    }

    event->accept();
    return;
}

void WhatIfSetupDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);

    std::string current_device_serial = GetCurrentDeviceSerial();

    if (current_device_serial.empty())
    {
        ResetDialog();
    }
    else
    {
        m_start_application_button->setEnabled(!m_runtime_data.m_cur_pkg.isEmpty());
    }
}
