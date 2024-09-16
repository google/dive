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

#include "absl/status/status.h"
#include "absl/status/statusor.h"
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

std::string DeviceInfo::GetDisplayName() const
{
    return absl::StrCat(m_manufacturer, " ", m_model, " (", m_serial, ")");
}

AndroidDevice::AndroidDevice(const std::string &serial) :
    m_serial(serial),
    m_adb(serial)
{
}

AndroidDevice::~AndroidDevice()
{
    if (!m_serial.empty())
    {
        CleanupDevice().IgnoreError();
    }
    LOGD("AndroidDevice destroyed.\n");
}

absl::Status AndroidDevice::Init()
{
    RETURN_IF_ERROR(Adb().Run("root"));
    RETURN_IF_ERROR(Adb().Run("wait-for-device"));

    ASSIGN_OR_RETURN(m_original_state.m_enforce, Adb().RunAndGetResult("shell getenforce"));
    m_dev_info.m_serial = m_serial;
    ASSIGN_OR_RETURN(m_dev_info.m_model, Adb().RunAndGetResult("shell getprop ro.product.model"));
    ASSIGN_OR_RETURN(m_dev_info.m_manufacturer,
                     Adb().RunAndGetResult("shell getprop ro.product.manufacturer"));

    LOGD("enforce: %s\n", m_original_state.m_enforce.c_str());
    LOGD("select: %s\n", GetDeviceDisplayName().c_str());
    LOGD("AndroidDevice created.\n");

    return absl::OkStatus();
}

std::string AndroidDevice::GetDeviceDisplayName() const
{
    return m_dev_info.GetDisplayName();
}

absl::StatusOr<std::vector<std::string>> AndroidDevice::ListPackage(PackageListOptions option) const
{
    std::vector<std::string> package_list;
    std::string              cmd = "shell pm list packages";
    if (!option.all && !option.non_debuggable_only)
    {
        cmd += " -3";
    }
    std::string                 output;
    absl::StatusOr<std::string> result = Adb().RunAndGetResult(cmd);
    if (!result.ok())
    {
        return result.status();
    }
    output = *result;
    std::vector<std::string> lines = absl::StrSplit(output, '\n');
    for (const auto &line : lines)
    {
        std::vector<std::string> fields = absl::StrSplit(line, ':');
        if (fields.size() == 2 && fields[0] == "package")
        {
            std::string package(absl::StripAsciiWhitespace(fields[1]));

            if (option.all)
            {
                package_list.push_back(package);
                continue;
            }

            if (option.debuggable_only)
            {
                result = Adb().RunAndGetResult("shell dumpsys package " + package);
                if (!result.ok())
                {
                    return result.status();
                }
                output = *result;
                // TODO: find out more reliable way to find if app is debuggable.
                if (absl::StrContains(output, "DEBUGGABLE"))
                {
                    package_list.push_back(package);
                }
            }
            
            if (option.non_debuggable_only)
            {
                result = Adb().RunAndGetResult("shell dumpsys package " + package);
                if (!result.ok())
                {
                    return result.status();
                }
                output = *result;
                if (!absl::StrContains(output, "DEBUGGABLE"))
                {
                    package_list.push_back(package);;
                }
            }
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
            lib_path = std::filesystem::canonical(lib_path);
            break;
        }
    }

    LOGD("%s is at %s \n", name.c_str(), lib_path.generic_string().c_str());
    return lib_path;
}

absl::Status AndroidDevice::SetupDevice()
{
    RETURN_IF_ERROR(Adb().Run("shell setenforce 0"));
    RETURN_IF_ERROR(Adb().Run("shell getenforce"));

    RETURN_IF_ERROR(Adb().Run(absl::StrFormat("push %s %s",
                                              ResolveAndroidLibPath(kWrapLibName).generic_string(),
                                              kTargetPath)));
    RETURN_IF_ERROR(
    Adb().Run(absl::StrFormat("push %s %s",
                              ResolveAndroidLibPath(kVkLayerLibName).generic_string(),
                              kTargetPath)));
    RETURN_IF_ERROR(
    Adb().Run(absl::StrFormat("push %s %s",
                              ResolveAndroidLibPath(kXrLayerLibName).generic_string(),
                              kTargetPath)));
    RETURN_IF_ERROR(Adb().Run(absl::StrFormat("forward tcp:%d tcp:%d", kPort, kPort)));
#if defined(DIVE_ENABLE_PERFETTO)
    RETURN_IF_ERROR(Adb().Run("shell setprop debug.graphics.gpu.profiler.perfetto 1"));
#endif
    return absl::OkStatus();
}

absl::Status AndroidDevice::CleanupDevice()
{
    LOGD("Cleanup device %s\n", m_serial.c_str());
    const auto &enforce = m_original_state.m_enforce;
    if (enforce.find("Enforcing") != enforce.npos)
    {
        LOGD("restore Enforcing to Enforcing\n");
        RETURN_IF_ERROR(Adb().Run("shell setenforce 1"));
    }
    else if (enforce.find("Permissive") != enforce.npos)
    {
        LOGD("restore Enforcing to Permissive\n");
        RETURN_IF_ERROR(Adb().Run("shell setenforce 0"));
    }

    RETURN_IF_ERROR(Adb().Run(absl::StrFormat("shell rm %s/%s", kTargetPath, kWrapLibName), true));
    RETURN_IF_ERROR(
    Adb().Run(absl::StrFormat("shell rm %s/%s", kTargetPath, kVkLayerLibName), true));
    RETURN_IF_ERROR(
    Adb().Run(absl::StrFormat("shell rm %s/%s", kTargetPath, kXrLayerLibName), true));
    RETURN_IF_ERROR(Adb().Run(absl::StrFormat("forward --remove tcp:%d", kPort), true));
    LOGD("Cleanup device %s done\n", m_serial.c_str());
#if defined(DIVE_ENABLE_PERFETTO)
    RETURN_IF_ERROR(Adb().Run("shell setprop debug.graphics.gpu.profiler.perfetto 0"));
#endif
    return absl::OkStatus();
}

