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

#include <cstdint>
#include <fstream>
#include <iostream>

#include "dump_entry.h"
#include "dump_resources_builder_consumer.h"

#include "gfxreconstruct/framework/decode/file_processor.h"
#include "gfxreconstruct/framework/generated/generated_vulkan_decoder.h"

namespace
{

using Dive::tools::DumpEntry;
using Dive::tools::DumpRenderPass;
using Dive::tools::DumpResourcesBuilderConsumer;

bool SaveAsJsonFile(const std::vector<DumpEntry>& dumpables, const char* filename)
{
    std::ofstream out(filename);
    if (!out.good() || !out.is_open())
    {
        // TODO dive_core log.h?
        std::cerr << "Failed to open output:" << filename << '\n';
        return false;
    }

    // TODO transform `dumpables` into c++ version of output instead of processing vector<Dump>
    out << "{\n";

    out << "  \"BeginCommandBuffer\": [";
    for (int i = 0; i < dumpables.size(); ++i)
    {
        const DumpEntry& entry = dumpables[i];
        if (i != 0)
        {
            out << ',';
        }
        out << entry.begin_command_buffer_block_index;
    }
    out << "],\n";

    out << "  \"RenderPass\": [";
    for (int i = 0; i < dumpables.size(); ++i)
    {
        const DumpEntry& entry = dumpables[i];
        if (i != 0)
        {
            out << ',';
        }
        out << '[';
        for (int ii = 0; ii < entry.render_passes.size(); ++ii)
        {
            const DumpRenderPass& render_pass = entry.render_passes[ii];
            if (ii != 0)
            {
                out << ',';
            }
            out << "[" << render_pass.begin_block_index << ',' << render_pass.end_block_index
                << ']';
        }
        out << ']';
    }
    out << "],\n";

    out << "  \"Draw\": [";
    for (int i = 0; i < dumpables.size(); ++i)
    {
        const DumpEntry& entry = dumpables[i];
        if (i != 0)
        {
            out << ',';
        }
        out << '[';
        for (int ii = 0; ii < entry.draws.size(); ++ii)
        {
            uint64_t draw = entry.draws[ii];
            if (ii != 0)
            {
                out << ',';
            }
            out << draw;
        }
        out << ']';
    }
    out << "],\n";

    out << "  \"QueueSubmit\": [";
    for (int i = 0; i < dumpables.size(); ++i)
    {
        const DumpEntry& entry = dumpables[i];
        if (i != 0)
        {
            out << ',';
        }
        out << entry.queue_submit_block_index;
    }
    out << "]\n";

    out << "}\n";

    out.close();
    if (!out.good())
    {
        std::cerr << "Failed to close output file: " << filename << '\n';
        return false;
    }

    return true;
}

}  // namespace

int main(int argc, char** argv)
{
    // TODO: Assertion failed: 'depth_img_view_info != nullptr'
    // (/usr/local/google/home/hitchens/git/gfxreconstruct/framework/decode/vulkan_replay_dump_resources_draw_calls.cpp:2326
    if (argc != 3)
    {
        std::cerr << "Usage: gfxr_dump_resources FILE.GFXR OUTPUT.JSON\n";
        return 1;
    }

    const char* input_filename = argv[1];
    const char* output_filename = argv[2];

    gfxrecon::decode::FileProcessor file_processor;
    if (!file_processor.Initialize(input_filename))
    {
        std::cerr << "Failed to open input:" << input_filename << '\n';
        return 1;
    }

    std::vector<DumpEntry> complete_dump_entries;

    gfxrecon::decode::VulkanDecoder vulkan_decoder;
    DumpResourcesBuilderConsumer    consumer([&complete_dump_entries](DumpEntry dump_entry) {
        complete_dump_entries.push_back(std::move(dump_entry));
    });
    vulkan_decoder.AddConsumer(&consumer);
    file_processor.AddDecoder(&vulkan_decoder);

    file_processor.ProcessAllFrames();

    // DEBUG OUTPUT REMOVE BEFORE COMMITTING
    std::cerr << "Found " << complete_dump_entries.size() << " dumpables\n";
    for (const DumpEntry& dump : complete_dump_entries)
    {
        std::cerr << " Dump\n";
        std::cerr << "  BeginCommandBuffer=" << dump.begin_command_buffer_block_index << '\n';
        std::cerr << "  RenderPass size=" << dump.render_passes.size() << '\n';
        for (const DumpRenderPass& render_pass : dump.render_passes)
        {
            std::cerr << "   RenderPass=" << render_pass.begin_block_index << ','
                      << render_pass.end_block_index << '\n';
        }
        std::cerr << "  Draws size=" << dump.draws.size() << '\n';
        for (const uint64_t& draw : dump.draws)
        {
            std::cerr << "   Draw=" << draw << '\n';
        }
        std::cerr << "  QueueSubmit=" << dump.queue_submit_block_index << '\n';
    }

    SaveAsJsonFile(complete_dump_entries, output_filename);

    return 0;
}