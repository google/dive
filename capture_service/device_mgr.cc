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

#include <iostream>
#include <memory>
#include <sstream>

#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "command_utils.h"
#include "log.h"

namespace Dive
{

namespace
{
const char kLayerLibName[] = "libVkLayer_dive.so";
const char kWrapLibName[] = "libwrap.so";
const char kTargetPath[] = "/data/local/tmp";
const char kVkLayerName[] = "VK_LAYER_Dive";
const int  kPort = 19999;
}  // namespace

AndroidDevice::AndroidDevice(const std::string &serial) :
    m_serial(serial),
    m_adb(serial)
{
    Adb().Run("root");
    Adb().Run("wait-for-device");

    m_original_state.m_enforce = Adb().Run("shell getenforce");
    m_dev_info.m_model = Adb().Run("shell getprop ro.product.model");
    m_dev_info.m_manufacturer = Adb().Run("shell getprop ro.product.manufacturer");

    LOGD("enforce: %s\n", m_original_state.m_enforce.c_str());
    LOGD("select: %s\n", GetDeviceDisplayName().c_str());

    SetupDevice();
}

AndroidDevice::~AndroidDevice()
{
    if (!m_serial.empty())
    {
        LOGD("Cleanup device %s\n", m_serial.c_str());
        if (!m_selected_package.empty())
        {
            StopApp(m_selected_package);
            CleanupAPP(m_selected_package);
        }
        CleanupDevice();
        LOGD("Cleanup device %s done\n", m_serial.c_str());
    }
}

std::string AndroidDevice::GetDeviceDisplayName() const
{
    return absl::StrCat(m_dev_info.m_manufacturer, " ", m_dev_info.m_model, " (", m_serial, ")");
}

std::vector<std::string> AndroidDevice::ListPackage() const
{
    std::vector<std::string> package_list;

    std::string              output = Adb().Run("shell pm list packages");
    std::vector<std::string> lines = absl::StrSplit(output, '\n');
    for (const auto &line : lines)
    {
        std::vector<std::string> fields = absl::StrSplit(line, ':');
        if (fields.size() == 2 && fields[0] == "package")
        {
            package_list.push_back(fields[1]);
        }
    }
    return package_list;
}

int GetIdention(const std::string &line)
{
    int idention = 0;
    for (int i = 0; i < line.size(); i++)
    {
        if (line[i] == ' ')
        {
            i++;
        }
        else
        {
            idention = i;
            break;
        }
    }
    return idention;
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
    int                      idention = 0;
    for (const auto &line : lines)
    {
        if (absl::StrContains(line, "Non-Data Actions"))
        {
            non_data_action_found = true;
            idention = GetIdention(line);
            continue;
        }

        if (!non_data_action_found)
            continue;

        int cur_idention = GetIdention(line);
        if (cur_idention <= idention)
        {
            non_data_action_found = false;
            break;
        }

        if (absl::StrContains(line, target_str))
        {
            std::string trimed_line = line;
            trimed_line = absl::StripAsciiWhitespace(trimed_line);
            std::vector<std::string> fileds = absl::StrSplit(trimed_line, " ");
            if (fileds.size() <= 2)
            {
                break;
            }
            for (const auto &f : fileds)
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

std::string AndroidDevice::GetMainActivity(const std::string &package) const
{

    std::string output = Adb().Run("shell dumpsys package " + package);
    return ParsePackageForActivity(output, package);
}

void AndroidDevice::SetupDevice()
{
    Adb().Run("shell setenforce 0");
    Adb().Run("shell getenforce");

    Adb().Run(absl::StrFormat("push %s %s", kWrapLibName, kTargetPath));
    Adb().Run(absl::StrFormat("push %s %s", kLayerLibName, kTargetPath));
    Adb().Run(absl::StrFormat("forward tcp:%d tcp:%d", kPort, kPort));
}

void AndroidDevice::CleanupDevice()
{
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

    Adb().Run(absl::StrFormat("shell rm %s/%s", kTargetPath, kWrapLibName));
    Adb().Run(absl::StrFormat("shell rm %s/%s", kTargetPath, kLayerLibName));
    Adb().Run(absl::StrFormat("forward --remove tcp:%d", kPort));
}

void AndroidDevice::SetupApp(const std::string &package)
{
    Adb().Run("root");
    Adb().Run("wait-for-device");
    StopApp(package);

    Adb().Run(absl::StrFormat("shell run-as %s cp %s/%s .", package, kTargetPath, kLayerLibName));
    Adb().Run("shell settings put global enable_gpu_debug_layers 1");
    Adb().Run(absl::StrFormat("shell settings put global gpu_debug_app %s", package));
    Adb().Run(absl::StrFormat("shell settings put global gpu_debug_layer_app %s", package));
    Adb().Run(absl::StrFormat("shell settings put global gpu_debug_layers %s", kVkLayerName));
    Adb().Run(
    absl::StrFormat("shell setprop wrap.%s  LD_PRELOAD=%s/%s", package, kTargetPath, kWrapLibName));
    Adb().Run(absl::StrFormat("shell getprop wrap.%s", package));
}

void AndroidDevice::CleanupAPP(const std::string &package)
{
    Adb().Run("root");
    Adb().Run("wait-for-device");

    Adb().Run(absl::StrFormat("shell run-as %s rm %s", package, kLayerLibName));

    Adb().Run("shell settings delete global enable_gpu_debug_layers");
    Adb().Run("shell settings delete global gpu_debug_app");
    Adb().Run("shell settings delete global gpu_debug_layers");
    Adb().Run("shell settings delete global gpu_debug_layer_app");
    Adb().Run("shell settings delete global gpu_debug_layers_gles");
    Adb().Run(absl::StrFormat("shell setprop wrap.%s \\\"\\\"", package));
    m_selected_package = "";
}

void AndroidDevice::StartApp(const std::string &package, const std::string &activity)
{
    Adb().Run("shell input keyevent KEYCODE_WAKEUP");
    Adb().Run(absl::StrFormat("shell am start %s/%s", package, activity));
    m_selected_package = package;
}

void AndroidDevice::StopApp(const std::string &package)
{
    Adb().Run(absl::StrFormat("shell am force-stop %s", package));
}

std::vector<std::string> DeviceManager::ListDevice() const
{
    std::vector<std::string> dev_list;
    std::string              output = RunCommand("adb devices");
    std::vector<std::string> lines = absl::StrSplit(output, '\n');

    for (const auto &line : lines)
    {
        std::vector<std::string> fields = absl::StrSplit(line, '\t');
        if (fields.size() == 2 && fields[1] == "device")
            dev_list.push_back(fields[0]);
    }

    return dev_list;
}

void AndroidDevice::RetrieveTraceFile(const std::string &trace_file_path,
                                        const std::string      &save_path)
{
    Adb().Run(absl::StrFormat("pull %s %s", trace_file_path, save_path));
}

}  // namespace Dive
