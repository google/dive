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

#include "dive/utils/version_info.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "dive/build_defs/version_defs.h"
#include "dive/os/command_utils.h"

namespace
{

constexpr size_t kShortSha = 7;
constexpr size_t kLongSha = 40;

// This is relative to install/
constexpr std::string_view kProfilingPluginShaPath = "dive_profiling_plugin/SHA";

// Upper bound of file size to avoid reading long files
constexpr size_t kMaxCharactersDeviceLibraryFile = 200;

absl::StatusOr<std::string> ReadFileCapped(const std::filesystem::path& file_path,
                                           size_t                       max_characters)
{
    if (!std::filesystem::exists(file_path))
    {
        return absl::FailedPreconditionError(
        absl::StrFormat("File does not exist: %s", file_path.generic_string()));
    }

    std::ifstream file(file_path);
    if (!file.is_open())
    {
        return absl::InternalError(
        absl::StrFormat("Failed to open file: %s", file_path.generic_string()));
    }

    assert((max_characters > 0) && (max_characters <= kMaxCharactersDeviceLibraryFile));
    char buffer[kMaxCharactersDeviceLibraryFile] = "";
    file.read(buffer, std::min(max_characters, sizeof(buffer) - 1));

    return buffer;
}

// Returns a string of the Dive repo SHA with the first n_digits digits:
std::string_view GetSHAString(std::string_view full_sha, size_t n_digits)
{
    assert((n_digits > 0) && (n_digits <= kLongSha));
    return full_sha.substr(0, n_digits);
}

}  // namespace

