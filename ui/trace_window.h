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
#include <QSortFilterProxyModel>
#include <QThread>
#include <cstdint>

#include "capture_service/device_mgr.h"
#include "device_dialog.h"
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

class ApplicationController;

class AppTypeFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

 public:
    explicit AppTypeFilterModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

    void setFilterActive(bool active);

 protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

 private:
    bool m_filter_active = false;
};

class TraceDialog : public DeviceDialog
{
    Q_OBJECT

 public:
    TraceDialog(ApplicationController& controller, QWidget* parent = 0);
    ~TraceDialog();
    void UpdatePackageList();
    void Cleanup() { Dive::GetDeviceManager().RemoveDevice(); }
    void ShowGfxrFields();
    void HideGfxrFields();
    void EnableCaptureTypeButtons(bool enable);
    void RetrieveGfxrCapture();

 protected:
    void closeEvent(QCloseEvent* event) override;

 private slots:
    void OnPackageSelected(const QString&);
    void OnStartClicked();
    void OnTraceClicked();
    void OnTraceAvailable(const QString&);
    void OnGFXRCaptureAvailable(const QString&);
    void OnDevListRefresh();
    void OnAppListRefresh();
    void OnInputCommand(const QString&);
    void OnInputArgs(const QString&);
    void OnPackageListFilter();
    void OnPackageListFilterApplied(const QString& filter);
    void OnGfxrCaptureClicked();
    void ShowMessage(const QString& message) override;
    absl::Status StopPackageAndCleanup();
    void OnCaptureTypeChanged(int id);
    void OnShowAdvancedOptions(bool show);

 signals:
    void TraceAvailable(const QString&);

 private:
    bool StartPackage(Dive::AndroidDevice* device, const std::string& app_type);
    void RetrieveGfxrCapture(Dive::AndroidDevice* device, const std::string& capture_directory);

    void OnDeviceSelected() override;
    void OnDeviceSelectionCleared() override;

    ApplicationController& m_controller;

    const QString kStart_Application = "&Start Application";
    const QString kStart_Gfxr_Runtime_Capture = "&Start GFXR Capture";
    const QString kRetrieve_Gfxr_Runtime_Capture = "&Retrieve GFXR Capture";

    QHBoxLayout* m_capture_layout;
    QLabel* m_dev_label;
    QPushButton* m_dev_refresh_button;

    QHBoxLayout* m_capture_type_layout;
    QLabel* m_capture_type_label;
    QButtonGroup* m_capture_type_button_group;
    QRadioButton* m_gfxr_capture_type_button;
    QRadioButton* m_pm4_capture_type_button;

    QHBoxLayout* m_capture_warning_layout;
    QLabel* m_capture_warning_label;

    QHBoxLayout* m_pkg_filter_layout;
    QLabel* m_pkg_filter_label;
    PackageFilter* m_pkg_filter;
    QHBoxLayout* m_pkg_layout;
    QLabel* m_pkg_label;
    QStandardItemModel* m_pkg_model;
    QComboBox* m_pkg_box;
    QPushButton* m_pkg_refresh_button;
    QPushButton* m_pkg_filter_button;
    Dive::AndroidDevice::PackageListOptions m_pkg_list_options;

    QHBoxLayout* m_type_layout;
    QLabel* m_app_type_label;
    QStandardItemModel* m_app_type_model;
    AppTypeFilterModel* m_app_type_filter_model;
    QComboBox* m_app_type_box;

    QPushButton* m_capture_button;
    QPushButton* m_run_button;
    QPushButton* m_gfxr_capture_button;
    QPushButton* m_gfxr_retrieve_button;
    QHBoxLayout* m_button_layout;

    QHBoxLayout* m_cmd_layout;
    QLabel* m_file_label;
    QPushButton* m_open_button;
    QLineEdit* m_cmd_input_box;

    QHBoxLayout* m_args_layout;
    QLabel* m_args_label;
    QLineEdit* m_args_input_box;

    QHBoxLayout* m_gfxr_capture_file_directory_layout;
    QLabel* m_gfxr_capture_file_on_device_directory_label;
    QLineEdit* m_gfxr_capture_file_directory_input_box;

    QLineEdit* m_capture_file_local_root_directory_input_box;

    QVBoxLayout* m_main_layout;
    std::vector<std::string> m_pkg_list;
    std::string m_cur_pkg;
    std::string m_executable;
    std::string m_command_args;
    bool m_gfxr_capture = false;
};
