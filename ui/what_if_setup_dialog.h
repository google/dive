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

#pragma once

#include <QSortFilterProxyModel>

#include "device_dialog.h"
#include "dive/ui/forward.h"

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
    WhatIfSetupDialog(QWidget* parent = nullptr);
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

    void OnDeviceSelected() override;
    void OnDeviceSelectionCleared() override;

    // --- Layout Creation ---
    QVBoxLayout* CreateHeaderLayout();
    QVBoxLayout* CreateRadioButtonLayout();
    void InitializeRuntimeOptions();
    void InitializeReplayOptions();
    QHBoxLayout* CreateButtonLayout();

    // --- What-If Type Section ---
    QRadioButton* m_runtime_what_if_type_button = nullptr;
    QRadioButton* m_replay_what_if_type_button = nullptr;

    // --- Options Section ---
    QWidget* m_runtime_options_widget = nullptr;
    QWidget* m_replay_options_widget = nullptr;

    // --- Package Section ---
    QStandardItemModel* m_pkg_model = nullptr;
    QComboBox* m_pkg_box = nullptr;
    QPushButton* m_pkg_refresh_button = nullptr;

    // --- Additional Args Section ---
    QLineEdit* m_args_input_box = nullptr;

    // --- App Type Section ---
    WhatIfAppTypeFilterModel* m_app_type_filter_model = nullptr;
    QComboBox* m_app_type_box = nullptr;

    // --- Start Button ---
    QPushButton* m_start_application_button = nullptr;

    // Runtime data that needs to be accessed across different methods
    struct RuntimeData
    {
        std::vector<std::string> pkg_list;
        QString cur_pkg;
        std::string command_args;
        Dive::AndroidDevice::PackageListOptions pkg_list_options;
    };

    RuntimeData m_runtime_data;
};
