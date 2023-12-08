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
inline constexpr char kVkLayerName[] = "VK_LAYER_Dive";
inline constexpr char kXrLayerName[] = "XR_APILAYER_dive";
inline constexpr char kTargetPath[] = "/data/local/tmp";
inline constexpr char kManifestFileName[] = "XrApiLayer_dive.json";
inline constexpr char kManifestFilePath[] = "/system/etc/openxr/1/api_layers/implicit.d/";
inline constexpr char kVulkanGlobalPath[] = "/data/local/debug/vulkan";
inline constexpr int  kPort = 19999;
inline constexpr int  kDownLoadFileChunkSize = 4096;
}  // namespace Dive