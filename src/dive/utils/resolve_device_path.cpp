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

#include "dive/utils/resolve_device_path.h"

#include <filesystem>

#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "dive/os/command_utils.h"

namespace Dive
{

// Returns potential positions of the device libraries relative to the executable, which can vary
// depending on the host platform and the release type.
absl::StatusOr<std::vector<std::filesystem::path>> GetPotentialDeviceLibraryDirs()
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

    // The host tool and device libraries have both been installed under pkg/
    search_paths.push_back(exe_dir / "../device/");

    // For dev builds, when the host tool is being launched from: build/host/<bin|ui>/<type>/ (a
    // common VS use case)
    //
    // And the device libraries have been installed under build/pkg/device/
    search_paths.push_back(exe_dir / "../../../pkg/device");

    return search_paths;
}

absl::StatusOr<std::filesystem::path> ResolveDevicePath(std::filesystem::path file_name)
{
    std::vector<std::filesystem::path> search_paths;
    {
        auto ret = GetPotentialDeviceLibraryDirs();
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