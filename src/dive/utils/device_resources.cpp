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

#include "dive/utils/device_resources.h"

#include <array>
#include <filesystem>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/types/span.h"
#include "dive/build_defs/resource_defs.h"
#include "dive/common/status.h"
#include "dive/os/command_utils.h"

namespace
{

// search_paths are relative to the executable dir
absl::StatusOr<std::filesystem::path> ResolvePath(
    absl::Span<const std::filesystem::path> search_paths, std::filesystem::path relative_file_path)
{
    assert(!search_paths.empty());

    absl::StatusOr<std::filesystem::path> exec_dir = Dive::GetExecutableDirectory();
    if (!exec_dir.ok())
    {
        return exec_dir.status();
    }

    std::vector<std::string> searched_paths_strings;

    for (const auto& p : search_paths)
    {
        const auto potential_path = *exec_dir / p / relative_file_path;
        if (std::filesystem::exists(potential_path))
        {
            return std::filesystem::canonical(potential_path);
        }
        searched_paths_strings.push_back(potential_path.generic_string());
    }

    std::string err_msg =
        absl::StrFormat("Cannot find file in deployment dir: %s, searched here: \n%s",
                        relative_file_path, absl::StrJoin(searched_paths_strings, ", \n"));
    return Dive::NotFoundError(err_msg);
}

}  // namespace

namespace Dive
{

absl::StatusOr<std::filesystem::path> ResolveHostResourcesLocalPath(
    std::filesystem::path relative_file_path)
{
    // Host resources should be in the same dir as the caller
    std::array search_dirs = {std::filesystem::path(".")};

    return ResolvePath(search_dirs, relative_file_path);
}

absl::StatusOr<std::filesystem::path> ResolveDeviceResourcesLocalPath(
    std::filesystem::path relative_file_path)
{
    // Determine device resources location relative to host tool
    std::filesystem::path base_dir = "..";
    std::array search_dirs = {// Most platforms
                              base_dir / DIVE_INSTALL_DEST_DEVICE,
                              // Apple bundle
                              base_dir / DIVE_MACOS_BUNDLE_RESOURCES};

    return ResolvePath(search_dirs, relative_file_path);
}

absl::StatusOr<std::filesystem::path> ResolveProfilingResourcesLocalPath(
    std::filesystem::path relative_file_path)
{
    // Determine profiling resources location relative to host tool
    std::filesystem::path base_dir = "..";
    std::array search_dirs = {
        // Most platforms
        base_dir / DIVE_PROFILING_PLUGIN_DIR,
        // Apple bundle
        base_dir / DIVE_MACOS_BUNDLE_RESOURCES / DIVE_PROFILING_PLUGIN_DIR,
    };

    return ResolvePath(search_dirs, relative_file_path);
}

}  // namespace Dive
