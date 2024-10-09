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

private:
    QProgressDialog *m_progress_bar;
};

class ProgressBarWorker : public QThread
{
    Q_OBJECT
    void run() override;

public:
    ProgressBarWorker(QProgressDialog *pd, const std::string &path, int64_t size) :
        m_progress_bar(pd),
        m_file_name(path),
        m_file_size(size)
    {
    }

private:
    QProgressDialog *m_progress_bar;
    std::string      m_file_name;
    int64_t          m_file_size;
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

private slots:
    void OnDeviceSelected(const QString &);
    void OnPackageSelected(const QString &);
    void OnStartClicked();
    void OnTraceClicked();
    void OnTraceAvailable(const QString &);
    void OnDevListRefresh();
    void OnAppListRefresh();
    void OnInputCommand(const QString &);
    void OnInputArgs(const QString &);
    void OnPackageListFilter();
    void OnPackageListFilterApplied(QSet<QString> filters);

signals:
    void TraceAvailable(const QString &);

private:
    bool StartPackage(Dive::AndroidDevice *device, const std::string &app_type);

    QHBoxLayout        *m_capture_layout;
    QLabel             *m_dev_label;
    QStandardItemModel *m_dev_model;
    QComboBox          *m_dev_box;
    QPushButton        *m_dev_refresh_button;

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
    QHBoxLayout *m_button_layout;

    QHBoxLayout *m_cmd_layout;
    QLabel      *m_file_label;
    QPushButton *m_open_button;
    QLineEdit   *m_cmd_input_box;

    QHBoxLayout *m_args_layout;
    QLabel      *m_args_label;
    QLineEdit   *m_args_input_box;

    QVBoxLayout                  *m_main_layout;
    std::vector<Dive::DeviceInfo> m_devices;
    std::string                   m_cur_dev;
    std::vector<std::string>      m_pkg_list;
    std::string                   m_cur_pkg;
    std::string                   m_executable;
    std::string                   m_command_args;
};
