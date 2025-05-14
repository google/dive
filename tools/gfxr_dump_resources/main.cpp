#include <cassert>
#include <cstdint>
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
        std::cerr << "Process_vkCmdEndRenderPass: commandBuffer=" << commandBuffer << '\n';
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
    if (argc != 2)
    {
        std::cerr << "Usage: gfxr_dump_resources FILE.GFXR\n";
        return 1;
    }

    const char* input_filename = argv[1];

    gfxrecon::decode::FileProcessor file_processor;
    if (!file_processor.Initialize(input_filename))
    {
        std::cerr << "Failed to open " << input_filename << '\n';
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

    std::cout << "Found " << complete_dump_entries.size() << " dumpables\n";
    for (const DumpEntry& dump : complete_dump_entries)
    {
        std::cout << " Dump\n";
        std::cout << "  BeginCommandBuffer=" << dump.begin_command_buffer_block_index << '\n';
        std::cout << "  RenderPass size=" << dump.render_passes.size() << '\n';
        for (const DumpRenderPass& render_pass : dump.render_passes) {
            std::cout << "   RenderPass=" << render_pass.begin_block_index << ',' << render_pass.end_block_index << '\n';
        }
        std::cout << "  Draws size=" << dump.draws.size() << '\n';
        for (const uint64_t& draw : dump.draws) {
            std::cout << "   Draw=" << draw << '\n';
        }
        std::cout << "  QueueSubmit=" << dump.queue_submit_block_index << '\n';
    }

    return 0;
}