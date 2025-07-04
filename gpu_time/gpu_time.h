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

#include "absl/status/status.h"
#include "vulkan/vulkan_core.h"
#include <set>
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>

namespace Dive
{

class GPUTime
{
public:
    GPUTime();
    ~GPUTime();

    VkDevice GetDevice() const { return m_device; }

    absl::Status OnCreateDevice(VkDevice                     device,
                                const VkAllocationCallbacks* allocator,
                                float                        timestampPeriod,
                                PFN_vkCreateQueryPool        pfnCreateQueryPool,
                                PFN_vkResetQueryPool         pfnResetQueryPool);

    absl::Status OnDestroyDevice(VkDevice               device,
                                 PFN_vkQueueWaitIdle    pfnQueueWaitIdle,
                                 PFN_vkDestroyQueryPool pfnDestroyQueryPool);

    absl::Status OnDestroyCommandPool(VkCommandPool commandPool);

    absl::Status OnAllocateCommandBuffers(const VkCommandBufferAllocateInfo* pAllocateInfo,
                                          VkCommandBuffer*                   pCommandBuffers);

    absl::Status OnFreeCommandBuffers(uint32_t               commandBufferCount,
                                      const VkCommandBuffer* pCommandBuffers);

    absl::Status OnResetCommandBuffer(VkCommandBuffer commandBuffer);

    absl::Status OnResetCommandPool(VkCommandPool commandPool);

    absl::Status OnBeginCommandBuffer(VkCommandBuffer           commandBuffer,
                                      VkCommandBufferUsageFlags flags,
                                      PFN_vkCmdWriteTimestamp   pfnCmdWriteTimestamp);

    absl::Status OnEndCommandBuffer(VkCommandBuffer         commandBuffer,
                                    PFN_vkCmdWriteTimestamp pfnCmdWriteTimestamp);

    absl::Status OnQueueSubmit(uint32_t                  submitCount,
                               const VkSubmitInfo*       pSubmits,
                               PFN_vkDeviceWaitIdle      pfnDeviceWaitIdle,
                               PFN_vkResetQueryPool      pfnResetQueryPool,
                               PFN_vkGetQueryPoolResults pfnGetQueryPoolResults);

    absl::Status OnGetDeviceQueue2(VkQueue* pQueue);
    absl::Status OnGetDeviceQueue(VkQueue* pQueue);

    absl::Status OnCmdInsertDebugUtilsLabelEXT(VkCommandBuffer             commandBuffer,
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
    std::string GetStatsString();

private:
    class FrameMetrics
    {
    public:
        FrameMetrics() = default;
        void AddFrameTime(double time);

        Stats GetStatistics() const;
        std::string GetStatsString(const Stats& stats);

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

    absl::Status UpdateFrameMetrics(PFN_vkGetQueryPoolResults pfnGetQueryPoolResults);

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
};

}  // namespace Dive
