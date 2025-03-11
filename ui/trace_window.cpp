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

#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "capture_service/android_application.h"
#include "capture_service/client.h"
#include "capture_service/constants.h"
#include "capture_service/device_mgr.h"

namespace
{
const std::vector<std::string> kAppTypes{ "Vulkan APK", "OpenXR APK", "Command Line Application" };
}

// =================================================================================================
// TraceDialog
// =================================================================================================
TraceDialog::TraceDialog(QWidget *parent)
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
    m_gfxr_retrieve_button = new QPushButton(kRetrieve_Gfxr_Capture, this);
    m_gfxr_retrieve_button->setEnabled(false);
    m_gfxr_retrieve_button->hide();

    m_dev_refresh_button = new QPushButton("&Refresh", this);
    m_pkg_refresh_button = new QPushButton("&Refresh", this);
    m_pkg_filter_button = new QPushButton(this);
    m_pkg_filter = new PackageFilter(this);
    m_pkg_filter_button->setIcon(QIcon(":/images/filter.png"));
    m_pkg_refresh_button->setDisabled(true);
    m_pkg_filter_button->setDisabled(true);
    m_pkg_filter_button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_pkg_filter->hide();
    m_pkg_list_options = Dive::AndroidDevice::PackageListOptions::kAll;

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
    m_button_layout->addWidget(m_gfxr_retrieve_button);

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
    QObject::connect(m_gfxr_retrieve_button,
                     &QPushButton::clicked,
                     this,
                     &TraceDialog::OnGfxrRetrieveClicked);

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
                     &PackageFilter::filtersApplied,
                     this,
                     &TraceDialog::OnPackageListFilterApplied);
}

TraceDialog::~TraceDialog()
{
    qDebug() << "TraceDialog destroyed.";
    Dive::GetDeviceManager().RemoveDevice();
}

