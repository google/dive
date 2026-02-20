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

#include "capture_worker.h"

#include <QDebug>

#include "absl/strings/str_cat.h"
#include "capture_service/device_mgr.h"
#include "dive/utils/device_resources.h"
#include "network/tcp_client.h"

//--------------------------------------------------------------------------------------------------
void CaptureWorker::SetTargetCaptureDir(const std::string& host_root_dir)
{
    if (auto ret = Dive::GetNextHostSessionPath(host_root_dir); ret.ok())
    {
        m_host_capture_dir = *ret;
    }
    else
    {
        std::string err_msg = std::string(ret.status().message());
        qDebug() << err_msg.c_str();
        emit ShowMessage(QString::fromStdString(err_msg));
    }
}

//--------------------------------------------------------------------------------------------------
void CaptureWorker::run()
{
    auto device = Dive::GetDeviceManager().GetDevice();
    if (device == nullptr)
    {
        std::string err_msg = "Failed to connect to device";
        qDebug() << err_msg.c_str();
        ShowMessage(QString::fromStdString(err_msg));
        return;
    }

    auto app = device->GetCurrentApplication();
    if (app == nullptr || !app->IsRunning())
    {
        std::string err_msg = "Application is not running, possibly crashed.";
        qDebug() << err_msg.c_str();
        emit ShowMessage(QString::fromStdString(err_msg));
        return;
    }

    Network::TcpClient client;
    const std::string host = "127.0.0.1";
    int port = device->Port();
    auto status = client.Connect(host, port);
    if (!status.ok())
    {
        std::string err_msg(status.message());
        qDebug() << "Connection failed: " << err_msg.c_str();
        return;
    }

    absl::StatusOr<std::string> capture_file_path = client.StartPm4Capture();
    if (capture_file_path.ok())
    {
        emit UpdateProgressDialog("Triggering PM4 Capture ...");
        qDebug() << "Trigger capture: " << (*capture_file_path).c_str();
    }
    else
    {
        std::string err_msg =
            absl::StrCat("Trigger capture failed: ", capture_file_path.status().message());
        qDebug() << err_msg.c_str();
        emit ShowMessage(QString::fromStdString(err_msg));
        return;
    }
    std::filesystem::path p(*capture_file_path);
    std::filesystem::path host_download_path(m_host_capture_dir);
    host_download_path /= p.filename();
    qDebug() << "Begin to download the capture file to "
             << host_download_path.generic_string().c_str();

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
        emit ShowMessage(QString::fromStdString(err_msg));
        return;
    }

    const qlonglong total_size = *file_size;

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    qDebug() << "Begin to download the capture file to "
             << host_download_path.generic_string().c_str();

    emit UpdateProgressDialog("Downloading ...");
    auto progress = [this, total_size](size_t size) {
        emit DownloadedSize(static_cast<qlonglong>(size), total_size);
    };
    status = client.DownloadFileFromServer(*capture_file_path, host_download_path.generic_string(),
                                           progress);
    if (status.ok())
    {
        qDebug() << "Capture saved at "
                 << std::filesystem::canonical(host_download_path).generic_string().c_str();
    }
    else
    {
        std::string err_msg =
            absl::StrCat("Failed to download capture file, error: ", status.message());
        qDebug() << err_msg.c_str();
        emit ShowMessage(QString::fromStdString(err_msg));
        return;
    }
    int64_t time_used_to_load_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                       std::chrono::steady_clock::now() - begin)
                                       .count();
    QString download_msg = QString("Capture downloaded to %1 in %2 seconds)")
                               .arg(host_download_path.generic_string().c_str())
                               .arg(time_used_to_load_ms / 1000.0);
    qDebug() << download_msg;

    QString capture_saved_path(host_download_path.generic_string().c_str());
    emit ShowMessage(download_msg);
    emit CaptureAvailable(capture_saved_path);
}
