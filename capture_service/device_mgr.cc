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
#include <future>
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
#include "common/defer.h"
#include "remote_files.h"
#include "utils/component_files.h"

namespace Dive
{

namespace
{

// adb shell setprop `property` `value`
absl::Status SetSystemProperty(const AdbSession &adb,
                               std::string_view  property,
                               std::string_view  value)
{
    return adb.Run(absl::StrFormat("shell setprop %s %s", property, value));
}

// Set the property to an empty string. The users of the Android API typically don't make a
// distinction between "set and empty" or "unset".
absl::Status UnsetSystemProperty(const AdbSession &adb, std::string_view property)
{
    // This command goes through 3 layers of interpretation:
    // 1. Device shell. Requires explicit empty string "" to perform unset.
    // 2. Host shell. Requires quotes to be escaped with backslash.
    // 3. C++ string literals. Requires quotes and backslashes to be escaped with backslash.
    return adb.Run(absl::StrFormat("shell setprop %s \\\"\\\"", property));
}

// Delete all persistent Android settings related to using Vulkan debug layers
absl::Status DisableVulkanLayer(const AdbSession &adb)
{
    // See https://developer.android.com/ndk/guides/graphics/validation-layer
    absl::Status status;
    status.Update(adb.Run("shell settings delete global enable_gpu_debug_layers"));
    status.Update(adb.Run("shell settings delete global gpu_debug_app"));
    status.Update(adb.Run("shell settings delete global gpu_debug_layers"));
    status.Update(adb.Run("shell settings delete global gpu_debug_layer_app"));
    status.Update(adb.Run("shell settings delete global gpu_debug_layers_gles"));
    return status;
}

// Set the required Android settings in order to implicitly load a `layer` when `app` is run.
// `layer_app` is the package that Android will search to find `layer`.
absl::Status EnableVulkanLayer(const AdbSession &adb,
                               std::string_view  app,
                               std::string_view  layer,
                               std::string_view  layer_app = "")
{
    // Start with a clean slate
    RETURN_IF_ERROR(DisableVulkanLayer(adb));
    // See https://developer.android.com/ndk/guides/graphics/validation-layer
    RETURN_IF_ERROR(adb.Run("shell settings put global enable_gpu_debug_layers 1"));
    RETURN_IF_ERROR(adb.Run(absl::StrFormat("shell settings put global gpu_debug_app %s", app)));
    RETURN_IF_ERROR(
    adb.Run(absl::StrFormat("shell settings put global gpu_debug_layers %s", layer)));
    if (!layer_app.empty())
    {
        RETURN_IF_ERROR(
        adb.Run(absl::StrFormat("shell settings put global gpu_debug_layer_app %s", layer_app)));
    }
    return absl::OkStatus();
}

absl::Status IsAppInstalled(const AdbSession &adb, std::string_view package)
{
    return adb.Run(absl::StrFormat("shell pm path %s", package));
}

absl::Status InstallVulkanLayer(const AdbSession &adb,
                                std::string_view  package,
                                std::string_view  layer_filename)
{
    RETURN_IF_ERROR(
    adb.Run(absl::StrFormat(R"(push "%s" "%s")",
                            ResolveAndroidLibPath(std::string(layer_filename)).generic_string(),
                            kTargetPath)));
    RETURN_IF_ERROR(adb.Run(
    absl::StrFormat(R"(shell run-as %s cp "%s/%s" .)", package, kTargetPath, layer_filename)));
    return absl::OkStatus();
}

absl::Status UninstallVulkanLayer(const AdbSession &adb,
                                  std::string_view  package,
                                  std::string_view  layer_filename)
{
    absl::Status status;
    status.Update(
    adb.Run(absl::StrFormat(R"(shell rm -rf -- "%s/%s")", kTargetPath, layer_filename)));
    status.Update(
    adb.Run(absl::StrFormat(R"(shell run-as %s rm -rf -- "%s")", package, layer_filename)));
    return status;
}

}  // namespace

DeviceManager &GetDeviceManager()
{
    static DeviceManager mgr;
    return mgr;
}

std::string DeviceInfo::GetDisplayName() const
{
    return absl::StrCat(m_manufacturer, " ", m_model, " (", m_serial, ")");
}

absl::StatusOr<GfxrReplaySettings> ValidateGfxrReplaySettings(const GfxrReplaySettings &settings,
                                                              bool is_adreno_gpu)
{
    // Early return if basic info not provided
    if (settings.remote_capture_path.empty())
    {
        return absl::InvalidArgumentError("Must provide remote_capture_path");
    }
    if (settings.local_download_dir.empty())
    {
        return absl::InvalidArgumentError("Must provide local_download_dir");
    }

    // Early return if replay_flags_str contains invalid characters
    if (absl::StrContains(settings.replay_flags_str, '='))
    {
        return absl::InvalidArgumentError("replay_flags_str cannot contain '='");
    }

    GfxrReplaySettings validated_settings = settings;
    validated_settings.replay_flags_str = "";

    // Parse relevant GFXR replay flags
    std::vector<std::string> split_args = absl::StrSplit(settings.replay_flags_str,
                                                         " ",
                                                         absl::SkipWhitespace());
    if (auto it = std::find(split_args.begin(), split_args.end(), "--loop-single-frame-count");
        it != split_args.end())
    {
        if (settings.loop_single_frame_count.has_value())
        {
            return absl::InvalidArgumentError(
            "Do not specify loop_single_frame_count in GfxrReplaySettings and also as flag "
            "--loop-single-frame-count");
        }
        try
        {
            validated_settings.loop_single_frame_count = std::stoi(*(it + 1));
        }
        catch (std::exception &e)
        {
            LOGD("Exception: %s", e.what());
            return absl::InvalidArgumentError(
            absl::StrFormat("Value specified for --loop-single-frame-count can't be parsed as "
                            "integer: %s",
                            *(it + 1)));
        }
        split_args.erase(it, it + 2);
    }
    if (auto it = std::find(split_args.begin(), split_args.end(), "--enable-gpu-time");
        it != split_args.end())
    {
        if (settings.run_type != GfxrReplayOptions::kNormal)
        {
            return absl::InvalidArgumentError(
            "Do not specify run_type = kGpuTiming in GfxrReplaySettings and also as flag "
            "--enable-gpu-time");
        }
        validated_settings.run_type = GfxrReplayOptions::kGpuTiming;
        split_args.erase(it);
    }

    // Check for run_type-specific settings
    switch (validated_settings.run_type)
    {
    case GfxrReplayOptions::kPm4Dump:
    {
        if (!is_adreno_gpu)
        {
            return absl::UnimplementedError("Dump PM4 is only implemented for Adreno GPU");
        }
        if (validated_settings.loop_single_frame_count.has_value())
        {
            return absl::InvalidArgumentError(
            "loop_single_frame_count is hardcoded for kPm4Dump, do not specify");
        }
        validated_settings.loop_single_frame_count = 2;
        if (!validated_settings.metrics.empty())
        {
            return absl::InvalidArgumentError(
            "Cannot use metrics except for kPerfCounters type run");
        }
        if (validated_settings.use_validation_layer)
        {
            return absl::InvalidArgumentError("use_validation_layer is only allowed for kNormal");
        }
        break;
    }
    case GfxrReplayOptions::kPerfCounters:
    {
        if (!is_adreno_gpu)
        {
            return absl::UnimplementedError(
            "Perf counters feature is only implemented for Adreno GPU");
        }
        if (validated_settings.loop_single_frame_count.has_value())
        {
            return absl::InvalidArgumentError(
            "loop_single_frame_count is hardcoded for kPerfCounters, do not specify");
        }
        validated_settings.loop_single_frame_count = 0;
        if (validated_settings.metrics.size() == 0)
        {
            // Profiling binary will provide its own set of default metrics for debugging purposes,
            // but when initiating profiling replay through Dive, user-specified metrics are
            // required.
            return absl::InvalidArgumentError("Must provide metrics for kPerfCounters type run");
        }
        if (validated_settings.use_validation_layer)
        {
            return absl::InvalidArgumentError("use_validation_layer is only allowed for kNormal");
        }
        break;
    }
    case GfxrReplayOptions::kRenderDoc:
    {
        if (validated_settings.loop_single_frame_count.has_value())
        {
            return absl::InvalidArgumentError(
            "loop_single_frame_count is hardcoded for kRenderDoc, do not specify");
        }
        // Since RenderDoc is a debugging tool, not a profiling tool, we only need one copy of all
        // draw calls. Having multiple copies just bloats the file size.
        validated_settings.loop_single_frame_count = 1;
        if (!validated_settings.metrics.empty())
        {
            return absl::InvalidArgumentError(
            "Cannot use metrics except for kPerfCounters type run");
        }
        if (validated_settings.use_validation_layer)
        {
            return absl::InvalidArgumentError("use_validation_layer is only allowed for kNormal");
        }
        break;
    }
    case GfxrReplayOptions::kGpuTiming:
    {
        if (!validated_settings.metrics.empty())
        {
            return absl::InvalidArgumentError(
            "Cannot use metrics except for kPerfCounters type run");
        }
        // value_or(1) since the default used by DiveFileProcessor is 1
        if (validated_settings.loop_single_frame_count.value_or(1) <= 0)
        {
            return absl::InvalidArgumentError(
            "loop_single_frame_count must be >0 for kGpuTiming and kNormal runs");
        }
        if (validated_settings.use_validation_layer)
        {
            return absl::InvalidArgumentError("use_validation_layer is only allowed for kNormal");
        }
        break;
    }
    case GfxrReplayOptions::kNormal:
    default:
    {
        if (!validated_settings.metrics.empty())
        {
            return absl::InvalidArgumentError(
            "Cannot use metrics except for kPerfCounters type run");
        }
        // value_or(1) since the default used by DiveFileProcessor is 1
        if (validated_settings.loop_single_frame_count.value_or(1) <= 0)
        {
            return absl::InvalidArgumentError(
            "loop_single_frame_count must be >0 for kGpuTiming and kNormal runs");
        }
        break;
    }
    }

    // Re-concatenate flags to form a validated replay_flags_str
    if (validated_settings.loop_single_frame_count.has_value())
    {
        assert(*(validated_settings.loop_single_frame_count) >= 0);
        split_args.push_back("--loop-single-frame-count");
        split_args.push_back(std::to_string(*(validated_settings.loop_single_frame_count)));
    }
    if (validated_settings.run_type == GfxrReplayOptions::kGpuTiming)
    {
        split_args.push_back("--enable-gpu-time");
    }
    if (validated_settings.run_type == GfxrReplayOptions::kRenderDoc)
    {
        // Renderdoc introduces some extensions that are preventing the replay.
        // This change fixed the issue of not being able to capture renderdoc for some gfxr captures
        split_args.push_back("--remove-unsupported");
    }
    validated_settings.replay_flags_str = absl::StrJoin(split_args, " ");

    LOGI("ValidateGfxrReplaySettings(): Validated replay_flags_str: %s\n",
         validated_settings.replay_flags_str.c_str());

    return validated_settings;
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

    absl::StatusOr<std::string> list_packages_output = Adb().RunAndGetResult(cmd);
    if (!list_packages_output.ok())
    {
        return list_packages_output.status();
    }
    std::vector<std::string> lines = absl::StrSplit(*list_packages_output, '\n');

    std::vector<std::string> all_packages;
    for (const auto &line : lines)
    {
        std::vector<std::string> fields = absl::StrSplit(line, ':');
        if (fields.size() == 2 && fields[0] == "package")
        {
            all_packages.push_back(std::string(absl::StripAsciiWhitespace(fields[1])));
        }
    }

    if (option == PackageListOptions::kAll)
    {
        package_list = all_packages;
    }
    else
    {
        std::vector<std::future<absl::StatusOr<std::string>>> futures;

        for (const auto &pkg : all_packages)
        {
            futures.push_back(std::async(std::launch::async, [this, pkg]() {
                return Adb().RunAndGetResult("shell dumpsys package " + pkg);
            }));
        }

        for (size_t i = 0; i < futures.size(); ++i)
        {
            absl::StatusOr<std::string> dumpsys_output = futures[i].get();
            if (!dumpsys_output.ok())
            {
                return dumpsys_output.status();
            }
            const std::string &current_package = all_packages[i];

            if (option == PackageListOptions::kDebuggableOnly)
            {
                if (absl::StrContains(*dumpsys_output, "DEBUGGABLE"))
                {
                    package_list.push_back(current_package);
                }
            }
            else if (option == PackageListOptions::kNonDebuggableOnly)
            {
                if (!absl::StrContains(*dumpsys_output, "DEBUGGABLE"))
                {
                    package_list.push_back(current_package);
                }
            }
        }
    }

    std::sort(package_list.begin(), package_list.end());
    return package_list;
}

std::filesystem::path ResolveAndroidLibPath(const std::string &name,
                                            const std::string &device_architecture)
{
    std::filesystem::path                 exe_dir;
    absl::StatusOr<std::filesystem::path> ret = GetExecutableDirectory();
    if (ret.ok())
    {
        exe_dir = *ret;
    }
    else
    {
        LOGW("Could not determine executable directory: %s. Search will not include "
             "executable-relative paths.",
             ret.status().message().data());
    }

    std::vector<std::filesystem::path> search_paths{
        exe_dir / "install", "./install", "../../build_android/Release/bin", "../../install", "./"
    };

    if (!device_architecture.empty())
    {
        const std::filesystem::path gfxr_sub_path = std::filesystem::path("gfxr_layer") / "jni" /
                                                    device_architecture;

        search_paths.push_back(exe_dir / "install" / gfxr_sub_path);
        search_paths.push_back("./install" / gfxr_sub_path);
        search_paths.push_back("../../install" / gfxr_sub_path);
        search_paths.push_back("." / gfxr_sub_path);
    }

    for (const auto &p : search_paths)
    {
        const auto potential_path = p / name;
        if (std::filesystem::exists(potential_path))
        {
            auto canonical_path = std::filesystem::canonical(potential_path);
            LOGD("Found %s at %s \n", name.c_str(), canonical_path.generic_string().c_str());
            return canonical_path;
        }
    }
    LOGE("Could not find '%s' in any of the search paths. \n", name.c_str());
    return {};
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
    Adb().Run(absl::StrFormat(R"(push "%s" "%s")",
                              ResolveAndroidLibPath(kWrapLibName, "").generic_string(),
                              kTargetPath)));
    if (!m_gfxr_enabled)
    {
        RETURN_IF_ERROR(RequestRootAccess());
        RETURN_IF_ERROR(
        Adb().Run(absl::StrFormat(R"(push "%s" "%s")",
                                  ResolveAndroidLibPath(kVkLayerLibName, "").generic_string(),
                                  kTargetPath)));
        RETURN_IF_ERROR(
        Adb().Run(absl::StrFormat(R"(push "%s" "%s")",
                                  ResolveAndroidLibPath(kXrLayerLibName, "").generic_string(),
                                  kTargetPath)));
        RETURN_IF_ERROR(ForwardFirstAvailablePort());
    }

