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

#include <filesystem>
#include <string>

#include "absl/status/statusor.h"
#include "component_files_constants.h"

namespace Dive
{

// Describes the presumed filepaths of all files associated with a particular Dive capture,
// including the GFXR capture components and other artifacts produced during Dive's GFXR replay.
// This does not guarantee their existence
struct ComponentFilePaths
{
    std::filesystem::path gfxr;
    std::filesystem::path gfxa;
    std::filesystem::path perf_counter_csv;
    std::filesystem::path gpu_timing_csv;
    std::filesystem::path pm4_rd;
    std::filesystem::path screenshot_png;
    std::filesystem::path renderdoc_rdc;
};

// Generates expected location of the host files, given the location and stem of the local GFXR
// file
absl::StatusOr<Dive::ComponentFilePaths> GetComponentFilesHostPaths(
const std::filesystem::path &parent_dir,
const std::string           &gfxr_stem);

}  // namespace Dive
