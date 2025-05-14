#pragma once

#include <cstdint>
#include <vector>

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
