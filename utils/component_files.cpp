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

namespace Dive::ComponentFiles
{

absl::StatusOr<Dive::ComponentFiles::Paths> GetHostPaths(const std::filesystem::path &parent_dir,
                                                         const std::string           &gfxr_stem)
{
    assert(!gfxr_stem.empty());
    assert(!parent_dir.empty());

    Dive::ComponentFiles::Paths artifacts = {};

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
    size_t      pos = gfxa_stem.find(Dive::ComponentFiles::kGfxrFileNameSubstr);
    if (pos == std::string::npos)
    {
        return absl::FailedPreconditionError(
        absl::StrFormat("unexpected name for gfxr file: %s, expecting name containing: %s",
                        gfxr_stem,
                        Dive::ComponentFiles::kGfxrFileNameSubstr));
    }
    int gfxr_string_length = sizeof(Dive::ComponentFiles::kGfxrFileNameSubstr) / sizeof(char);
    gfxa_stem.replace(pos, gfxr_string_length - 1, Dive::ComponentFiles::kGfxaFileNameSubstr);

    artifacts.gfxr = parent_dir / (gfxr_stem + Dive::ComponentFiles::kGfxrExt);
    artifacts.gfxa = parent_dir / (gfxa_stem + Dive::ComponentFiles::kGfxaExt);
    artifacts.perf_counter_csv = parent_dir /
                                 (gfxr_stem + Dive::ComponentFiles::kProfilingMetricsHostSuffix +
                                  Dive::ComponentFiles::kCsvExt);
    artifacts.gpu_timing_csv = parent_dir /
                               (gfxr_stem + Dive::ComponentFiles::kGpuTimingHostSuffix +
                                Dive::ComponentFiles::kCsvExt);
    artifacts.pm4_rd = parent_dir / (gfxr_stem + Dive::ComponentFiles::kRdExt);
    artifacts.renderdoc_rdc = parent_dir / (gfxr_stem + Dive::ComponentFiles::kRenderDocHostSuffix +
                                            Dive::ComponentFiles::kRdcExt);
    return artifacts;
}

}  // namespace Dive::ComponentFiles
