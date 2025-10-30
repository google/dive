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
#include "capture_service/android_application.h"
#include "capture_service/constants.h"
#include "capture_service/device_mgr.h"
#include "network/tcp_client.h"
#include "utils/component_files.h"

namespace
{
const std::vector<std::string> kAppTypes{ "Vulkan APK", "OpenXR APK", "Command Line Application" };
}

// =================================================================================================
// TraceDialog
// =================================================================================================
TraceDialog::TraceDialog(QWidget *parent) :
    QDialog(parent)
{
    qDebug() << "TraceDialog created.";
    m_capture_layout = new QHBoxLayout();
    m_dev_label = new QLabel(tr("Devices:"));
    m_pkg_label = new QLabel(tr("Packages:"));
    m_app_type_label = new QLabel(tr("Application Type:"));
    m_gfxr_capture_file_on_device_directory_label = new QLabel(
    tr("On Device GFXR Capture File Directory Name:"));
    m_gfxr_capture_file_local_directory_label = new QLabel(tr("Local GFXR Capture Save Location:"));

    m_dev_model = new QStandardItemModel();
    m_pkg_model = new QStandardItemModel();
    m_app_type_model = new QStandardItemModel();

    m_dev_box = new QComboBox();
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

    m_dev_refresh_button = new QPushButton("&Refresh", this);
    m_pkg_refresh_button = new QPushButton("&Refresh", this);
    m_pkg_filter_button = new QPushButton(this);
    m_pkg_filter = new PackageFilter(this);
    m_pkg_filter_button->setIcon(QIcon(":/images/filter.png"));
    m_pkg_refresh_button->setDisabled(true);
    m_pkg_filter_button->setDisabled(true);
    m_pkg_filter_button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_pkg_filter->hide();
    m_pkg_list_options = Dive::AndroidDevice::PackageListOptions::kDebuggableOnly;

    m_main_layout = new QVBoxLayout();

    m_devices = Dive::GetDeviceManager().ListDevice();
    UpdateDeviceList(false);
    for (const auto &ty : kAppTypes)
    {
        QStandardItem *item = new QStandardItem(ty.c_str());
        m_app_type_model->appendRow(item);
    }
    m_dev_box->setModel(m_dev_model);
    m_dev_box->setCurrentIndex(0);
    m_dev_box->setCurrentText("Please select a device");

    m_pkg_box->setModel(m_pkg_model);
    m_pkg_box->setCurrentIndex(-1);
    m_pkg_box->setCurrentText("Please select a package");
    m_pkg_box->setMinimumSize(m_dev_box->sizeHint());
    m_pkg_box->setSizeAdjustPolicy(
    QComboBox::SizeAdjustPolicy::AdjustToMinimumContentsLengthWithIcon);
    m_pkg_box->setEditable(true);
    QSortFilterProxyModel *filterModel = new QSortFilterProxyModel(m_pkg_box);
    filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    filterModel->setSourceModel(m_pkg_box->model());
    QCompleter *completer = new QCompleter(filterModel, m_pkg_box);
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    m_pkg_box->setCompleter(completer);

    m_app_type_box->setModel(m_app_type_model);

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

    m_capture_layout->addWidget(m_dev_label);
    m_capture_layout->addWidget(m_dev_box);
    m_capture_layout->addWidget(m_dev_refresh_button);

    m_pkg_filter_layout = new QHBoxLayout();
    m_pkg_filter_label = new QLabel("Package Filters:");
    m_pkg_filter_label->hide();
    m_pkg_filter_layout->addWidget(m_pkg_filter_label);
    m_pkg_filter_layout->addWidget(m_pkg_filter);

    m_pkg_layout = new QHBoxLayout();
    m_pkg_layout->addWidget(m_pkg_label);
    m_pkg_layout->addWidget(m_pkg_box);
    m_pkg_layout->addWidget(m_pkg_refresh_button);
    m_pkg_layout->addWidget(m_pkg_filter_button);

    m_type_layout = new QHBoxLayout();
    m_type_layout->addWidget(m_app_type_label);
    m_type_layout->addWidget(m_app_type_box);

    m_gfxr_capture_file_directory_layout = new QHBoxLayout();
    m_gfxr_capture_file_directory_input_box = new QLineEdit();
    m_gfxr_capture_file_directory_input_box->setPlaceholderText(
    "Input a name for the capture directory");
    m_gfxr_capture_file_directory_layout->addWidget(m_gfxr_capture_file_on_device_directory_label);
    m_gfxr_capture_file_directory_layout->addWidget(m_gfxr_capture_file_directory_input_box);
    m_gfxr_capture_file_on_device_directory_label->hide();
    m_gfxr_capture_file_directory_input_box->hide();

    m_gfxr_capture_file_local_directory_layout = new QHBoxLayout();
    m_gfxr_capture_file_local_directory_input_box = new QLineEdit();
    m_gfxr_capture_file_local_directory_input_box->setPlaceholderText(
    "Input the location to save the directory to");
    m_gfxr_capture_file_local_directory_layout->addWidget(
    m_gfxr_capture_file_local_directory_label);
    m_gfxr_capture_file_local_directory_layout->addWidget(
    m_gfxr_capture_file_local_directory_input_box);
    m_gfxr_capture_file_local_directory_label->hide();
    m_gfxr_capture_file_local_directory_input_box->hide();

    m_button_layout->addWidget(m_run_button);
    m_button_layout->addWidget(m_capture_button);
    m_button_layout->addWidget(m_gfxr_capture_button);

    m_main_layout->addLayout(m_capture_layout);
    m_main_layout->addLayout(m_cmd_layout);
    m_main_layout->addLayout(m_pkg_filter_layout);
    m_main_layout->addLayout(m_pkg_layout);
    m_main_layout->addLayout(m_gfxr_capture_file_directory_layout);
    m_main_layout->addLayout(m_gfxr_capture_file_local_directory_layout);
    m_main_layout->addLayout(m_args_layout);

    m_main_layout->addLayout(m_type_layout);

    m_main_layout->addLayout(m_button_layout);
    setLayout(m_main_layout);

    QObject::connect(m_dev_box,
                     SIGNAL(currentIndexChanged(const QString &)),
                     this,
                     SLOT(OnDeviceSelected(const QString &)));
    QObject::connect(m_pkg_box,
                     SIGNAL(currentIndexChanged(const QString &)),
                     this,
                     SLOT(OnPackageSelected(const QString &)));
    QObject::connect(m_pkg_box->lineEdit(),
                     &QLineEdit::textEdited,
                     filterModel,
                     &QSortFilterProxyModel::setFilterFixedString);
    QObject::connect(m_run_button, &QPushButton::clicked, this, &TraceDialog::OnStartClicked);
    QObject::connect(m_capture_button, &QPushButton::clicked, this, &TraceDialog::OnTraceClicked);
    QObject::connect(m_gfxr_capture_button,
                     &QPushButton::clicked,
                     this,
                     &TraceDialog::OnGfxrCaptureClicked);
    QObject::connect(m_dev_refresh_button,
                     &QPushButton::clicked,
                     this,
                     &TraceDialog::OnDevListRefresh);
    QObject::connect(m_pkg_refresh_button,
                     &QPushButton::clicked,
                     this,
                     &TraceDialog::OnAppListRefresh);
    QObject::connect(m_pkg_filter_button,
                     &QPushButton::clicked,
                     this,
                     &TraceDialog::OnPackageListFilter);
    QObject::connect(m_cmd_input_box, &QLineEdit::textEdited, this, &TraceDialog::OnInputCommand);
    QObject::connect(m_args_input_box, &QLineEdit::textEdited, this, &TraceDialog::OnInputArgs);
    QObject::connect(m_pkg_filter,
                     &PackageFilter::FiltersApplied,
                     this,
                     &TraceDialog::OnPackageListFilterApplied);
}

