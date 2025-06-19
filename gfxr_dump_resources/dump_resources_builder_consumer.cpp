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

#include <iostream>
#include "state_machine.h"

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
    std::cerr << "Process_vkBeginCommandBuffer: commandBuffer=" << commandBuffer << '\n';

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
        std::cerr << "Command buffer " << commandBuffer
                  << " never submitted! Discarding previous state...\n";
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
    std::cerr << "Process_vkCmdBeginRenderPass: commandBuffer=" << commandBuffer << '\n';
    auto it = incomplete_dumps_.find(commandBuffer);
    if (it == incomplete_dumps_.end())
    {
        std::cerr << "Command buffer " << commandBuffer << " never started! Ignoring...\n";
        return;
    }

    StateMachine& state_machine = *it->second;
    state_machine.state().Process_vkCmdBeginRenderPass(call_info,
                                                       commandBuffer,
                                                       pRenderPassBegin,
                                                       contents);
}

void DumpResourcesBuilderConsumer::Process_vkCmdDraw(const gfxrecon::decode::ApiCallInfo& call_info,
                                                     gfxrecon::format::HandleId commandBuffer,
                                                     uint32_t                   vertexCount,
                                                     uint32_t                   instanceCount,
                                                     uint32_t                   firstVertex,
                                                     uint32_t                   firstInstance)
{
    std::cerr << "Process_vkCmdDraw: commandBuffer=" << commandBuffer << '\n';
    auto it = incomplete_dumps_.find(commandBuffer);
    if (it == incomplete_dumps_.end())
    {
        std::cerr << "Command buffer " << commandBuffer << " never started! Ignoring...\n";
        return;
    }

    StateMachine& state_machine = *it->second;
    state_machine.state().Process_vkCmdDraw(call_info,
                                            commandBuffer,
                                            vertexCount,
                                            instanceCount,
                                            firstVertex,
                                            firstInstance);
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
    std::cerr << "Process_vkCmdDrawIndexed: commandBuffer=" << commandBuffer << '\n';
    auto it = incomplete_dumps_.find(commandBuffer);
    if (it == incomplete_dumps_.end())
    {
        std::cerr << "Command buffer " << commandBuffer << " never started! Ignoring...\n";
        return;
    }

    StateMachine& state_machine = *it->second;
    state_machine.state().Process_vkCmdDrawIndexed(call_info,
                                                   commandBuffer,
                                                   indexCount,
                                                   instanceCount,
                                                   firstIndex,
                                                   vertexOffset,
                                                   firstInstance);
}

void DumpResourcesBuilderConsumer::Process_vkCmdEndRenderPass(
const gfxrecon::decode::ApiCallInfo& call_info,
gfxrecon::format::HandleId           commandBuffer)
{
    std::cerr << "Process_vkCmdEndRenderPass: commandBuffer=" << commandBuffer << '\n';
    auto it = incomplete_dumps_.find(commandBuffer);
    if (it == incomplete_dumps_.end())
    {
        std::cerr << "Command buffer " << commandBuffer << " never started! Ignoring...\n";
        return;
    }

    StateMachine& state_machine = *it->second;
    state_machine.state().Process_vkCmdEndRenderPass(call_info, commandBuffer);
}

void DumpResourcesBuilderConsumer::Process_vkQueueSubmit(
const gfxrecon::decode::ApiCallInfo&                                            call_info,
VkResult                                                                        returnValue,
gfxrecon::format::HandleId                                                      queue,
uint32_t                                                                        submitCount,
gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkSubmitInfo>* pSubmits,
gfxrecon::format::HandleId                                                      fence)
{
    std::cerr << "Process_vkQueueSubmit\n";
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
            std::cerr << "... for commandBuffer=" << command_buffer_id << '\n';
            if (auto it = incomplete_dumps_.find(command_buffer_id); it != incomplete_dumps_.end())
            {
                StateMachine& state_machine = *it->second;
                state_machine.state()
                .Process_vkQueueSubmit(call_info, returnValue, queue, submitCount, pSubmits, fence);
            }
            else
            {
                std::cerr << "Command buffer " << command_buffer_id << " never started!\n";
            }
        }
    }
}

}  // namespace Dive::gfxr
