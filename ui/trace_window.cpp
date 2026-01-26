/*
 Copyright 2023 Google LLC

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

#include "trace_window.h"

#include <qboxlayout.h>
#include <qspinbox.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QRadioButton>
#include <QSizePolicy>
#include <QSortFilterProxyModel>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QThread>
#include <QVBoxLayout>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "application_controller.h"
#include "capture_service/android_application.h"
#include "capture_service/constants.h"
#include "capture_service/device_mgr.h"
#include "dive/common/app_types.h"
#include "dive/utils/device_resources.h"
#include "gfxr_capture_worker.h"
#include "network/tcp_client.h"
#include "utils/component_files.h"

namespace
{

constexpr size_t kNumGfxrCaptureAppTypes =
    std::count_if(Dive::kAppTypeInfos.begin(), Dive::kAppTypeInfos.end(),
                  [](const Dive::AppTypeInfo& info) { return info.is_gfxr_capture_supported; });
const int kGfxrCaptureButtonId = 1;
const int kPm4CaptureButtonId = 2;
}  // namespace

// =================================================================================================
// TraceDialog
// =================================================================================================
TraceDialog::TraceDialog(ApplicationController& controller, QWidget* parent)
    : DeviceDialog(parent), m_controller(controller)
{
    qDebug() << "TraceDialog created.";
    m_capture_layout = new QHBoxLayout();
    m_device_label = new QLabel(tr("Devices:"));
    m_pkg_label = new QLabel(tr("Packages:"));
    m_app_type_label = new QLabel(tr("Application Type:"));
    m_gfxr_capture_file_on_device_directory_label =
        new QLabel(tr("On Device GFXR Capture File Directory Name:"));
    QLabel* capture_file_local_directory_label = new QLabel(tr("Local Capture Save Location:"));

    m_pkg_model = new QStandardItemModel();
    m_app_type_model = new QStandardItemModel();

    m_device_box = new QComboBox();
    m_pkg_box = new QComboBox();
    m_app_type_box = new QComboBox();

    m_button_layout = new QHBoxLayout();
    m_run_button = new QPushButton(kStart_Application, this);
    m_run_button->setEnabled(false);
    m_capture_button = new QPushButton("&Trace", this);
    m_capture_button->setEnabled(false);
    m_gfxr_capture_button = new QPushButton(kStart_Gfxr_Runtime_Capture, this);
    m_gfxr_capture_button->setEnabled(false);
    m_gfxr_capture_button->hide();

    m_device_refresh_button = new QPushButton("&Refresh", this);
    m_pkg_refresh_button = new QPushButton("&Refresh", this);
    m_pkg_filter_button = new QPushButton(this);
    m_pkg_filter = new PackageFilter(this);
    m_pkg_filter_button->setIcon(QIcon(":/images/filter.png"));
    m_pkg_refresh_button->setDisabled(true);
    m_pkg_filter_button->setDisabled(true);
    m_pkg_filter_button->hide();
    m_pkg_filter_button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_pkg_filter->hide();
    m_pkg_list_options = Dive::AndroidDevice::PackageListOptions::kDebuggableOnly;

    m_main_layout = new QVBoxLayout();

    for (const auto& ty : Dive::kAppTypeInfos)
    {
        QStandardItem* item = new QStandardItem(ty.ui_name.data());
        m_app_type_model->appendRow(item);
    }
    m_app_type_filter_model = new AppTypeFilterModel(this);
    m_app_type_filter_model->setSourceModel(m_app_type_model);

    m_device_box->setModel(m_device_model);
    m_device_box->setCurrentIndex(0);
    m_device_box->setCurrentText("Please select a device");

    m_pkg_box->setModel(m_pkg_model);
    m_pkg_box->setCurrentIndex(-1);
    m_pkg_box->setCurrentText("Please select a package");
    m_pkg_box->setMinimumSize(m_device_box->sizeHint());
    m_pkg_box->setSizeAdjustPolicy(
        QComboBox::SizeAdjustPolicy::AdjustToMinimumContentsLengthWithIcon);
    m_pkg_box->setEditable(true);
    QSortFilterProxyModel* filterModel = new QSortFilterProxyModel(m_pkg_box);
    filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    filterModel->setSourceModel(m_pkg_box->model());
    QCompleter* completer = new QCompleter(filterModel, m_pkg_box);
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    m_pkg_box->setCompleter(completer);

    // Capture Debuggable Applications Only Warning
    m_capture_warning_layout = new QHBoxLayout();
    m_capture_warning_label = new QLabel(tr(
        "⚠ The list below displays debuggable APKs available for capture on the selected device."));
    m_capture_warning_label->setWordWrap(true);
    m_capture_warning_layout->addWidget(m_capture_warning_label);

    m_app_type_box->setModel(m_app_type_model);

    m_capture_type_layout = new QHBoxLayout();
    m_capture_type_label = new QLabel("Capture Type:");
    m_capture_type_button_group = new QButtonGroup(this);
    m_gfxr_capture_type_button = new QRadioButton(tr("GFXR"));
    m_pm4_capture_type_button = new QRadioButton(tr("PM4"));
    m_gfxr_capture_type_button->setChecked(true);
    m_capture_type_button_group->addButton(m_gfxr_capture_type_button, kGfxrCaptureButtonId);
    m_capture_type_button_group->addButton(m_pm4_capture_type_button, kPm4CaptureButtonId);

    m_capture_type_layout->addWidget(m_capture_type_label);
    m_capture_type_layout->addWidget(m_gfxr_capture_type_button);
    m_capture_type_layout->addWidget(m_pm4_capture_type_button);
    m_capture_type_layout->addStretch(1);

    m_cmd_layout = new QHBoxLayout();
    m_file_label = new QLabel("Executable:");
    m_cmd_input_box = new QLineEdit();
    m_cmd_input_box->setPlaceholderText("Input a command or select from the package list");
    m_cmd_input_box->setClearButtonEnabled(true);
    m_cmd_layout->addWidget(m_file_label);
    m_cmd_layout->addWidget(m_cmd_input_box);

    m_args_layout = new QHBoxLayout();
    m_args_label = new QLabel("Additional Args:");
    m_args_input_box = new QLineEdit();
    m_args_layout->addWidget(m_args_label);
    m_args_layout->addWidget(m_args_input_box);

    m_capture_layout->addWidget(m_device_label);
    m_capture_layout->addWidget(m_device_box, 1);
    m_capture_layout->addWidget(m_device_refresh_button);

    m_pkg_filter_layout = new QHBoxLayout();
    m_pkg_filter_label = new QLabel("Package Filters:");
    m_pkg_filter_label->hide();
    m_pkg_filter_layout->addWidget(m_pkg_filter_label);
    m_pkg_filter_layout->addWidget(m_pkg_filter);

    m_pkg_layout = new QHBoxLayout();
    m_pkg_layout->addWidget(m_pkg_label);
    m_pkg_layout->addWidget(m_pkg_box, 1);
    m_pkg_layout->addWidget(m_pkg_refresh_button);
    m_pkg_layout->addWidget(m_pkg_filter_button);

    m_type_layout = new QHBoxLayout();
    m_type_layout->addWidget(m_app_type_label);
    m_type_layout->addWidget(m_app_type_box, 1);

    m_gfxr_capture_file_directory_layout = new QHBoxLayout();
    m_gfxr_capture_file_directory_input_box = new QLineEdit();
    m_gfxr_capture_file_directory_input_box->setPlaceholderText(
        "Input a name for the capture directory");
    m_gfxr_capture_file_directory_layout->addWidget(m_gfxr_capture_file_on_device_directory_label);
    m_gfxr_capture_file_directory_layout->addWidget(m_gfxr_capture_file_directory_input_box);
    m_gfxr_capture_file_on_device_directory_label->hide();
    m_gfxr_capture_file_directory_input_box->hide();

    QHBoxLayout* capture_file_local_directory_layout = new QHBoxLayout();
    m_capture_file_local_root_directory_input_box = new QLineEdit();
    m_capture_file_local_root_directory_input_box->setPlaceholderText(
        "Input the location to save the root directory to");
    capture_file_local_directory_layout->addWidget(capture_file_local_directory_label);
    capture_file_local_directory_layout->addWidget(m_capture_file_local_root_directory_input_box);

    m_button_layout->addWidget(m_run_button);
    m_button_layout->addWidget(m_capture_button);
    m_button_layout->addWidget(m_gfxr_capture_button);

    m_main_layout->addLayout(m_capture_layout);
    m_main_layout->addLayout(m_capture_type_layout);
    m_main_layout->addLayout(m_cmd_layout);
    m_main_layout->addLayout(m_pkg_filter_layout);
    m_main_layout->addLayout(m_capture_warning_layout);
    m_main_layout->addLayout(m_pkg_layout);
    m_main_layout->addLayout(m_gfxr_capture_file_directory_layout);
    m_main_layout->addLayout(capture_file_local_directory_layout);
    m_main_layout->addLayout(m_args_layout);

    m_main_layout->addLayout(m_type_layout);

    m_main_layout->addLayout(m_button_layout);
    m_main_layout->setSizeConstraint(QLayout::SetDefaultConstraint);
    setSizeGripEnabled(true);
    setLayout(m_main_layout);

    QObject::connect(m_device_box, SIGNAL(currentIndexChanged(const QString&)), this,
                     SLOT(OnDeviceSelectionChanged(const QString&)));
    QObject::connect(m_pkg_box, SIGNAL(currentIndexChanged(const QString&)), this,
                     SLOT(OnPackageSelected(const QString&)));
    QObject::connect(m_pkg_box->lineEdit(), &QLineEdit::textEdited, filterModel,
                     &QSortFilterProxyModel::setFilterFixedString);
    QObject::connect(m_run_button, &QPushButton::clicked, this, &TraceDialog::OnStartClicked);
    QObject::connect(m_capture_button, &QPushButton::clicked, this, &TraceDialog::OnTraceClicked);
    QObject::connect(m_gfxr_capture_button, &QPushButton::clicked, this,
                     &TraceDialog::OnGfxrCaptureClicked);
    QObject::connect(m_device_refresh_button, &QPushButton::clicked, this,
                     &TraceDialog::OnDevListRefresh);
    QObject::connect(m_pkg_refresh_button, &QPushButton::clicked, this,
                     &TraceDialog::OnAppListRefresh);
    QObject::connect(m_pkg_filter_button, &QPushButton::clicked, this,
                     &TraceDialog::OnPackageListFilter);
    QObject::connect(m_cmd_input_box, &QLineEdit::textEdited, this, &TraceDialog::OnInputCommand);
    QObject::connect(m_args_input_box, &QLineEdit::textEdited, this, &TraceDialog::OnInputArgs);
    QObject::connect(m_pkg_filter, &PackageFilter::FiltersApplied, this,
                     &TraceDialog::OnPackageListFilterApplied);

    QObject::connect(m_capture_type_button_group, QOverload<int>::of(&QButtonGroup::buttonClicked),
                     this, &TraceDialog::OnCaptureTypeChanged);

    QObject::connect(&m_controller, &ApplicationController::AdvancedOptionToggled, this,
                     &TraceDialog::OnShowAdvancedOptions);

    OnCaptureTypeChanged(kGfxrCaptureButtonId);
}

TraceDialog::~TraceDialog()
{
    qDebug() << "TraceDialog destroyed.";
    Dive::GetDeviceManager().RemoveDevice();
}

void TraceDialog::ShowMessage(const QString& message)
{
    auto message_box = new QMessageBox(this);
    message_box->setAttribute(Qt::WA_DeleteOnClose, true);
    message_box->setText(message);
    message_box->open();
}

void TraceDialog::OnDeviceSelected() { UpdatePackageList(); }

void TraceDialog::OnDeviceSelectionCleared() { m_run_button->setEnabled(false); }

absl::Status TraceDialog::StopPackageAndCleanup()
{
    auto device = Dive::GetDeviceManager().GetDevice();
    if (device == nullptr)
    {
        return absl::OkStatus();
    }
    auto cur_app = device->GetCurrentApplication();
    if (cur_app == nullptr || !cur_app->IsRunning())
    {
        return absl::OkStatus();
    }

    if (m_gfxr_capture)
    {
        if (m_gfxr_capture_button->text() == kRetrieve_Gfxr_Runtime_Capture &&
            m_gfxr_capture_button->isEnabled())
        {
            return absl::FailedPreconditionError(
                "GFXR capture is in process. Please retrieve the "
                "capture before stopping the application.");
        }
        // Only delete the on device capture directory when the application is closed.
        std::string on_device_capture_directory =
            absl::StrCat(Dive::DeviceResourcesConstants::kDeviceDownloadPath, "/",
                         m_gfxr_capture_file_directory_input_box->text().toStdString());
        auto ret =
            device->Adb().Run(absl::StrFormat("shell rm -rf %s", on_device_capture_directory));
        m_gfxr_capture_button->setEnabled(false);
        m_gfxr_capture_button->setText(kStart_Gfxr_Runtime_Capture);
    }
    else
    {
        m_capture_button->setEnabled(false);
    }

    device->StopApp().IgnoreError();
    absl::Status cleanup_status = cur_app->Cleanup();
    if (!cleanup_status.ok())
    {
        return absl::Status(cleanup_status.code(), absl::StrCat("Failed to cleanup application: ",
                                                                cleanup_status.message()));
    }
    return absl::OkStatus();
}

void TraceDialog::closeEvent(QCloseEvent* event)
{
    absl::Status status = StopPackageAndCleanup();

    // The operation was successful, close normally.
    if (status.ok())
    {
        m_run_button->setEnabled(false);
        m_run_button->setText(kStart_Application);
        m_gfxr_capture_button->setEnabled(false);
        EnableCaptureTypeButtons(true);
        m_gfxr_capture_type_button->setChecked(true);
        OnCaptureTypeChanged(kGfxrCaptureButtonId);
        event->accept();
        return;
    }

    // A specific error occurred that should prevent closing the window.
    if (status.code() == absl::StatusCode::kFailedPrecondition)
    {
        qDebug() << "Preventing window close: " << status.ToString().c_str();
        ShowMessage(QString::fromUtf8(status.message().data(), (int)status.message().size()));
        event->ignore();
    }
    else
    {
        // For any other error, we still allow the window to close to avoid trapping the user.
        qDebug() << "Closing window despite non-critical error: " << status.ToString().c_str();
        ShowMessage(QString::fromUtf8(status.message().data(), (int)status.message().size()));
        m_run_button->setEnabled(true);
        m_run_button->setText(kStart_Application);
        event->accept();
    }
}

void TraceDialog::showEvent(QShowEvent* event)
{
    if (!m_cur_dev.empty())
    {
        QModelIndexList matches =
            m_dev_model->match(m_dev_model->index(0, 0),
                               Qt::UserRole,                // Search this role
                               QString(m_cur_dev.c_str()),  // For the current device serial
                               1,                           // Stop after 1 match
                               Qt::MatchExactly);

        if (matches.empty())
        {
            ResetDialog();
        }
        else
        {
            m_run_button->setEnabled(!m_cur_pkg.empty());
        }
    }

    QDialog::showEvent(event);
}

void TraceDialog::OnCaptureTypeChanged(int button_id)
{
    m_gfxr_capture = (button_id == kGfxrCaptureButtonId);
    if (m_gfxr_capture)
    {
        ShowGfxrFields();
    }
    else
    {
        HideGfxrFields();
    }
}

void TraceDialog::OnShowAdvancedOptions(bool show)
{
    m_pkg_filter_button->setVisible(show);

    m_capture_warning_label->setVisible(!show);

    if (!show)
    {
        m_pkg_list_options = Dive::AndroidDevice::PackageListOptions::kDebuggableOnly;
        UpdatePackageList();
    }
}

void TraceDialog::OnPackageSelected(const QString& s)
{
    int cur_index = m_pkg_box->currentIndex();
    qDebug() << "Package selected: " << s << ", index: " << cur_index;
    if ((s.isEmpty() || cur_index == -1))
    {
        return;
    }
    if (m_cur_pkg != m_pkg_list[cur_index])
    {
        m_cur_pkg = m_pkg_list[cur_index];
        m_app_type_box->setCurrentIndex(-1);
    }
    m_run_button->setEnabled(true);
    m_cmd_input_box->setText(m_cur_pkg.c_str());
}

void TraceDialog::OnInputCommand(const QString& text)
{
    qDebug() << "Input changed to " << text;
    m_run_button->setEnabled(true);
    m_cur_pkg = text.toStdString();
    m_pkg_box->setCurrentIndex(-1);
    m_app_type_box->setCurrentIndex(-1);
}

void TraceDialog::OnInputArgs(const QString& text)
{
    qDebug() << "Args changed to " << text;
    m_command_args = text.toStdString();
}

bool TraceDialog::StartPackage(Dive::AndroidDevice* device, const std::string& app_type)
{
    if (device == nullptr)
    {
        return false;
    }

    device->CleanupApp().IgnoreError();
    m_run_button->setText("&Starting..");
    m_run_button->setDisabled(true);
    EnableCaptureTypeButtons(false);

    absl::Status ret;
    qDebug() << "Start app on dev: " << m_cur_device.c_str() << ", package: " << m_cur_pkg.c_str()
             << ", type: " << app_type.c_str() << ", args: " << m_command_args.c_str();

    if (m_gfxr_capture)
    {
        m_gfxr_capture_button->setText(kStart_Gfxr_Runtime_Capture);
        m_gfxr_capture_button->setEnabled(true);

        if (m_gfxr_capture_file_directory_input_box->text() == "")
        {
            m_gfxr_capture_file_directory_input_box->setText(
                QString::fromUtf8(Dive::DeviceResourcesConstants::kDeviceStagingDirectoryName));
        }
    }

    if (app_type == Dive::kAppTypeInfos[static_cast<size_t>(Dive::AppType::kVulkan_OpenXR)]
                        .ui_name.data() ||
        app_type ==
            Dive::kAppTypeInfos[static_cast<size_t>(Dive::AppType::kGLES_OpenXR)].ui_name.data())
    {
        ret = device->SetupApp(m_cur_pkg, Dive::ApplicationType::OPENXR_APK, m_command_args,
                               m_gfxr_capture_file_directory_input_box->text().toStdString());
    }
    else if (app_type == Dive::kAppTypeInfos[static_cast<size_t>(Dive::AppType::kVulkan_Non_OpenXR)]
                             .ui_name.data())
    {
        ret = device->SetupApp(m_cur_pkg, Dive::ApplicationType::VULKAN_APK, m_command_args,
                               m_gfxr_capture_file_directory_input_box->text().toStdString());
    }
    else if (app_type == Dive::kAppTypeInfos[static_cast<size_t>(Dive::AppType::kGLES_Non_OpenXR)]
                             .ui_name.data())
    {
        ret = device->SetupApp(m_cur_pkg, Dive::ApplicationType::GLES_APK, m_command_args,
                               m_gfxr_capture_file_directory_input_box->text().toStdString());
    }
    else if (app_type ==
             Dive::kAppTypeInfos[static_cast<size_t>(Dive::AppType::kVulkanCLI_Non_OpenXR)]
                 .ui_name.data())
    {
        m_executable = m_cmd_input_box->text().toStdString();
        if (m_executable.empty())
        {
            std::string err_msg = "Please input a valid command to execute";
            qDebug() << err_msg.c_str();
            ShowMessage(QString::fromStdString(err_msg));
            return false;
        }
        qDebug() << "exe: " << m_executable.c_str() << " args: " << m_command_args.c_str();
        ret = device->SetupApp(m_executable, m_command_args, Dive::ApplicationType::VULKAN_CLI,
                               m_gfxr_capture_file_directory_input_box->text().toStdString());
    }
    if (!ret.ok())
    {
        std::string err_msg =
            absl::StrCat("Fail to setup for package ", m_cur_pkg, " error: ", ret.message());
        qDebug() << err_msg.c_str();
        ShowMessage(QString::fromStdString(err_msg));
        return false;
    }
    ret = device->StartApp();
    if (!ret.ok())
    {
        std::string err_msg =
            absl::StrCat("Fail to start package ", m_cur_pkg, " error: ", ret.message());
        qDebug() << err_msg.c_str();
        ShowMessage(QString::fromStdString(err_msg));
        return false;
    }
    auto cur_app = device->GetCurrentApplication();

    if (!cur_app->IsRunning())
    {
        std::string err_msg =
            absl::StrCat("Process for package ", m_cur_pkg, " not found, possibly crashed.");
        qDebug() << err_msg.c_str();
        ShowMessage(QString::fromStdString(err_msg));
        return false;
    }

    if (cur_app)
    {
        m_run_button->setDisabled(false);
        if (m_gfxr_capture)
        {
            m_run_button->setText("&Stop Application");
            m_gfxr_capture_button->setEnabled(true);
        }
        else
        {
            m_run_button->setText("&Stop");
            m_capture_button->setEnabled(true);
        }
    }
    return true;
}

void TraceDialog::OnStartClicked()
{
    qDebug() << "Command: " << m_cmd_input_box->text();
    auto device = Dive::GetDeviceManager().GetDevice();
    if (!device)
    {
        std::string err_msg =
            "No device/application selected. Please select a device and application and "
            "then try again.";
        ShowMessage(QString::fromStdString(err_msg));
        return;
    }
    device->EnableGfxr(m_gfxr_capture);
    absl::Status ret = device->SetupDevice();
    if (!ret.ok())
    {
        std::string err_msg = absl::StrCat("Fail to setup device: ", ret.message());
        qDebug() << err_msg.c_str();
        ShowMessage(QString::fromStdString(err_msg));
        return;
    }
    int ty = m_app_type_box->currentIndex();
    if (ty == -1)
    {
        ShowMessage(QString("Please select application type"));
        return;
    }
    std::string ty_str = Dive::kAppTypeInfos[ty].ui_name.data();

    if (m_run_button->text() == QString(kStart_Application))
    {
        if (!StartPackage(device, ty_str))
        {
            m_run_button->setDisabled(false);
            m_run_button->setText(kStart_Application);
            EnableCaptureTypeButtons(true);
        }
    }
    else
    {
        qDebug() << "Stop package and cleanup: " << m_cur_pkg.c_str();
        absl::Status status = StopPackageAndCleanup();
        if (!status.ok())
        {
            qDebug() << "Failed to stop package or cleanup: " << status.ToString().c_str();
            ShowMessage(QString::fromUtf8(status.message().data(), (int)status.message().size()));

            // Only exit without resetting the button if the error is a precondition
            // failure (GFXR capture in progress). For other cleanup errors, we still
            // want to reset the UI to a "startable" state.
            if (status.code() == absl::StatusCode::kFailedPrecondition)
            {
                return;
            }
        }
        m_run_button->setEnabled(true);
        m_run_button->setText(kStart_Application);
        EnableCaptureTypeButtons(true);
    }
}

void TraceDialog::OnTraceClicked()
{
    QProgressDialog* progress_bar =
        new QProgressDialog("Capturing PM4 Data ... ", nullptr, 0, 100, this);
    progress_bar->setMinimumWidth(this->minimumWidth() + 50);
    progress_bar->setMinimumHeight(this->minimumHeight() + 50);
    progress_bar->setAutoReset(true);
    progress_bar->setAutoClose(true);
    progress_bar->setMinimumDuration(0);
    CaptureWorker* workerThread = new CaptureWorker(progress_bar);

    if (m_capture_file_local_root_directory_input_box->text() == "")
    {
        std::filesystem::path host_root_path = Dive::ResolveHostRootPath();
        m_capture_file_local_root_directory_input_box->setText(
            QString::fromStdString(host_root_path.string()));
    }

    workerThread->SetTargetCaptureDir(
        m_capture_file_local_root_directory_input_box->text().toStdString());
    connect(workerThread, &CaptureWorker::CaptureAvailable, this, &TraceDialog::OnTraceAvailable);
    connect(workerThread, &CaptureWorker::finished, workerThread, &QObject::deleteLater);
    connect(workerThread, &CaptureWorker::ShowMessage, this, &TraceDialog::ShowMessage);
    connect(workerThread, &CaptureWorker::DownloadedSize, progress_bar,
            [progress_bar](int64_t downloaded_size, int64_t total_size) {
                int percentage = 0;
                if (total_size > 0)
                {
                    percentage = (int)(((double)downloaded_size / total_size) * 100.0);
                }
                progress_bar->setValue(percentage);
            });
    connect(workerThread, &CaptureWorker::UpdateProgressDialog, progress_bar,
            &QProgressDialog::setLabelText);
    workerThread->start();
    std::cout << "OnTraceClicked done " << std::endl;
}

void TraceDialog::OnTraceAvailable(QString const& trace_path) { emit TraceAvailable(trace_path); }

void TraceDialog::OnDevListRefresh() { UpdateDeviceList(); }

void TraceDialog::OnAppListRefresh() { UpdatePackageList(); }

void TraceDialog::UpdatePackageList()
{
    auto device = Dive::GetDeviceManager().GetDevice();
    if (device == nullptr)
    {
        return;
    }

    auto ret = device->ListPackage(m_pkg_list_options);
    if (!ret.ok())
    {
        std::string err_msg = absl::StrCat("Failed to list package for device ", m_cur_device,
                                           " error: ", ret.status().message());
        qDebug() << err_msg.c_str();
        ShowMessage(QString::fromStdString(err_msg));
        return;
    }
    m_pkg_list = *ret;

    const QSignalBlocker blocker(
        m_pkg_box);  // Do not emit index changed event when update the model
    m_pkg_model->clear();
    for (size_t i = 0; i < m_pkg_list.size(); i++)
    {
        QStandardItem* item = new QStandardItem(m_pkg_list[i].c_str());
        m_pkg_model->appendRow(item);
    }
    m_pkg_box->setCurrentIndex(-1);
    m_pkg_refresh_button->setDisabled(false);
    m_pkg_filter_button->setDisabled(false);
}

void TraceDialog::OnPackageListFilter()
{
    if (m_pkg_filter->isHidden())
    {
        m_pkg_filter_label->show();
        m_pkg_filter->show();
    }
    else
    {
        m_pkg_filter_label->hide();
        m_pkg_filter->hide();
    }
}

void TraceDialog::OnPackageListFilterApplied(const QString& filter)
{
    if (filter == "All")
    {
        m_pkg_list_options = Dive::AndroidDevice::PackageListOptions::kAll;
    }
    else if (filter == "Debuggable")
    {
        m_pkg_list_options = Dive::AndroidDevice::PackageListOptions::kDebuggableOnly;
    }
    else if (filter == "Non-Debuggable")
    {
        m_pkg_list_options = Dive::AndroidDevice::PackageListOptions::kNonDebuggableOnly;
    }
    UpdatePackageList();
    m_pkg_filter_label->hide();
    m_pkg_filter->hide();
}

void TraceDialog::ShowGfxrFields()
{
    m_app_type_filter_model->setFilterActive(true);
    m_capture_button->hide();
    m_gfxr_capture_button->show();
    m_gfxr_capture_file_on_device_directory_label->show();
    m_gfxr_capture_file_directory_input_box->show();
}

void TraceDialog::HideGfxrFields()
{
    m_app_type_filter_model->setFilterActive(false);
    m_capture_button->show();
    m_gfxr_capture_button->hide();
    m_gfxr_capture_file_on_device_directory_label->hide();
    m_gfxr_capture_file_directory_input_box->hide();
}

void TraceDialog::EnableCaptureTypeButtons(bool enable)
{
    m_gfxr_capture_type_button->setEnabled(enable);
    m_pm4_capture_type_button->setEnabled(enable);
}

void TraceDialog::OnGfxrCaptureClicked()
{
    auto device = Dive::GetDeviceManager().GetDevice();
    absl::Status ret;
    if (m_gfxr_capture_button->text() == kRetrieve_Gfxr_Runtime_Capture)
    {
        ret = device->Adb().Run("shell setprop debug.gfxrecon.capture_android_trigger false");
        if (!ret.ok())
        {
            std::string err_msg = absl::StrCat("Failed to stop runtime gfxr capture ", m_cur_pkg,
                                               " error: ", ret.message());
            qDebug() << err_msg.c_str();
            ShowMessage(QString::fromStdString(err_msg));
            return;
        }

        RetrieveGfxrCapture();

        m_gfxr_capture_button->setText(kStart_Gfxr_Runtime_Capture);
        m_gfxr_capture_button->setEnabled(true);
        m_run_button->setEnabled(true);
    }
    else if (m_gfxr_capture_button->text() == kStart_Gfxr_Runtime_Capture)
    {
        ret = device->Adb().Run("shell setprop debug.gfxrecon.capture_android_trigger true");
        if (!ret.ok())
        {
            std::string err_msg = absl::StrCat("Failed to start runtime gfxr capture ", m_cur_pkg,
                                               " error: ", ret.message());
            qDebug() << err_msg.c_str();
            ShowMessage(QString::fromStdString(err_msg));
            return;
        }

        std::filesystem::path capture_path(
            m_gfxr_capture_file_directory_input_box->text().toStdString());
        ret = device->TriggerScreenCapture(capture_path);
        if (!ret.ok())
        {
            std::string err_msg =
                absl::StrCat("Failed to create capture screenshot: ", ret.message());
            qDebug() << err_msg.c_str();
            ShowMessage(QString::fromStdString(err_msg));
            return;
        }

        m_gfxr_capture_button->setText(kRetrieve_Gfxr_Runtime_Capture);
        m_run_button->setEnabled(false);
    }
}

void TraceDialog::RetrieveGfxrCapture()
{
    auto device = Dive::GetDeviceManager().GetDevice();

    if (device == nullptr)
    {
        qDebug() << "Failed to connect to device";
        return;
    }

    if (m_capture_file_local_root_directory_input_box->text() == "")
    {
        std::filesystem::path host_root_path = Dive::ResolveHostRootPath();
        m_capture_file_local_root_directory_input_box->setText(
            QString::fromStdString(host_root_path.string()));
    }

    std::string on_device_capture_file_directory =
        absl::StrCat(std::string(Dive::DeviceResourcesConstants::kDeviceDownloadPath), "/",
                     m_gfxr_capture_file_directory_input_box->text().toStdString());

    QProgressDialog* progress_bar =
        new QProgressDialog("Downloading GFXR Capture ... ", nullptr, 0, 100, this);
    progress_bar->setObjectName("gfxr_download_progress");
    progress_bar->setMinimumWidth(this->minimumWidth() + 50);
    progress_bar->setMinimumHeight(this->minimumHeight() + 50);
    progress_bar->setAutoReset(true);
    progress_bar->setAutoClose(true);

    GfxrCaptureWorker* workerThread = new GfxrCaptureWorker(progress_bar);
    workerThread->SetGfxrSourceCaptureDir(on_device_capture_file_directory);

    workerThread->SetTargetCaptureDir(
        m_capture_file_local_root_directory_input_box->text().toStdString());

    connect(workerThread, &GfxrCaptureWorker::CaptureAvailable, this,
            &TraceDialog::OnGFXRCaptureAvailable);
    connect(workerThread, &GfxrCaptureWorker::finished, workerThread, &QObject::deleteLater);
    connect(workerThread, &GfxrCaptureWorker::ShowMessage, this, &TraceDialog::ShowMessage);
    connect(workerThread, &GfxrCaptureWorker::DownloadedSize, progress_bar,
            [progress_bar](int64_t downloaded_size, int64_t total_size) {
                int percentage = 0;
                if (total_size > 0)
                {
                    percentage = (int)(((double)downloaded_size / total_size) * 100.0);
                }
                progress_bar->setValue(percentage);
            });
    workerThread->start();

    m_gfxr_capture_button->setEnabled(false);
}

void TraceDialog::OnGFXRCaptureAvailable(QString const& capture_path)
{
    QProgressDialog* progress_bar = findChild<QProgressDialog*>("gfxr_download_progress");
    if (progress_bar)
    {
        progress_bar->close();
        progress_bar->deleteLater();
    }
    std::string success_msg = "Capture successfully saved at " + capture_path.toStdString();
    qDebug() << success_msg.c_str();
    ShowMessage(QString::fromStdString(success_msg));
    emit TraceAvailable(capture_path);
}

void TraceDialog::ResetDialog()
{
    if (m_gfxr_capture)
    {
        m_gfxr_capture_button->setEnabled(false);
        m_gfxr_capture_button->setText(kStart_Gfxr_Runtime_Capture);
        m_gfxr_capture_file_directory_input_box->clear();
    }
    m_cur_dev.clear();
    m_cur_pkg.clear();
    m_cmd_input_box->clear();
    m_args_input_box->clear();
    m_capture_file_local_directory_input_box->clear();
    m_app_type_box->setCurrentIndex(-1);
    m_pkg_box->setCurrentIndex(-1);
    m_run_button->setEnabled(false);
}

// =================================================================================================
// AppTypeFilterModel
// =================================================================================================

void AppTypeFilterModel::setFilterActive(bool active)
{
    m_filter_active = active;
    invalidateFilter();
}

//--------------------------------------------------------------------------------------------------
bool AppTypeFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
{
    if (m_filter_active)
    {
        return sourceRow < static_cast<int>(kNumGfxrCaptureAppTypes);
    }
    return true;
}