TraceDialog::~TraceDialog()
{
    qDebug() << "TraceDialog destroyed.";
    Dive::GetDeviceManager().RemoveDevice();
}

void TraceDialog::ShowErrorMessage(const QString &err_msg)
{
    QMessageBox msgBox;
    msgBox.setText(err_msg);
    msgBox.exec();
    return;
}

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
            return absl::FailedPreconditionError("GFXR capture is in process. Please retrieve the "
                                                 "capture before stopping the application.");
        }
        // Only delete the on device capture directory when the application is closed.
        std::string
             on_device_capture_directory = absl::StrCat(Dive::kDeviceCapturePath,
                                                   "/",
                                                   m_gfxr_capture_file_directory_input_box->text()
                                                   .toStdString());
        auto ret = device->Adb().Run(
        absl::StrFormat("shell rm -rf %s", on_device_capture_directory));
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
        return absl::Status(cleanup_status.code(),
                            absl::StrCat("Failed to cleanup application: ",
                                         cleanup_status.message()));
    }
    return absl::OkStatus();
}

void TraceDialog::closeEvent(QCloseEvent *event)
{
    absl::Status status = StopPackageAndCleanup();

    // The operation was successful, close normally.
    if (status.ok())
    {
        m_run_button->setEnabled(true);
        m_run_button->setText(kStart_Application);
        event->accept();
        return;
    }

    // A specific error occurred that should prevent closing the window.
    if (status.code() == absl::StatusCode::kFailedPrecondition)
    {
        qDebug() << "Preventing window close: " << status.ToString().c_str();
        ShowErrorMessage(QString::fromUtf8(status.message().data(), (int)status.message().size()));
        event->ignore();
    }
    else
    {
        // For any other error, we still allow the window to close to avoid trapping the user.
        qDebug() << "Closing window despite non-critical error: " << status.ToString().c_str();
        ShowErrorMessage(QString::fromUtf8(status.message().data(), (int)status.message().size()));
        m_run_button->setEnabled(true);
        m_run_button->setText(kStart_Application);
        event->accept();
    }
}

