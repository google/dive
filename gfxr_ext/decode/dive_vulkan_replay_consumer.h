/*
Copyright 2025 Google Inc.

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

#ifndef GFXRECON_DECODE_VULKAN_DIVE_CONSUMER_H
#define GFXRECON_DECODE_VULKAN_DIVE_CONSUMER_H

#include "generated/generated_vulkan_replay_consumer.h"
#include "gpu_time/gpu_time.h"
#include <set>
#include <vector>

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

class DiveVulkanReplayConsumer : public VulkanReplayConsumer
{
public:
    DiveVulkanReplayConsumer(std::shared_ptr<application::Application> application,
                             const VulkanReplayOptions&                options);

    ~DiveVulkanReplayConsumer() override;

    void Process_vkCreateDevice(const ApiCallInfo&                                   call_info,
                                VkResult                                             returnValue,
                                format::HandleId                                     physicalDevice,
                                StructPointerDecoder<Decoded_VkDeviceCreateInfo>*    pCreateInfo,
                                StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
                                HandlePointerDecoder<VkDevice>* pDevice) override;

    void Process_vkDestroyDevice(
    const ApiCallInfo&                                   call_info,
    format::HandleId                                     device,
    StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator) override;

    void Process_vkDestroyCommandPool(
    const ApiCallInfo&                                   call_info,
    format::HandleId                                     device,
    format::HandleId                                     commandPool,
    StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator) override;

    void Process_vkAllocateCommandBuffers(
    const ApiCallInfo&                                         call_info,
    VkResult                                                   returnValue,
    format::HandleId                                           device,
    StructPointerDecoder<Decoded_VkCommandBufferAllocateInfo>* pAllocateInfo,
    HandlePointerDecoder<VkCommandBuffer>*                     pCommandBuffers) override;

    void Process_vkFreeCommandBuffers(
    const ApiCallInfo&                     call_info,
    format::HandleId                       device,
    format::HandleId                       commandPool,
    uint32_t                               commandBufferCount,
    HandlePointerDecoder<VkCommandBuffer>* pCommandBuffers) override;

    void Process_vkResetCommandBuffer(const ApiCallInfo&        call_info,
                                      VkResult                  returnValue,
                                      format::HandleId          commandBuffer,
                                      VkCommandBufferResetFlags flags) override;

    void Process_vkResetCommandPool(const ApiCallInfo&      call_info,
                                    VkResult                returnValue,
                                    format::HandleId        device,
                                    format::HandleId        commandPool,
                                    VkCommandPoolResetFlags flags) override;

    void Process_vkQueueSubmit(const ApiCallInfo&                          call_info,
                               VkResult                                    returnValue,
                               format::HandleId                            queue,
                               uint32_t                                    submitCount,
                               StructPointerDecoder<Decoded_VkSubmitInfo>* pSubmits,
                               format::HandleId                            fence) override;

    void Process_vkBeginCommandBuffer(
    const ApiCallInfo&                                      call_info,
    VkResult                                                returnValue,
    format::HandleId                                        commandBuffer,
    StructPointerDecoder<Decoded_VkCommandBufferBeginInfo>* pBeginInfo) override;

    void Process_vkEndCommandBuffer(const ApiCallInfo& call_info,
                                    VkResult           returnValue,
                                    format::HandleId   commandBuffer) override;

    void Process_vkGetDeviceQueue2(const ApiCallInfo&                                call_info,
                                   format::HandleId                                  device,
                                   StructPointerDecoder<Decoded_VkDeviceQueueInfo2>* pQueueInfo,
                                   HandlePointerDecoder<VkQueue>* pQueue) override;

    void Process_vkGetDeviceQueue(const ApiCallInfo&             call_info,
                                  format::HandleId               device,
                                  uint32_t                       queueFamilyIndex,
                                  uint32_t                       queueIndex,
                                  HandlePointerDecoder<VkQueue>* pQueue) override;

    void Process_vkCmdInsertDebugUtilsLabelEXT(
    const ApiCallInfo&                                  call_info,
    format::HandleId                                    commandBuffer,
    StructPointerDecoder<Decoded_VkDebugUtilsLabelEXT>* pLabelInfo) override;

    void Process_vkCmdBeginRenderPass(
    const ApiCallInfo&                                   call_info,
    format::HandleId                                     commandBuffer,
    StructPointerDecoder<Decoded_VkRenderPassBeginInfo>* pRenderPassBegin,
    VkSubpassContents                                    contents) override;

    void Process_vkCmdEndRenderPass(const ApiCallInfo& call_info,
                                    format::HandleId   commandBuffer) override;

    void Process_vkCmdBeginRenderPass2(
    const ApiCallInfo&                                   call_info,
    format::HandleId                                     commandBuffer,
    StructPointerDecoder<Decoded_VkRenderPassBeginInfo>* pRenderPassBegin,
    StructPointerDecoder<Decoded_VkSubpassBeginInfo>*    pSubpassBeginInfo) override;

    void Process_vkCmdEndRenderPass2(
    const ApiCallInfo&                              call_info,
    format::HandleId                                commandBuffer,
    StructPointerDecoder<Decoded_VkSubpassEndInfo>* pSubpassEndInfo) override;

    void SetEnableGPUTime(bool enable) { enable_gpu_time_ = enable; }

private:
    Dive::GPUTime gpu_time_;
    bool          enable_gpu_time_ = false;
};

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)

#endif  // GFXRECON_DECODE_VULKAN_REPLAY_CONSUMER_BASE_H
