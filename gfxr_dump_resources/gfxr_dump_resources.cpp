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

#include "gfxr_dump_resources.h"

#include <fstream>
#include <iostream>
#include <optional>
#include <vector>

#include "dump_entry.h"
#include "dump_resources_builder_consumer.h"

#include "third_party/gfxreconstruct/framework/decode/file_processor.h"
#include "third_party/gfxreconstruct/framework/generated/generated_vulkan_decoder.h"

namespace Dive::gfxr
{

std::optional<std::vector<DumpEntry>> FindDumpableResources(const char* filename)
{
    gfxrecon::decode::FileProcessor file_processor;
    if (!file_processor.Initialize(filename))
    {
        std::cerr << "Failed to open input:" << filename << '\n';
        return std::nullopt;
    }

    std::vector<DumpEntry> complete_dump_entries;

    gfxrecon::decode::VulkanDecoder vulkan_decoder;
    DumpResourcesBuilderConsumer    consumer([&complete_dump_entries](DumpEntry dump_entry) {
        complete_dump_entries.push_back(std::move(dump_entry));
    });
    vulkan_decoder.AddConsumer(&consumer);
    file_processor.AddDecoder(&vulkan_decoder);

    file_processor.ProcessAllFrames();

    return complete_dump_entries;
}

bool SaveAsJsonFile(const std::vector<DumpEntry>& dumpables, const char* filename)
{
    std::ofstream out(filename);
    if (!out.good() || !out.is_open())
    {
        std::cerr << "Failed to open output:" << filename << '\n';
        return false;
    }

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

}  // namespace Dive::gfxr