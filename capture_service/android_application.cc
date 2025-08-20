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

#include "android_application.h"

#include "absl/status/status.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "absl/strings/string_view.h"
#include "constants.h"
#include "device_mgr.h"
#include "common/log.h"
#include "common/macros.h"

namespace Dive
{

int GetIndentation(const std::string &line)
{
    int indention = 0;
    for (int i = 0; i < line.size(); i++)
    {
        if (line[i] == ' ')
        {
            i++;
        }
        else
        {
            indention = i;
            break;
        }
    }
    return indention;
}

/*
Parsing dumpsys package to get the main activity of the package.
An example output from the dumpsys command:
flame:/ # dumpsys package de.saschawillems.vulkanBloom
Activity Resolver Table:
  Non-Data Actions:
      android.intent.action.MAIN:
        1e368bb de.saschawillems.vulkanBloom/de.saschawillems.vulkanSample.VulkanActivity filter
35ec1d8 Action: "android.intent.action.MAIN" Category: "android.intent.category.LAUNCHER"
...
*/
std::string ParsePackageForActivity(const std::string &input, const std::string &package)
{
    bool        non_data_action_found = false;
    std::string activity;
    std::string target_str = package + "/";

    std::vector<std::string> lines = absl::StrSplit(input, '\n');
    int                      indention = 0;
    for (const auto &line : lines)
    {
        if (absl::StrContains(line, "Non-Data Actions"))
        {
            non_data_action_found = true;
            indention = GetIndentation(line);
            continue;
        }

        if (!non_data_action_found)
            continue;

        int cur_indention = GetIndentation(line);
        if (cur_indention <= indention)
        {
            non_data_action_found = false;
            break;
        }

        if (absl::StrContains(line, target_str))
        {
            std::string trimmed_line = line;
            trimmed_line = absl::StripAsciiWhitespace(trimmed_line);
            std::vector<std::string> fields = absl::StrSplit(trimmed_line, " ");
            if (fields.size() <= 2)
            {
                break;
            }
            for (const auto &f : fields)
            {
                if (absl::StrContains(f, package))
                {
                    std::vector<std::string> pa = absl::StrSplit(f, "/");
                    if (pa.size() == 2 && pa[0] == package)
                    {
                        activity = pa[1];
                    }
                }
            }
        }
    }

    return activity;
}

AndroidApplication::AndroidApplication(AndroidDevice  &dev,
                                       std::string     package,
                                       ApplicationType type,
                                       std::string     command_args) :
    m_dev(dev),
    m_package(std::move(package)),
    m_type(type),
    m_command_args(std::move(command_args)),
    m_started(false),
    m_is_debuggable(false)
{
}

absl::Status AndroidApplication::ParsePackage()
{
    std::string output;
    ASSIGN_OR_RETURN(output, m_dev.Adb().RunAndGetResult("shell dumpsys package " + m_package));

    m_main_activity = ParsePackageForActivity(output, m_package);
    m_is_debuggable = absl::StrContains(output, "DEBUGGABLE");
    return absl::OkStatus();
}

absl::Status AndroidApplication::Start()
{
    RETURN_IF_ERROR(m_dev.Adb().Run("shell input keyevent KEYCODE_WAKEUP"));
    RETURN_IF_ERROR(m_dev.Adb().Run(absl::StrFormat("shell am start -S -W %s -n %s/%s",
                                                    m_command_args,
                                                    m_package,
                                                    m_main_activity)));
    m_started = IsRunning();
    return absl::OkStatus();
}

absl::Status AndroidApplication::Stop()
{
    RETURN_IF_ERROR(m_dev.Adb().Run(absl::StrFormat("shell am force-stop %s", m_package)));
    m_started = false;
    return absl::OkStatus();
}

bool AndroidApplication::IsRunning() const
{
    return m_dev.IsProcessRunning(m_package);
}

VulkanApplication::~VulkanApplication()
{
    if (m_started)
    {
        Stop().IgnoreError();
    }
    Cleanup().IgnoreError();
}

absl::Status VulkanApplication::Setup()
{
    LOGD("Setup Vulkan application: %s\n", m_package.c_str());
    RETURN_IF_ERROR(GrantAllFilesAccess());
    Stop().IgnoreError();
    if (m_gfxr_enabled)
    {
        RETURN_IF_ERROR(GfxrSetup());
    }
    else
    {
        RETURN_IF_ERROR(m_dev.Adb().Run(
        absl::StrFormat("shell run-as %s cp %s/%s .", m_package, kTargetPath, kVkLayerLibName)));
        RETURN_IF_ERROR(m_dev.Adb().Run("shell settings put global enable_gpu_debug_layers 1"));
        RETURN_IF_ERROR(
        m_dev.Adb().Run(absl::StrFormat("shell settings put global gpu_debug_app %s", m_package)));
        RETURN_IF_ERROR(m_dev.Adb().Run(
        absl::StrFormat("shell settings put global gpu_debug_layer_app %s", m_package)));
        RETURN_IF_ERROR(m_dev.Adb().Run(
        absl::StrFormat("shell settings put global gpu_debug_layers %s", kVkLayerName)));
        RETURN_IF_ERROR(m_dev.Adb().Run(absl::StrFormat("shell setprop wrap.%s  LD_PRELOAD=%s/%s",
                                                        m_package,
                                                        kTargetPath,
                                                        kWrapLibName)));
        RETURN_IF_ERROR(m_dev.Adb().Run(absl::StrFormat("shell getprop wrap.%s", m_package)));
    }
    LOGD("Setup Vulkan application %s done\n", m_package.c_str());
    return absl::OkStatus();
}

absl::Status VulkanApplication::Cleanup()
{
    LOGD("Cleanup Vulkan application %s\n", m_package.c_str());
    if (m_gfxr_enabled)
    {
        RETURN_IF_ERROR(m_dev.Adb().Run("shell setprop debug.gfxrecon.capture_file \\\"\\\""));
        RETURN_IF_ERROR(
        m_dev.Adb().Run("shell setprop debug.gfxrecon.capture_trigger_frames \\\"\\\""));
        RETURN_IF_ERROR(
        m_dev.Adb().Run("shell setprop debug.gfxrecon.capture_use_asset_file false"));
        RETURN_IF_ERROR(
        m_dev.Adb().Run("shell setprop debug.gfxrecon.capture_android_trigger \\\"\\\""));
        RETURN_IF_ERROR(
        m_dev.Adb().Run(absl::StrFormat("shell run-as %s rm %s", m_package, kVkGfxrLayerLibName)));
    }
    else
    {
        RETURN_IF_ERROR(
        m_dev.Adb().Run(absl::StrFormat("shell run-as %s rm %s", m_package, kVkLayerLibName)));
        RETURN_IF_ERROR(
        m_dev.Adb().Run(absl::StrFormat("shell setprop wrap.%s \\\"\\\"", m_package)));
    }

    // Cleanup of Vulkan layers and corresponding settings is automatically handled in
    // AndroidDevice::CleanupDevice.

    LOGD("Cleanup Vulkan application %s done\n", m_package.c_str());
    return absl::OkStatus();
}

void AndroidApplication::SetGfxrEnabled(bool enable)
{
    m_gfxr_enabled = enable;
}

absl::Status AndroidApplication::CreateGfxrDirectory(const std::string directory)
{

    RETURN_IF_ERROR(m_dev.Adb().Run(absl::StrFormat("shell mkdir -p %s", directory)));

    return absl::OkStatus();
}

absl::Status AndroidApplication::GfxrSetup()
{
    RETURN_IF_ERROR(m_dev.Adb().Run(
    absl::StrFormat("push %s %s",
                    ResolveAndroidLibPath(kVkGfxrLayerLibName, m_device_architecture)
                    .generic_string(),
                    kTargetPath)));

    RETURN_IF_ERROR(m_dev.Adb().Run(
    absl::StrFormat("shell run-as %s cp %s/%s .", m_package, kTargetPath, kVkGfxrLayerLibName)));

    RETURN_IF_ERROR(
    m_dev.Adb().Run(absl::StrFormat("shell run-as %s ls %s", m_package, kVkGfxrLayerLibName)));

    RETURN_IF_ERROR(m_dev.Adb().Run("shell settings put global enable_gpu_debug_layers 1"));

    RETURN_IF_ERROR(
    m_dev.Adb().Run(absl::StrFormat("shell settings put global gpu_debug_app %s", m_package)));

    RETURN_IF_ERROR(m_dev.Adb().Run(
    absl::StrFormat("shell settings put global gpu_debug_layers %s", kVkGfxrLayerName)));

    RETURN_IF_ERROR(m_dev.Adb().Run(
    absl::StrFormat("shell settings put global gpu_debug_layer_app %s", m_package)));

    std::string gfxr_capture_directory = absl::StrCat(kDeviceCapturePath,
                                                      "/",
                                                      m_gfxr_capture_file_directory);

    std::string capture_file_location = absl::StrCat(gfxr_capture_directory,
                                                     "/",
                                                     m_package,
                                                     ".gfxr");
    RETURN_IF_ERROR(CreateGfxrDirectory(gfxr_capture_directory));

    RETURN_IF_ERROR(
    m_dev.Adb().Run("shell setprop debug.gfxrecon.capture_file " + capture_file_location));

    RETURN_IF_ERROR(m_dev.Adb().Run("shell setprop debug.gfxrecon.capture_trigger_frames 1"));

    RETURN_IF_ERROR(m_dev.Adb().Run("shell setprop debug.gfxrecon.capture_use_asset_file true"));

    // capture_android_trigger must be set in order for GFXR to listen for triggers.
    RETURN_IF_ERROR(m_dev.Adb().Run("shell setprop debug.gfxrecon.capture_android_trigger false"));

    LOGD("GFXR capture setup for %s done\n", m_package.c_str());
    return absl::OkStatus();
}

absl::Status AndroidApplication::HasInternetPermission()
{
    const std::string cmd = absl::
    StrFormat("shell dumpsys package %s | grep 'android.permission.INTERNET: granted=true'",
              m_package);
    auto res = m_dev.Adb().RunAndGetResult(cmd);
    if (!res.ok())
    {
        return res.status();
    }
    if ((*res).empty())
    {
        return absl::Status(absl::StatusCode::kUnknown,
                            "The android.permission.INTERNET permission wasn't granted. Add it to "
                            "your AndroidManifest.xml file");
    }
    return absl::OkStatus();
}

absl::Status AndroidApplication::GrantAllFilesAccess()
{
    return m_dev.Adb().Run(
    absl::StrFormat("shell appops set --uid %s MANAGE_EXTERNAL_STORAGE allow", m_package));
}

absl::Status OpenXRApplication::Setup()
{
    LOGD("OpenXRApplication %s Setup\n", m_package.c_str());
    RETURN_IF_ERROR(GrantAllFilesAccess());
    if (m_gfxr_enabled)
    {
        RETURN_IF_ERROR(GfxrSetup());
    }
    else
    {
        RETURN_IF_ERROR(m_dev.Adb().Run("remount"));
        RETURN_IF_ERROR(m_dev.Adb().Run(absl::StrFormat("shell mkdir -p %s", kManifestFilePath)));
        RETURN_IF_ERROR(m_dev.Adb().Run(
        absl::StrFormat("push %s %s",
                        ResolveAndroidLibPath(kManifestFileName, "").generic_string().c_str(),
                        kManifestFilePath)));
        RETURN_IF_ERROR(m_dev.Adb().Run(absl::StrFormat("shell setprop wrap.%s  LD_PRELOAD=%s/%s",
                                                        m_package,
                                                        kTargetPath,
                                                        kWrapLibName)));
        RETURN_IF_ERROR(m_dev.Adb().Run(absl::StrFormat("shell getprop wrap.%s", m_package)));
    }
    LOGD("OpenXRApplication %s Setup done.\n", m_package.c_str());
    return absl::OkStatus();
}

absl::Status OpenXRApplication::Cleanup()
{
    LOGD("OpenXRApplication %s cleanup.\n", m_package.c_str());
    if (m_gfxr_enabled)
    {
        RETURN_IF_ERROR(m_dev.Adb().Run("shell setprop debug.gfxrecon.capture_file \\\"\\\""));
        RETURN_IF_ERROR(
        m_dev.Adb().Run("shell setprop debug.gfxrecon.capture_trigger_frames \\\"\\\""));
        RETURN_IF_ERROR(
        m_dev.Adb().Run("shell setprop debug.gfxrecon.capture_use_asset_file false"));
        RETURN_IF_ERROR(
        m_dev.Adb().Run("shell setprop debug.gfxrecon.capture_android_trigger \\\"\\\""));
        // TODO(b/426541653): remove this after all branches in AndroidXR accept the prop of
        // `debug.openxr.enable_frame_delimiter`
        RETURN_IF_ERROR(m_dev.Adb().Run("shell setprop openxr.enable_frame_delimiter false"));
        RETURN_IF_ERROR(m_dev.Adb().Run("shell setprop debug.openxr.enable_frame_delimiter false"));
        RETURN_IF_ERROR(
        m_dev.Adb().Run(absl::StrFormat("shell run-as %s rm %s", m_package, kVkGfxrLayerLibName)));
    }
    else
    {
        RETURN_IF_ERROR(m_dev.Adb().Run("remount"));
        RETURN_IF_ERROR(m_dev.Adb().Run(absl::StrFormat("shell rm -r %s", kManifestFilePath)));
        RETURN_IF_ERROR(
        m_dev.Adb().Run(absl::StrFormat("shell setprop wrap.%s \\\"\\\"", m_package)));
    }
    LOGD("OpenXRApplication %s cleanup done.\n", m_package.c_str());
    return absl::OkStatus();
}

OpenXRApplication::~OpenXRApplication()
{
    if (m_started)
    {
        Stop().IgnoreError();
    }
    Cleanup().IgnoreError();
}

VulkanCliApplication::~VulkanCliApplication()
{
    if (m_started)
    {
        Stop().IgnoreError();
    }
    Cleanup().IgnoreError();
}

// For CLI Vulkan application, the layer library is being put at
// kVulkanGlobalPath(/data/local/debug/vulkan), which is different from the layer for apk
// application. For apk application, the layer is copied to the application's own storage instead of
// the global path.
absl::Status VulkanCliApplication::Setup()
{
    RETURN_IF_ERROR(GrantAllFilesAccess());
    RETURN_IF_ERROR(m_dev.Adb().Run(absl::StrFormat("shell mkdir -p %s", kVulkanGlobalPath)));
    RETURN_IF_ERROR(
    m_dev.Adb().Run(absl::StrFormat("push %s %s",
                                    ResolveAndroidLibPath(kVkLayerLibName, "").generic_string(),
                                    kVulkanGlobalPath)));
    RETURN_IF_ERROR(
    m_dev.Adb().Run(absl::StrFormat("shell setprop debug.vulkan.layers %s", kVkLayerName)));

    return absl::OkStatus();
}

absl::Status VulkanCliApplication::Cleanup()
{
    RETURN_IF_ERROR(m_dev.Adb().Run(absl::StrFormat("shell rm -fr %s", kVulkanGlobalPath)));
    RETURN_IF_ERROR(m_dev.Adb().Run(absl::StrFormat("shell setprop debug.vulkan.layers \"''\"")));

    return absl::OkStatus();
}

absl::Status VulkanCliApplication::Start()
{
    const std::string cmd = absl::StrFormat("shell LD_PRELOAD=%s/%s %s %s",
                                            kTargetPath,
                                            kWrapLibName,
                                            m_command,
                                            m_command_args);

    RETURN_IF_ERROR(m_dev.Adb().RunCommandBackground(cmd));
    ASSIGN_OR_RETURN(m_pid, m_dev.Adb().RunAndGetResult("shell pidof " + m_command));
    m_started = true;
    return absl::OkStatus();
}

absl::Status VulkanCliApplication::Stop()
{
    RETURN_IF_ERROR(m_dev.Adb().Run(absl::StrFormat("shell kill -9 %s", m_pid)));
    m_started = false;
    return absl::OkStatus();
}
bool VulkanCliApplication::IsRunning() const
{
    return m_dev.IsProcessRunning(m_command);
}

}  // namespace Dive
