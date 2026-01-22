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
    // clang-format off
/*

On the host machine, resources are aggregated and installed into the "base resources dir" for convenience.

                            Most platforms (dev & release)                                  Apple bundle (DIVE_ROOT/build/pkg/dive.app)
---------------------------------------------------------------------------------------------------------------------------------------
Base resources dir:         DIVE_ROOT/build/pkg/                                            dive.app/Contents/
Host resources dir:         DIVE_ROOT/build/pkg/CMAKE_GENERATED_INSTALL_DEST_HOST/          dive.app/Contents/MacOS/
Device resources dir:       DIVE_ROOT/build/pkg/CMAKE_GENERATED_INSTALL_DEST_DEVICE/        dive.app/Contents/CMAKE_GENERATED_DIVE_MACOS_BUNDLE_RESOURCES/
Plugin parent dir:          DIVE_ROOT/build/pkg/CMAKE_GENERATED_PLUGINS_PARENT_DIR/         dive.app/Contents/CMAKE_GENERATED_DIVE_MACOS_BUNDLE_RESOURCES/CMAKE_GENERATED_PLUGINS_PARENT_DIR/
Profiling resources dir:    DIVE_ROOT/build/pkg/CMAKE_GENERATED_PROFILING_PLUGIN_DIR/       dive.app/Contents/CMAKE_GENERATED_DIVE_MACOS_BUNDLE_RESOURCES/CMAKE_GENERATED_PLUGINS_PARENT_DIR/CMAKE_GENERATED_PROFILING_PLUGIN_DIR/

*/
    // clang-format on

    // Device resources:
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

    // Profiling plugin resources:
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