void ShowErrorMessage(const std::string &err_msg)
{
    QMessageBox msgBox;
    msgBox.setText(err_msg.c_str());
    msgBox.exec();
    return;
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
        ShowErrorMessage(err_msg);
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

    device->CleanupAPP().IgnoreError();
    m_run_button->setText("&Starting..");
    m_run_button->setDisabled(true);

    absl::Status ret;
    qDebug() << "Start app on dev: " << m_cur_dev.c_str() << ", package: " << m_cur_pkg.c_str()
             << ", type: " << app_type.c_str() << ", args: " << m_command_args.c_str();

    std::string device_architecture = "";
    if (m_gfxr_capture)
    {
        auto retrieve_device_architecture = device->Adb()
                                            .RunAndGetResult("shell getprop ro.product.cpu.abi",
                                                             true);
        device_architecture = retrieve_device_architecture.value_or("");
        m_gfxr_capture_button->setText(kStart_Gfxr_Runtime_Capture);
        m_gfxr_capture_button->setEnabled(true);

        if (m_gfxr_capture_file_directory_input_box->text() == "")
        {
            m_gfxr_capture_file_directory_input_box->setText("gfxr_capture");
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
            ShowErrorMessage(err_msg);
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
        ShowErrorMessage(err_msg);
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
        ShowErrorMessage(err_msg);
        return false;
    }
    auto cur_app = device->GetCurrentApplication();

    if (!cur_app->IsRunning())
    {
        std::string err_msg = absl::StrCat("Process for package ",
                                           m_cur_pkg,
                                           " not found, possibly crashed.");
        qDebug() << err_msg.c_str();
        ShowErrorMessage(err_msg);
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
        ShowErrorMessage(err_msg);
        return;
    }
    device->EnableGfxr(m_gfxr_capture);
    absl::Status ret = device->SetupDevice();
    if (!ret.ok())
    {
        std::string err_msg = absl::StrCat("Fail to setup device: ", ret.message());
        qDebug() << err_msg.c_str();
        ShowErrorMessage(err_msg);
        return;
    }
    int ty = m_app_type_box->currentIndex();
    if (ty == -1)
    {
        ShowErrorMessage("Please select application type");
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
        qDebug() << "Stop package " << m_cur_pkg.c_str();

        if (m_gfxr_capture)
        {
            if (m_gfxr_capture_button->text() == kStop_Gfxr_Runtime_Capture &&
                m_gfxr_capture_button->isEnabled())
            {
                std::string err_msg = "Failed to stop application. Gfxr capture in process. Please "
                                      "stop the capture before stopping the application.";
                qDebug() << err_msg.c_str();
                ShowErrorMessage(err_msg);
                return;
            }

            m_gfxr_capture_button->setEnabled(false);
        }
        else
        {
            m_capture_button->setEnabled(false);
        }

        device->StopApp().IgnoreError();

        ret = device->GetCurrentApplication()->Cleanup();
        if (!ret.ok())
        {
            std::string err_msg = absl::StrCat("Failed to cleanup application: ", ret.message());
            qDebug() << err_msg.c_str();
            ShowErrorMessage(err_msg);
        }
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
    TraceWorker *workerThread = new TraceWorker(progress_bar);
    connect(workerThread, &TraceWorker::TraceAvailable, this, &TraceDialog::OnTraceAvailable);
    connect(workerThread, &TraceWorker::finished, workerThread, &QObject::deleteLater);
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

    if (m_gfxr_capture)
    {
        m_progress_bar->show();

        if (m_capture_size == 0)
        {  // Handle empty directory
            m_progress_bar->setValue(100);
            QCoreApplication::processEvents();
            return;
        }

        if (std::filesystem::exists(m_capture_name) &&
            std::filesystem::is_directory(m_capture_name))
        {
            for (const auto &entry : std::filesystem::recursive_directory_iterator(m_capture_name))
            {
                if (std::filesystem::is_regular_file(entry))
                {
                    try
                    {
                        cur_size += std::filesystem::file_size(entry);
                        percent = static_cast<int>(
                        cur_size * 100 / m_capture_size);  // Cast to int after calculation
                        m_progress_bar->setValue(percent);
                        QCoreApplication::processEvents();
                        QThread::msleep(10);  // 10 milliseconds
                    }
                    catch (const std::filesystem::filesystem_error &e)
                    {
                        std::cerr << "Error getting file size: " << e.what() << std::endl;
                    }
                }
            }
        }
    }
    else
    {
        m_progress_bar->show();

        while (m_capture_size && cur_size < m_capture_size)
        {
            cur_size = GetDownloadedSize();
            percent = cur_size * 100 / m_capture_size;
            m_progress_bar->setValue(percent);
            std::cout << "percent " << percent << ", cursize: " << cur_size << ", total "
                      << m_capture_size << std::endl;
            QThread::msleep(10);  // 10 milliseconds
        }
        m_progress_bar->setValue(100);
    }
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
        ShowErrorMessage(err_msg);
        return;
    }
    const std::string server_str = absl::StrFormat("localhost:%d", device->Port());

    Dive::DiveClient client(grpc::CreateChannel(server_str, grpc::InsecureChannelCredentials()));
    absl::StatusOr<std::string> reply = client.TestConnection();
    if (reply.ok())
    {
        qDebug() << "Test connection succeed";
    }
    else
    {
        std::string err_msg(reply.status().message());
        qDebug() << "Test connection failed: " << err_msg.c_str();
        return;
    }

    absl::StatusOr<std::string> trace_file_path = client.RequestStartTrace();
    if (trace_file_path.ok())
    {
        qDebug() << "Trigger capture: " << (*trace_file_path).c_str();
    }
    else
    {
        std::string err_msg = absl::StrCat("Trigger capture failed: ",
                                           trace_file_path.status().message());
        qDebug() << err_msg.c_str();
        ShowErrorMessage(err_msg);
        return;
    }
    std::string           capture_path = ".";
    std::filesystem::path p(*trace_file_path);
    std::filesystem::path target(capture_path);
    target /= p.filename();
    qDebug() << "Begin to download the trace file to " << target.generic_string().c_str();
    auto    ret = client.GetTraceFileSize(p.generic_string());
    int64_t file_size = 0;
    if (ret.ok())
    {
        file_size = *ret;
        std::cout << " Trace file size: " << file_size << std::endl;
    }
    else
    {
        std::string err_msg = absl::StrCat("Failed to retrieve trace file size, error: ",
                                           ret.status().message());
        qDebug() << err_msg.c_str();
        ShowErrorMessage(err_msg);
        return;
    }
    // m_progress_bar->reset();

    ProgressBarWorker *progress_bar_worker = new ProgressBarWorker(m_progress_bar,
                                                                   target.generic_string(),
                                                                   file_size,
                                                                   false);
    connect(progress_bar_worker,
            &TraceWorker::finished,
            progress_bar_worker,
            &QObject::deleteLater);
    connect(this,
            &TraceWorker::DownloadedSize,
            progress_bar_worker,
            &ProgressBarWorker::SetDownloadedSize);

    progress_bar_worker->start();

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    qDebug() << "Begin to download the trace file to " << target.generic_string().c_str();

    auto progress = [this](int64_t size) { emit DownloadedSize(size); };
    auto r = client.DownloadFile(*trace_file_path, target.generic_string(), progress);
    if (r.ok())
        qDebug() << "Capture saved at "
                 << std::filesystem::canonical(target).generic_string().c_str();
    else
    {
        std::string err_msg = absl::StrCat("Failed to retrieve trace file, error: ", r.message());
        qDebug() << err_msg.c_str();
        ShowErrorMessage(err_msg);
        return;
    }
#if defined(DIVE_ENABLE_PERFETTO)
    // Download perfetto trace file
    std::string on_device_path = *trace_file_path;
    on_device_path += ".perfetto";
    std::string download_path = target.generic_string() + ".perfetto";
    r = device->RetrieveTrace(on_device_path, download_path, false);
    if (r.ok())
    {
        qDebug() << "Capture saved at " << download_path.c_str();
    }
    else
    {
        std::string err_msg = absl::StrCat("Failed to retrieve trace file, error: ", r.message());
        qDebug() << err_msg.c_str();
        ShowErrorMessage(err_msg);
        return;
    }
#endif
    int64_t time_used_to_load_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now() - begin)
                                   .count();
    qDebug() << "Time used to download the capture is " << (time_used_to_load_ms / 1000.0)
             << " seconds.";

    QString capture_saved_path(target.generic_string().c_str());
    emit    TraceAvailable(capture_saved_path);
}

