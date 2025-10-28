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

namespace Dive::ComponentFiles
{

// File extensions
inline constexpr char kGfxrExt[] = ".gfxr";
inline constexpr char kGfxaExt[] = ".gfxa";
inline constexpr char kRdExt[] = ".rd";
inline constexpr char kPngExt[] = ".png";
inline constexpr char kCsvExt[] = ".csv";
inline constexpr char kRdcExt[] = ".rdc";

// Filename substrings recognized by GFXR
inline constexpr char kGfxrFileNameSubstr[] = "_trim_trigger_";
inline constexpr char kGfxaFileNameSubstr[] = "_asset_file_";

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
inline constexpr char kProfilingMetricsHostSuffix[] = "_profiling_metrics";
inline constexpr char kGpuTimingHostSuffix[] = "_gpu_time";
inline constexpr char kRenderDocHostSuffix[] = "_capture";

// ----------------------------------------------------------------------------
// On device side, the group of files associated with the same GFXR capture can be in different
// directories
//
// TODO: Migrate some constants from capture_service/constants.h

}  // namespace Dive::ComponentFiles
