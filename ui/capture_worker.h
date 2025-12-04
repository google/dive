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

#pragma once

#include <QObject>
#include <QProgressDialog>
#include <QThread>
#include <filesystem>

class CaptureWorker : public QThread
{
    Q_OBJECT

public:
    CaptureWorker(QObject *parent = nullptr) :
        QThread(parent)
    {
    }

    void run() override;
    // Appends/increments the numerical suffix "_#" to target_capture_path for a fresh directory, if
    // the directory already exists
    void SetTargetCaptureDir(const std::string &target_capture_dir);
signals:
    void CaptureAvailable(const QString &);
    void DownloadedSize(qlonglong size, qlonglong total_size);
    void ErrorMessage(const QString &err_msg);

protected:
    std::filesystem::path m_target_capture_dir;
};
