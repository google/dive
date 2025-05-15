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

#include "states.h"

#include "dump_entry.h"
#include "gfxreconstruct/framework/decode/api_decoder.h"
#include "gfxreconstruct/framework/decode/struct_pointer_decoder.h"
#include "gfxreconstruct/framework/format/format.h"
#include "gfxreconstruct/framework/generated/generated_vulkan_consumer.h"
#include "gfxreconstruct/framework/generated/generated_vulkan_struct_decoders.h"

#include <iostream>

#include "state_machine.h"

LookingForDraw::LookingForDraw(StateMachine& parent, gfxrecon::decode::VulkanConsumer& found_end) :
    parent_(parent),
    found_end_(found_end)
{
}

void LookingForDraw::Process_vkCmdDraw(const gfxrecon::decode::ApiCallInfo& call_info,
                                       gfxrecon::format::HandleId           commandBuffer,
                                       uint32_t                             vertexCount,
                                       uint32_t                             instanceCount,
                                       uint32_t                             firstVertex,
                                       uint32_t                             firstInstance)
{
    parent_.dump_entry().draws.push_back(call_info.index);
    // Stay in this state and keep accumulating any subsequent draws.
}

void LookingForDraw::Process_vkCmdDrawIndexed(const gfxrecon::decode::ApiCallInfo& call_info,
                                              gfxrecon::format::HandleId           commandBuffer,
                                              uint32_t                             indexCount,
                                              uint32_t                             instanceCount,
                                              uint32_t                             firstIndex,
                                              int32_t                              vertexOffset,
                                              uint32_t                             firstInstance)
{
    parent_.dump_entry().draws.push_back(call_info.index);
    // Stay in this state and keep accumulating any subsequent draws.
}

void LookingForDraw::Process_vkCmdEndRenderPass(const gfxrecon::decode::ApiCallInfo& call_info,
                                                gfxrecon::format::HandleId           commandBuffer)
{
    parent_.dump_entry().render_passes.back().end_block_index = call_info.index;
    parent_.Transition(found_end_);
}

LookingForRenderPass::LookingForRenderPass(StateMachine&                     parent,
                                           gfxrecon::decode::VulkanConsumer& found_begin) :
    parent_(parent),
    found_begin_(found_begin)
{
}

void LookingForRenderPass::Process_vkCmdBeginRenderPass(
const gfxrecon::decode::ApiCallInfo& call_info,
gfxrecon::format::HandleId           commandBuffer,
gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkRenderPassBeginInfo>*
                  pRenderPassBegin,
VkSubpassContents contents)
{
    parent_.dump_entry().render_passes.push_back(
    DumpRenderPass{ .begin_block_index = call_info.index });
    parent_.Transition(found_begin_);
}

void LookingForRenderPass::Process_vkQueueSubmit(
const gfxrecon::decode::ApiCallInfo&                                            call_info,
VkResult                                                                        returnValue,
gfxrecon::format::HandleId                                                      queue,
uint32_t                                                                        submitCount,
gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkSubmitInfo>* pSubmits,
gfxrecon::format::HandleId                                                      fence)
{
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
            if (command_buffer_id != parent_.command_buffer())
            {
                continue;
            }

            parent_.dump_entry().queue_submit_block_index = call_info.index;
            // Could be accept or reject depending on what's been accumulated so far...
            parent_.Done();
            return;
        }
    }
}

LookingForBegin::LookingForBegin(StateMachine&                     parent,
                                 gfxrecon::decode::VulkanConsumer& found_begin) :
    parent_(parent),
    found_begin_(found_begin)
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
    parent_.Transition(found_begin_);
}
