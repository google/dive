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

#include <filesystem>
#include <vector>

#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/types/span.h"
#include "dive/build_defs/version_defs.h"
#include "dive/os/command_utils.h"

namespace
{

absl::StatusOr<std::filesystem::path> ResolvePath(
    absl::Span<const std::filesystem::path> search_paths, std::filesystem::path relative_file_path)
{
    assert(!search_paths.empty());

    std::vector<std::string> searched_paths_strings;

    for (const auto& p : search_paths)
    {
        const auto potential_path = p / relative_file_path;
        if (std::filesystem::exists(potential_path))
        {
            return std::filesystem::canonical(potential_path);
        }
        searched_paths_strings.push_back(potential_path.generic_string());
    }

    std::string err_msg =
        absl::StrFormat("Cannot find file in deployment dir: %s, searched here: \n%s",
                        relative_file_path, absl::StrJoin(searched_paths_strings, ", \n"));
    return absl::NotFoundError(err_msg);
}

}  // namespace

namespace Dive
{

absl::StatusOr<std::filesystem::path> ResolveHostResourcesLocalPath(
    std::filesystem::path relative_file_path)
{
    absl::StatusOr<std::filesystem::path> exec_dir = Dive::GetExecutableDirectory();
    if (!exec_dir.ok())
    {
        return exec_dir.status();
    }

    // Host resources should be in the same dir as the caller
    std::vector<std::filesystem::path> search_dirs = {*exec_dir};

    return ResolvePath(search_dirs, relative_file_path);
}

absl::StatusOr<std::filesystem::path> ResolveDeviceResourcesLocalPath(
    std::filesystem::path relative_file_path)
{
    absl::StatusOr<std::filesystem::path> exec_dir = Dive::GetExecutableDirectory();
    if (!exec_dir.ok())
    {
        return exec_dir.status();
    }

    // Determine device resources location relative to host tool
    std::vector<std::filesystem::path> search_dirs;
    // Most platforms
    search_dirs.push_back(*exec_dir / ".." / DIVE_INSTALL_DEST_DEVICE);
    // Apple bundle
    search_dirs.push_back(*exec_dir / ".." / DIVE_MACOS_BUNDLE_RESOURCES);

    return ResolvePath(search_dirs, relative_file_path);
}

absl::StatusOr<std::filesystem::path> ResolveProfilingResourcesLocalPath(
    std::filesystem::path relative_file_path)
{
    absl::StatusOr<std::filesystem::path> exec_dir = Dive::GetExecutableDirectory();
    if (!exec_dir.ok())
    {
        return exec_dir.status();
    }

    // Determine device resources location relative to host tool
    std::vector<std::filesystem::path> search_dirs;
    // Most platforms
    search_dirs.push_back(*exec_dir / ".." / DIVE_PROFILING_PLUGIN_DIR);
    // Apple bundle
    search_dirs.push_back(*exec_dir / ".." / DIVE_MACOS_BUNDLE_RESOURCES /
                          DIVE_PROFILING_PLUGIN_DIR);

    return ResolvePath(search_dirs, relative_file_path);
}

}  // namespace Dive
