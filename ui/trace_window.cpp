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
#include <QVBoxLayout>
#include <filesystem>

#include "capture_service/client.h"
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
    m_capture_button = new QPushButton("&Trace", this);

    m_main_layout = new QVBoxLayout();

    m_devices = Dive::GetDeviceManager().ListDevice();
    for (size_t i = 0; i < m_devices.size(); i++)
    {
        QStandardItem *item = new QStandardItem(m_devices[i].GetDisplayName().c_str());
        m_dev_model->appendRow(item);
    }
    if (!m_devices.empty())
    {
        m_cur_dev = m_devices[0].m_serial;
        qDebug() << "Device selected: " << m_devices[0].GetDisplayName().c_str();
    }
    if (!m_cur_dev.empty())
    {
        auto device = Dive::GetDeviceManager().SelectDevice(m_cur_dev);
        device->SetupDevice();
        m_pkg_list = device->ListPackage();
    }

    for (size_t i = 0; i < m_pkg_list.size(); i++)
    {
        QStandardItem *item = new QStandardItem(m_pkg_list[i].c_str());
        m_pkg_model->appendRow(item);
    }
    for (const auto &ty : kAppTypes)
    {
        QStandardItem *item = new QStandardItem(ty.c_str());
        m_app_type_model->appendRow(item);
    }
    m_dev_box->setModel(m_dev_model);
    m_pkg_box->setModel(m_pkg_model);
    m_app_type_box->setModel(m_app_type_model);

    m_capture_layout->addWidget(m_dev_label);
    m_capture_layout->addWidget(m_dev_box);
    m_capture_layout->addWidget(m_pkg_label);
    m_capture_layout->addWidget(m_pkg_box);
    m_capture_layout->addWidget(m_app_type_label);
    m_capture_layout->addWidget(m_app_type_box);
    m_button_layout->addWidget(m_run_button);
    m_button_layout->addWidget(m_capture_button);

    m_main_layout->addLayout(m_capture_layout);
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
    QObject::connect(m_capture_button, &QPushButton::clicked, this, &TraceDialog::OnCaptureClicked);
}

TraceDialog::~TraceDialog()
{
    qDebug() << "TraceDialog destroyed.";
    Dive::GetDeviceManager().RemoveDevice();
}

void TraceDialog::OnDeviceSelected(const QString &s)
{
    if (s.isEmpty())
    {
        qDebug() << "No devices selected";
        return;
    }
    int dev_index = m_dev_box->currentIndex();
    qDebug() << "Device selected: " << m_cur_dev.c_str();
    assert(static_cast<size_t>(dev_index) < m_devices.size());
    m_cur_dev = m_devices[dev_index].m_serial;
    auto device = Dive::GetDeviceManager().SelectDevice(m_cur_dev);
    m_pkg_list = device->ListPackage();
    m_pkg_model->clear();
    for (size_t i = 0; i < m_pkg_list.size(); i++)
    {
        QStandardItem *item = new QStandardItem(m_pkg_list[i].c_str());
        m_pkg_model->appendRow(item);
    }
}

void TraceDialog::OnPackageSelected(const QString &s)
{
    qDebug() << "Package selected: " << s << " " << m_pkg_box->currentIndex();
    m_cur_pkg = m_pkg_list[m_pkg_box->currentIndex()];
}

void TraceDialog::OnStartClicked()
{
    auto device = Dive::GetDeviceManager().GetDevice();
    if (!device)
    {
        // TODO: add a warning message.
        QMessageBox msgBox;
        msgBox.setText("No device/application selected. Please select a device and application and "
                       "then try again.");
        msgBox.exec();
        return;
    }

    device->SetupDevice();
    int         ty = m_app_type_box->currentIndex();
    std::string ty_str = kAppTypes[ty];
    if (m_run_button->text() == QString("&Start Application"))
    {
        device->CleanupAPP();
        m_run_button->setText("&Starting..");
        m_run_button->setDisabled(true);

        qDebug() << "Start app on dev: " << m_cur_dev.c_str() << ", package: " << m_cur_pkg.c_str()
                 << ", type: " << ty_str.c_str();
        if (ty_str == "OpenXR")
        {
            device->SetupApp(m_cur_pkg, Dive::ApplicationType::OPENXR);
        }
        else if (ty_str == "Vulkan")
        {
            device->SetupApp(m_cur_pkg, Dive::ApplicationType::VULKAN);
        }

        device->StartApp();
        m_run_button->setDisabled(false);
        m_run_button->setText("&Stop");
    }
    else
    {
        qDebug() << "Stop package " << m_cur_pkg.c_str();
        device->StopApp();
        m_run_button->setText("&Start Application");
    }
}

void TraceDialog::OnCaptureClicked()
{
    const std::string server_str = "localhost:19999";

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
        std::string err_msg(trace_file_path.status().message());
        qDebug() << "Trigger capture failed: " << err_msg.c_str();
        return;
    }
    std::string           capture_path = ".";
    std::filesystem::path p(*trace_file_path);
    std::filesystem::path target(capture_path);
    target /= p.filename();
    Dive::GetDeviceManager().GetDevice()->RetrieveTraceFile(*trace_file_path,
                                                            target.generic_string());
    qDebug() << "Capture saved at " << target.generic_string().c_str();

    QString capture_saved_path(target.generic_string().c_str());
    emit    TraceAvailable(capture_saved_path);
}
