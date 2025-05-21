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

#pragma once

#include <vector>
#include <optional>

#include "dump_entry.h"

namespace Dive::gfxr {

// From a GFXR file, produce block indices that can be used with GXR --dump-resources.
//
// Returns std::nullopt on error.
std::optional<std::vector<DumpEntry>> FindDumpableResources(const char* filename);

// Serialize a list of complete dumpable to a JSON file.
//
// Returns false on error.
bool SaveAsJsonFile(const std::vector<DumpEntry>& dumpables, const char* filename);

}