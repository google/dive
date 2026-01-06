/*
 Copyright 2025 Google LLC
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

#include <filesystem>

#include "absl/status/statusor.h"

namespace Dive
{

// Returns the full local path of relative_file_path, which represents a host resource file
absl::StatusOr<std::filesystem::path> ResolveHostResourcesLocalPath(
    std::filesystem::path relative_file_path);

// Returns the full local path of relative_file_path, which represents a device resource file
absl::StatusOr<std::filesystem::path> ResolveDeviceResourcesLocalPath(
    std::filesystem::path relative_file_path);

// Returns the full local path of relative_file_path, which represents a profiling resource file
absl::StatusOr<std::filesystem::path> ResolveProfilingResourcesLocalPath(
    std::filesystem::path relative_file_path);

}  // namespace Dive
