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

#include "device_dialog.h"

#include <QComboBox>
#include <QDebug>
#include <QStandardItemModel>
#include <QString>

#include "absl/strings/str_cat.h"

DeviceDialog::DeviceDialog(QWidget* parent) : QDialog(parent)
{
    m_device_model = new QStandardItemModel(this);
}

DeviceDialog::~DeviceDialog() {}

void DeviceDialog::UpdateDeviceList()
{
    m_device_model->clear();
    m_devices = Dive::GetDeviceManager().ListDevice();
    if (m_devices.empty())
    {
        QStandardItem* item = new QStandardItem("No devices found");
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        m_device_model->appendRow(item);
        m_device_box->setCurrentIndex(0);
        OnDeviceSelectionCleared();
        return;
    }

    QStandardItem* placeholder = new QStandardItem("Please select a device");
    placeholder->setFlags(placeholder->flags() & ~Qt::ItemIsSelectable);
    m_device_model->appendRow(placeholder);

    // Default to placeholder
    int index_to_select = 0;
    for (size_t i = 0; i < m_devices.size(); ++i)
    {
        QStandardItem* item = new QStandardItem(m_devices[i].GetDisplayName().c_str());
        m_device_model->appendRow(item);
        // Track previously selected device (offset by 1 due to placeholder)
        if (m_cur_device == m_devices[i].m_serial)
        {
            index_to_select = static_cast<int>(i) + 1;
        }
    }

    if (m_devices.size() == 1)
    {
        index_to_select = 1;
    }
    m_device_box->setCurrentIndex(index_to_select);
}

void DeviceDialog::OnDeviceSelectionChanged(const QString& s)
{
    if (s.isEmpty() || m_device_box->currentIndex() == 0)
    {
        qDebug() << "No devices selected";
        OnDeviceSelectionCleared();
        return;
    }

    int device_index = m_device_box->currentIndex() - 1;
    assert(device_index >= 0 && static_cast<size_t>(device_index) < m_devices.size());

    const auto& target_device = m_devices[device_index];
    if (m_cur_device == target_device.m_serial)
    {
        qDebug() << "Device already selected: " << m_cur_device.c_str();
        OnDeviceSelected();
        return;
    }
    Dive::DeviceManager& device_manager = Dive::GetDeviceManager();
    if (!m_cur_device.empty())
    {
        qDebug() << "Deselecting device: " << m_cur_device.c_str();
        m_cur_device.clear();
        device_manager.RemoveDevice();
    }

    m_cur_device = target_device.m_serial;
    auto dev_ret = device_manager.SelectDevice(m_cur_device);
    if (!dev_ret.ok())
    {
        std::string err_msg = absl::StrCat("Failed to select device ", m_cur_device.c_str(),
                                           ", error: ", dev_ret.status().message());
        qDebug() << err_msg.c_str();
        ShowMessage(QString::fromStdString(err_msg));
        return;
    }
    qDebug() << "Device selected: " << m_cur_device.c_str();
    OnDeviceSelected();
}
