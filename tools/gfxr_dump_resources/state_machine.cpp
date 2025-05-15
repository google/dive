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

#include "state_machine.h"

#include <functional>
#include <iostream>

namespace Dive::tools
{

StateMachine::StateMachine(gfxrecon::format::HandleId command_buffer,
                           AcceptingFunction          accept,
                           RejectingFunction          reject) :
    command_buffer_(command_buffer),
    accept_(accept),
    reject_(reject),
    find_draw_(*this, find_render_pass_),
    find_render_pass_(*this, find_draw_),
    begin_state_(*this, find_render_pass_),
    state_(&begin_state_)
{
}

void StateMachine::Transition(gfxrecon::decode::VulkanConsumer& new_state)
{
    state_ = &new_state;
    // TODO call TransitionTo
}

DumpEntry& StateMachine::dump_entry()
{
    return dump_entry_;
}

gfxrecon::format::HandleId& StateMachine::command_buffer()
{
    return command_buffer_;
}

void StateMachine::Done()
{
    if (dump_entry_.IsComplete())
    {
        std::cerr << "Accept! ID=" << command_buffer_ << '\n';
        accept_(std::move(dump_entry_));
    }
    else
    {
        std::cerr << "Reject! ID=" << command_buffer_ << '\n';
        reject_();
    }
}

void StateMachine::Process_vkBeginCommandBuffer(
const gfxrecon::decode::ApiCallInfo& call_info,
VkResult                             returnValue,
gfxrecon::format::HandleId           commandBuffer,
gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkCommandBufferBeginInfo>*
pBeginInfo)
{
    // Consumer will never be called if state != begin_state_
    assert(state_ == &begin_state_);
    state_->Process_vkBeginCommandBuffer(call_info, returnValue, commandBuffer, pBeginInfo);
}

void StateMachine::Process_vkCmdBeginRenderPass(
const gfxrecon::decode::ApiCallInfo& call_info,
gfxrecon::format::HandleId           commandBuffer,
gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkRenderPassBeginInfo>*
                  pRenderPassBegin,
VkSubpassContents contents)
{
    state_->Process_vkCmdBeginRenderPass(call_info, commandBuffer, pRenderPassBegin, contents);
}

void StateMachine::Process_vkCmdDraw(const gfxrecon::decode::ApiCallInfo& call_info,
                                     gfxrecon::format::HandleId           commandBuffer,
                                     uint32_t                             vertexCount,
                                     uint32_t                             instanceCount,
                                     uint32_t                             firstVertex,
                                     uint32_t                             firstInstance)
{
    state_->Process_vkCmdDraw(call_info,
                              commandBuffer,
                              vertexCount,
                              instanceCount,
                              firstVertex,
                              firstInstance);
}

void StateMachine::Process_vkCmdDrawIndexed(const gfxrecon::decode::ApiCallInfo& call_info,
                                            gfxrecon::format::HandleId           commandBuffer,
                                            uint32_t                             indexCount,
                                            uint32_t                             instanceCount,
                                            uint32_t                             firstIndex,
                                            int32_t                              vertexOffset,
                                            uint32_t                             firstInstance)
{
    state_->Process_vkCmdDrawIndexed(call_info,
                                     commandBuffer,
                                     indexCount,
                                     instanceCount,
                                     firstIndex,
                                     vertexOffset,
                                     firstInstance);
}

void StateMachine::Process_vkCmdEndRenderPass(const gfxrecon::decode::ApiCallInfo& call_info,
                                              gfxrecon::format::HandleId           commandBuffer)
{
    state_->Process_vkCmdEndRenderPass(call_info, commandBuffer);
}

void StateMachine::Process_vkQueueSubmit(
const gfxrecon::decode::ApiCallInfo&                                            call_info,
VkResult                                                                        returnValue,
gfxrecon::format::HandleId                                                      queue,
uint32_t                                                                        submitCount,
gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkSubmitInfo>* pSubmits,
gfxrecon::format::HandleId                                                      fence)
{
    state_->Process_vkQueueSubmit(call_info, returnValue, queue, submitCount, pSubmits, fence);
}

}  // namespace Dive::tools
