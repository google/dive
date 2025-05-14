#include "states.h"

#include "gfxreconstruct/framework/decode/api_decoder.h"
#include "gfxreconstruct/framework/decode/struct_pointer_decoder.h"
#include "gfxreconstruct/framework/format/format.h"
#include "gfxreconstruct/framework/generated/generated_vulkan_consumer.h"
#include "gfxreconstruct/framework/generated/generated_vulkan_struct_decoders.h"

#include <iostream>

#include "state_machine.h"

LookingForQueueSubmit::LookingForQueueSubmit(StateMachine& parent) :
    parent_(parent)
{
}

void LookingForQueueSubmit::Process_vkQueueSubmit(
const gfxrecon::decode::ApiCallInfo&                                            call_info,
VkResult                                                                        returnValue,
gfxrecon::format::HandleId                                                      queue,
uint32_t                                                                        submitCount,
gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkSubmitInfo>* pSubmits,
gfxrecon::format::HandleId                                                      fence)
{
    std::cerr << "LookingForQueueSubmit\n";
    for (int submit_index = 0; submit_index < submitCount; submit_index++)
    {
        std::cerr << "submit\n";
        const gfxrecon::decode::Decoded_VkSubmitInfo&
        submit = pSubmits->GetMetaStructPointer()[submit_index];
        for (int command_buffer_index = 0;
             command_buffer_index < pSubmits->GetPointer()->commandBufferCount;
             command_buffer_index++)
        {
            std::cerr << "byffer\n";
            gfxrecon::format::HandleId command_buffer_id = submit.pCommandBuffers
                                                           .GetPointer()[command_buffer_index];
            std::cerr << "command_buffer_id=" << command_buffer_id << " vs " << parent_.command_buffer() << '\n';
            if (command_buffer_id != parent_.command_buffer())
            {
                continue;
            }

            parent_.dump_entry().queue_submit_block_index = call_info.index;
            // TODO assert complete
            parent_.Accept();
        }
    }
}

LookingForBegin::LookingForBegin(StateMachine&                     parent,
                                 gfxrecon::decode::VulkanConsumer& find_queue_submit) :
    parent_(parent),
    find_queue_submit_(find_queue_submit)
{
}

void LookingForBegin::Process_vkBeginCommandBuffer(
const gfxrecon::decode::ApiCallInfo& call_info,
VkResult                             returnValue,
gfxrecon::format::HandleId           commandBuffer,
gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkCommandBufferBeginInfo>*
pBeginInfo)
{
    parent_.dump_entry().begin_command_buffer_block_index = call_info.index;
    parent_.Transition(find_queue_submit_);
}