void TraceDialog::UpdateDeviceList(bool isInitialized)
{
    auto cur_list = Dive::GetDeviceManager().ListDevice();
    qDebug() << "m_dev_box->currentIndex() " << m_dev_box->currentIndex();
    if (cur_list == m_devices && isInitialized)
    {
        qDebug() << "No changes from the list of the connected devices. ";
        return;
    }

    m_devices = cur_list;
    m_dev_model->clear();

    if (m_devices.empty())
    {
        QStandardItem *item = new QStandardItem("No devices found");
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        m_dev_model->appendRow(item);
        m_dev_box->setCurrentIndex(0);
    }
    else
    {
        for (size_t i = 0; i < m_devices.size(); i++)
        {
            if (i == 0)
            {
                QStandardItem *item = new QStandardItem("Please select a device");
                item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
                m_dev_model->appendRow(item);
                m_dev_box->setCurrentIndex(0);
            }

            QStandardItem *item = new QStandardItem(m_devices[i].GetDisplayName().c_str());
            m_dev_model->appendRow(item);
            // Keep the original selected devices as selected.
            if (m_cur_dev == m_devices[i].m_serial)
            {
                m_dev_box->setCurrentIndex(static_cast<int>(i));
            }
        }
    }
}

void TraceDialog::OnDeviceSelected(const QString &s)
{
    if (s.isEmpty() || m_dev_box->currentIndex() == 0)
    {
        qDebug() << "No devices selected";
        return;
    }
    int dev_index = m_dev_box->currentIndex() - 1;
    assert(static_cast<size_t>(dev_index) < m_devices.size());

    qDebug() << "Device selected: " << m_cur_dev.c_str() << ", index " << dev_index
             << ", m_devices[dev_index].m_serial " << m_devices[dev_index].m_serial.c_str();
    if (m_cur_dev == m_devices[dev_index].m_serial)
    {
        return;
    }

    m_cur_dev = m_devices[dev_index].m_serial;
    auto dev_ret = Dive::GetDeviceManager().SelectDevice(m_cur_dev);
    if (!dev_ret.ok())
    {
        std::string err_msg = absl::StrCat("Failed to select device ",
                                           m_cur_dev.c_str(),
                                           ", error: ",
                                           dev_ret.status().message());
        qDebug() << err_msg.c_str();
        ShowErrorMessage(QString::fromStdString(err_msg));
        return;
    }

    UpdatePackageList();
}

void TraceDialog::OnPackageSelected(const QString &s)
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

void TraceDialog::OnInputCommand(const QString &text)
{
    qDebug() << "Input changed to " << text;
    m_run_button->setEnabled(true);
    m_cur_pkg = text.toStdString();
    m_pkg_box->setCurrentIndex(-1);
    m_app_type_box->setCurrentIndex(-1);
}

void TraceDialog::OnInputArgs(const QString &text)
{
    qDebug() << "Args changed to " << text;
    m_command_args = text.toStdString();
}

