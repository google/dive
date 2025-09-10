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
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "android_application.h"
#include "command_utils.h"
#include "constants.h"
#include "common/log.h"
#include "common/macros.h"

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
    m_adb(serial),
    m_gfxr_enabled(false),
    m_port(kFirstPort)
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
    m_dev_info.m_serial = m_serial;
    ASSIGN_OR_RETURN(m_dev_info.m_model, Adb().RunAndGetResult("shell getprop ro.product.model"));
    ASSIGN_OR_RETURN(m_dev_info.m_manufacturer,
                     Adb().RunAndGetResult("shell getprop ro.product.manufacturer"));

    LOGD("select: %s\n", GetDeviceDisplayName().c_str());
    LOGD("AndroidDevice created.\n");
    // Determine if the adb was running in root.
    m_original_state.m_is_root_shell = false;
    auto res = Adb().RunAndGetResult("shell whoami");
    if (res.ok())
    {
        m_original_state.m_is_root_shell = (*res == "root");
    }
    res = Adb().RunAndGetResult("shell getprop ro.hardware.vulkan");
    if (res.ok())
    {
        m_dev_info.m_is_adreno_gpu = (*res == "adreno");
    }
    LOGD("is_adreno_gpu: %d\n", m_dev_info.m_is_adreno_gpu);
    return absl::OkStatus();
}

absl::Status AndroidDevice::RequestRootAccess()
{
    RETURN_IF_ERROR(Adb().Run("root"));
    RETURN_IF_ERROR(Adb().Run("wait-for-device"));
    m_original_state.m_root_access_requested = true;

    ASSIGN_OR_RETURN(m_original_state.m_enforce, Adb().RunAndGetResult("shell getenforce"));

    RETURN_IF_ERROR(Adb().Run("shell setenforce 0"));

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
    RETURN_IF_ERROR(
    Adb().Run(absl::StrFormat("push %s %s",
                              ResolveAndroidLibPath(kWrapLibName, "").generic_string(),
                              kTargetPath)));
    if (!m_gfxr_enabled)
    {
        RETURN_IF_ERROR(RequestRootAccess());
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

    return absl::OkStatus();
}

absl::Status AndroidDevice::CleanupDevice()
{
    LOGD("Cleanup device %s\n", m_serial.c_str());
    if (m_original_state.m_root_access_requested)
    {
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
        if (!m_original_state.m_is_root_shell)
        {
            Adb().Run("unroot").IgnoreError();
        }
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
        if (m_gfxr_enabled)
        {
            // Need root to set openxr.enable_frame_delimiter
            // TODO(b/426541653): remove this after all branches in AndroidXR accept the prop of
            // `debug.openxr.enable_frame_delimiter`
            RETURN_IF_ERROR(RequestRootAccess());
            RETURN_IF_ERROR(Adb().Run("shell setprop openxr.enable_frame_delimiter true"));
            // New prop is prefixed with debug.
            RETURN_IF_ERROR(Adb().Run("shell setprop debug.openxr.enable_frame_delimiter true"));
        }
    }
    if (m_app == nullptr)
    {
        return absl::InternalError("Failed allocate memory for AndroidApplication");
    }
    if (m_gfxr_enabled)
    {
        std::string cpu_abi = device_architecture;
        if (cpu_abi.empty())
        {
            ASSIGN_OR_RETURN(cpu_abi, Adb().RunAndGetResult("shell getprop ro.product.cpu.abi"));
        }
        m_app->SetArchitecture(cpu_abi);
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

absl::Status AndroidDevice::CleanupApp()
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
                                         const std::string &replay_args,
                                         bool               dump_pm4,
                                         const std::string &download_path)
{
    LOGD("RunReplayApk(): starting\n");

    std::string updated_replay_args = replay_args;

    // Enable pm4 capture
    if (dump_pm4)
    {
        if (!m_device->IsAdrenoGpu())
        {
            return absl::UnimplementedError("Dump PM4 is only implemented for Adreno GPU");
        }
        if (absl::StrContains(replay_args, "--loop-single-frame") ||
            absl::StrContains(replay_args, "--loop-single-frame-count"))
        {
            return absl::InvalidArgumentError("PM4 capture doesn't support replay arguments "
                                              "--loop-single-frame or --loop-single-frame-count");
        }
        std::string enable_pm4_dump_cmd = absl::StrFormat("shell setprop %s 1",
                                                          kEnableReplayPm4DumpPropertyName);
        m_device->Adb().Run(enable_pm4_dump_cmd).IgnoreError();

        std::string
        dump_pm4_file_name = std::filesystem::path(capture_path).filename().stem().string() + ".rd";
        LOGD("Enable pm4 capture file name is %s\n", dump_pm4_file_name.c_str());
        std::string set_pm4_dump_file_name_cmd = absl::StrFormat("shell setprop %s \"%s\"",
                                                                 kReplayPm4DumpFileNamePropertyName,
                                                                 dump_pm4_file_name);

        m_device->Adb().Run(set_pm4_dump_file_name_cmd).IgnoreError();

        // Loop is needed in the replay to be able to start/stop PM4 capture.
        updated_replay_args += " --loop-single-frame --loop-single-frame-count 2";
    }

    std::string recon_py_path = ResolveAndroidLibPath(kGfxrReconPyPath, "").generic_string();
    std::string cmd = absl::StrFormat("python %s replay %s %s",
                                      recon_py_path,
                                      capture_path,
                                      updated_replay_args);
    absl::StatusOr<std::string> res = RunCommand(cmd);
    // Cleanup PM4 capture
    if (dump_pm4)
    {
        // Wait application to exit before trying to retrieve the trace file.
        do
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } while (m_device->IsProcessRunning(kGfxrReplayAppName));

        std::string disable_pm4_dump_cmd = absl::StrFormat("shell setprop %s 0",
                                                           kEnableReplayPm4DumpPropertyName);
        m_device->Adb().Run(disable_pm4_dump_cmd).IgnoreError();

        std::string
        set_pm4_dump_file_name_cmd = absl::StrFormat("shell setprop %s \\\"\\\"",
                                                     kReplayPm4DumpFileNamePropertyName);
        m_device->Adb().Run(set_pm4_dump_file_name_cmd).IgnoreError();

        std::string on_device_trace_path = absl::StrFormat("%s/%s.rd",
                                                           kDeviceCapturePath,
                                                           std::filesystem::path(capture_path)
                                                           .filename()
                                                           .stem()
                                                           .string()
                                                           .c_str());

        std::string on_device_trace_path_in_progress = absl::StrFormat("%s.inprogress",
                                                                       on_device_trace_path
                                                                       .c_str());

        // Wait for trace file to be written to.
        do
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } while (m_device->FileExists(on_device_trace_path_in_progress));

        auto status = m_device->RetrieveTrace(on_device_trace_path, download_path);
        if (status.ok())
        {
            LOGI("Trace file %s downloaded to %s\n",
                 on_device_trace_path.c_str(),
                 download_path.c_str());
        }
        else
        {
            LOGI("Failed to download the trace file %s\n", on_device_trace_path.c_str());
        }
    }

    if (absl::StrContains(replay_args, "--enable-gpu-time"))
    {
        // Wait application to exit before trying to retrieve the gpu time file.
        do
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } while (m_device->IsProcessRunning(kGfxrReplayAppName));

        // TODO(b/444228664) Prevent gpt time file overwriting for different captures
        std::string on_device_file_path = absl::StrFormat("%s/gpu_time.csv",
                                                          std::filesystem::path(capture_path)
                                                          .parent_path()
                                                          .string()
                                                          .c_str());

        auto status = m_device->RetrieveTrace(on_device_file_path, download_path);
        if (status.ok())
        {
            LOGI("Gpu time file %s downloaded to %s\n",
                 on_device_file_path.c_str(),
                 download_path.c_str());
        }
        else
        {
            LOGI("Failed to download the trace file %s\n", on_device_file_path.c_str());
        }
    }

    if (!res.ok())
    {
        LOGD("ERROR: RunReplayApk(): running replay and args: %s %s\n",
             capture_path.c_str(),
             replay_args.c_str());

        return res.status();
    }

    LOGD("RunReplayApk(): completed\n");
    return absl::OkStatus();
}

