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

#include <qcombobox.h>
#include <qdebug.h>
#include <qmessagebox.h>
#include <qprogressdialog.h>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QThread>
#include <QVBoxLayout>
#include <cstdint>
#include <filesystem>

#include "absl/strings/str_cat.h"
#include "capture_service/client.h"
#include "capture_service/device_mgr.h"

namespace
{
const std::vector<std::string> kAppTypes{ "Vulkan", "OpenXR" };
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

    m_dev_model = new QStandardItemModel();
    m_pkg_model = new QStandardItemModel();
    m_app_type_model = new QStandardItemModel();

    m_dev_box = new QComboBox();
    m_pkg_box = new QComboBox();
    m_app_type_box = new QComboBox();

    m_button_layout = new QHBoxLayout();
    m_run_button = new QPushButton("&Start Application", this);
    m_run_button->setEnabled(false);
    m_capture_button = new QPushButton("&Trace", this);
    m_capture_button->setEnabled(false);

    m_dev_refresh_button = new QPushButton("&Refresh", this);
    m_pkg_refresh_button = new QPushButton("&Refresh", this);
    m_pkg_refresh_button->setDisabled(true);

    m_main_layout = new QVBoxLayout();

    m_devices = Dive::GetDeviceManager().ListDevice();
    for (size_t i = 0; i < m_devices.size(); i++)
    {
        QStandardItem *item = new QStandardItem(m_devices[i].GetDisplayName().c_str());
        m_dev_model->appendRow(item);
    }
    for (const auto &ty : kAppTypes)
    {
        QStandardItem *item = new QStandardItem(ty.c_str());
        m_app_type_model->appendRow(item);
    }
    m_dev_box->setModel(m_dev_model);
    m_dev_box->setCurrentIndex(-1);
    m_dev_box->setCurrentText("Please select a device");

    m_pkg_box->setModel(m_pkg_model);
    m_pkg_box->setCurrentIndex(-1);
    m_pkg_box->setCurrentText("Please select a package");
    m_pkg_box->setMinimumSize(m_dev_box->sizeHint());
    m_pkg_box->setSizeAdjustPolicy(
    QComboBox::SizeAdjustPolicy::AdjustToMinimumContentsLengthWithIcon);

    m_app_type_box->setModel(m_app_type_model);

    m_capture_layout->addWidget(m_dev_label);
    m_capture_layout->addWidget(m_dev_box);
    m_capture_layout->addWidget(m_dev_refresh_button);

    m_pkg_layout = new QHBoxLayout();
    m_pkg_layout->addWidget(m_pkg_label);
    m_pkg_layout->addWidget(m_pkg_box);
    m_pkg_layout->addWidget(m_pkg_refresh_button);

    m_type_layout = new QHBoxLayout();
    m_type_layout->addWidget(m_app_type_label);
    m_type_layout->addWidget(m_app_type_box);

    m_button_layout->addWidget(m_run_button);
    m_button_layout->addWidget(m_capture_button);

    m_main_layout->addLayout(m_capture_layout);
    m_main_layout->addLayout(m_pkg_layout);
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
    QObject::connect(m_run_button, &QPushButton::clicked, this, &TraceDialog::OnStartClicked);
    QObject::connect(m_capture_button, &QPushButton::clicked, this, &TraceDialog::OnTraceClicked);