absl::Status AndroidDevice::SetupApp(const std::string    &package,
                                     const ApplicationType type,
                                     const std::string    &command_args)
{
    if (type == ApplicationType::VULKAN_APK)
    {
        app = std::make_unique<VulkanApplication>(*this, package, command_args);
    }

    else if (type == ApplicationType::OPENXR_APK)
    {
        app = std::make_unique<OpenXRApplication>(*this, package, command_args);
    }
    if (app == nullptr)
    {
        return absl::InternalError("Failed allocate memory for AndroidApplication");
    }
    return app->Setup();
}

absl::Status AndroidDevice::SetupApp(const std::string    &command,
                                     const std::string    &command_args,
                                     const ApplicationType type)
{
    assert(type == ApplicationType::VULKAN_CLI);
    app = std::make_unique<VulkanCliApplication>(*this, command, command_args);

    if (app == nullptr)
    {
        return absl::InternalError("Failed allocate memory for VulkanCliApplication");
    }

    return app->Setup();
}

absl::Status AndroidDevice::CleanupAPP()
{
    app = nullptr;
    return absl::OkStatus();
}

absl::Status AndroidDevice::StartApp()
{
    if (app)
    {
        return app->Start();
    }
    return absl::OkStatus();
}

absl::Status AndroidDevice::StopApp()
{
    if (app)
    {
        return app->Stop();
    }
    return absl::OkStatus();
}

std::vector<DeviceInfo> DeviceManager::ListDevice() const
{
    std::vector<std::string> serial_list;
    std::vector<DeviceInfo>  dev_list;

    std::string                 output;
    absl::StatusOr<std::string> result = RunCommand("adb devices");
    if (result.ok())
    {
        output = *result;
    }
    else
    {
        return dev_list;
    }

    std::vector<std::string> lines = absl::StrSplit(output, '\n');

    for (const auto &line : lines)
    {
        std::vector<std::string> fields = absl::StrSplit(line, '\t');
        if (fields.size() == 2 && fields[1] == "device")
            serial_list.push_back(fields[0]);
    }

    for (auto &serial : serial_list)
    {
        DeviceInfo dev;
        AdbSession adb(serial);

        dev.m_serial = std::move(serial);
        result = adb.RunAndGetResult("shell getprop ro.product.manufacturer");
        if (result.ok())
        {
            dev.m_manufacturer = *result;
        }
        else
        {
            continue;
        }

        result = adb.RunAndGetResult("shell getprop ro.product.model");
        if (result.ok())
        {
            dev.m_model = *result;
        }
        else
        {
            continue;
        }

        dev_list.emplace_back(std::move(dev));
    }
    return dev_list;
}

absl::StatusOr<AndroidDevice *> DeviceManager::SelectDevice(const std::string &serial)
{
    assert(!serial.empty());
    if (serial.empty())
    {
        return absl::InvalidArgumentError("Device Serial is empty");
    }

    m_device = std::make_unique<AndroidDevice>(serial);
    if (!m_device)
    {
        return absl::UnavailableError("Failed to allocate memory for AndroidDevice");
    }

    absl::Status status = m_device->Init();
    if (!status.ok())
    {
        return status;
    }

    return m_device.get();
}

absl::Status DeviceManager::Cleanup(const std::string &serial, const std::string &package)
{
    AdbSession adb(serial);
    // Remove installed libs and libraries on device.
    RETURN_IF_ERROR(adb.Run("root"));
    RETURN_IF_ERROR(adb.Run("wait-for-device"));
    RETURN_IF_ERROR(adb.Run("remount"));

    RETURN_IF_ERROR(adb.Run(absl::StrFormat("shell rm %s/%s", kTargetPath, kWrapLibName), true));
    RETURN_IF_ERROR(adb.Run(absl::StrFormat("shell rm %s/%s", kTargetPath, kVkLayerLibName), true));
    RETURN_IF_ERROR(adb.Run(absl::StrFormat("shell rm %s/%s", kTargetPath, kXrLayerLibName), true));
    RETURN_IF_ERROR(adb.Run(absl::StrFormat("shell rm -r %s", kManifestFilePath), true));
    RETURN_IF_ERROR(adb.Run(absl::StrFormat("forward --remove tcp:%d", kPort), true));

    RETURN_IF_ERROR(adb.Run("shell settings delete global enable_gpu_debug_layers"));
    RETURN_IF_ERROR(adb.Run("shell settings delete global gpu_debug_app"));
    RETURN_IF_ERROR(adb.Run("shell settings delete global gpu_debug_layers"));
    RETURN_IF_ERROR(adb.Run("shell settings delete global gpu_debug_layer_app"));
    RETURN_IF_ERROR(adb.Run("shell settings delete global gpu_debug_layers_gles"));

    // If package specified, remove package related settings.
    if (!package.empty())
    {
        RETURN_IF_ERROR(adb.Run(absl::StrFormat("shell setprop wrap.%s \\\"\\\"", package)));
    }
    return absl::OkStatus();
}

absl::Status AndroidDevice::RetrieveTraceFile(const std::string &trace_file_path,
                                              const std::string &save_path)
{
    RETURN_IF_ERROR(Adb().Run(absl::StrFormat("pull %s %s", trace_file_path, save_path)));
    return Adb().Run(absl::StrFormat("shell rm %s", trace_file_path));
}

}  // namespace Dive
