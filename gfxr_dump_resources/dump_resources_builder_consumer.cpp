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

#include "dump_resources_builder_consumer.h"

#include "state_machine.h"

#include "third_party/gfxreconstruct/framework/util/logging.h"

namespace Dive::gfxr
{

DumpResourcesBuilderConsumer::DumpResourcesBuilderConsumer(
std::function<void(DumpEntry)> dump_found_callback) :
    dump_found_callback_(std::move(dump_found_callback))
{
}

void DumpResourcesBuilderConsumer::Process_vkBeginCommandBuffer(
const gfxrecon::decode::ApiCallInfo& call_info,
VkResult                             returnValue,
gfxrecon::format::HandleId           commandBuffer,
gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkCommandBufferBeginInfo>*
pBeginInfo)
{
    GFXRECON_LOG_DEBUG("Process_vkBeginCommandBuffer: commandBuffer=%lu", commandBuffer);
    auto [it, inserted] = incomplete_dumps_
                          .insert_or_assign(commandBuffer,
                                            std::make_unique<StateMachine>(
                                            commandBuffer,
                                            [this, commandBuffer](DumpEntry dump_entry) {
                                                dump_found_callback_(std::move(dump_entry));
                                                incomplete_dumps_.erase(commandBuffer);
                                            },
                                            [this, commandBuffer] {
                                                incomplete_dumps_.erase(commandBuffer);
                                            }));
    if (!inserted)
    {
        GFXRECON_LOG_DEBUG("Command buffer %lu never submitted! Discarding previous state...",
                           commandBuffer);
    }

    StateMachine& state_machine = *it->second;
    state_machine.state().Process_vkBeginCommandBuffer(call_info,
                                                       returnValue,
                                                       commandBuffer,
                                                       pBeginInfo);
}

void DumpResourcesBuilderConsumer::Process_vkCmdBeginRenderPass(
const gfxrecon::decode::ApiCallInfo& call_info,
gfxrecon::format::HandleId           commandBuffer,
gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkRenderPassBeginInfo>*
                  pRenderPassBegin,
VkSubpassContents contents)
{
    GFXRECON_LOG_DEBUG("Process_vkCmdBeginRenderPass: commandBuffer=%lu", commandBuffer);
    InvokeIfFound(commandBuffer, [&](gfxrecon::decode::VulkanConsumer& consumer) {
        consumer.Process_vkCmdBeginRenderPass(call_info, commandBuffer, pRenderPassBegin, contents);
    });
}

void DumpResourcesBuilderConsumer::Process_vkCmdDraw(const gfxrecon::decode::ApiCallInfo& call_info,
                                                     gfxrecon::format::HandleId commandBuffer,
                                                     uint32_t                   vertexCount,
                                                     uint32_t                   instanceCount,
                                                     uint32_t                   firstVertex,
                                                     uint32_t                   firstInstance)
{
    GFXRECON_LOG_DEBUG("Process_vkCmdDraw: commandBuffer=%lu", commandBuffer);
    InvokeIfFound(commandBuffer, [&](gfxrecon::decode::VulkanConsumer& consumer) {
        consumer.Process_vkCmdDraw(call_info,
                                   commandBuffer,
                                   vertexCount,
                                   instanceCount,
                                   firstVertex,
                                   firstInstance);
    });
}

void DumpResourcesBuilderConsumer::Process_vkCmdDrawIndexed(
const gfxrecon::decode::ApiCallInfo& call_info,
gfxrecon::format::HandleId           commandBuffer,
uint32_t                             indexCount,
uint32_t                             instanceCount,
uint32_t                             firstIndex,
int32_t                              vertexOffset,
uint32_t                             firstInstance)
{
    GFXRECON_LOG_DEBUG("Process_vkCmdDrawIndexed: commandBuffer=%lu", commandBuffer);
    InvokeIfFound(commandBuffer, [&](gfxrecon::decode::VulkanConsumer& consumer) {
        consumer.Process_vkCmdDrawIndexed(call_info,
                                          commandBuffer,
                                          indexCount,
                                          instanceCount,
                                          firstIndex,
                                          vertexOffset,
                                          firstInstance);
    });
}

void DumpResourcesBuilderConsumer::Process_vkCmdEndRenderPass(
const gfxrecon::decode::ApiCallInfo& call_info,
gfxrecon::format::HandleId           commandBuffer)
{
    GFXRECON_LOG_DEBUG("Process_vkCmdEndRenderPass: commandBuffer=%lu", commandBuffer);
    InvokeIfFound(commandBuffer, [&](gfxrecon::decode::VulkanConsumer& consumer) {
        consumer.Process_vkCmdEndRenderPass(call_info, commandBuffer);
    });
}

void DumpResourcesBuilderConsumer::Process_vkQueueSubmit(
const gfxrecon::decode::ApiCallInfo&                                            call_info,
VkResult                                                                        returnValue,
gfxrecon::format::HandleId                                                      queue,
uint32_t                                                                        submitCount,
gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkSubmitInfo>* pSubmits,
gfxrecon::format::HandleId                                                      fence)
{
    GFXRECON_LOG_DEBUG("Process_vkQueueSubmit");
    for (uint32_t submit_index = 0; submit_index < submitCount; submit_index++)
    {
        const gfxrecon::decode::Decoded_VkSubmitInfo&
        submit = pSubmits->GetMetaStructPointer()[submit_index];
        for (uint32_t command_buffer_index = 0;
             command_buffer_index < pSubmits->GetPointer()->commandBufferCount;
             command_buffer_index++)
        {
            gfxrecon::format::HandleId command_buffer_id = submit.pCommandBuffers
                                                           .GetPointer()[command_buffer_index];
            GFXRECON_LOG_DEBUG("... for commandBuffer=%lu", command_buffer_id);
            InvokeIfFound(command_buffer_id, [&](gfxrecon::decode::VulkanConsumer& consumer) {
                consumer
                .Process_vkQueueSubmit(call_info, returnValue, queue, submitCount, pSubmits, fence);
            });
        }
    }
}

void DumpResourcesBuilderConsumer::InvokeIfFound(
gfxrecon::format::HandleId                                    command_buffer,
const std::function<void(gfxrecon::decode::VulkanConsumer&)>& function)
{
    auto it = incomplete_dumps_.find(command_buffer);
    if (it == incomplete_dumps_.end())
    {
        GFXRECON_LOG_DEBUG("Command buffer %lu never started! Ignoring...", command_buffer);
        return;
    }

    StateMachine& state_machine = *it->second;
    function(state_machine.state());
}

}  // namespace Dive::gfxr
