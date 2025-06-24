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

#include <iostream>
#include <optional>
#include <vector>

#include "dump_entry.h"
#include "gfxr_dump_resources.h"

#include "third_party/gfxreconstruct/framework/util/logging.h"

namespace
{

using Dive::gfxr::DumpEntry;
using Dive::gfxr::FindDumpableResources;
using Dive::gfxr::SaveAsJsonFile;
using gfxrecon::util::Log;

}  // namespace

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: gfxr_dump_resources FILE.GFXR OUTPUT.JSON\n";
        return 1;
    }

    const char* input_filename = argv[1];
    const char* output_filename = argv[2];

#ifdef NDEBUG
    Log::Init(Log::kInfoSeverity);
#else
    Log::Init(Log::kDebugSeverity);
#endif

    std::optional<std::vector<DumpEntry>> dumpables = FindDumpableResources(input_filename);
    if (!dumpables.has_value())
    {
        std::cerr << "Failed to find resources in " << input_filename << '\n';
        return 1;
    }

    if (!SaveAsJsonFile(*dumpables, output_filename))
    {
        std::cerr << "Failed to serialize to " << output_filename << '\n';
        return 1;
    }

    return 0;
}
