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

#include <string_view>

namespace Dive
{

struct ComponentFileConstants
{
    // File extensions
    static constexpr std::string_view kGfxrExt = ".gfxr";
    static constexpr std::string_view kGfxaExt = ".gfxa";
    static constexpr std::string_view kRdExt = ".rd";
    static constexpr std::string_view kPngExt = ".png";
    static constexpr std::string_view kCsvExt = ".csv";
    static constexpr std::string_view kRdcExt = ".rdc";

    // Filename substrings recognized by GFXR
    static constexpr std::string_view kGfxrFileNameSubstr = "_trim_trigger_";
    static constexpr std::string_view kGfxaFileNameSubstr = "_asset_file_";

    // ----------------------------------------------------------------------------
    // On host side, the group of files associated with the same GFXR capture will in the same
    // directories and named:
    //
    // gfxr:            <package>_trim_trigger_<id>.gfxr
    // gfxa:            <package>_asset_file_<id>.gfxr
    // perf counter:    <package>_trim_trigger_<id>_profiling_metrics.csv
    // gpu timing:      <package>_trim_trigger_<id>_gpu_time.csv
    // pm4:             <package>_trim_trigger_<id>.rd
    // screenshot:      <package>_trim_trigger_<id>.png
    // renderdoc:       <package>_trim_trigger_<id>_capture.rdc (not loaded in UI)

    // Substrings used for host names
    static constexpr std::string_view kProfilingMetricsHostSuffix = "_profiling_metrics";
    static constexpr std::string_view kGpuTimingHostSuffix = "_gpu_time";
    static constexpr std::string_view kRenderDocHostSuffix = "_capture";

    // ----------------------------------------------------------------------------
    // On device side, the group of files associated with the same GFXR capture can be in different
    // directories
    //
    // TODO: Migrate some constants from capture_service/constants.h
};

}  // namespace Dive