bool TraceDialog::StartPackage(Dive::AndroidDevice *device, const std::string &app_type)
{
    if (device == nullptr)
    {
        return false;
    }

    device->CleanupApp().IgnoreError();
    m_run_button->setText("&Starting..");
    m_run_button->setDisabled(true);

    absl::Status ret;
    qDebug() << "Start app on dev: " << m_cur_dev.c_str() << ", package: " << m_cur_pkg.c_str()
             << ", type: " << app_type.c_str() << ", args: " << m_command_args.c_str();

    std::string device_architecture = "";
    if (m_gfxr_capture)
    {
        auto retrieve_device_architecture = device->Adb().RunAndGetResult(
        "shell getprop ro.product.cpu.abi");
        device_architecture = retrieve_device_architecture.value_or("");
        m_gfxr_capture_button->setText(kStart_Gfxr_Runtime_Capture);
        m_gfxr_capture_button->setEnabled(true);

        if (m_gfxr_capture_file_directory_input_box->text() == "")
        {
            m_gfxr_capture_file_directory_input_box->setText(
            QString::fromUtf8(Dive::kDefaultCaptureFolderName));
        }
    }

    if (app_type == "OpenXR APK")
    {
        ret = device->SetupApp(m_cur_pkg,
                               Dive::ApplicationType::OPENXR_APK,
                               m_command_args,
                               device_architecture,
                               m_gfxr_capture_file_directory_input_box->text().toStdString());
    }
    else if (app_type == "Vulkan APK")
    {
        ret = device->SetupApp(m_cur_pkg,
                               Dive::ApplicationType::VULKAN_APK,
                               m_command_args,
                               device_architecture,
                               m_gfxr_capture_file_directory_input_box->text().toStdString());
    }
    else if (app_type == "Command Line Application")
    {
        m_executable = m_cmd_input_box->text().toStdString();
        if (m_executable.empty())
        {
            std::string err_msg = "Please input a valid command to execute";
            qDebug() << err_msg.c_str();
            ShowErrorMessage(QString::fromStdString(err_msg));
            return false;
        }
        qDebug() << "exe: " << m_executable.c_str() << " args: " << m_command_args.c_str();
        ret = device->SetupApp(m_executable, m_command_args, Dive::ApplicationType::VULKAN_CLI);
    }
    if (!ret.ok())
    {
        std::string err_msg = absl::StrCat("Fail to setup for package ",
                                           m_cur_pkg,
                                           " error: ",
                                           ret.message());
        qDebug() << err_msg.c_str();
        ShowErrorMessage(QString::fromStdString(err_msg));
        return false;
    }
    ret = device->StartApp();
    if (!ret.ok())
    {
        std::string err_msg = absl::StrCat("Fail to start package ",
                                           m_cur_pkg,
                                           " error: ",
                                           ret.message());
        qDebug() << err_msg.c_str();
        ShowErrorMessage(QString::fromStdString(err_msg));
        return false;
    }
    auto cur_app = device->GetCurrentApplication();

    if (!cur_app->IsRunning())
    {
        std::string err_msg = absl::StrCat("Process for package ",
                                           m_cur_pkg,
                                           " not found, possibly crashed.");
        qDebug() << err_msg.c_str();
        ShowErrorMessage(QString::fromStdString(err_msg));
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
        std::string
        err_msg = "No device/application selected. Please select a device and application and "
                  "then try again.";
        ShowErrorMessage(QString::fromStdString(err_msg));
        return;
    }
    device->EnableGfxr(m_gfxr_capture);
    absl::Status ret = device->SetupDevice();
    if (!ret.ok())
    {
        std::string err_msg = absl::StrCat("Fail to setup device: ", ret.message());
        qDebug() << err_msg.c_str();
        ShowErrorMessage(QString::fromStdString(err_msg));
        return;
    }
    int ty = m_app_type_box->currentIndex();
    if (ty == -1)
    {
        ShowErrorMessage(QString("Please select application type"));
        return;
    }
    std::string ty_str = kAppTypes[ty];

    if (m_run_button->text() == QString(kStart_Application))
    {
        if (!StartPackage(device, ty_str))
        {
            m_run_button->setDisabled(false);
            m_run_button->setText(kStart_Application);
        }
    }
    else
    {
        qDebug() << "Stop package and cleanup: " << m_cur_pkg.c_str();
        absl::Status status = StopPackageAndCleanup();
        if (!status.ok())
        {
            qDebug() << "Failed to stop package or cleanup: " << status.ToString().c_str();
            ShowErrorMessage(
            QString::fromUtf8(status.message().data(), (int)status.message().size()));

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
    }
}

void TraceDialog::OnTraceClicked()
{
    QProgressDialog *progress_bar = new QProgressDialog("Downloading ... ", nullptr, 0, 100, this);
    progress_bar->setMinimumWidth(this->minimumWidth() + 50);
    progress_bar->setMinimumHeight(this->minimumHeight() + 50);
    progress_bar->setAutoReset(true);
    progress_bar->setAutoClose(true);
    progress_bar->setMinimumDuration(0);
    TraceWorker *workerThread = new TraceWorker(progress_bar);
    connect(workerThread, &TraceWorker::TraceAvailable, this, &TraceDialog::OnTraceAvailable);
    connect(workerThread, &TraceWorker::finished, workerThread, &QObject::deleteLater);
    connect(workerThread, &TraceWorker::ErrorMessage, this, &TraceDialog::ShowErrorMessage);
    workerThread->start();
    std::cout << "OnTraceClicked done " << std::endl;
}

void TraceDialog::OnTraceAvailable(QString const &trace_path)
{
    emit TraceAvailable(trace_path);
}

void ProgressBarWorker::run()
{
    int64_t cur_size = 0;
    int     percent = 0;
    while (m_capture_size && cur_size < m_capture_size)
    {
        QThread::msleep(10);  // 10 milliseconds
        cur_size = GetDownloadedSize();
        percent = cur_size * 100 / m_capture_size;
        emit SetProgressBarValue(percent);
        std::cout << "percent " << percent << ", cursize: " << cur_size << ", total "
                  << m_capture_size << std::endl;
    }
    emit SetProgressBarValue(100);
}

void TraceWorker::run()
{
    auto device = Dive::GetDeviceManager().GetDevice();
    if (device == nullptr)
    {
        qDebug() << "Failed to connect to device";
        return;
    }
    auto app = device->GetCurrentApplication();
    if (app == nullptr || !app->IsRunning())
    {
        std::string err_msg = "Application is not running, possibly crashed.";
        qDebug() << err_msg.c_str();
        emit ErrorMessage(QString::fromStdString(err_msg));
        return;
    }

    Network::TcpClient client;
    const std::string  host = "127.0.0.1";
    int                port = device->Port();
    auto               status = client.Connect(host, port);
    if (!status.ok())
    {
        std::string err_msg(status.message());
        qDebug() << "Connection failed: " << err_msg.c_str();
        return;
    }

    absl::StatusOr<std::string> capture_file_path = client.StartPm4Capture();
    if (capture_file_path.ok())
    {
        qDebug() << "Trigger capture: " << (*capture_file_path).c_str();
    }
    else
    {
        std::string err_msg = absl::StrCat("Trigger capture failed: ",
                                           capture_file_path.status().message());
        qDebug() << err_msg.c_str();
        emit ErrorMessage(QString::fromStdString(err_msg));
        return;
    }
    std::string           download_path = ".";
    std::filesystem::path p(*capture_file_path);
    std::filesystem::path target_download_path(download_path);
    target_download_path /= p.filename();
    qDebug() << "Begin to download the capture file to "
             << target_download_path.generic_string().c_str();

    auto file_size = client.GetCaptureFileSize(p.generic_string());
    if (file_size.ok())
    {
        qDebug() << "Capture file size: " << std::to_string(*file_size).c_str();
    }
    else
    {
        std::string err_msg = absl::StrCat("Failed to retrieve capture file size, error: ",
                                           file_size.status().message());
        qDebug() << err_msg.c_str();
        emit ErrorMessage(QString::fromStdString(err_msg));
        return;
    }

    ProgressBarWorker *progress_bar_worker = new ProgressBarWorker(m_progress_bar,
                                                                   target_download_path
                                                                   .generic_string(),
                                                                   *file_size,
                                                                   false);
    connect(progress_bar_worker,
            &ProgressBarWorker::finished,
            progress_bar_worker,
            &QObject::deleteLater);
    connect(this,
            &TraceWorker::DownloadedSize,
            progress_bar_worker,
            &ProgressBarWorker::SetDownloadedSize);
    connect(progress_bar_worker,
            &ProgressBarWorker::SetProgressBarValue,
            m_progress_bar,
            &QProgressDialog::setValue);

    progress_bar_worker->start();

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    qDebug() << "Begin to download the capture file to "
             << target_download_path.generic_string().c_str();

    auto progress = [this](size_t size) { emit DownloadedSize(size); };
    status = client.DownloadFileFromServer(*capture_file_path,
                                           target_download_path.generic_string(),
                                           progress);
    if (status.ok())
    {
        qDebug() << "Capture saved at "
                 << std::filesystem::canonical(target_download_path).generic_string().c_str();
    }
    else
    {
        std::string err_msg = absl::StrCat("Failed to download capture file, error: ",
                                           status.message());
        qDebug() << err_msg.c_str();
        emit ErrorMessage(QString::fromStdString(err_msg));
        return;
    }
    int64_t time_used_to_load_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now() - begin)
                                   .count();
    qDebug() << "Time used to download the capture is " << (time_used_to_load_ms / 1000.0)
             << " seconds.";

    QString capture_saved_path(target_download_path.generic_string().c_str());
    emit    TraceAvailable(capture_saved_path);
}

