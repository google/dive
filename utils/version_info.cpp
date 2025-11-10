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

#include "version_info.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"

namespace
{
// These are relative to install/
static constexpr std::string_view
kDiveAndroidLibrariesBuildTypePath = "TARGETTING_ANDROID_BUILD_TYPE.txt";
static constexpr std::string_view kProfilingPluginShaPath = "dive_profiling_plugin/SHA";

absl::StatusOr<std::string> ReadFileSingleLine(std::filesystem::path file_path)
{
    if (!std::filesystem::exists(file_path))
    {
        return absl::FailedPreconditionError(
        absl::StrFormat("File does not exist: %s", file_path.string()));
    }

    std::ifstream file(file_path);
    if (!file.is_open())
    {
        return absl::InternalError(absl::StrFormat("Failed to open file: %s", file_path.string()));
    }

    // TODO: Use stringutils trim
    std::string line;
    if (!std::getline(file, line))
    {
        return absl::InternalError(
        absl::StrFormat("Failed to read line from file: %s", file_path.string()));
    }
    return line;
}

}  // namespace

namespace Dive
{

std::string GetVersionNumberString()
{
    std::stringstream ss;
    ss << DIVE_VERSION_MAJOR << "." << DIVE_VERSION_MINOR << "." << DIVE_VERSION_REVISION;
    return ss.str();
}

std::string GetVersionDetailedSummary(std::filesystem::path install_dir_path_generic,
                                      std::string           host_tools_build_type)
{
    std::stringstream ss;
    ss << "Host Platform: " << DIVE_HOST_PLATFORM_STRING << std::endl;
    ss << "Dive Version: " << GetVersionNumberString() << std::endl;
    ss << "Dive SHA: " << DIVE_VERSION_SHA1 << std::endl;
    ss << "Dive Host Tools Build Type: " << host_tools_build_type << std::endl;

    std::filesystem::path
    android_libraries_build_type = absl::StrFormat("%s/%s",
                                                   install_dir_path_generic.generic_string(),
                                                   kDiveAndroidLibrariesBuildTypePath);
    if (absl::StatusOr<std::string> ret = ReadFileSingleLine(android_libraries_build_type);
        ret.ok())
    {
        ss << "Dive Android Libraries Build Type: " << *ret << std::endl;
    }
    else
    {
        // TODEL UNICORN
        std::cerr << ret.status().message() << std::endl;
    }

    std::filesystem::path profiling_plugin_sha = absl::StrFormat("%s/%s",
                                                                 install_dir_path_generic
                                                                 .generic_string(),
                                                                 kProfilingPluginShaPath);
    if (absl::StatusOr<std::string> ret = ReadFileSingleLine(profiling_plugin_sha); ret.ok())
    {
        ss << "Dive Profiling Plugin SHA: " << *ret << std::endl;
    }
    else
    {
        // TODEL UNICORN
        std::cerr << ret.status().message() << std::endl;
    }
    return ss.str();
}

}  // namespace Dive