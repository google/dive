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

#pragma once

#include "vulkan/vulkan_core.h"
#include <set>
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <limits>

namespace Dive
{

class GPUTime
{
public:
    static constexpr const char*
    kVulkanVrFrameDelimiterString = "vr-marker,frame_end,type,application";
    struct GpuTimeStatus
    {
        std::string message;
        bool        success = true;
    };

    GPUTime() = default;
    ~GPUTime() = default;

    VkDevice GetDevice() const { return m_device; }

    GpuTimeStatus OnCreateDevice(VkDevice                     device,
                                 const VkAllocationCallbacks* allocator,
                                 float                        timestampPeriod,
                                 PFN_vkCreateQueryPool        pfnCreateQueryPool,
                                 PFN_vkResetQueryPool         pfnResetQueryPool);

    GpuTimeStatus OnDestroyDevice(VkDevice               device,
                                  PFN_vkQueueWaitIdle    pfnQueueWaitIdle,
                                  PFN_vkDestroyQueryPool pfnDestroyQueryPool);

    GpuTimeStatus OnDestroyCommandPool(VkCommandPool commandPool);

    GpuTimeStatus OnAllocateCommandBuffers(const VkCommandBufferAllocateInfo* pAllocateInfo,
                                           VkCommandBuffer*                   pCommandBuffers);

    GpuTimeStatus OnFreeCommandBuffers(uint32_t               commandBufferCount,
                                       const VkCommandBuffer* pCommandBuffers);

    GpuTimeStatus OnResetCommandBuffer(VkCommandBuffer commandBuffer);

    GpuTimeStatus OnResetCommandPool(VkCommandPool commandPool);

    GpuTimeStatus OnBeginCommandBuffer(VkCommandBuffer           commandBuffer,
                                       VkCommandBufferUsageFlags flags,
                                       PFN_vkCmdWriteTimestamp   pfnCmdWriteTimestamp);

    GpuTimeStatus OnEndCommandBuffer(VkCommandBuffer         commandBuffer,
                                     PFN_vkCmdWriteTimestamp pfnCmdWriteTimestamp);

    GpuTimeStatus OnQueueSubmit(uint32_t                  submitCount,
                                const VkSubmitInfo*       pSubmits,
                                PFN_vkDeviceWaitIdle      pfnDeviceWaitIdle,
                                PFN_vkResetQueryPool      pfnResetQueryPool,
                                PFN_vkGetQueryPoolResults pfnGetQueryPoolResults);

    GpuTimeStatus OnGetDeviceQueue2(VkQueue* pQueue);
    GpuTimeStatus OnGetDeviceQueue(VkQueue* pQueue);

    GpuTimeStatus OnCmdInsertDebugUtilsLabelEXT(VkCommandBuffer             commandBuffer,
                                                const VkDebugUtilsLabelEXT* pLabelInfo);

    struct Stats
    {
        double average = 0.0;
        double median = 0.0;
        double min = std::numeric_limits<double>::max();
        double max = std::numeric_limits<double>::lowest();
        double stddev = 0.0;
    };
    Stats       GetStats() const { return m_metrics.GetStatistics(); }
    std::string GetStatsString() const;

private:
    class FrameMetrics
    {
    public:
        FrameMetrics() = default;
        void AddFrameTime(double time);

        Stats GetStatistics() const;

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

    GpuTimeStatus UpdateFrameMetrics(PFN_vkGetQueryPoolResults pfnGetQueryPoolResults);

    FrameMetrics m_metrics;

    std::set<VkQueue>                                      m_queues;
    std::unordered_map<VkCommandBuffer, CommandBufferInfo> m_cmds;
    std::vector<VkCommandBuffer>                           m_frame_cmds;

    VkDevice                     m_device = VK_NULL_HANDLE;
    const VkAllocationCallbacks* m_allocator = nullptr;
    VkQueryPool                  m_query_pool = VK_NULL_HANDLE;
    uint64_t                     m_frame_index = 0;
    uint32_t                     m_timestamp_counter = 0;
    float                        m_timestamp_period = 0.0f;
    bool                         m_valid_frame = true;
};

}  // namespace Dive
