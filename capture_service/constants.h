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

#pragma once

namespace Dive
{
inline constexpr char kWrapLibName[] = "libwrap.so";
inline constexpr char kVkLayerLibName[] = "libVkLayer_dive.so";
inline constexpr char kXrLayerLibName[] = "libXrApiLayer_dive.so";
inline constexpr char kVkGfxrLayerLibName[] = "libVkLayer_gfxreconstruct.so";
inline constexpr char kGfxrReplayApkName[] = "gfxr-replay.apk";
inline constexpr char kGfxrReplayAppName[] = "com.lunarg.gfxreconstruct.replay";
inline constexpr char kVkGfxrLayerName[] = "VK_LAYER_LUNARG_gfxreconstruct";
inline constexpr char kVkLayerName[] = "VK_LAYER_Dive";
inline constexpr char kXrLayerName[] = "XR_APILAYER_dive";
inline constexpr char kTargetPath[] = "/data/local/tmp";
inline constexpr char kGfxrTargetPath[] = "/data/data/";
inline constexpr char kGfxrReconPyPath[] = "third_party/gfxreconstruct/android/scripts/gfxrecon.py";
inline constexpr char kDeviceCapturePath[] = "/sdcard/Download";
inline constexpr char kManifestFileName[] = "XrApiLayer_dive.json";
inline constexpr char kManifestFilePath[] = "/system/etc/openxr/1/api_layers/implicit.d/";
inline constexpr char kVulkanGlobalPath[] = "/data/local/debug/vulkan";
inline constexpr int  kDownLoadFileChunkSize = 4096;
inline constexpr char kUnixAbstractPath[] = "dive_abstract";
inline constexpr char kEnableReplayPm4DumpPropertyName[] = "debug.dive.replay.capture_pm4";
inline constexpr char
kReplayPm4DumpFileNamePropertyName[] = "debug.dive.replay.capture_pm4_file_name";
inline constexpr char kDefaultCaptureFolderName[] = "gfxr_capture";
inline constexpr char kDefaultReplayFolderName[] = "gfxr_replay_downloads";
inline constexpr char kProfilingPluginFolderName[] = "dive_profiling_plugin";
inline constexpr char kProfilingPluginName[] = "dive_drawcall_metrics";

inline constexpr int
kFirstPort = 49391;  // A port number within the dynamic port range (49152 to 65535)
inline constexpr int
kPortRange = 7;  // A small range of ports should be enough to find an available one

}  // namespace Dive