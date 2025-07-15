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

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "third_party/gfxreconstruct/framework/util/logging.h"

ABSL_FLAG(bool,
          last_draw_only,
          false,
          "If specified, only dump the final draw call for a render pass. This should speed up "
          "dumping while still providing a useful result.");

namespace
{

using Dive::gfxr::DumpEntry;
using Dive::gfxr::FindDumpableResources;
using Dive::gfxr::SaveAsJsonFile;
using gfxrecon::util::Log;

}  // namespace

int main(int argc, char** argv)
{
    absl::SetProgramUsageMessage("Usage: gfxr_dump_resources FILE.GFXR OUTPUT.JSON");
    std::vector<char*> positional_args = absl::ParseCommandLine(argc, argv);
    if (positional_args.size() != 3)
    {
        std::cerr << absl::ProgramUsageMessage() << '\n';
        return 1;
    }

    const char* input_filename = positional_args[1];
    const char* output_filename = positional_args[2];

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

    if (absl::GetFlag(FLAGS_last_draw_only))
    {
        // Only keep the final draw call. This should represent the image presented to the user.
        // For validation purposes, this is typically fine and saves a lot of time (since each draw
        // call can take 2-3 seconds to dump).
        for (DumpEntry& dumpable : *dumpables)
        {
            std::vector<uint64_t>& draws = dumpable.draws;
            if (draws.size() == 1)
            {
                continue;
            }
            draws.erase(draws.begin(), draws.end() - 1);
        }
    }

    if (!SaveAsJsonFile(*dumpables, output_filename))
    {
        std::cerr << "Failed to serialize to " << output_filename << '\n';
        return 1;
    }

    return 0;
}