void GfxrCaptureWorker::SetGfxrSourceCaptureDir(const std::string &source_capture_dir)
{
    m_source_capture_dir = source_capture_dir;
}

void GfxrCaptureWorker::SetGfxrTargetCaptureDir(const std::string &target_capture_dir)
{
    if (!std::filesystem::exists(target_capture_dir))
    {

        std::error_code ec;
        if (!std::filesystem::create_directories(target_capture_dir, ec))
        {
            std::string err_msg = absl::StrCat("Error creating directory: ", ec.message());
            qDebug() << err_msg.c_str();
            emit ErrorMessage(QString::fromStdString(err_msg));
            return;
        }

        m_target_capture_dir = target_capture_dir;
    }
    else
    {
        // If the target directory already exists on the local machine, append a number to it to
        // differentiate.
        int                   counter = 1;
        std::filesystem::path newDirPath;
        while (true)
        {
            newDirPath = std::filesystem::path(target_capture_dir + "_" + std::to_string(counter));
            if (!std::filesystem::exists(newDirPath))
            {
                std::error_code ec;

                if (!std::filesystem::create_directories(newDirPath, ec))
                {
                    std::string err_msg = absl::StrCat("Error creating directory: ", ec.message());
                    qDebug() << err_msg.c_str();
                    emit ErrorMessage(QString::fromStdString(err_msg));
                    return;
                }
                m_target_capture_dir = newDirPath;
                break;
            }
            counter++;
        }
    }
}

