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

#include <qspinbox.h>
#include <QDialog>
#include <QThread>
#include <cstdint>

#include "capture_service/device_mgr.h"
#include "package_filter.h"

#pragma once

// Forward declarations
class QLabel;
class QHBoxLayout;
class QPlainTextEdit;
class QPushButton;
class QVBoxLayout;
class QComboBox;
class QStandardItemModel;
class QProgressDialog;
class QLineEdit;
class QSpinBox;
class QCheckBox;
class QRadioButton;
class QButtonGroup;

class TraceWorker : public QThread
{
    Q_OBJECT
    void run() override;

public:
    TraceWorker(QProgressDialog *pd) :
        m_progress_bar(pd)
    {
    }
signals:
    void TraceAvailable(const QString &);
    void DownloadedSize(uint64_t size);
    void ErrorMessage(const QString &err_msg);

private:
    QProgressDialog *m_progress_bar;
};

class GfxrCaptureWorker : public QThread
{
    Q_OBJECT
    void run() override;

public:
    GfxrCaptureWorker(QProgressDialog *pd) :
        m_progress_bar(pd)
    {
    }
    void SetGfxrSourceCaptureDir(const std::string &source_capture_dir);

    // Appends/increments the numerical suffix "_#" to target_capture_path for a fresh directory, if
    // the directory already exists
    void SetGfxrTargetCaptureDir(const std::string &target_capture_dir);
    bool AreTimestampsCurrent(Dive::AndroidDevice                      *device,
                              const std::map<std::string, std::string> &previous_timestamps);

    absl::StatusOr<int64_t> getGfxrCaptureDirectorySize(Dive::AndroidDevice *device);
signals:
    void DownloadedSize(uint64_t size);
    void GfxrCaptureAvailable(const QString &);
    void ErrorMessage(const QString &err_msg);

private:
    QProgressDialog *m_progress_bar;
    std::string      m_source_capture_dir;  // On Android, better to keep as std::string since the
                                            // host platform delimiter may be inconsistent
    std::filesystem::path    m_target_capture_dir;
    std::vector<std::string> m_file_list;
};

class ProgressBarWorker : public QThread
{
    Q_OBJECT
    void run() override;

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

class TraceDialog : public QDialog
{
    Q_OBJECT

public:
    TraceDialog(QWidget *parent = 0);
    ~TraceDialog();
    void UpdateDeviceList(bool isInitialized);
    void UpdatePackageList();
    void Cleanup() { Dive::GetDeviceManager().RemoveDevice(); }
    void ShowGfxrFields();
    void HideGfxrFields();
    void EnableCaptureTypeButtons(bool enable);
    void RetrieveGfxrCapture();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void         OnDeviceSelected(const QString &);
    void         OnPackageSelected(const QString &);
    void         OnStartClicked();
    void         OnTraceClicked();
    void         OnTraceAvailable(const QString &);
    void         OnGFXRCaptureAvailable(const QString &);
    void         OnDevListRefresh();
    void         OnAppListRefresh();
    void         OnInputCommand(const QString &);
    void         OnInputArgs(const QString &);
    void         OnPackageListFilter();
    void         OnPackageListFilterApplied(const QString &filter);
    void         OnGfxrCaptureClicked();
    void         ShowErrorMessage(const QString &err_msg);
    absl::Status StopPackageAndCleanup();
    void         OnCaptureTypeChanged(int id);

signals:
    void TraceAvailable(const QString &);

private:
    bool StartPackage(Dive::AndroidDevice *device, const std::string &app_type);
    void RetrieveGfxrCapture(Dive::AndroidDevice *device, const std::string &capture_directory);

    const QString kStart_Application = "&Start Application";
    const QString kStart_Gfxr_Runtime_Capture = "&Start GFXR Capture";
    const QString kRetrieve_Gfxr_Runtime_Capture = "&Retrieve GFXR Capture";

    QHBoxLayout        *m_capture_layout;
    QLabel             *m_dev_label;
    QStandardItemModel *m_dev_model;
    QComboBox          *m_dev_box;
    QPushButton        *m_dev_refresh_button;

    QHBoxLayout  *m_capture_type_layout;
    QLabel       *m_capture_type_label;
    QButtonGroup *m_capture_type_button_group;
    QRadioButton *m_gfxr_capture_type_button;
    QRadioButton *m_pm4_capture_type_button;

    QHBoxLayout                            *m_pkg_filter_layout;
    QLabel                                 *m_pkg_filter_label;
    PackageFilter                          *m_pkg_filter;
    QHBoxLayout                            *m_pkg_layout;
    QLabel                                 *m_pkg_label;
    QStandardItemModel                     *m_pkg_model;
    QComboBox                              *m_pkg_box;
    QPushButton                            *m_pkg_refresh_button;
    QPushButton                            *m_pkg_filter_button;
    Dive::AndroidDevice::PackageListOptions m_pkg_list_options;

    QHBoxLayout        *m_type_layout;
    QLabel             *m_app_type_label;
    QStandardItemModel *m_app_type_model;
    QComboBox          *m_app_type_box;

    QPushButton *m_capture_button;
    QPushButton *m_run_button;
    QPushButton *m_gfxr_capture_button;
    QPushButton *m_gfxr_retrieve_button;
    QHBoxLayout *m_button_layout;

    QHBoxLayout *m_cmd_layout;
    QLabel      *m_file_label;
    QPushButton *m_open_button;
    QLineEdit   *m_cmd_input_box;

    QHBoxLayout *m_args_layout;
    QLabel      *m_args_label;
    QLineEdit   *m_args_input_box;

    QHBoxLayout *m_gfxr_capture_file_directory_layout;
    QLabel      *m_gfxr_capture_file_on_device_directory_label;
    QLineEdit   *m_gfxr_capture_file_directory_input_box;

    QHBoxLayout *m_gfxr_capture_file_local_directory_layout;
    QLabel      *m_gfxr_capture_file_local_directory_label;
    QLineEdit   *m_gfxr_capture_file_local_directory_input_box;

    QVBoxLayout                  *m_main_layout;
    std::vector<Dive::DeviceInfo> m_devices;
    std::string                   m_cur_dev;
    std::vector<std::string>      m_pkg_list;
    std::string                   m_cur_pkg;
    std::string                   m_executable;
    std::string                   m_command_args;
    bool                          m_gfxr_capture = false;
};
