#include <cassert>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <unordered_map>

#include "dump_entry.h"
#include "gfxreconstruct/framework/decode/api_decoder.h"
#include "gfxreconstruct/framework/decode/file_processor.h"
#include "gfxreconstruct/framework/decode/struct_pointer_decoder.h"
#include "gfxreconstruct/framework/format/format.h"
#include "gfxreconstruct/framework/generated/generated_vulkan_consumer.h"
#include "gfxreconstruct/framework/generated/generated_vulkan_decoder.h"
#include "gfxreconstruct/framework/generated/generated_vulkan_struct_decoders.h"

#include "state_machine.h"
#include "states.h"

namespace
{

// TODO rename
class MyConsumer : public gfxrecon::decode::VulkanConsumer
{
public:
    MyConsumer(std::function<void(DumpEntry)> dump_found_callback) :
        dump_found_callback_(std::move(dump_found_callback))
    {
    }

    void Process_vkBeginCommandBuffer(
    const gfxrecon::decode::ApiCallInfo& call_info,
    VkResult                             returnValue,
    gfxrecon::format::HandleId           commandBuffer,
    gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkCommandBufferBeginInfo>*
    pBeginInfo) override
    {
        std::cerr << "Process_vkBeginCommandBuffer: commandBuffer=" << commandBuffer << '\n';
        // TODO could reduce # of lookups by using `it`
        if (auto it = incomplete_dumps_.find(commandBuffer); it != incomplete_dumps_.end())
        {
            std::cerr << "Command buffer " << commandBuffer
                      << " never submitted! Discarding previous state...\n";
        }
        incomplete_dumps_[commandBuffer] = std::make_unique<StateMachine>(
        commandBuffer,
        [this, commandBuffer](DumpEntry dump_entry) {
            dump_found_callback_(std::move(dump_entry));
            incomplete_dumps_.erase(commandBuffer);
        },
        [this, commandBuffer] { incomplete_dumps_.erase(commandBuffer); });
        // TODO reduce # of lookups
        incomplete_dumps_[commandBuffer]->Process_vkBeginCommandBuffer(call_info,
                                                                       returnValue,
                                                                       commandBuffer,
                                                                       pBeginInfo);
    }

    void Process_vkCmdBeginRenderPass(
    const gfxrecon::decode::ApiCallInfo& call_info,
    gfxrecon::format::HandleId           commandBuffer,
    gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkRenderPassBeginInfo>*
                      pRenderPassBegin,
    VkSubpassContents contents) override
    {
        std::cerr << "Process_vkCmdBeginRenderPass: commandBuffer=" << commandBuffer << '\n';
        if (auto it = incomplete_dumps_.find(commandBuffer); it == incomplete_dumps_.end())
        {
            std::cerr << "Command buffer " << commandBuffer << " never started! Ignoring...\n";
            return;
        }
        else
        {
            StateMachine& state_machine = *it->second;
            state_machine.Process_vkCmdBeginRenderPass(call_info,
                                                       commandBuffer,
                                                       pRenderPassBegin,
                                                       contents);
        }
    }

    void Process_vkCmdDraw(const gfxrecon::decode::ApiCallInfo& call_info,
                           gfxrecon::format::HandleId           commandBuffer,
                           uint32_t                             vertexCount,
                           uint32_t                             instanceCount,
                           uint32_t                             firstVertex,
                           uint32_t                             firstInstance) override
    {
        std::cerr << "Process_vkCmdDraw: commandBuffer=" << commandBuffer << '\n';
        if (auto it = incomplete_dumps_.find(commandBuffer); it == incomplete_dumps_.end())
        {
            std::cerr << "Command buffer " << commandBuffer << " never started! Ignoring...\n";
            return;
        }
        else
        {
            StateMachine& state_machine = *it->second;
            state_machine.Process_vkCmdDraw(call_info,
                                            commandBuffer,
                                            vertexCount,
                                            instanceCount,
                                            firstVertex,
                                            firstInstance);
        }
    }

    void Process_vkCmdDrawIndexed(const gfxrecon::decode::ApiCallInfo& call_info,
                                  gfxrecon::format::HandleId           commandBuffer,
                                  uint32_t                             indexCount,
                                  uint32_t                             instanceCount,
                                  uint32_t                             firstIndex,
                                  int32_t                              vertexOffset,
                                  uint32_t                             firstInstance) override
    {
        std::cerr << "Process_vkCmdDrawIndexed: commandBuffer=" << commandBuffer << '\n';
        if (auto it = incomplete_dumps_.find(commandBuffer); it == incomplete_dumps_.end())
        {
            std::cerr << "Command buffer " << commandBuffer << " never started! Ignoring...\n";
            return;
        }
        else
        {
            StateMachine& state_machine = *it->second;
            state_machine.Process_vkCmdDrawIndexed(call_info,
                                                   commandBuffer,
                                                   indexCount,
                                                   instanceCount,
                                                   firstIndex,
                                                   vertexOffset,
                                                   firstInstance);
        }
    }

