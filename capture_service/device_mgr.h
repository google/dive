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

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "android_application.h"
#include "command_utils.h"

#include <cassert>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace Dive
{

struct DeviceInfo
{
    std::string m_serial;
    std::string m_manufacturer;
    std::string m_model;

    std::string GetDisplayName() const;
};

inline bool operator==(const DeviceInfo &lhs, const DeviceInfo &rhs)
{
    return lhs.m_serial == rhs.m_serial && lhs.m_manufacturer == rhs.m_manufacturer &&
           lhs.m_model == rhs.m_model;
}

inline bool operator!=(const DeviceInfo &lhs, const DeviceInfo &rhs)
{
    return !(lhs == rhs);
}

struct DeviceState
{
    std::string m_enforce;
};

class AndroidDevice
{
public:
    explicit AndroidDevice(const std::string &serial);
    ~AndroidDevice();

    AndroidDevice &operator=(const AndroidDevice &) = delete;
    AndroidDevice(const AndroidDevice &) = delete;

    absl::Status Init();
    absl::Status SetupDevice();
    absl::Status CleanupDevice();

    struct PackageListOptions
    {
        bool with_system_package;
        bool debuggable_only;
    };

    absl::StatusOr<std::vector<std::string>> ListPackage(PackageListOptions option = { 0,
                                                                                       1 }) const;
    std::string                              GetDeviceDisplayName() const;
    absl::Status        SetupApp(const std::string &package, const ApplicationType type);
    absl::Status        CleanupAPP();
    absl::Status        StartApp();
    absl::Status        StopApp();
    const AdbSession   &Adb() const { return m_adb; }
    AndroidApplication *GetCurrentApplication() { return app.get(); }
    absl::Status        RetrieveTraceFile(const std::string &trace_file_path,
                                          const std::string &save_path);

private:
    const std::string                   m_serial;
    DeviceInfo                          m_dev_info;
    AdbSession                          m_adb;
    DeviceState                         m_original_state;
    std::unique_ptr<AndroidApplication> app;
};

class DeviceManager
{
public:
    DeviceManager() = default;
    DeviceManager &operator=(const DeviceManager &) = delete;
    DeviceManager(const DeviceManager &) = delete;

    std::vector<DeviceInfo>         ListDevice() const;
    absl::StatusOr<AndroidDevice *> SelectDevice(const std::string &serial);
    void                            RemoveDevice() { m_device = nullptr; }
    AndroidDevice                  *GetDevice() const { return m_device.get(); }
    absl::Status                    Cleanup(const std::string &serial, const std::string &package);

private:
    std::unique_ptr<AndroidDevice> m_device{ nullptr };
};

std::filesystem::path ResolveAndroidLibPath(const std::string &name);

DeviceManager &GetDeviceManager();
}  // namespace Dive