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
    absl::Status ForwardFirstAvailablePort();
    absl::Status SetupDevice();
    absl::Status CleanupDevice();
    absl::Status CleanupPackage(const std::string &package);
    void         EnableGfxr(bool enable_gfxr);

    enum class PackageListOptions
    {
        kAll,
        kDebuggableOnly,
        kNonDebuggableOnly,
    };

    absl::StatusOr<std::vector<std::string>> ListPackage(
    PackageListOptions option = PackageListOptions::kAll) const;
    std::string  GetDeviceDisplayName() const;
    absl::Status SetupApp(const std::string    &package,
                          const ApplicationType type,
                          const std::string    &command_args,
                          const std::string    &device_architecture,
                          const std::string    &gfxr_capture_directory);
    absl::Status SetupApp(const std::string    &binary,
                          const std::string    &args,
                          const ApplicationType type);

    absl::Status      CleanupAPP();
    absl::Status      StartApp();
    absl::Status      StopApp();
    const AdbSession &Adb() const { return m_adb; }
    AdbSession       &Adb() { return m_adb; }
    int               Port() const { return m_port; }

    AndroidApplication *GetCurrentApplication() { return m_app.get(); }
    absl::Status RetrieveTrace(const std::string &trace_file_path, const std::string &save_path);

private:
    const std::string                   m_serial;
    DeviceInfo                          m_dev_info;
    AdbSession                          m_adb;
    DeviceState                         m_original_state;
    std::unique_ptr<AndroidApplication> m_app;
    bool                                m_gfxr_enabled;
    int                                 m_port;
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

    absl::Status DeployReplayApk(const std::string &serial);
    absl::Status RunReplayApk(const std::string &capture_path, const std::string &replay_args);

private:
    std::unique_ptr<AndroidDevice> m_device{ nullptr };
};

std::filesystem::path ResolveAndroidLibPath(const std::string &name,
                                            const std::string &device_architecture);

DeviceManager &GetDeviceManager();
}  // namespace Dive