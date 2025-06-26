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
#include <set>

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

#include <vector>

class FrameMetrics
{
public:
    struct Stats
    {
        double average = 0.0;
        double median = 0.0;
        double min = std::numeric_limits<double>::max();
        double max = std::numeric_limits<double>::lowest();
        double stddev = 0.0;
    };

    FrameMetrics() = default;
    void AddFrameTime(double time);

    Stats GetStatistics() const;

    void PrintStats(const Stats& stats);

private:
    double CalculateAverage() const;
    double CalculateMedian() const;
    double CalculateStdDev(double average) const;

    std::deque<double> m_frame_data;
};

struct CommandBufferInfo
{
    void Reset()
    {
        is_frameboundary = false;
        usage_one_submit = false;
    }
    const static uint32_t kInvalidTimeStampOffset = static_cast<uint32_t>(-1);

    VkCommandPool pool = VK_NULL_HANDLE;
    uint32_t      timestamp_offset = kInvalidTimeStampOffset;
    bool          is_frameboundary = false;
    bool          usage_one_submit = false;
};

class DiveVulkanReplayConsumer : public VulkanReplayConsumer
{
public:
    DiveVulkanReplayConsumer(std::shared_ptr<application::Application> application,
                             const VulkanReplayOptions&                options);

    ~DiveVulkanReplayConsumer() override;

    virtual void Process_vkCreateDevice(
    const ApiCallInfo&                                   call_info,
    VkResult                                             returnValue,
    format::HandleId                                     physicalDevice,
    StructPointerDecoder<Decoded_VkDeviceCreateInfo>*    pCreateInfo,
    StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
    HandlePointerDecoder<VkDevice>*                      pDevice) override;

    virtual void Process_vkDestroyDevice(
    const ApiCallInfo&                                   call_info,
    format::HandleId                                     device,
    StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator) override;

    virtual void Process_vkDestroyCommandPool(
    const ApiCallInfo&                                   call_info,
    format::HandleId                                     device,
    format::HandleId                                     commandPool,
    StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator) override;

    virtual void Process_vkAllocateCommandBuffers(
    const ApiCallInfo&                                         call_info,
    VkResult                                                   returnValue,
    format::HandleId                                           device,
    StructPointerDecoder<Decoded_VkCommandBufferAllocateInfo>* pAllocateInfo,
    HandlePointerDecoder<VkCommandBuffer>*                     pCommandBuffers) override;

    virtual void Process_vkFreeCommandBuffers(
    const ApiCallInfo&                     call_info,
    format::HandleId                       device,
    format::HandleId                       commandPool,
    uint32_t                               commandBufferCount,
    HandlePointerDecoder<VkCommandBuffer>* pCommandBuffers) override;

    virtual void Process_vkResetCommandBuffer(const ApiCallInfo&        call_info,
                                              VkResult                  returnValue,
                                              format::HandleId          commandBuffer,
                                              VkCommandBufferResetFlags flags) override;

    virtual void Process_vkResetCommandPool(const ApiCallInfo&      call_info,
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

    virtual void Process_vkBeginCommandBuffer(
    const ApiCallInfo&                                      call_info,
    VkResult                                                returnValue,
    format::HandleId                                        commandBuffer,
    StructPointerDecoder<Decoded_VkCommandBufferBeginInfo>* pBeginInfo) override;

    virtual void Process_vkEndCommandBuffer(const ApiCallInfo& call_info,
                                            VkResult           returnValue,
                                            format::HandleId   commandBuffer) override;

    virtual void Process_vkGetDeviceQueue2(
    const ApiCallInfo&                                call_info,
    format::HandleId                                  device,
    StructPointerDecoder<Decoded_VkDeviceQueueInfo2>* pQueueInfo,
    HandlePointerDecoder<VkQueue>*                    pQueue) override;

    virtual void Process_vkGetDeviceQueue(const ApiCallInfo&             call_info,
                                          format::HandleId               device,
                                          uint32_t                       queueFamilyIndex,
                                          uint32_t                       queueIndex,
                                          HandlePointerDecoder<VkQueue>* pQueue) override;

    virtual void Process_vkCmdInsertDebugUtilsLabelEXT(
    const ApiCallInfo&                                  call_info,
    format::HandleId                                    commandBuffer,
    StructPointerDecoder<Decoded_VkDebugUtilsLabelEXT>* pLabelInfo) override;

private:
    // Helper function to destroy the query pool
    void DestroyQueryPool();
    void UpdateFrameMetrics(VkDevice device);

    FrameMetrics metrics_;

    std::set<VkQueue>                                      queues_;
    std::unordered_map<VkCommandBuffer, CommandBufferInfo> cmds_;
    std::vector<VkCommandBuffer>                           frame_cmds_;

    VkAllocationCallbacks* allocator_;
    VkQueryPool            query_pool_;
    VkDevice               device_;
    uint64_t               frame_index_ = 0;
    uint32_t               timestamp_counter_;
    float                  timestamp_period_;
};

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)

#endif  // GFXRECON_DECODE_VULKAN_REPLAY_CONSUMER_BASE_H