absl::Status DeviceManager::RunProfilingOnReplay(const std::string              &capture_path,
                                                 const std::vector<std::string> &metrics,
                                                 const std::string              &download_path)
{
    if (!m_device->IsAdrenoGpu())
    {
        return absl::UnimplementedError("Dump perf counter is only implemented for Adreno GPU");
    }

    LOGD("RunProfilingOnReplay(): starting\n");

    // Deploy libraries and binaries
    std::string copy_cmd = absl::StrFormat("push %s %s",
                                           ResolveAndroidLibPath(kProfilingPluginFolderName, "")
                                           .generic_string(),
                                           kTargetPath);
    RETURN_IF_ERROR(m_device->Adb().Run(copy_cmd));

    // Construct replay arguments for profiling
    // Start the profiling binary
    std::string binary_path_on_device = absl::StrCat(kTargetPath,
                                                     "/",
                                                     kProfilingPluginFolderName,
                                                     "/",
                                                     kProfilingPluginName);
    // Run replay with profiling arguments
    std::string metrics_str = absl::StrJoin(metrics, " ");
    std::string cmd = absl::StrFormat("shell %s %s %s",
                                      binary_path_on_device,
                                      capture_path,
                                      metrics_str);
    RETURN_IF_ERROR(m_device->Adb().Run(cmd));

    // Get the results file path
    std::string output_path = capture_path;
    size_t      dot_pos = output_path.rfind('.');
    if (dot_pos != std::string::npos)
    {
        output_path.replace(dot_pos, std::string::npos, ".csv");
    }
    else
    {
        output_path += ".csv";
    }
    LOGD("Result is at %s \n", output_path.c_str());

    // Get the CSV file.
    auto status = m_device->RetrieveTrace(output_path, download_path);
    if (status.ok())
    {
        LOGI("Trace file %s downloaded to %s\n", output_path.c_str(), download_path.c_str());
    }
    else
    {
        LOGI("Failed to download the trace file %s\n", output_path.c_str());
    }

    // cleanup the library and binary
    std::string clean_cmd = absl::StrFormat("shell rm -rf -- %s/%s",
                                            kTargetPath,
                                            kProfilingPluginFolderName);
    RETURN_IF_ERROR(m_device->Adb().Run(clean_cmd));

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

void AndroidDevice::EnableGfxr(bool enable_gfxr)
{
    m_gfxr_enabled = enable_gfxr;
}

bool AndroidDevice::IsProcessRunning(absl::string_view process_name) const
{
    auto res = Adb().RunAndGetResult(absl::StrCat("shell pidof ", process_name));
    if (!res.ok())
        return false;
    std::string pid = *res;
    if (pid.empty())
        return false;
    return true;
}

bool AndroidDevice::FileExists(const std::string &file_path)
{
    // Checks if the file file exists. If the file returns exists, 0 is returned, if not, 1 is
    // returned.
    std::string cmd = absl::StrFormat("shell test -e \"%s\"", file_path.c_str());

    absl::StatusOr<std::string> result = Adb().Run(cmd);

    return result.ok();
}

}  // namespace Dive
