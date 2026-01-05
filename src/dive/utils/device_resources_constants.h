/*
Copyright 2025 Google Inc.

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

#include <string_view>

namespace Dive
{

struct DeviceResourcesConstants
{
    // -------------------------------------------------------------------------
    // On the host machine, device resources are aggregated into a folder for
    // convenience. The following paths are defined relative to that folder.
    //
    // Currently the device resources folder is DIVE_INSTALL_DIR_PATH (for host build)
    //
    // TODO(b/462767957): Remove build_defs DIVE_INSTALL_DIR_PATH and instead predict device
    // resources folder location based on the executable dir
    static constexpr std::string_view kVkLayerLibName = "libVkLayer_dive.so";
    static constexpr std::string_view kXrLayerLibName = "libXrApiLayer_dive.so";
    static constexpr std::string_view kManifestFileName = "XrApiLayer_dive.json";

    // third_party/freedreno
    static constexpr std::string_view kWrapLibName = "libwrap.so";
    // third_party/Vulkan-ValidationLayers
    static constexpr std::string_view kVkValidationLayerLibName =
        "libVkLayer_khronos_validation.so";
    // third_party/gfxreconstruct
    static constexpr std::string_view kVkGfxrLayerLibName = "libVkLayer_gfxreconstruct.so";
    static constexpr std::string_view kGfxrReplayApkName = "gfxr-replay.apk";
    static constexpr std::string_view kGfxrReconPyName = "gfxrecon.py";

    // Profiling folder, and paths relative to it
    static constexpr std::string_view kProfilingPluginFolderName = "dive_profiling_plugin";
    static constexpr std::string_view kProfilingPluginShaName = "SHA";
    static constexpr std::string_view kProfilingPluginMetricsFileName = "available_metrics.csv";

    // -------------------------------------------------------------------------
    // On the device, device resources are deployed into these locations
    static constexpr std::string_view kDeployFolderPath = "/data/local/tmp/dive";
    static constexpr std::string_view kDeployVulkanGlobalFolderPath = "/data/local/debug/vulkan";
    static constexpr std::string_view kDeployManifestFolderPath =
        "/system/etc/openxr/1/api_layers/implicit.d/";
};

}  // namespace Dive