bool GfxrCaptureWorker::areTimestampsCurrent(Dive::AndroidDevice     *device,
                                             std::vector<std::string> previous_timestamps)
{
    std::vector<std::string> current_time_stamps;
    std::string              get_first_current_timestamp_command = absl::StrCat("shell stat -c %Y ",
                                                                   m_source_capture_dir,
                                                                   "/",
                                                                   m_file_list[0].data());
    std::string get_second_current_timestamp_command = absl::StrCat("shell stat -c %Y ",
                                                                    m_source_capture_dir,
                                                                    "/",
                                                                    m_file_list[1].data());
    std::string get_third_current_timestamp_command = absl::StrCat("shell stat -c %Y ",
                                                                   m_source_capture_dir,
                                                                   "/",
                                                                   m_file_list[2].data());
    absl::StatusOr<std::string> first_current_timestamp = device->Adb().RunAndGetResult(
    get_first_current_timestamp_command);
    absl::StatusOr<std::string> second_current_timestamp = device->Adb().RunAndGetResult(
    get_second_current_timestamp_command);
    absl::StatusOr<std::string> third_current_timestamp = device->Adb().RunAndGetResult(
    get_third_current_timestamp_command);

    current_time_stamps.push_back(first_current_timestamp->data());
    current_time_stamps.push_back(second_current_timestamp->data());
    current_time_stamps.push_back(third_current_timestamp->data());
    return (current_time_stamps[0] == previous_timestamps[0] &&
            current_time_stamps[1] == previous_timestamps[1] &&
            current_time_stamps[2] == previous_timestamps[2]);
}

absl::StatusOr<int64_t> GfxrCaptureWorker::getGfxrCaptureDirectorySize(Dive::AndroidDevice *device)
{
    // Retrieve the names of the files in the capture directory on the device.
    std::string                 ls_command = "shell ls " + m_source_capture_dir;
    absl::StatusOr<std::string> ls_output = device->Adb().RunAndGetResult(ls_command);

    if (!ls_output.ok())
    {
        std::cout << "Error getting capture files: " << ls_output.status().message() << std::endl;
        return ls_output.status();
    }

    m_file_list = absl::StrSplit(std::string(ls_output->data()), '\n');

    for (std::string &file_with_trailing : m_file_list)
    {
        // Windows-style line endings use \r\n. When absl::StrSplit splits by \n, the \r remains
        // at the end of each line if the input string originated from a Windows-style line
        // ending.
        if (!file_with_trailing.empty() && file_with_trailing.back() == '\r')
        {
            file_with_trailing.pop_back();
        }
    }

    // Ensure that the .gfxa, .gfxr, and .png file sizes are set and neither is being written to.
    int64_t                  size = 0;
    std::vector<std::string> current_timestamps;

    while (true)
    {
        for (std::string file : m_file_list)
        {
            std::string path = absl::StrCat(m_source_capture_dir, "/", file.data());

            // Get the size of the file.
            std::string get_file_size_command = "shell stat -c %s " + path;

            // Get the timestamp for last time the file was updated.
            std::string get_file_update_timestamp_command = "shell stat -c %Y " + path;

            absl::StatusOr<std::string> str_num = device->Adb().RunAndGetResult(
            get_file_size_command);
            absl::StatusOr<std::string> file_update_timestamp = device->Adb().RunAndGetResult(
            get_file_update_timestamp_command);
            int64_t num = std::stoll(str_num->c_str());
            // If a file size is 0, then the file has finished being written to yet. Sleep and
            // restart the size calculation.
            if (num == 0)
            {
                QThread::msleep(10);
                size = 0;
                current_timestamps.clear();
                break;
            }

            // Add the timestamp for the last time the file was udpated.
            current_timestamps.push_back(file_update_timestamp->data());

            // Update the total size of the gfxr capture directory.
            size += std::stoll(str_num->c_str());
        }

        // If the size is greater than zero and the timestamps have been recorded check if the
        // timestamps are current.
        if (size > 0 && !current_timestamps.empty())
        {

            // If the timestamps are current, return the size of the directory.
            if (areTimestampsCurrent(device, current_timestamps))
            {
                current_timestamps.clear();
                return size;
            }
            current_timestamps.clear();
            size = 0;
        }
    }
}

