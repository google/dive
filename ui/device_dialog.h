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

#include <QDialog>

#include "capture_service/device_mgr.h"

#pragma once

// Forward declarations
class QComboBox;
class QStandardItemModel;

class DeviceDialog : public QDialog
{
    Q_OBJECT

 public:
    explicit DeviceDialog(QWidget* parent = nullptr);
    virtual ~DeviceDialog();
    void UpdateDeviceList();

 protected:
    virtual void ShowMessage(const QString& message) = 0;
    virtual void OnDeviceSelected() = 0;
    virtual void OnDeviceSelectionCleared() = 0;

 protected slots:
    void OnDeviceSelectionChanged(const QString& s);

 protected:
    std::vector<Dive::DeviceInfo> m_devices;
    std::string m_cur_device;
    QStandardItemModel* m_device_model;
    QComboBox* m_device_box;
};