void GfxrCaptureWorker::SetGfxrCapturePath(const std::string &capture_path)
{
    m_capture_path = capture_path;
}

absl::StatusOr<int64_t> GfxrCaptureWorker::getGfxrCaptureDirectorySize(Dive::AndroidDevice *device)
{
    std::string                 command = "shell du -sb " + m_capture_path;
    absl::StatusOr<std::string> output = device->Adb().RunAndGetResult(command);

    if (!output.ok())
    {
        std::cout << "Error checking directory size: " << output.status().message() << std::endl;
    }

    std::stringstream ss(output->c_str());
    int64_t           size;
    ss >> size;

    return size;
}

void GfxrCaptureWorker::run()
{
    const std::string server_str = "localhost:19999";

    auto device = Dive::GetDeviceManager().GetDevice();
    if (device == nullptr)
    {
        qDebug() << "Failed to connect to device";
        return;
    }

    std::string           target_capture_path = ".";
    std::filesystem::path p(m_capture_path);
    std::filesystem::path target(target_capture_path);

    auto ret = getGfxrCaptureDirectorySize(device);
    if (!ret.ok())
    {
        std::string err_msg = absl::StrCat("Failed to get size of gfxr capture directory",
                                           " error: ",
                                           ret.status().message());
        qDebug() << err_msg.c_str();
        ShowErrorMessage(err_msg);
        return;
    }

    int64_t file_size = *ret;

    ProgressBarWorker *progress_bar_worker = new ProgressBarWorker(m_progress_bar,
                                                                   target.generic_string(),
                                                                   file_size,
                                                                   true);
    connect(progress_bar_worker,
            &TraceWorker::finished,
            progress_bar_worker,
            &QObject::deleteLater);

    progress_bar_worker->start();

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    qDebug() << "Begin to download the trace file to " << target.generic_string().c_str();
    auto r = device->RetrieveTrace(m_capture_path, target.generic_string());
    m_progress_bar->setValue(100);
    progress_bar_worker->terminate();
    if (!r.ok())
    {
        std::string err_msg = absl::StrCat("Failed to retrieve gfxr capture directory, error: ",
                                           r.message());
        qDebug() << err_msg.c_str();
        ShowErrorMessage(err_msg);
        return;
    }
    int64_t time_used_to_load_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now() - begin)
                                   .count();
    qDebug() << "Time used to download the gfxr capture directory is "
             << (time_used_to_load_ms / 1000.0) << " seconds.";

    QString capture_saved_path(target.generic_string().c_str());
    emit    GfxrCaptureAvailable(capture_saved_path);
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
        ShowErrorMessage(err_msg);
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

