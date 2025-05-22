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

#include "../dive_core/common/common.h"
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
    m_serial(serial), m_adb(serial), m_gfxr_enabled(false), m_port(kFirstPort)
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

std::string AndroidDevice::GetDeviceDisplayName() const { return m_dev_info.GetDisplayName(); }

absl::StatusOr<std::vector<std::string>> AndroidDevice::ListPackage(PackageListOptions option) const
{
    std::vector<std::string> package_list;
    std::string              cmd = "shell pm list packages";
    if (option != PackageListOptions::kAll && option != PackageListOptions::kNonDebuggableOnly)
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
            switch (option)
            {
            case PackageListOptions::kAll:
                package_list.push_back(package);
                break;
            case PackageListOptions::kDebuggableOnly:
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
                break;
            case PackageListOptions::kNonDebuggableOnly:
                result = Adb().RunAndGetResult("shell dumpsys package " + package);
                if (!result.ok())
                {
                    return result.status();
                }
                output = *result;
                if (!absl::StrContains(output, "DEBUGGABLE"))
                {
                    package_list.push_back(package);
                }
                break;
            default:
                DIVE_ASSERT(false);
                break;  // Unknown Package List Option.
            }
        }
    }
    std::sort(package_list.begin(), package_list.end());
    return package_list;
}

std::filesystem::path ResolveAndroidLibPath(const std::string &name,
                                            const std::string &device_architecture)
{
    LOGD("cwd: %s\n", std::filesystem::current_path().c_str());
    std::vector<std::filesystem::path> search_paths{ std::filesystem::path{ "./install" },
                                                     std::filesystem::path{
                                                     "../../build_android/Release/bin" },
                                                     std::filesystem::path{ "../../install" },
                                                     std::filesystem::path{ "./" } };

    if (device_architecture != "")
    {
        const std::string gfxr_layer_path = "/gfxr_layer/jni/" + device_architecture;
        search_paths.push_back(std::filesystem::path{ "./install" + gfxr_layer_path });
        search_paths.push_back(std::filesystem::path{ "../../install" + gfxr_layer_path });
        search_paths.push_back(std::filesystem::path{ "." + gfxr_layer_path });
    }

    std::filesystem::path lib_path{ name };

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

absl::Status AndroidDevice::ForwardFirstAvailablePort()
{
    for (int p = kFirstPort; p <= kFirstPort + kPortRange; p++)
    {
        auto res = Adb().RunAndGetResult(
        absl::StrFormat("forward tcp:%d localabstract:%s", p, kUnixAbstractPath));
        if (res.ok())
        {
            m_port = p;
            return absl::OkStatus();
        }
    }
    return absl::Status(absl::StatusCode::kUnknown,
                        absl::StrFormat("Failed to forward available port between %d and %d",
                                        kFirstPort,
                                        kFirstPort + kPortRange));
}

absl::Status AndroidDevice::SetupDevice()
{
    RETURN_IF_ERROR(Adb().Run("shell setenforce 0"));
    RETURN_IF_ERROR(Adb().Run("shell getenforce"));

    if (!m_gfxr_enabled)
    {
        RETURN_IF_ERROR(
        Adb().Run(absl::StrFormat("push %s %s",
                                  ResolveAndroidLibPath(kWrapLibName, "").generic_string(),
                                  kTargetPath)));
        RETURN_IF_ERROR(
        Adb().Run(absl::StrFormat("push %s %s",
                                  ResolveAndroidLibPath(kVkLayerLibName, "").generic_string(),
                                  kTargetPath)));
        RETURN_IF_ERROR(
        Adb().Run(absl::StrFormat("push %s %s",
                                  ResolveAndroidLibPath(kXrLayerLibName, "").generic_string(),
                                  kTargetPath)));
        RETURN_IF_ERROR(ForwardFirstAvailablePort());
    }

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
        Adb().Run("shell setenforce 1").IgnoreError();
    }
    else if (enforce.find("Permissive") != enforce.npos)
    {
        LOGD("restore Enforcing to Permissive\n");
        Adb().Run("shell setenforce 0").IgnoreError();
    }

    Adb().Run(absl::StrFormat("shell rm -f -- %s/%s", kTargetPath, kWrapLibName)).IgnoreError();
    Adb().Run(absl::StrFormat("shell rm -f -- %s/%s", kTargetPath, kVkLayerLibName)).IgnoreError();
    Adb().Run(absl::StrFormat("shell rm -f -- %s/%s", kTargetPath, kXrLayerLibName)).IgnoreError();
    Adb()
    .Run(absl::StrFormat("shell rm -f -- %s/%s", kTargetPath, kVkGfxrLayerLibName))
    .IgnoreError();
    Adb().Run(absl::StrFormat("shell rm -rf -- %s", kManifestFilePath)).IgnoreError();
    absl::StatusOr<std::string> output = Adb().RunAndGetResult(absl::StrFormat("forward --list"));
    if (output.ok())
    {
        if (output->find(std::to_string(Port())) != std::string::npos)
        {
            Adb().Run(absl::StrFormat("forward --remove tcp:%d", Port())).IgnoreError();
        }
    }

    Adb().Run("shell settings delete global enable_gpu_debug_layers").IgnoreError();
    Adb().Run("shell settings delete global gpu_debug_app").IgnoreError();
    Adb().Run("shell settings delete global gpu_debug_layers").IgnoreError();
    Adb().Run("shell settings delete global gpu_debug_layer_app").IgnoreError();
    Adb().Run("shell settings delete global gpu_debug_layers_gles").IgnoreError();

#if defined(DIVE_ENABLE_PERFETTO)
    Adb().Run("shell setprop debug.graphics.gpu.profiler.perfetto 0").IgnoreError();
#endif

    LOGD("Cleanup device %s done\n", m_serial.c_str());
    return absl::OkStatus();
}

