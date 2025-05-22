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

namespace Dive::gfxr
{

// A subelement of a dumpable that tracks the begin and end of a renderpass.
struct DumpRenderPass
{
    // Block index for vkCmdBeginRenderPass.
    uint64_t begin_block_index = 0;
    // TODO: Record subpasses
    // Block index for vkCmdEndRenderPass.
    uint64_t end_block_index = 0;

    // Does the dumpable render pass have all the information that GFXR needs for replay.
    bool IsComplete() const { return begin_block_index != 0 && end_block_index != 0; }
};

// Mirrors dump resources JSON schema for a single entry. During DumpResourcesBuilderConsumer, this
// may be incomplete and missing info. When DumpResourcesBuilderConsumer calls `dump_found_callback`
// is run, it should be complete and ready for `--dump-resources`.
//
// Each uint64_t is a block index in the .gfxr file. This is an autoincrementing number used to
// uniquely identify each function call, etc. This is what GFXR wants when `--dump-resources` is
// used. 0 is the sentinel used for "information missing".
struct DumpEntry
{
    // Block index for vkBeginCommandBuffer.
    uint64_t begin_command_buffer_block_index = 0;
    // Render pass block indices. A single command buffer may have multiple render passes.
    std::vector<DumpRenderPass> render_passes;
    // All block indices for all vkCmdDraw* calls.
    std::vector<uint64_t> draws;
    // Block index for vkQueueSubmit.
    uint64_t queue_submit_block_index = 0;

    // Does the dumpable have all the information that GFXR needs for replay.
    bool IsComplete() const
    {
        bool complete = begin_command_buffer_block_index != 0 && queue_submit_block_index != 0 &&
                        !render_passes.empty() && !draws.empty();
        for (const DumpRenderPass& render_pass : render_passes)
        {
            complete &= render_pass.IsComplete();
        }
        // TODO: If a render pass is incomplete, consider throwing away just the incomplete state so
        // that any complete state will survive.
        return complete;
    }
};

}  // namespace Dive::gfxr