    void Process_vkCmdEndRenderPass(const gfxrecon::decode::ApiCallInfo& call_info,
                                    gfxrecon::format::HandleId           commandBuffer) override
    {
        std::cerr << "Process_vkCmdEndRenderPass: commandBuffer=" << commandBuffer << '\n';
        if (auto it = incomplete_dumps_.find(commandBuffer); it == incomplete_dumps_.end())
        {
            std::cerr << "Command buffer " << commandBuffer << " never started! Ignoring...\n";
            return;
        }
        else
        {
            StateMachine& state_machine = *it->second;
            state_machine.Process_vkCmdEndRenderPass(call_info, commandBuffer);
        }
    }

    void Process_vkQueueSubmit(
    const gfxrecon::decode::ApiCallInfo&                                            call_info,
    VkResult                                                                        returnValue,
    gfxrecon::format::HandleId                                                      queue,
    uint32_t                                                                        submitCount,
    gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkSubmitInfo>* pSubmits,
    gfxrecon::format::HandleId                                                      fence) override
    {
        std::cerr << "Process_vkQueueSubmit\n";
        for (int submit_index = 0; submit_index < submitCount; submit_index++)
        {
            const gfxrecon::decode::Decoded_VkSubmitInfo&
            submit = pSubmits->GetMetaStructPointer()[submit_index];
            for (int command_buffer_index = 0;
                 command_buffer_index < pSubmits->GetPointer()->commandBufferCount;
                 command_buffer_index++)
            {
                gfxrecon::format::HandleId command_buffer_id = submit.pCommandBuffers
                                                               .GetPointer()[command_buffer_index];
                std::cerr << "... for commandBuffer=" << command_buffer_id << '\n';
                if (auto it = incomplete_dumps_.find(command_buffer_id);
                    it != incomplete_dumps_.end())
                {
                    StateMachine& state_machine = *it->second;
                    state_machine.Process_vkQueueSubmit(call_info,
                                                        returnValue,
                                                        queue,
                                                        submitCount,
                                                        pSubmits,
                                                        fence);
                    break;
                }
                else
                {
                    std::cerr << "Command buffer " << command_buffer_id << " never started!\n";
                }
            }
        }
    }

private:
    // Function run when a complete dump entry has been formed. This is ready to be written to disk,
    // etc.
    std::function<void(DumpEntry)> dump_found_callback_;
    // Incomplete dumps for each command buffer
    // std::unique_ptr for pointer stability
    std::unordered_map<gfxrecon::format::HandleId, std::unique_ptr<StateMachine>> incomplete_dumps_;
    // TODO separate map of open command buffers?
};

}  // namespace

int main(int argc, char** argv)
{
    // TODO: Assertion failed: 'depth_img_view_info != nullptr' (/usr/local/google/home/hitchens/git/gfxreconstruct/framework/decode/vulkan_replay_dump_resources_draw_calls.cpp:2326
    if (argc != 3)
    {
        std::cerr << "Usage: gfxr_dump_resources FILE.GFXR OUTPUT.JSON\n";
        return 1;
    }

    // TODO std::filesystem?
    const char* input_filename = argv[1];
    const char* output_filename = argv[2];

    gfxrecon::decode::FileProcessor file_processor;
    if (!file_processor.Initialize(input_filename))
    {
        std::cerr << "Failed to open input:" << input_filename << '\n';
        return 1;
    }

    std::ofstream out(output_filename);
    if (!out.good() || !out.is_open())
    {
        std::cerr << "Failed to open output:" << output_filename << '\n';
        return 1;
    }

    std::vector<DumpEntry> complete_dump_entries;

    gfxrecon::decode::VulkanDecoder vulkan_decoder;
    MyConsumer                      my_consumer([&complete_dump_entries](DumpEntry dump_entry) {
        complete_dump_entries.push_back(std::move(dump_entry));
    });
    vulkan_decoder.AddConsumer(&my_consumer);
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

    // TODO transform into c++ version of output instead of processing vector<Dump>
    out << "{\n";

    out << "  \"BeginCommandBuffer\": [";
    for (int i = 0; i < complete_dump_entries.size(); ++i)
    {
        DumpEntry& entry = complete_dump_entries[i];
        if (i != 0)
        {
            out << ',';
        }
        out << entry.begin_command_buffer_block_index;
    }
    out << "],\n";

    out << "  \"RenderPass\": [";
    for (int i = 0; i < complete_dump_entries.size(); ++i)
    {
        DumpEntry& entry = complete_dump_entries[i];
        if (i != 0)
        {
            out << ',';
        }
        out << '[';
        for (int ii = 0; ii < entry.render_passes.size(); ++ii)
        {
            DumpRenderPass& render_pass = entry.render_passes[ii];
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
    for (int i = 0; i < complete_dump_entries.size(); ++i)
    {
        DumpEntry& entry = complete_dump_entries[i];
        if (i != 0)
        {
            out << ',';
        }
        out << '[';
        for (int ii = 0; ii < entry.draws.size(); ++ii)
        {
            uint64_t& draw = entry.draws[ii];
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
    for (int i = 0; i < complete_dump_entries.size(); ++i)
    {
        DumpEntry& entry = complete_dump_entries[i];
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
        std::cerr << "Failed to close output file: " << output_filename << '\n';
        return 1;
    }

    return 0;
}