    return absl::OkStatus();
}

absl::Status AndroidDevice::CleanupDevice()
{
    LOGD("Cleanup device %s\n", m_serial.c_str());

    UnpinGpuClock().IgnoreError();
    Adb().Run("shell setprop compositor.high_priority 1").IgnoreError();

    // TODO(b/426541653): remove this after all branches in AndroidXR accept the prop of
    // `debug.openxr.enable_frame_delimiter`
    Adb().Run("shell setprop openxr.enable_frame_delimiter false").IgnoreError();
    Adb().Run("shell setprop debug.openxr.enable_frame_delimiter false").IgnoreError();

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
    Adb().Run(absl::StrFormat("shell rm -rf -- %s", kReplayStateLoadedSignalFile)).IgnoreError();
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

    // clean up for gfxr renderdoc capture
    UnsetSystemProperty(Adb(), kReplayCreateRenderDocCapture).IgnoreError();

    // cleanup for gfxr replay with validation layer
    UninstallVulkanLayer(Adb(),
                         /*app=*/kGfxrReplayAppName,
                         /*layer_filename=*/kVkValidationLayerLibName)
    .IgnoreError();

    // clean up for gfxr replay app
    Adb()
    .Run(absl::StrFormat("shell appops set %s MANAGE_EXTERNAL_STORAGE default", kGfxrReplayAppName))
    .IgnoreError();
    Adb().Run(absl::StrFormat("uninstall %s", kGfxrReplayAppName)).IgnoreError();

    // cleanup for gfxr PM4 capture
    Adb()
    .Run(absl::StrFormat("shell setprop %s 0", kEnableReplayPm4DumpPropertyName))
    .IgnoreError();
    Adb()
    .Run(absl::StrFormat("shell setprop %s \\\"\\\"", kReplayPm4DumpFileNamePropertyName))
    .IgnoreError();

    // cleanup for profiling plugin
    Adb()
    .Run(absl::StrFormat("shell rm -rf -- %s/%s", kTargetPath, kProfilingPluginFolderName))
    .IgnoreError();

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

absl::Status DeviceManager::RunReplayGfxrScript(const GfxrReplaySettings &settings) const
{
    const AdbSession &adb = m_device->Adb();
    Defer             cleanup([&]() {
        LOGD("RunReplayGfxrScript(): CLEANUP\n");
        if (settings.run_type == GfxrReplayOptions::kPm4Dump)
        {
            adb.Run(absl::StrFormat("shell setprop %s 0", kEnableReplayPm4DumpPropertyName))
            .IgnoreError();
            adb
            .Run(absl::StrFormat("shell setprop %s \\\"\\\"", kReplayPm4DumpFileNamePropertyName))
            .IgnoreError();
        }
        else if (settings.run_type == GfxrReplayOptions::kRenderDoc)
        {
            UnsetSystemProperty(adb, kReplayCreateRenderDocCapture).IgnoreError();
            DisableVulkanLayer(adb).IgnoreError();
        }

        if (settings.use_validation_layer)
        {
            DisableVulkanLayer(adb).IgnoreError();
            UninstallVulkanLayer(adb,
                                 /*app=*/kGfxrReplayAppName,
                                 /*layer_filename=*/kVkValidationLayerLibName)
            .IgnoreError();
        }
    });
    LOGD("RunReplayGfxrScript(): SETUP\n");
    std::filesystem::path parse_remote_capture = settings.remote_capture_path;

    // These are only used if kPm4Dump
    std::string dump_pm4_file_name = parse_remote_capture.stem().string() + ".rd";
    std::string remote_pm4_path = absl::StrFormat("%s/%s",
                                                  kDeviceCapturePath,
                                                  dump_pm4_file_name.c_str());
    std::string remote_pm4_inprogress_path = absl::StrFormat("%s.inprogress",
                                                             remote_pm4_path.c_str());

    if (settings.run_type == GfxrReplayOptions::kPm4Dump)
    {
        LOGD("RunReplayGfxrScript(): PM4 capture file name is %s\n", dump_pm4_file_name.c_str());
        std::string cmd = absl::StrFormat("shell setprop %s 1", kEnableReplayPm4DumpPropertyName);
        RETURN_IF_ERROR(m_device->Adb().Run(cmd));
        cmd = absl::StrFormat("shell setprop %s \"%s\"",
                              kReplayPm4DumpFileNamePropertyName,
                              dump_pm4_file_name);
        RETURN_IF_ERROR(m_device->Adb().Run(cmd));
    }
    else if (settings.run_type == GfxrReplayOptions::kRenderDoc)
    {
        if (absl::Status status = IsAppInstalled(adb, kRenderDocAppName); !status.ok())
        {
            return absl::FailedPreconditionError(
            absl::StrFormat("Can't perform a RenderDoc capture since the RenderDoc app is not "
                            "installed on the target device. Either install the RenderDoc app "
                            "using the UI or by manually running adb install --force-queryable "
                            "%s.apk. The APK is provided in every RenderDoc release.",
                            kRenderDocAppName));
        }
        RETURN_IF_ERROR(SetSystemProperty(adb, kReplayCreateRenderDocCapture, "1"));
        RETURN_IF_ERROR(EnableVulkanLayer(adb,
                                          /*app=*/kGfxrReplayAppName,
                                          /*layer=*/kRenderDocCaptureLayerName,
                                          /*layer_app=*/kRenderDocAppName));
    }

    if (settings.use_validation_layer)
    {
        RETURN_IF_ERROR(InstallVulkanLayer(adb,
                                           /*app=*/kGfxrReplayAppName,
                                           /*layer_filename=*/kVkValidationLayerLibName));
        RETURN_IF_ERROR(EnableVulkanLayer(adb,
                                          /*app=*/kGfxrReplayAppName,
                                          /*layer=*/kVkValidationLayerName));
    }

    LOGD("RunReplayGfxrScript(): RUN\n");
    std::string local_recon_py_path = ResolveAndroidLibPath(kGfxrReconPyPath, "").generic_string();
    std::string cmd = absl::StrFormat("python %s replay %s %s",
                                      local_recon_py_path,
                                      settings.remote_capture_path,
                                      settings.replay_flags_str);
    if (absl::StatusOr<std::string> res = RunCommand(cmd); !res.ok())
    {
        return res.status();
    }

    LOGD("RunReplayGfxrScript(): RETRIEVE ARTIFACTS\n");
    // Wait for application to exit
    do
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    } while (m_device->IsProcessRunning(kGfxrReplayAppName));

    if (settings.run_type == GfxrReplayOptions::kPm4Dump)
    {
        // Wait for PM4 trace file to be written to.
        do
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        } while (m_device->FileExists(remote_pm4_inprogress_path));

        if (absl::Status s = m_device->RetrieveFile(remote_pm4_path, settings.local_download_dir);
            !s.ok())
        {
            return absl::InternalError(
            absl::StrFormat("Failed to download the trace file (%s), error:%s\n",
                            remote_pm4_path,
                            s.message()));
        }
        LOGI("Trace file %s downloaded to %s\n",
             remote_pm4_path.c_str(),
             settings.local_download_dir.c_str());
    }
    else if (settings.run_type == GfxrReplayOptions::kGpuTiming)
    {
        // TODO: Refactor for remote component file paths
        std::string
        remote_gpu_time_path = absl::StrFormat("%s/%s",
                                               parse_remote_capture.parent_path().string().c_str(),
                                               kGpuTimingFile);

        std::string gpu_time_csv_local_name = "";
        {
            absl::StatusOr<Dive::ComponentFilePaths>
            ret = GetComponentFilesHostPaths(settings.local_download_dir,
                                             parse_remote_capture.stem().string());
            if (!ret.ok())
            {
                return ret.status();
            }
            gpu_time_csv_local_name = ret->gpu_timing_csv.filename().string();
        }
        if (absl::Status s = m_device->RetrieveFile(remote_gpu_time_path,
                                                    settings.local_download_dir,
                                                    /*delete_after_retrieve=*/true,
                                                    gpu_time_csv_local_name);
            !s.ok())
        {
            return absl::InternalError(
            absl::StrFormat("Failed to download the gpu time csv file: %s\n",
                            remote_gpu_time_path.c_str()));
        }
        LOGI("Gpu time file %s downloaded to %s\n",
             remote_gpu_time_path.c_str(),
             settings.local_download_dir.c_str());
    }
    else if (settings.run_type == GfxrReplayOptions::kRenderDoc)
    {
        std::string remote_renderdoc_capture = GetRenderDocCaptureFilePath(
                                               settings.remote_capture_path)
                                               .generic_string();
        if (absl::Status status = m_device->RetrieveFile(remote_renderdoc_capture,
                                                         settings.local_download_dir,
                                                         /*delete_after_retrieve=*/true);
            !status.ok())
        {
            return absl::InternalError(
            absl::StrFormat("Failed to download the RenderDoc time rdc file (%s) error: %s",
                            remote_renderdoc_capture,
                            status.message()));
        }
    }

    return absl::OkStatus();
}

