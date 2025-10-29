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

#include "component_files.h"

#include "absl/status/status.h"
#include "absl/strings/str_format.h"

namespace Dive
{

absl::StatusOr<Dive::ComponentFilePaths> GetComponentFilesHostPaths(
const std::filesystem::path &parent_dir,
const std::string           &gfxr_stem)
{
    if (gfxr_stem.empty())
    {
        return absl::FailedPreconditionError(absl::StrFormat("gfxr_stem cannot be empty"));
    }

    if (parent_dir.empty())
    {
        return absl::FailedPreconditionError(absl::StrFormat("parent_dir cannot be empty"));
    }

    Dive::ComponentFileConstants constants;
    Dive::ComponentFilePaths     artifacts = {};

    // check format of gfxr_stem
    // dots in filename are allowed
    size_t slash_pos = gfxr_stem.find('/');
    size_t backslash_pos = gfxr_stem.find('\\');
    if ((slash_pos != std::string::npos) || (backslash_pos != std::string::npos))
    {
        return absl::FailedPreconditionError(
        absl::StrFormat("unexpected name for gfxr file: %s, not a stem", gfxr_stem));
    }

    // .gfxa files have a different stem from the .gfxr file
    std::string gfxa_stem = gfxr_stem;
    size_t      pos = gfxa_stem.find(constants.kGfxrFileNameSubstr);
    if (pos == std::string::npos)
    {
        return absl::FailedPreconditionError(
        absl::StrFormat("unexpected name for gfxr file: %s, expecting name containing: %s",
                        gfxr_stem,
                        constants.kGfxrFileNameSubstr));
    }
    gfxa_stem.replace(pos, constants.kGfxrFileNameSubstr.size(), constants.kGfxaFileNameSubstr);

    artifacts.gfxr = parent_dir / absl::StrFormat("%s%s", gfxr_stem, constants.kGfxrExt);
    artifacts.gfxa = parent_dir / absl::StrFormat("%s%s", gfxa_stem, constants.kGfxaExt);
    artifacts.perf_counter_csv = parent_dir / absl::StrFormat("%s%s%s",
                                                              gfxr_stem,
                                                              constants.kProfilingMetricsHostSuffix,
                                                              constants.kCsvExt);
    artifacts.gpu_timing_csv = parent_dir / absl::StrFormat("%s%s%s",
                                                            gfxr_stem,
                                                            constants.kGpuTimingHostSuffix,
                                                            constants.kCsvExt);
    artifacts.pm4_rd = parent_dir / absl::StrFormat("%s%s", gfxr_stem, constants.kRdExt);
    artifacts.screenshot_png = parent_dir / absl::StrFormat("%s%s", gfxr_stem, constants.kPngExt);
    artifacts.renderdoc_rdc = parent_dir / absl::StrFormat("%s%s%s",
                                                           gfxr_stem,
                                                           constants.kRenderDocHostSuffix,
                                                           constants.kRdcExt);
    return artifacts;
}

}  // namespace Dive