namespace Dive
{

std::string GetHostShortVersionString()
{
    std::string      full_sha = DIVE_VERSION_SHA1;
    std::string_view short_sha = GetSHAString(full_sha, kShortSha);
    return absl::StrFormat("%s-%s-%s", DIVE_VERSION, DIVE_RELEASE_TYPE, short_sha);
}

std::string GetHostToolsVersionInfo()
{
    std::string      full_sha = DIVE_VERSION_SHA1;
    std::string_view short_sha = GetSHAString(full_sha, kShortSha);
    std::string      host_tools_build_string = absl::StrFormat("%s-%s-%s-%s",
                                                          DIVE_VERSION,
                                                          DIVE_RELEASE_TYPE,
                                                          DIVE_HOST_PLATFORM_STRING,
                                                          short_sha);

    return absl::StrFormat("Host Tools Build: %s\nHost Tools Build Type: %s\nHost Tools SHA: %s\n",
                           host_tools_build_string,
                           DIVE_BUILD_TYPE,
                           GetSHAString(DIVE_VERSION_SHA1, kLongSha));
}

std::string GetDiveDescription()
{
    return absl::StrFormat("%s (%s)\n\n%s\n\n%s\n",
                           DIVE_PRODUCT_NAME,
                           GetHostShortVersionString(),
                           DIVE_PRODUCT_DESCRIPTION,
                           DIVE_COPYRIGHT_DESCRIPTION);
}

absl::StatusOr<std::map<std::string_view, std::string_view>> GetDeviceLibraryVersionMap(
const std::string& csv_content)
{
    std::map<std::string_view, std::string_view> device_info_map;
    std::vector<std::string_view>                rows = absl::StrSplit(csv_content, '\n');
    for (std::string_view row : rows)
    {
        std::vector<std::string_view> key_value = absl::StrSplit(row, ',');
        if (key_value.size() != 2)
        {
            std::string err_msg = absl::
            StrFormat("GetDeviceLibraryVersionMap() unexpected number of fields in row: %s", row);
            return absl::FailedPreconditionError(err_msg);
        }
        device_info_map[key_value[0]] = key_value[1];
    }

    // Validate map
    if (device_info_map.size() != VersionInfoConstants::kKeyCount)
    {
        return absl::FailedPreconditionError(
        absl::
        StrFormat("GetDeviceLibraryVersionMap() invalid device map: want key count %d, got: %d",
                  VersionInfoConstants::kKeyCount,
                  device_info_map.size()));
    }
    if (device_info_map.count(VersionInfoConstants::kNameSha) != 1)
    {
        return absl::FailedPreconditionError(
        absl::StrFormat("GetDeviceLibraryVersionMap() invalid device map: key error: %s",
                        VersionInfoConstants::kNameSha));
    }
    if (device_info_map.count(VersionInfoConstants::kNameVersion) != 1)
    {
        return absl::FailedPreconditionError(
        absl::StrFormat("GetDeviceLibraryVersionMap() invalid device map: key error: %s",
                        VersionInfoConstants::kNameVersion));
    }
    if (device_info_map.count(VersionInfoConstants::kNameBuildType) != 1)
    {
        return absl::FailedPreconditionError(
        absl::StrFormat("GetDeviceLibraryVersionMap() invalid device map: key error: %s",
                        VersionInfoConstants::kNameBuildType));
    }
    if (device_info_map.count(VersionInfoConstants::kNameReleaseType) != 1)
    {
        return absl::FailedPreconditionError(
        absl::StrFormat("GetDeviceLibraryVersionMap() invalid device map: key error: %s",
                        VersionInfoConstants::kNameReleaseType));
    }
    if (device_info_map.count(VersionInfoConstants::kNameAbi) != 1)
    {
        return absl::FailedPreconditionError(
        absl::StrFormat("GetDeviceLibraryVersionMap() invalid device map: key error: %s",
                        VersionInfoConstants::kNameAbi));
    }

    return device_info_map;
}

// This parses the device libraries version CSV file generated by top-level cmake
std::string GetDeviceLibrariesVersionInfo(const std::string& csv_content)
{
    std::map<std::string_view, std::string_view> device_info_map;
    {
        auto ret = GetDeviceLibraryVersionMap(csv_content);
        if (!ret.ok())
        {
            std::cerr << ret.status().message() << std::endl;
            return "";
        }
        device_info_map = *ret;
    }

    std::string_view short_sha = GetSHAString(device_info_map[VersionInfoConstants::kNameSha],
                                              kShortSha);
    std::string      device_libraries_build_string = absl::
    StrFormat("%s-%s-%s-%s",
              device_info_map[VersionInfoConstants::kNameVersion],
              device_info_map[VersionInfoConstants::kNameReleaseType],
              device_info_map[VersionInfoConstants::kNameAbi],
              short_sha);

    return absl::StrFormat("Device Libraries Build: %s\nDevice Libraries Build Type: %s\nDevice "
                           "Libraries SHA: %s\n",
                           device_libraries_build_string,
                           device_info_map[VersionInfoConstants::kNameBuildType],
                           GetSHAString(device_info_map[VersionInfoConstants::kNameSha], kLongSha));
}

std::filesystem::path ResolvePath(std::string_view file_name)
{
    std::vector<std::filesystem::path> search_paths;
    if (auto exe_dir = Dive::GetExecutableDirectory(); exe_dir.ok())
    {
        search_paths.push_back(*exe_dir);
        search_paths.push_back(*exe_dir / "install");
    }
    else
    {
        std::cerr << exe_dir.status().message() << std::endl;
    }
    search_paths.push_back(DIVE_INSTALL_DIR_PATH);

    for (const auto& p : search_paths)
    {
        const auto potential_path = p / file_name;
        if (std::filesystem::exists(potential_path))
        {
            auto canonical_path = std::filesystem::canonical(potential_path);
            std::cout << "Found " << file_name << " at " << canonical_path << "\n";
            return canonical_path;
        }
    }
    std::cerr << "Could not find '" << file_name << "' in any of the search paths.\n";
    return {};
}

std::string GetLongVersionString()
{
    std::string summary = GetHostToolsVersionInfo();

    std::filesystem::path device_libraries_version_path = ResolvePath(
    DIVE_DEVICE_LIBRARIES_VERSION_FILENAME);
    if (absl::StatusOr<std::string> ret = ReadFileCapped(device_libraries_version_path,
                                                         kMaxCharactersDeviceLibraryFile);
        ret.ok())
    {
        summary += "\n" + GetDeviceLibrariesVersionInfo(*ret);
    }
    else
    {
        std::cerr << ret.status().message() << std::endl;
    }

    std::filesystem::path profiling_plugin_version_path = ResolvePath(kProfilingPluginShaPath);
    if (absl::StatusOr<std::string> ret = ReadFileCapped(profiling_plugin_version_path, kLongSha);
        ret.ok())
    {
        std::string profiling_plugin_section = absl::StrFormat("Profiling Plugin SHA: %s\n", *ret);
        summary += "\n" + profiling_plugin_section;
    }
    // Want silent failure for profiling plugin or else the CLI stdout can confuse users

    return summary;
}

std::string GetCompleteVersionString()
{
    return absl::StrFormat("Dive (%s)\n\n%s", GetHostShortVersionString(), GetLongVersionString());
}

absl::StatusOr<std::string> GetDeviceLibraryInfo(std::string_view key)
{
    std::filesystem::path device_libraries_version_path = ResolvePath(
    DIVE_DEVICE_LIBRARIES_VERSION_FILENAME);
    std::string csv_content;
    {
        absl::StatusOr<std::string> ret = ReadFileCapped(device_libraries_version_path,
                                                         kMaxCharactersDeviceLibraryFile);
        if (!ret.ok())
        {
            return ret.status();
        }
        csv_content = *ret;
    }

    std::map<std::string_view, std::string_view> device_info_map;
    {
        auto ret = GetDeviceLibraryVersionMap(csv_content);
        if (!ret.ok())
        {
            return ret.status();
        }
        device_info_map = *ret;
    }

    return std::string(device_info_map[key]);
}

}  // namespace Dive