absl::Status AndroidDevice::CleanupPackage(const std::string &package)
{
    LOGD("Cleanup package %s\n", package.c_str());
    Adb().Run(absl::StrFormat("shell setprop wrap.%s \\\"\\\"", package)).IgnoreError();
    LOGD("Cleanup package %s done\n", package.c_str());
    return absl::OkStatus();
}

absl::Status AndroidDevice::SetupApp(const std::string    &package,
                                     const ApplicationType type,
                                     const std::string    &command_args,
                                     const std::string    &device_architecture,
                                     const std::string    &gfxr_capture_directory)
{
    if (type == ApplicationType::VULKAN_APK)
    {
        m_app = std::make_unique<VulkanApplication>(*this, package, command_args);
    }

    else if (type == ApplicationType::OPENXR_APK)
    {
        m_app = std::make_unique<OpenXRApplication>(*this, package, command_args);
    }
    if (m_app == nullptr)
    {
        return absl::InternalError("Failed allocate memory for AndroidApplication");
    }
    if (m_gfxr_enabled)
    {
        m_app->SetArchitecture(device_architecture);
        m_app->SetGfxrCaptureFileDirectory(gfxr_capture_directory);
        m_app->SetGfxrEnabled(true);
    }
    else
    {
        m_app->SetGfxrEnabled(false);
    }
    return m_app->Setup();
}

absl::Status AndroidDevice::SetupApp(const std::string    &command,
                                     const std::string    &command_args,
                                     const ApplicationType type)
{
    assert(type == ApplicationType::VULKAN_CLI);
    m_app = std::make_unique<VulkanCliApplication>(*this, command, command_args);

    if (m_app == nullptr)
    {
        return absl::InternalError("Failed allocate memory for VulkanCliApplication");
    }

    return m_app->Setup();
}

absl::Status AndroidDevice::CleanupAPP()
{
    m_app = nullptr;
    return absl::OkStatus();
}

absl::Status AndroidDevice::StartApp()
{
    if (m_app)
    {
        return m_app->Start();
    }
    return absl::OkStatus();
}

absl::Status AndroidDevice::StopApp()
{
    if (m_app)
    {
        return m_app->Stop();
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

absl::Status DeviceManager::DeployReplayApk(const std::string &serial)
{
    LOGD("DeployReplayApk(): starting\n");

    std::string replay_apk_path = ResolveAndroidLibPath(kGfxrReplayApkName, "").generic_string();
    std::string recon_py_path = ResolveAndroidLibPath(kGfxrReconPyPath, "").generic_string();
    std::string cmd = absl::StrFormat("python %s install-apk %s -s %s",
                                      recon_py_path,
                                      replay_apk_path,
                                      serial);
    absl::StatusOr<std::string> res = RunCommand(cmd);
    if (!res.ok())
    {
        LOGD("ERROR: DeployReplayApk(): deploying apk at: %s\n", replay_apk_path.c_str());
        return res.status();
    }

    cmd = absl::StrFormat("adb shell appops set %s MANAGE_EXTERNAL_STORAGE allow",
                          kGfxrReplayAppName);
    res = RunCommand(cmd);
    if (!res.ok())
    {
        LOGD("ERROR: DeployReplayApk(): setting MANAGE_EXTERNAL_STORAGE allow\n");
        return res.status();
    }

    LOGD("DeployReplayApk(): completed\n");
    return absl::OkStatus();
}

absl::Status DeviceManager::RunReplayApk(const std::string &capture_path,
                                         const std::string &replay_args)
{
    LOGD("RunReplayApk(): starting\n");

    std::string recon_py_path = ResolveAndroidLibPath(kGfxrReconPyPath, "").generic_string();
    std::string cmd = absl::StrFormat("python %s replay %s %s",
                                      recon_py_path,
                                      capture_path,
                                      replay_args);
    absl::StatusOr<std::string> res = RunCommand(cmd);
    if (!res.ok())
    {
        LOGD("ERROR: RunReplayApk(): running capture and args: %s %s\n",
             capture_path.c_str(),
             replay_args.c_str());
        ;
        return res.status();
    }

    LOGD("RunReplayApk(): completed\n");
    return absl::OkStatus();
}

absl::Status DeviceManager::Cleanup(const std::string &serial, const std::string &package)
{
    // If package specified, remove package related settings.
    if (!package.empty())
    {
        GetDevice()->CleanupPackage(package).IgnoreError();
    }

    // Cleanup of device settings and installed libraries is handled in
    // AndroidDevice::CleanupDevice.

    return absl::OkStatus();
}

absl::Status AndroidDevice::RetrieveTrace(const std::string &trace_path,
                                          const std::string &save_path)
{
    RETURN_IF_ERROR(Adb().Run(absl::StrFormat("pull %s %s", trace_path, save_path)));
    return Adb().Run(absl::StrFormat("shell rm -rf %s", trace_path));
}

void AndroidDevice::EnableGfxr(bool enable_gfxr) { m_gfxr_enabled = enable_gfxr; }

}  // namespace Dive