absl::Status DeviceManager::RunReplayProfilingBinary(const GfxrReplaySettings &settings) const
{
    LOGD("RunReplayProfilingBinary(): SETUP\n");
    LOGD("RunReplayProfilingBinary(): Deploy libraries and binaries\n");
    std::string copy_cmd = absl::StrFormat(R"(push "%s" "%s")",
                                           ResolveAndroidLibPath(kProfilingPluginFolderName, ""),
                                           kTargetPath);
    RETURN_IF_ERROR(m_device->Adb().Run(copy_cmd));
    std::string remote_profiling_dir = absl::StrFormat("%s/%s",
                                                       kTargetPath,
                                                       kProfilingPluginFolderName);
    Defer       cleanup([&]() {
        LOGD("RunReplayProfilingBinary(): CLEANUP\n");
        std::string clean_cmd = absl::StrFormat("shell rm -rf -- %s", remote_profiling_dir);
        m_device->Adb().Run(clean_cmd).IgnoreError();
    });

    std::string binary_path_on_device = absl::StrFormat("%s/%s",
                                                        remote_profiling_dir,
                                                        kProfilingPluginName);
    RETURN_IF_ERROR(m_device->Adb().Run(absl::StrCat("shell chmod +x ", binary_path_on_device)));

    LOGD("RunReplayProfilingBinary(): RUN\n");
    std::string metrics_str = absl::StrJoin(settings.metrics, " ");
    std::string gfxr_replay_flag = settings.replay_flags_str.empty() ?
                                   "" :
                                   absl::StrFormat("--gfxr_replay_flags \\\"%s\\\"",
                                                   settings.replay_flags_str);
    std::string cmd = absl::StrFormat("shell %s %s %s %s",
                                      binary_path_on_device,
                                      settings.remote_capture_path,
                                      gfxr_replay_flag,
                                      metrics_str);
    // TODO(b/449174476): Remove this redundant statement when the command is logged before it hangs
    LOGD("Profiling binary cmd: %s\n", cmd.c_str());
    RETURN_IF_ERROR(m_device->Adb().Run(cmd));

    LOGD("RunReplayProfilingBinary(): RETRIEVE ARTIFACTS\n");
    std::filesystem::path parse_remote_path = settings.remote_capture_path;
    std::string           perf_counter_csv_local_name = "";
    {
        absl::StatusOr<Dive::ComponentFilePaths>
        ret = GetComponentFilesHostPaths(settings.local_download_dir,
                                         parse_remote_path.stem().string());
        if (!ret.ok())
        {
            return ret.status();
        }
        perf_counter_csv_local_name = ret->perf_counter_csv.filename().string();
    }
    // TODO: Refactor for remote component file paths
    std::string csv_remote_file_path = parse_remote_path.replace_extension(".csv").string();

    if (absl::Status s = m_device->RetrieveFile(csv_remote_file_path,
                                                settings.local_download_dir,
                                                /*delete_after_retrieve=*/true,
                                                perf_counter_csv_local_name);
        !s.ok())
    {
        return absl::InternalError(
        absl::StrFormat("Failed to download the .csv file (%s), error:%s\n",
                        csv_remote_file_path,
                        s.message()));
    }
    LOGI("RunReplayProfilingBinary(): .csv file %s downloaded to %s\n",
         csv_remote_file_path.c_str(),
         settings.local_download_dir.c_str());

    return absl::OkStatus();
}