void GfxrCaptureWorker::run()
{
    auto device = Dive::GetDeviceManager().GetDevice();
    if (device == nullptr)
    {
        qDebug() << "Failed to connect to device";
        return;
    }

    int64_t capture_directory_size = 0;
    {
        absl::StatusOr<int64_t> ret = getGfxrCaptureDirectorySize(device);
        if (!ret.ok())
        {
            std::string err_msg = absl::StrCat("Failed to get size of gfxr capture directory",
                                               " error: ",
                                               ret.status().message());
            qDebug() << err_msg.c_str();
            emit ErrorMessage(QString::fromStdString(err_msg));
            return;
        }
        capture_directory_size = *ret;
    }

    ProgressBarWorker *progress_bar_worker = new ProgressBarWorker(m_progress_bar,
                                                                   m_target_capture_dir
                                                                   .generic_string(),
                                                                   capture_directory_size,
                                                                   true);
    connect(progress_bar_worker,
            &ProgressBarWorker::finished,
            progress_bar_worker,
            &QObject::deleteLater);

    connect(this,
            &GfxrCaptureWorker::DownloadedSize,
            progress_bar_worker,
            &ProgressBarWorker::SetDownloadedSize);

    progress_bar_worker->start();

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    qDebug() << "Begin to download the trace file to "
             << m_target_capture_dir.generic_string().c_str();

    int64_t     size = 0;
    std::string gfxr_stem;
    std::string original_screenshot_path;

    std::string gfxr_capture_file_path;
    // Retrieve each file in the capture directory (capture file and asset file).
    for (std::string file : m_file_list)
    {
        std::filesystem::path filename = file.data();
        std::filesystem::path target_path = m_target_capture_dir;
        target_path /= filename;

        // Source path is intended for Android, cannot use std::filesystem here
        std::string source_file = absl::StrCat(m_source_capture_dir, "/", filename.string());

        auto retrieve_file = device->RetrieveFile(source_file, m_target_capture_dir.string());

        if (!retrieve_file.ok())
        {
            std::cout << "Failed to retrieve gfxr capture: " << retrieve_file.message()
                      << std::endl;
            qDebug() << retrieve_file.message().data();
            emit ErrorMessage(QString::fromStdString(retrieve_file.message().data()));
            return;
        }

        if (Dive::IsGfxrFile(filename))
        {
            if (gfxr_stem.empty())
            {
                gfxr_stem = filename.stem().string();
            }
            gfxr_capture_file_path = target_path.string();
        }
        else if (Dive::IsPngFile(filename))
        {
            original_screenshot_path = target_path.string();
        }

        size += static_cast<int64_t>(std::filesystem::file_size(target_path.string()));
        emit DownloadedSize(size);
    }

    if (!original_screenshot_path.empty() && !gfxr_stem.empty())
    {
        Dive::ComponentFilePaths component_files = {};
        {
            absl::StatusOr<Dive::ComponentFilePaths> ret = Dive::GetComponentFilesHostPaths(m_target_capture_dir, gfxr_stem);
            component_files = *ret;
        }

        std::error_code error_code;
        std::filesystem::rename(original_screenshot_path,
                                component_files.screenshot_png,
                                error_code);

        if (error_code)
        {
            qDebug() << "Failed to rename screenshot file from " << original_screenshot_path.c_str()
                     << " to " << component_files.screenshot_png.c_str() << ": "
                     << error_code.message().c_str();
        }
    }

    int64_t time_used_to_load_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now() - begin)
                                   .count();
    qDebug() << "Time used to download the gfxr capture directory is "
             << (time_used_to_load_ms / 1000.0) << " seconds.";

    QString capture_saved_path(gfxr_capture_file_path.c_str());

    emit GfxrCaptureAvailable(capture_saved_path);
}

void TraceDialog::OnDevListRefresh()
{
    UpdateDeviceList(true);
}

void TraceDialog::OnAppListRefresh()
{
    UpdatePackageList();
}

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
        std::string err_msg = absl::StrCat("Failed to list package for device ",
                                           m_cur_dev,
                                           " error: ",
                                           ret.status().message());
        qDebug() << err_msg.c_str();
        ShowErrorMessage(QString::fromStdString(err_msg));
        return;
    }
    m_pkg_list = *ret;

    const QSignalBlocker blocker(
    m_pkg_box);  // Do not emit index changed event when update the model
    m_pkg_model->clear();
    for (size_t i = 0; i < m_pkg_list.size(); i++)
    {
        QStandardItem *item = new QStandardItem(m_pkg_list[i].c_str());
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

void TraceDialog::OnPackageListFilterApplied(const QString &filter)
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
    m_capture_button->hide();
    m_gfxr_capture_button->show();
    m_gfxr_capture_file_on_device_directory_label->show();
    m_gfxr_capture_file_directory_input_box->show();
    m_gfxr_capture_file_local_directory_label->show();
    m_gfxr_capture_file_local_directory_input_box->show();
}

