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

#include <filesystem>
#include <string>

#include "common/dive_version.h"

namespace Dive
{
// Returns a string with the following structure:
//
// <DIVE_VERSION_MAJOR>.<DIVE_VERSION_MINOR>.<DIVE_VERSION_REVISION>
std::string GetVersionNumberString();

// Returns a string with the following structure:
//
// Host Platform: <>
// Dive Version: <>
// Dive SHA: <>
// Dive Host Tools Build Type: <>
// Dive Android Libraries Build Type: <> (added if it exists under install/)
// Dive Profiling Plugin SHA: <> (added if it exists under install/)
std::string GetVersionDetailedSummary(std::filesystem::path install_dir_path,
                                      std::string           host_tools_build_type);

}  // namespace Dive