    QObject::connect(m_dev_refresh_button,
                     &QPushButton::clicked,
                     this,
                     &TraceDialog::OnDevListRefresh);
    QObject::connect(m_pkg_refresh_button,
                     &QPushButton::clicked,
                     this,
                     &TraceDialog::OnAppListRefresh);
    setMinimumWidth(500);
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

void TraceDialog::UpdateDeviceList()
{
    auto cur_list = Dive::GetDeviceManager().ListDevice();
    qDebug() << "m_dev_box->currentIndex() " << m_dev_box->currentIndex();
    if (cur_list == m_devices)
    {
        qDebug() << "No changes from the list of the connected devices. ";
        return;
    }

    m_devices = cur_list;
    m_dev_model->clear();

    for (size_t i = 0; i < m_devices.size(); i++)
    {
        QStandardItem *item = new QStandardItem(m_devices[i].GetDisplayName().c_str());
        m_dev_model->appendRow(item);
        // Keep the original selected devices as selected.
        if (m_cur_dev == m_devices[i].m_serial)
        {
            m_dev_box->setCurrentIndex(static_cast<int>(i));
        }
    }
}

void TraceDialog::OnDeviceSelected(const QString &s)
{
    if (s.isEmpty() || m_dev_box->currentIndex() == -1)
    {
        qDebug() << "No devices selected";
        return;
    }
    int dev_index = m_dev_box->currentIndex();
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
    if ((s.isEmpty() || m_pkg_box->currentIndex() == -1))
    {
        return;
    }
    qDebug() << "Package selected: " << s << " " << m_pkg_box->currentIndex();
    if (m_cur_pkg != m_pkg_list[m_pkg_box->currentIndex()])
    {
        m_cur_pkg = m_pkg_list[m_pkg_box->currentIndex()];
        m_app_type_box->setCurrentIndex(-1);
    }
    m_run_button->setEnabled(true);
}

void TraceDialog::OnStartClicked()
{
    auto device = Dive::GetDeviceManager().GetDevice();
    if (!device)
    {
        std::string
        err_msg = "No device/application selected. Please select a device and application and "
                  "then try again.";
        ShowErrorMessage(err_msg);
        return;
    }
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

    if (m_run_button->text() == QString("&Start Application"))
    {

        device->CleanupAPP().IgnoreError();
        m_run_button->setText("&Starting..");
        m_run_button->setDisabled(true);

        qDebug() << "Start app on dev: " << m_cur_dev.c_str() << ", package: " << m_cur_pkg.c_str()
                 << ", type: " << ty_str.c_str();
        if (ty_str == "OpenXR")
        {
            ret = device->SetupApp(m_cur_pkg, Dive::ApplicationType::OPENXR_APK);
        }
        else if (ty_str == "Vulkan")
        {
            ret = device->SetupApp(m_cur_pkg, Dive::ApplicationType::VULKAN_APK);
        }
        if (!ret.ok())
        {
            std::string err_msg = absl::StrCat("Fail to setup for package ",
                                               m_cur_pkg,
                                               " error: ",
                                               ret.message());
            qDebug() << err_msg.c_str();
            ShowErrorMessage(err_msg);
            return;
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
            return;
        }
        auto cur_app = device->GetCurrentApplication();

        if (!cur_app->IsRunning())
        {
            std::string err_msg = absl::StrCat("Process for package ",
                                               m_cur_pkg,
                                               " not found, possibly crashed.");
            qDebug() << err_msg.c_str();
            ShowErrorMessage(err_msg);
            return;
        }

        if (cur_app)
        {
            m_run_button->setDisabled(false);
            m_run_button->setText("&Stop");
            m_capture_button->setEnabled(true);
        }
    }
    else
    {
        qDebug() << "Stop package " << m_cur_pkg.c_str();
        device->StopApp().IgnoreError();
        m_run_button->setText("&Start Application");
        m_capture_button->setEnabled(false);
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
    while (m_file_size && cur_size <= m_file_size)
    {
        if (std::filesystem::exists(m_file_name))
        {
            cur_size = std::filesystem::file_size(m_file_name);
            percent = cur_size * 100 / m_file_size;
            m_progress_bar->setValue(percent);
            m_progress_bar->show();
            std::cout << "percent " << percent << ", cursize: " << cur_size << ", total "
                      << m_file_size << std::endl;
        }
        if (cur_size == m_file_size)
            break;
        sleep(1);
    }
    m_progress_bar->setValue(100);
}

void TraceWorker::run()
{
    const std::string server_str = "localhost:19999";

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
                                                                   target,
                                                                   file_size);
    connect(progress_bar_worker,
            &TraceWorker::finished,
            progress_bar_worker,
            &QObject::deleteLater);

    progress_bar_worker->start();

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    qDebug() << "Begin to download the trace file to " << target.generic_string().c_str();
    auto r = device->RetrieveTraceFile(*trace_file_path, target.generic_string());
    progress_bar_worker->terminate();
    m_progress_bar->setValue(100);
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
    int64_t time_used_to_load_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now() - begin)
                                   .count();
    qDebug() << "Time used to download the capture is " << (time_used_to_load_ms / 1000.0)
             << " seconds.";

    QString capture_saved_path(target.generic_string().c_str());
    emit    TraceAvailable(capture_saved_path);
}

void TraceDialog::OnDevListRefresh()
{
    UpdateDeviceList();
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

    auto ret = device->ListPackage();
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
    m_pkg_model->clear();
    for (size_t i = 0; i < m_pkg_list.size(); i++)
    {
        QStandardItem *item = new QStandardItem(m_pkg_list[i].c_str());
        m_pkg_model->appendRow(item);
    }
    m_pkg_box->setCurrentIndex(-1);
    m_pkg_refresh_button->setDisabled(false);
}