void TraceDialog::HideGfxrFields()
{
    m_capture_button->show();
    m_gfxr_capture_button->hide();
    m_gfxr_capture_file_on_device_directory_label->hide();
    m_gfxr_capture_file_directory_input_box->hide();
    m_gfxr_capture_file_local_directory_label->hide();
    m_gfxr_capture_file_local_directory_input_box->hide();
}

void TraceDialog::UseGfxrCapture(bool enable)
{
    if (enable)
    {
        ShowGfxrFields();
    }
    else
    {
        HideGfxrFields();
    }

    m_gfxr_capture = enable;
}

void TraceDialog::OnGfxrCaptureClicked()
{
    auto         device = Dive::GetDeviceManager().GetDevice();
    absl::Status ret;
    if (m_gfxr_capture_button->text() == kRetrieve_Gfxr_Runtime_Capture)
    {
        ret = device->Adb().Run("shell setprop debug.gfxrecon.capture_android_trigger false");
        if (!ret.ok())
        {
            std::string err_msg = absl::StrCat("Failed to stop runtime gfxr capture ",
                                               m_cur_pkg,
                                               " error: ",
                                               ret.message());
            qDebug() << err_msg.c_str();
            ShowErrorMessage(QString::fromStdString(err_msg));
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
            std::string err_msg = absl::StrCat("Failed to start runtime gfxr capture ",
                                               m_cur_pkg,
                                               " error: ",
                                               ret.message());
            qDebug() << err_msg.c_str();
            ShowErrorMessage(QString::fromStdString(err_msg));
            return;
        }

        std::filesystem::path capture_path(
        m_gfxr_capture_file_directory_input_box->text().toStdString());
        ret = device->TriggerScreenCapture(capture_path);
        if (!ret.ok())
        {
            std::string err_msg = absl::StrCat("Failed to create capture screenshot: ",
                                               ret.message());
            qDebug() << err_msg.c_str();
            ShowErrorMessage(QString::fromStdString(err_msg));
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

    if (m_gfxr_capture_file_local_directory_input_box->text() == "")
    {
        m_gfxr_capture_file_local_directory_input_box->setText(
        "./" + QString::fromUtf8(Dive::kDefaultCaptureFolderName));
    }

    std::string
    on_device_capture_file_directory = absl::StrCat(std::string(Dive::kDeviceCapturePath),
                                                    "/",
                                                    m_gfxr_capture_file_directory_input_box->text()
                                                    .toStdString());

    QProgressDialog *progress_bar = new QProgressDialog("Downloading GFXR Capture ... ",
                                                        nullptr,
                                                        0,
                                                        100,
                                                        this);
    progress_bar->setObjectName("gfxr_download_progress");
    progress_bar->setMinimumWidth(this->minimumWidth() + 50);
    progress_bar->setMinimumHeight(this->minimumHeight() + 50);
    progress_bar->setAutoReset(true);
    progress_bar->setAutoClose(true);

    GfxrCaptureWorker *workerThread = new GfxrCaptureWorker(progress_bar);
    workerThread->SetGfxrSourceCaptureDir(on_device_capture_file_directory);

    workerThread->SetGfxrTargetCaptureDir(
    m_gfxr_capture_file_local_directory_input_box->text().toStdString());

    connect(workerThread,
            &GfxrCaptureWorker::GfxrCaptureAvailable,
            this,
            &TraceDialog::OnGFXRCaptureAvailable);
    connect(workerThread, &GfxrCaptureWorker::finished, workerThread, &QObject::deleteLater);
    connect(workerThread, &GfxrCaptureWorker::ErrorMessage, this, &TraceDialog::ShowErrorMessage);
    workerThread->start();

    m_gfxr_capture_button->setEnabled(false);
}

void TraceDialog::OnGFXRCaptureAvailable(QString const &capture_path)
{
    QProgressDialog *progress_bar = findChild<QProgressDialog *>("gfxr_download_progress");
    if (progress_bar)
    {
        progress_bar->close();
        progress_bar->deleteLater();
    }
    std::string success_msg = "Capture successfully saved at " + capture_path.toStdString();
    qDebug() << success_msg.c_str();
    ShowErrorMessage(QString::fromStdString(success_msg));
    emit TraceAvailable(capture_path);
}
