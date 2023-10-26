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

#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "constants.h"
#include "device_mgr.h"
#include "log.h"

namespace Dive
{

int GetIndention(const std::string &line)
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
            indention = GetIndention(line);
            continue;
        }

        if (!non_data_action_found)
            continue;

        int cur_indention = GetIndention(line);
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
                                       ApplicationType type) :
    m_dev(dev),
    m_package(std::move(package)),
    m_type(type),
    m_started(false)
{
    m_main_activity = std::move(GetMainActivity());
}

std::string AndroidApplication::GetMainActivity()
{
    std::string output = m_dev.Adb().Run("shell dumpsys package " + m_package, true).Out();
    return ParsePackageForActivity(output, m_package);
}

void AndroidApplication::Start()
{
    m_dev.Adb().Run("shell input keyevent KEYCODE_WAKEUP");
    m_dev.Adb().Run(absl::StrFormat("shell am start -S -W %s/%s", m_package, m_main_activity));
    m_started = true;
}

void AndroidApplication::Stop()
{
    m_dev.Adb().Run(absl::StrFormat("shell am force-stop %s", m_package));
}

VulkanApplication::~VulkanApplication()
{
    if (m_started)
    {
        Stop();
    }
    Cleanup();
}

void VulkanApplication::Setup()
{
    LOGD("Setup Vulkan application");
    m_dev.Adb().Run("root");
    m_dev.Adb().Run("wait-for-device");
    Stop();

    m_dev.Adb().Run(
    absl::StrFormat("shell run-as %s cp %s/%s .", m_package, kTargetPath, kVkLayerLibName));
    m_dev.Adb().Run("shell settings put global enable_gpu_debug_layers 1");
    m_dev.Adb().Run(absl::StrFormat("shell settings put global gpu_debug_app %s", m_package));
    m_dev.Adb().Run(absl::StrFormat("shell settings put global gpu_debug_layer_app %s", m_package));
    m_dev.Adb().Run(absl::StrFormat("shell settings put global gpu_debug_layers %s", kVkLayerName));
    m_dev.Adb().Run(absl::StrFormat("shell setprop wrap.%s  LD_PRELOAD=%s/%s",
                                    m_package,
                                    kTargetPath,
                                    kWrapLibName));
    m_dev.Adb().Run(absl::StrFormat("shell getprop wrap.%s", m_package));
    LOGD("Setup Vulkan application done");
}

void VulkanApplication::Cleanup()
{
    LOGD("Cleanup Vulkan application");
    m_dev.Adb().Run("root");
    m_dev.Adb().Run("wait-for-device");

    m_dev.Adb().Run(absl::StrFormat("shell run-as %s rm %s", m_package, kVkLayerLibName), true);
    m_dev.Adb().Run("shell settings delete global enable_gpu_debug_layers");
    m_dev.Adb().Run("shell settings delete global gpu_debug_app");
    m_dev.Adb().Run("shell settings delete global gpu_debug_layers");
    m_dev.Adb().Run("shell settings delete global gpu_debug_layer_app");
    m_dev.Adb().Run("shell settings delete global gpu_debug_layers_gles");
    m_dev.Adb().Run(absl::StrFormat("shell setprop wrap.%s \\\"\\\"", m_package));
    LOGD("Cleanup Vulkan application done");
}

void OpenXRApplication::Setup()
{
    LOGD("OpenXRApplication Setup");
    m_dev.Adb().Run("root");
    m_dev.Adb().Run("wait-for-device");
    m_dev.Adb().Run("remount");
    m_dev.Adb().Run(absl::StrFormat("shell mkdir -p %s", kManifestFilePath));
    m_dev.Adb().Run(absl::StrFormat("push %s %s", kManifestFileName, kManifestFilePath));
    m_dev.Adb().Run(absl::StrFormat("shell setprop wrap.%s  LD_PRELOAD=%s/%s",
                                    m_package,
                                    kTargetPath,
                                    kWrapLibName));
    m_dev.Adb().Run(absl::StrFormat("shell getprop wrap.%s", m_package));
    LOGD("OpenXRApplication Setup done.");
}

void OpenXRApplication::Cleanup()
{
    LOGD("OpenXRApplication Cleanup");
    m_dev.Adb().Run("root");
    m_dev.Adb().Run("wait-for-device");
    m_dev.Adb().Run("remount");
    m_dev.Adb().Run(absl::StrFormat("shell rm -r %s", kManifestFilePath));
    m_dev.Adb().Run(absl::StrFormat("shell setprop wrap.%s \\\"\\\"", m_package));
    LOGD("OpenXRApplication Cleanup done.");
}

OpenXRApplication::~OpenXRApplication()
{
    if (m_started)
    {
        Stop();
    }
    Cleanup();
}

}  // namespace Dive
