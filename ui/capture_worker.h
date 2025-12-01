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

#include <QThread>
#include <QProgressDialog>
#include <filesystem>

#pragma once

class ProgressBarWorker : public QThread
{
    Q_OBJECT

public:
    ProgressBarWorker(QProgressDialog   *pd,
                      const std::string &path,
                      int64_t            size,
                      const bool         is_gfxr_capture) :
        m_progress_bar(pd),
        m_capture_name(path),
        m_capture_size(size),
        m_gfxr_capture(is_gfxr_capture),
        m_downloaded_size(0)
    {
    }

    void    run() override;
    int64_t GetDownloadedSize() const { return m_downloaded_size; }

public slots:
    void SetDownloadedSize(uint64_t size) { m_downloaded_size = size; }

signals:
    void SetProgressBarValue(int percentage);

private:
    QProgressDialog *m_progress_bar;
    std::string      m_capture_name;
    int64_t          m_capture_size;
    bool             m_gfxr_capture;
    int64_t          m_downloaded_size;
};

class CaptureWorker : public QThread
{
    Q_OBJECT

public:
    CaptureWorker(QProgressDialog *pd) :
        m_progress_bar(pd)
    {
    }

    void run() override;
    // Appends/increments the numerical suffix "_#" to target_capture_path for a fresh directory, if
    // the directory already exists
    void SetTargetCaptureDir(const std::string &target_capture_dir);
signals:
    void CaptureAvailable(const QString &);
    void DownloadedSize(uint64_t size);
    void ErrorMessage(const QString &err_msg);

protected:
    QProgressDialog      *m_progress_bar;
    std::filesystem::path m_target_capture_dir;
};
