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

#include "common/dive_cmake_generated.h"

#define DIVE_VERSION_SHA1 CMAKE_GENERATED_DIVE_VERSION_SHA1

#define DIVE_VERSION CMAKE_GENERATED_DIVE_VERSION
#define DIVE_VERSION_MAJOR CMAKE_GENERATED_DIVE_VERSION_MAJOR
#define DIVE_VERSION_MINOR CMAKE_GENERATED_DIVE_VERSION_MINOR
#define DIVE_VERSION_REVISION CMAKE_GENERATED_DIVE_VERSION_PATCH

#define DIVE_PRODUCT_NAME "Dive GPU Profiler"
#define DIVE_PRODUCT_DESCRIPTION                                                                   \
    "Dive is a powerful GPU profiler designed to help developers inspect low-level graphics data " \
    "to understand and optimize their applications. It currently supports AndroidXR devices that " \
    "use Qualcomm Adreno 7xx series GPUs."
#define DIVE_COPYRIGHT_DESCRIPTION "Copyright 2025 Google LLC. All rights reserved."

// (win|linux|mac)
#define DIVE_HOST_PLATFORM_STRING CMAKE_GENERATED_HOST_PLATFORM
// (Debug|Release)
#define DIVE_BUILD_TYPE CMAKE_GENERATED_BUILD_TYPE
// (dev|release|internal|canary)
#define DIVE_RELEASE_TYPE CMAKE_GENERATED_RELEASE_TYPE

#define DIVE_INSTALL_DIR_PATH CMAKE_GENERATED_INSTALL_DIR_PATH
#define DIVE_DEVICE_LIBRARIES_VERSION_FILENAME CMAKE_GENERATED_DEVICE_LIBRARIES_VERSION_FILENAME
