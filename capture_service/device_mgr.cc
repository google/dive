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

#include "device_mgr.h"

#include <filesystem>
#include <memory>

#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "android_application.h"
#include "command_utils.h"
#include "constants.h"
#include "log.h"

namespace Dive
{

DeviceManager &GetDeviceManager()
{
    static DeviceManager mgr;
    return mgr;
}

AndroidDevice::AndroidDevice(const std::string &serial) :
    m_serial(serial),
    m_adb(serial)
{
    Adb().Run("root");
    Adb().Run("wait-for-device");
    m_original_state.m_enforce = Adb().Run("shell getenforce").Out();
    m_dev_info.m_model = Adb().Run("shell getprop ro.product.model").Out();
    m_dev_info.m_manufacturer = Adb().Run("shell getprop ro.product.manufacturer").Out();

    LOGD("enforce: %s\n", m_original_state.m_enforce.c_str());
    LOGD("select: %s\n", GetDeviceDisplayName().c_str());
    LOGD("AndroidDevice created.\n");
}

AndroidDevice::~AndroidDevice()
{
    if (!m_serial.empty())
    {
        CleanupDevice();
    }
    LOGD("AndroidDevice destroyed.\n");
}

std::string AndroidDevice::GetDeviceDisplayName() const
{
    return absl::StrCat(m_dev_info.m_manufacturer, " ", m_dev_info.m_model, " (", m_serial, ")");
}

std::vector<std::string> AndroidDevice::ListPackage(PackageListOptions option) const
{
    std::vector<std::string> package_list;
    std::string              cmd = "shell pm list packages";
    if (!option.with_system_package)
    {
        cmd += " -3";
    }
    std::string              output = Adb().Run(cmd).Out();
    std::vector<std::string> lines = absl::StrSplit(output, '\n');
    for (const auto &line : lines)
    {
        std::vector<std::string> fields = absl::StrSplit(line, ':');
        if (fields.size() == 2 && fields[0] == "package")
        {
            std::string package(absl::StripAsciiWhitespace(fields[1]));
            if (option.debuggable_only)
            {
                std::string output = Adb().Run("shell dumpsys package " + package).Out();
                // TODO: find out more reliable way to find if app is debuggable.
                if (!absl::StrContains(output, "DEBUGGABLE"))
                {
                    continue;
                }
            }
            package_list.push_back(package);
        }
    }
    std::sort(package_list.begin(), package_list.end());
    return package_list;
}

std::filesystem::path ResolveAndroidLibPath(const std::string &name)
{
    LOGD("cwd: %s\n", std::filesystem::current_path().c_str());
    std::vector<std::filesystem::path> search_paths{ std::filesystem::path{ "./install" },
                                                     std::filesystem::path{
                                                     "../../build_android/Release/bin" },
                                                     std::filesystem::path{ "../../install" },
                                                     std::filesystem::path{ "./" } };
    std::filesystem::path              lib_path{ name };

    for (const auto &p : search_paths)
    {
        lib_path = p / name;
        if (std::filesystem::exists(lib_path))
        {
            break;
        }
    }

    lib_path = std::filesystem::canonical(lib_path);
    LOGD("%s is at %s \n", name.c_str(), lib_path.generic_string().c_str());
    return lib_path;
}

void AndroidDevice::SetupDevice()
{
    Adb().Run("shell setenforce 0");
    Adb().Run("shell getenforce");

    Adb().Run(absl::StrFormat("push %s %s",
                              ResolveAndroidLibPath(kWrapLibName).generic_string(),
                              kTargetPath));
    Adb().Run(absl::StrFormat("push %s %s",
                              ResolveAndroidLibPath(kVkLayerLibName).generic_string(),
                              kTargetPath));
    Adb().Run(absl::StrFormat("push %s %s",
                              ResolveAndroidLibPath(kXrLayerLibName).generic_string(),
                              kTargetPath));
    Adb().Run(absl::StrFormat("forward tcp:%d tcp:%d", kPort, kPort));
}

void AndroidDevice::CleanupDevice()
{
    LOGD("Cleanup device %s\n", m_serial.c_str());
    const auto &enforce = m_original_state.m_enforce;
    if (enforce.find("Enforcing") != enforce.npos)
    {
        LOGD("restore Enforcing to Enforcing\n");
        Adb().Run("shell setenforce 1");
    }
    else if (enforce.find("Permissive") != enforce.npos)
    {
        LOGD("restore Enforcing to Permissive\n");
        Adb().Run("shell setenforce 0");
    }

    Adb().Run(absl::StrFormat("shell rm %s/%s", kTargetPath, kWrapLibName), true);
    Adb().Run(absl::StrFormat("shell rm %s/%s", kTargetPath, kVkLayerLibName), true);
    Adb().Run(absl::StrFormat("shell rm %s/%s", kTargetPath, kXrLayerLibName), true);
    Adb().Run(absl::StrFormat("forward --remove tcp:%d", kPort), true);
    LOGD("Cleanup device %s done\n", m_serial.c_str());
}

void AndroidDevice::SetupApp(const std::string &package, const ApplicationType type)
{
    if (type == ApplicationType::VULKAN)
    {
        app = std::make_unique<VulkanApplication>(*this, package, type);
    }

    else if (type == ApplicationType::OPENXR)
    {
        app = std::make_unique<OpenXRApplication>(*this, package, type);
    }
}

void AndroidDevice::CleanupAPP()
{
    app = nullptr;
}

void AndroidDevice::StartApp()
{
    if (app)
    {
        app->Start();
    }
}

void AndroidDevice::StopApp()
{
    if (app)
    {
        app->Stop();
    }
}

std::vector<std::string> DeviceManager::ListDevice() const
{
    std::vector<std::string> dev_list;
    std::string              output = RunCommand("adb devices").Out();
    std::vector<std::string> lines = absl::StrSplit(output, '\n');

    for (const auto &line : lines)
    {
        std::vector<std::string> fields = absl::StrSplit(line, '\t');
        if (fields.size() == 2 && fields[1] == "device")
            dev_list.push_back(fields[0]);
    }
    std::sort(dev_list.begin(), dev_list.end());

    return dev_list;
}
void DeviceManager::Cleanup(const std::string &serial, const std::string &package)
{
    AdbSession adb(serial);
    // Remove installed libs and libraries on device.
    adb.Run("root");
    adb.Run("wait-for-device");
    adb.Run("remount");

    adb.Run(absl::StrFormat("shell rm %s/%s", kTargetPath, kWrapLibName), true);
    adb.Run(absl::StrFormat("shell rm %s/%s", kTargetPath, kVkLayerLibName), true);
    adb.Run(absl::StrFormat("shell rm %s/%s", kTargetPath, kXrLayerLibName), true);
    adb.Run(absl::StrFormat("shell rm -r %s", kManifestFilePath), true);
    adb.Run(absl::StrFormat("forward --remove tcp:%d", kPort), true);

    adb.Run("shell settings delete global enable_gpu_debug_layers");
    adb.Run("shell settings delete global gpu_debug_app");
    adb.Run("shell settings delete global gpu_debug_layers");
    adb.Run("shell settings delete global gpu_debug_layer_app");
    adb.Run("shell settings delete global gpu_debug_layers_gles");

    // If package specified, remove package related settings.
    if (!package.empty())
    {
        adb.Run(absl::StrFormat("shell setprop wrap.%s \\\"\\\"", package));
    }
}

void AndroidDevice::RetrieveTraceFile(const std::string &trace_file_path,
                                      const std::string &save_path)
{
    Adb().Run(absl::StrFormat("pull %s %s", trace_file_path, save_path));
}

}  // namespace Dive