absl::Status DeviceManager::RunReplayApk(const GfxrReplaySettings &settings) const
{
    LOGD("RunReplayApk(): Check settings before run\n");
    absl::StatusOr<Dive::GfxrReplaySettings>
    validated_settings = ValidateGfxrReplaySettings(settings, m_device->IsAdrenoGpu());
    if (!validated_settings.ok())
    {
        return validated_settings.status();
    }

    LOGD("RunReplayApk(): Attempt to pin GPU clock frequency\n");
    bool trouble_pinning_clock = false;
    auto ret = m_device->Adb().Run("shell setprop compositor.high_priority 0");
    if (!ret.ok())
    {
        LOGW("WARNING: Could not disable the compositor preemption: %s\n",
             std::string(ret.message()).c_str());
        trouble_pinning_clock = true;
    }

    if (!trouble_pinning_clock)
    {
        ret = m_device->PinGpuClock(kPinGpuClockMHz);
        if (!ret.ok())
        {
            LOGW("WARNING: Could not pin GPU clock: %s\n", std::string(ret.message()).c_str());
            trouble_pinning_clock = true;
        }
    }

    LOGD("RunReplayApk(): Starting replay\n");
    absl::Status ret_run;
    if (validated_settings->run_type == GfxrReplayOptions::kPerfCounters)
    {
        ret_run = RunReplayProfilingBinary(*validated_settings);
    }
    else
    {
        ret_run = RunReplayGfxrScript(*validated_settings);
    }
    if (!ret_run.ok())
    {
        return ret_run;
    }

    LOGD("RunReplayApk(): Attempt to unpin GPU clock frequency\n");
    if (!trouble_pinning_clock)
    {
        auto ret = m_device->IsGpuClockPinned(kPinGpuClockMHz);
        if (!ret.ok())
        {
            LOGW("WARNING: GPU clock was not pinned: %s\n", std::string(ret.message()).c_str());
        }

        ret = m_device->UnpinGpuClock();
        if (!ret.ok())
        {
            LOGW("WARNING: Could not unpin GPU clock: %s\n", std::string(ret.message()).c_str());
        }
    }

    ret = m_device->Adb().Run("shell setprop compositor.high_priority 1");
    if (!ret.ok())
    {
        LOGW("WARNING: Could not re-enable the compositor preemption: %s\n",
             std::string(ret.message()).c_str());
    }

    LOGD("RunReplayApk(): Completed successfully\n");
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

absl::Status AndroidDevice::RetrieveFile(const std::string &remote_file_path,
                                         const std::string &local_save_dir,
                                         bool               delete_after_retrieve,
                                         const std::string &new_file_name)
{
    if (!std::filesystem::is_directory(local_save_dir))
    {
        return absl::FailedPreconditionError("Invalid local_save_dir: " + local_save_dir);
    }

    std::filesystem::path local_save_path = local_save_dir;

    if (new_file_name.size() > 0)
    {
        auto original_ext = std::filesystem::path(remote_file_path).extension();
        auto new_ext = std::filesystem::path(new_file_name).extension();
        if (original_ext.string() != new_ext.string())
        {
            std::string error_msg = absl::StrFormat("Invalid new_file_name extension (%s), needs "
                                                    "to be consistent with original extension (%s)",
                                                    remote_file_path,
                                                    new_file_name);
            return absl::FailedPreconditionError(error_msg);
        }
        local_save_path /= new_file_name;
    }

    // "adb pull" can handle a destination directory or file path
    RETURN_IF_ERROR(
    Adb().Run(absl::StrFormat(R"(pull "%s" "%s")", remote_file_path, local_save_path)));

    if (!delete_after_retrieve)
    {
        return absl::OkStatus();
    }

    return Adb().Run(absl::StrFormat("shell rm -rf %s", remote_file_path));
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

absl::Status AndroidDevice::PinGpuClock(uint32_t freq_mhz) const
{
    std::string cmd = "shell surfaceflinger --enable-spf-gpu-lock";
    RETURN_IF_ERROR(m_adb.Run(cmd));

    cmd = absl::StrFormat("shell \"echo %d> %s\"", freq_mhz, kDeviceGpuMinClockPath);
    RETURN_IF_ERROR(m_adb.Run(cmd));

    cmd = absl::StrFormat("shell \"echo %d> %s\"", freq_mhz, kDeviceGpuMaxClockPath);
    RETURN_IF_ERROR(m_adb.Run(cmd));

    return absl::OkStatus();
}

absl::Status AndroidDevice::UnpinGpuClock() const
{
    std::string cmd = "shell surfaceflinger --disable-spf-gpu-lock";
    RETURN_IF_ERROR(m_adb.Run(cmd));

    return absl::OkStatus();
}

absl::StatusOr<uint32_t> AndroidDevice::GetGpuFrequency() const
{
    std::string cmd = absl::StrFormat("shell cat %s", kDeviceCurFreqPath);
    auto        res = m_adb.RunAndGetResult(cmd);
    if (!res.ok())
    {
        return res.status();
    }

    // Note: omitting some parsing checks here because this system file is expected to only
    // contain digits
    uint32_t freq = stoi(*res);

    return freq;
}

absl::Status AndroidDevice::IsGpuClockPinned(uint32_t expected_freq_mhz) const
{
    auto res = GetGpuFrequency();
    if (!res.ok())
    {
        std::string err_str = absl::StrFormat("Could not check GPU clock frequency: %s\n",
                                              res.status().message());
        return absl::InternalError(err_str);
    }

    uint32_t expected_freq_hz = expected_freq_mhz * 1000000;
    if (*res != expected_freq_hz)
    {
        std::string err_str = absl::StrFormat("Expecting GPU clock frequency %d, got %d\n",
                                              expected_freq_hz,
                                              *res);
        return absl::InternalError(err_str);
    }

    return absl::OkStatus();
}

absl::Status AndroidDevice::TriggerScreenCapture(
const std::filesystem::path &on_device_screenshot_dir)
{
    // If the path segment has an extension, it is invalid for a directory name.
    if (on_device_screenshot_dir.has_extension())
    {
        return absl::InvalidArgumentError(
        absl::
        StrFormat("Invalid screenshot directory '%s'. Path appears to contain a file extension "
                  "and must be a directory.",
                  on_device_screenshot_dir.string()));
    }

    std::filesystem::path full_capture_path = std::filesystem::path(Dive::kDeviceCapturePath) /
                                              on_device_screenshot_dir /
                                              Dive::kCaptureScreenshotFile;

    std::string on_device_capture_screen_shot = full_capture_path.generic_string();

    absl::Status ret = m_adb.Run(
    absl::StrFormat("shell screencap -p %s", on_device_capture_screen_shot));
    return ret;
}

}  // namespace Dive
