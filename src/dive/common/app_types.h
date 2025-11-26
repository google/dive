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

#include <algorithm>
#include <array>
#include <string_view>

namespace Dive
{

enum class AppType
{
    kVulkan_OpenXR,         // OpenXR Vulkan app
    kVulkan_Non_OpenXR,     // Vulkan app
    kVulkanCLI_Non_OpenXR,  // Vulkan command line app
    kGLES_OpenXR,           // OpenXR GLES app
    kGLES_Non_OpenXR,       // GLES app
};

struct AppTypeInfo
{
    AppType          type;
    std::string_view ui_name;
    std::string_view cli_name;
    bool             is_gfxr_capture_supported;
    std::string_view description;
};

inline constexpr std::array<AppTypeInfo, 5> kAppTypeInfos = { {
{ AppType::kVulkan_OpenXR,
  "Vulkan (OpenXR)",
  "vulkan_openxr",
  true,
  "For Vulkan OpenXR applications(apk)" },
{ AppType::kVulkan_Non_OpenXR,
  "Vulkan (Non-OpenXR)",
  "vulkan_non_openxr",
  true,
  "For Vulkan applications(apk)" },
{ AppType::kVulkanCLI_Non_OpenXR,
  "Vulkan CLI (Non-OpenXR)",
  "vulkan_cli_non_openxr",
  true,
  "For command line Vulkan application" },
{ AppType::kGLES_OpenXR,
  "GLES (OpenXR)",
  "gles_openxr",
  false,
  "For GLES OpenXR applications(apk)" },
{ AppType::kGLES_Non_OpenXR,
  "GLES (Non-OpenXR)",
  "gles_non_openxr",
  false,
  "for GLES applications(apk)" },
} };

}  // namespace Dive
