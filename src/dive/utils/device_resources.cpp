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

#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "dive/build_defs/version_defs.h"
#include "dive/os/command_utils.h"

namespace Dive
{

// Returns possible locations for the device resources folder on the host machine
absl::StatusOr<std::vector<std::filesystem::path>> GetPotentialDeviceResourcesDirs()
{
    std::vector<std::filesystem::path> search_paths;

    absl::StatusOr<std::filesystem::path> ret = Dive::GetExecutableDirectory();
    if (!ret.ok())
    {
        std::string warn_msg = absl::
        StrFormat("Could not determine executable directory: %s. Search will not include "
                  "executable-relative paths.",
                  ret.status().message().data());
        return absl::NotFoundError(warn_msg);
    }

    std::filesystem::path exe_dir = *ret;

    // TODO(b/462767957): Update below based on new arrangement of device resources folder

    // For host tools, dev build
    search_paths.push_back(DIVE_INSTALL_DIR_PATH);

    // Predict location for release builds
    search_paths.push_back(exe_dir / "install");

    return search_paths;
}

absl::StatusOr<std::filesystem::path> ResolveResourcesLocalPath(std::filesystem::path file_name)
{
    std::vector<std::filesystem::path> search_paths;
    {
        auto ret = GetPotentialDeviceResourcesDirs();
        if (!ret.ok())
        {
            return ret.status();
        }
        search_paths = *ret;
    }

    std::vector<std::string> searched_paths_strings;

    assert(!search_paths.empty());

    for (const auto &p : search_paths)
    {
        const auto potential_path = p / file_name;
        if (std::filesystem::exists(potential_path))
        {
            auto canonical_path = std::filesystem::canonical(potential_path);
            return canonical_path;
        }
        searched_paths_strings.push_back(potential_path.generic_string());
    }

    std::string
    err_msg = absl::StrFormat("Cannot find file in deployment dir: %s, searched here: \n%s",
                              file_name,
                              absl::StrJoin(searched_paths_strings, ", \n"));
    return absl::NotFoundError(err_msg);
}

}  // namespace Dive
