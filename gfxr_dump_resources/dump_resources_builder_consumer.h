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

#include <functional>
#include <memory>
#include <unordered_map>

#include "dump_entry.h"
#include "state_machine.h"

#include "gfxreconstruct/framework/decode/api_decoder.h"
#include "gfxreconstruct/framework/decode/struct_pointer_decoder.h"
#include "gfxreconstruct/framework/format/format.h"
#include "gfxreconstruct/framework/generated/generated_vulkan_consumer.h"
#include "gfxreconstruct/framework/generated/generated_vulkan_struct_decoders.h"

namespace Dive::gfxr
{

// Processes Vulkan commands originating from a .gfxr, looking for candidates for
// `--dump-resources`. This mostly just tracks whether vkBeginCommandBuffer has been called; most of
// the logic is in handled in a state of the state machine (see StateMachine).
class DumpResourcesBuilderConsumer : public gfxrecon::decode::VulkanConsumer
{
public:
    // `dump_found_callback` is run when a complete DumpEntry is found which is suitable for being
    // used with `--dump-resources`.
    DumpResourcesBuilderConsumer(std::function<void(DumpEntry)> dump_found_callback);

    // Start tracking the command buffer. If the command buffer already has state (likely
    // QueueSubmit was not called), then the command buffer state is reset.
    void Process_vkBeginCommandBuffer(
    const gfxrecon::decode::ApiCallInfo& call_info,
    VkResult                             returnValue,
    gfxrecon::format::HandleId           commandBuffer,
    gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkCommandBufferBeginInfo>*
    pBeginInfo) override;

    void Process_vkCmdBeginRenderPass(
    const gfxrecon::decode::ApiCallInfo& call_info,
    gfxrecon::format::HandleId           commandBuffer,
    gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkRenderPassBeginInfo>*
                      pRenderPassBegin,
    VkSubpassContents contents) override;

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

    void Process_vkQueueSubmit(
    const gfxrecon::decode::ApiCallInfo&                                            call_info,
    VkResult                                                                        returnValue,
    gfxrecon::format::HandleId                                                      queue,
    uint32_t                                                                        submitCount,
    gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkSubmitInfo>* pSubmits,
    gfxrecon::format::HandleId                                                      fence) override;

private:
    // Function run when a complete dump entry has been formed. This is ready to be written to disk,
    // etc.
    std::function<void(DumpEntry)> dump_found_callback_;
    // Incomplete dumps for each command buffer. Each command buffer is tracked independently in
    // case commands are interleaved. std::unique_ptr is used for pointer stability.
    std::unordered_map<gfxrecon::format::HandleId, std::unique_ptr<StateMachine>> incomplete_dumps_;
};

}  // namespace Dive::gfxr
