#pragma once

#include <cstdint>
#include <vector>

struct DumpRenderPass
{
    uint64_t begin_block_index;
    // TODO subpass
    uint64_t end_block_index;
};

// Mirrors dump resources JSON schema.
// During MyConsumer, this may be incomplete and missing info.
// When dump_found_callback is run, it should be complete and ready for `--dump-resources`.
struct DumpEntry
{
    uint64_t                    begin_command_buffer_block_index = 0;
    std::vector<DumpRenderPass> render_passes;
    uint64_t                    queue_submit_block_index = 0;
};
