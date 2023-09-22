/*
Copyright 2023 Google Inc.

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

#include "command_utils.h"

#include <memory>
#include <string>
#include <vector>

namespace Dive
{

struct DeviceInfo
{
    std::string m_manufacturer;
    std::string m_model;
};

struct DeviceState
{
    std::string m_enforce;
};

class AndroidDevice
{
public:
    explicit AndroidDevice(const std::string &serial);
    ~AndroidDevice();

    std::vector<std::string> ListPackage();
    std::string              GetMainActivity(const std::string &package);
    std::string              GetDeviceDisplayName();

    void        SetupApp(const std::string &package);
    void        CleanupAPP(const std::string &package);
    void        StartApp(const std::string &package, const std::string &activity);
    void        StopApp(const std::string &package);
    AdbSession &Adb() { return m_adb; }

    void SetupDevice();
    void CleanupDevice();

    void RetrieveTraceFile(const std::string &trace_file_path, const std::string &save_path);

private:
    const std::string m_serial;
    DeviceInfo        m_dev_info;
    AdbSession        m_adb;
    DeviceState       m_original_state;
    std::string       m_selected_package;
};

class DeviceManager
{
public:
    std::vector<std::string> ListDevice();
    AndroidDevice           *SelectDevice(const std::string &serial)
    {
        m_device = std::make_unique<AndroidDevice>(serial);
        return m_device.get();
    }

    AndroidDevice *GetDevice() { return m_device.get(); }

private:
    std::unique_ptr<AndroidDevice> m_device;
};

}  // namespace Dive