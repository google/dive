/*
 Copyright 2026 Google LLC
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

#include <QSortFilterProxyModel>

#include "device_dialog.h"
#include "dive/ui/forward.h"

#pragma once

class WhatIfAppTypeFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

 public:
    explicit WhatIfAppTypeFilterModel(QObject* parent = nullptr) : QSortFilterProxyModel(parent) {}

    void setFilterActive(bool active);

 protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

 private:
    bool m_filter_active = false;
};

class WhatIfSetupDialog : public DeviceDialog
{
    Q_OBJECT

 public:
    WhatIfSetupDialog(ApplicationController& controller, QWidget* parent = 0);
    ~WhatIfSetupDialog();
    void UpdatePackageList();
    void Cleanup() { Dive::GetDeviceManager().RemoveDevice(); }
    void EnableWhatIfTypeButtons(bool enable);

 protected:
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;

 public slots:
    void OnStopRuntimeWhatIf();

 private slots:
    void OnAppListRefresh();
    void OnDevListRefresh();
    void OnInputArgs(const QString&);
    void OnPackageSelected(const QString&);
    void OnStartClicked();
    void OnWhatIfTypeChanged(int id);
    void ShowMessage(const QString& message) override;

 signals:
    void RuntimeWhatIfEnabled(const QString& package_name, bool is_runtime_what_if_enabled);

 private:
    void ResetDialog();
    bool StartPackage(Dive::AndroidDevice* device, const std::string& app_type);
    void StopPackage();
    void ShowRuntimeWhatIfFields();
    void ShowReplayWhatIfFields();

    void OnDeviceSelected() override;
    void OnDeviceSelectionCleared() override;

    ApplicationController& m_controller;

    const QString kStart_Application = "&Start Application";
    const QString kStop_Application = "&Stop Application";
    const QString kDismiss = "&Dismiss";

    QHBoxLayout* m_what_if_title_layout;
    QLabel* m_what_if_title_label;

    QHBoxLayout* m_what_if_info_layout;
    QLabel* m_what_if_info_label;

    QHBoxLayout* m_what_if_type_layout;
    QButtonGroup* m_what_if_type_button_group;
    QVBoxLayout* m_runtime_what_if_type_button_layout;
    QLabel* m_runtime_what_if_type_label;
    QRadioButton* m_runtime_what_if_type_button;
    QVBoxLayout* m_replay_what_if_type_button_layout;
    QLabel* m_replay_what_if_type_label;
    QRadioButton* m_replay_what_if_type_button;

    QHBoxLayout* m_device_layout;
    QLabel* m_device_label;
    QPushButton* m_device_refresh_button;

    QHBoxLayout* m_pkg_layout;
    QLabel* m_pkg_label;
    QStandardItemModel* m_pkg_model;
    QComboBox* m_pkg_box;
    QPushButton* m_pkg_refresh_button;
    Dive::AndroidDevice::PackageListOptions m_pkg_list_options;

    QHBoxLayout* m_args_layout;
    QLabel* m_args_label;
    QLineEdit* m_args_input_box;

    QHBoxLayout* m_type_layout;
    QLabel* m_app_type_label;
    QStandardItemModel* m_app_type_model;
    WhatIfAppTypeFilterModel* m_app_type_filter_model;
    QComboBox* m_app_type_box;

    QHBoxLayout* m_button_layout;
    QPushButton* m_dismiss_button;
    QPushButton* m_start_application_button;

    QVBoxLayout* m_main_layout;
    std::vector<std::string> m_pkg_list;
    QString m_cur_pkg;
    std::string m_executable;
    std::string m_command_args;
    bool m_runtime_what_if_enabled;
};