void TraceDialog::OnPackageListFilterApplied(QSet<QString> filters)
{
    if (filters.contains("All"))
    {
        m_pkg_list_options = Dive::AndroidDevice::PackageListOptions::kAll;
    }
    else if (filters.contains("Debuggable"))
    {
        m_pkg_list_options = Dive::AndroidDevice::PackageListOptions::kDebuggableOnly;
    }
    else if (filters.contains("Non-Debuggable"))
    {
        m_pkg_list_options = Dive::AndroidDevice::PackageListOptions::kNonDebuggableOnly;
    }
    UpdatePackageList();
    m_pkg_filter_label->hide();
    m_pkg_filter->hide();
}

void TraceDialog::ShowGfxrFields()
{
    m_args_label->hide();
    m_args_input_box->hide();
    m_capture_button->hide();
    m_gfxr_capture_button->show();
    m_gfxr_retrieve_button->show();
    m_gfxr_capture_file_on_device_directory_label->show();
    m_gfxr_capture_file_directory_input_box->show();
    m_gfxr_capture_file_local_directory_label->show();
    m_gfxr_capture_file_local_directory_input_box->show();
}

void TraceDialog::HideGfxrFields()
{
    m_args_label->show();
    m_args_input_box->show();
    m_capture_button->show();
    m_gfxr_capture_button->hide();
    m_gfxr_retrieve_button->hide();
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
    if (m_gfxr_capture_button->text() == kStop_Gfxr_Runtime_Capture)
    {
        ret = device->Adb().Run("shell setprop debug.gfxrecon.capture_android_trigger false");
        if (!ret.ok())
        {
            std::string err_msg = absl::StrCat("Failed to stop runtime gfxr capture ",
                                               m_cur_pkg,
                                               " error: ",
                                               ret.message());
            qDebug() << err_msg.c_str();
            ShowErrorMessage(err_msg);
            return;
        }

        m_gfxr_capture_button->setText(kStart_Gfxr_Runtime_Capture);
        m_gfxr_retrieve_button->setEnabled(true);
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
            ShowErrorMessage(err_msg);
            return;
        }
        m_gfxr_capture_button->setText(kStop_Gfxr_Runtime_Capture);
        m_gfxr_retrieve_button->setEnabled(false);
    }
}

void TraceDialog::OnGfxrRetrieveClicked()
{
    auto device = Dive::GetDeviceManager().GetDevice();

    if (device == nullptr)
    {
        qDebug() << "Failed to connect to device";
        return;
    }

    std::string capture_path = ".";
    if (m_gfxr_capture_file_local_directory_input_box->text() == "")
    {
        m_gfxr_capture_file_local_directory_input_box->setText(".");
    }

    std::string capture_file_directory = Dive::kDeviceCaptureDirectory +
                                         m_gfxr_capture_file_directory_input_box->text()
                                         .toStdString();

    QProgressDialog *progress_bar = new QProgressDialog("Downloading GFXR Capture ... ",
                                                        nullptr,
                                                        0,
                                                        100,
                                                        this);
    progress_bar->setMinimumWidth(this->minimumWidth() + 50);
    progress_bar->setMinimumHeight(this->minimumHeight() + 50);
    progress_bar->setAutoReset(true);
    progress_bar->setAutoClose(true);

    GfxrCaptureWorker *workerThread = new GfxrCaptureWorker(progress_bar);
    workerThread->SetGfxrCapturePath(capture_file_directory);

    connect(workerThread,
            &GfxrCaptureWorker::GfxrCaptureAvailable,
            this,
            &TraceDialog::OnGFXRCaptureAvailable);
    connect(workerThread, &TraceWorker::finished, workerThread, &QObject::deleteLater);
    workerThread->start();

    m_gfxr_retrieve_button->setEnabled(false);
    m_gfxr_capture_button->setEnabled(false);
}

void TraceDialog::OnGFXRCaptureAvailable(QString const &capture_path)
{
    std::string success_msg = "Capture successfully saved at " + capture_path.toStdString();
    qDebug() << success_msg.c_str();
    ShowErrorMessage(success_msg);
    // TODO: Load gfxr capture properly
    // emit TraceAvailable(capture_path);
}
