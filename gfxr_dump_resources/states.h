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

#include "third_party/gfxreconstruct/framework/decode/api_decoder.h"
#include "third_party/gfxreconstruct/framework/decode/struct_pointer_decoder.h"
#include "third_party/gfxreconstruct/framework/format/format.h"
#include "third_party/gfxreconstruct/framework/generated/generated_vulkan_consumer.h"
#include "third_party/gfxreconstruct/framework/generated/generated_vulkan_struct_decoders.h"

// This file contains the states used by the state machine. See StateMachine for more docs.
//
// Each state has a weak reference to the owning state machine. This allows the state machine to
// hold shared state that all states can reference, such as the current dumpable.

namespace Dive::gfxr
{

// forward decl to break recursive includes
class StateMachine;

// Found vkBeginCommandBuffer, vkCmdBeginRenderPass.
// Looking for vkCmdDraw or vkCmdEndRenderPass.
class LookingForDraw : public gfxrecon::decode::VulkanConsumer
{
public:
    // `found_end` is the state to transition to when vkCmdEndRenderPass is found.
    LookingForDraw(StateMachine& parent, gfxrecon::decode::VulkanConsumer& found_end);

    void Process_vkCmdDraw(const gfxrecon::decode::ApiCallInfo& call_info,
                           gfxrecon::format::HandleId           commandBuffer,
                           uint32_t                             vertexCount,
                           uint32_t                             instanceCount,
                           uint32_t                             firstVertex,
                           uint32_t                             firstInstance) override;

    void Process_vkCmdDrawIndexed(const gfxrecon::decode::ApiCallInfo& call_info,
                                  gfxrecon::format::HandleId           commandBuffer,
                                  uint32_t                             indexCount,
                                  uint32_t                             instanceCount,
                                  uint32_t                             firstIndex,
                                  int32_t                              vertexOffset,
                                  uint32_t                             firstInstance) override;

    void Process_vkCmdEndRenderPass(const gfxrecon::decode::ApiCallInfo& call_info,
                                    gfxrecon::format::HandleId           commandBuffer) override;

    // TODO: Subpass
    // TODO: Other draws

private:
    StateMachine&                     parent_;
    gfxrecon::decode::VulkanConsumer& found_end_;
};

// Found vkBeginCommandBuffer.
// Looking for vkCmdBeginRenderPass or vkQueueSubmit.
class LookingForBeginRenderPass : public gfxrecon::decode::VulkanConsumer
{
public:
    // `found_begin` is the state to transition to when vkCmdBeginRenderPass is found.
    LookingForBeginRenderPass(StateMachine& parent, gfxrecon::decode::VulkanConsumer& found_begin);

    // Accept or reject depending on if the dumpable is complete.
    void Process_vkQueueSubmit(
    const gfxrecon::decode::ApiCallInfo&                                            call_info,
    VkResult                                                                        returnValue,
    gfxrecon::format::HandleId                                                      queue,
    uint32_t                                                                        submitCount,
    gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkSubmitInfo>* pSubmits,
    gfxrecon::format::HandleId                                                      fence) override;

    void Process_vkCmdBeginRenderPass(
    const gfxrecon::decode::ApiCallInfo& call_info,
    gfxrecon::format::HandleId           commandBuffer,
    gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkRenderPassBeginInfo>*
                      pRenderPassBegin,
    VkSubpassContents contents) override;

private:
    StateMachine&                     parent_;
    gfxrecon::decode::VulkanConsumer& found_begin_;
};

// Looking for vkBeginCommandBuffer. This is the first state in the state machine.
class LookingForBeginCommandBuffer : public gfxrecon::decode::VulkanConsumer
{
public:
    // `found_begin` is the state to transition to when vkBeginCommandBuffer is found.
    LookingForBeginCommandBuffer(StateMachine&                     parent,
                                 gfxrecon::decode::VulkanConsumer& found_begin);

    void Process_vkBeginCommandBuffer(
    const gfxrecon::decode::ApiCallInfo& call_info,
    VkResult                             returnValue,
    gfxrecon::format::HandleId           commandBuffer,
    gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkCommandBufferBeginInfo>*
    pBeginInfo) override;

private:
    StateMachine&                     parent_;
    gfxrecon::decode::VulkanConsumer& found_begin_;
};

}  // namespace Dive::gfxr
