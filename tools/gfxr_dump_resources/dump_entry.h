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

#include <cstdint>
#include <vector>

namespace Dive::tools
{

struct DumpRenderPass
{
    uint64_t begin_block_index = 0;
    // TODO subpass
    uint64_t end_block_index = 0;

    bool IsComplete() const { return begin_block_index != 0 && end_block_index != 0; }
};

// Mirrors dump resources JSON schema.
// During MyConsumer, this may be incomplete and missing info.
// When dump_found_callback is run, it should be complete and ready for `--dump-resources`.
struct DumpEntry
{
    uint64_t                    begin_command_buffer_block_index = 0;
    std::vector<DumpRenderPass> render_passes;
    std::vector<uint64_t>       draws;
    uint64_t                    queue_submit_block_index = 0;

    bool IsComplete() const
    {
        bool complete = begin_command_buffer_block_index != 0 && queue_submit_block_index != 0 &&
                        !render_passes.empty() && !draws.empty();
        // TODO std::acumulate?
        for (const DumpRenderPass& render_pass : render_passes)
        {
            complete &= render_pass.IsComplete();
        }
        return complete;
    }
};

}  // namespace Dive::tools
