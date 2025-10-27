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

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "constants.h"

namespace Dive
{

// Describes the presumed filepaths of all files associated with a GFXR file, does not guarantee
// their existence
struct GfxrReplayArtifactsPaths
{
    // <package>_trim_trigger_<id>.gfxr
    std::filesystem::path gfxr;
    // <package>_asset_file_<id>.gfxr
    std::filesystem::path gfxa;
    // <package>_trim_trigger_<id>_profiling_metrics.csv
    std::filesystem::path perf_counter_csv;
    // <package>_trim_trigger_<id>_gpu_time.csv
    std::filesystem::path gpu_timing_csv;
    // <package>_trim_trigger_<id>.rd
    std::filesystem::path pm4_rd;
    // <package>_trim_trigger_<id>_capture.rdc
    std::filesystem::path renderdoc_rdc;
};

// Generates expected location of all replay artifacts, given the location and name of the GFXR file
absl::StatusOr<GfxrReplayArtifactsPaths> GetGfxrReplayArtifactsPathsLocal(
const std::filesystem::path &parent_dir,
const std::string           &gfxr_stem);

}  // namespace Dive