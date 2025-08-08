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
#include <atomic>

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

    struct SubmitStatus
    {
        GpuTimeStatus gpu_time_status;
        bool          contains_frame_boundary = false;
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

    SubmitStatus OnQueueSubmit(uint32_t                  submitCount,
                               const VkSubmitInfo*       pSubmits,
                               PFN_vkDeviceWaitIdle      pfnDeviceWaitIdle,
                               PFN_vkResetQueryPool      pfnResetQueryPool,
                               PFN_vkGetQueryPoolResults pfnGetQueryPoolResults);

    GpuTimeStatus OnGetDeviceQueue2(VkQueue* pQueue);
    GpuTimeStatus OnGetDeviceQueue(VkQueue* pQueue);

    GpuTimeStatus OnCmdInsertDebugUtilsLabelEXT(VkCommandBuffer             commandBuffer,
                                                const VkDebugUtilsLabelEXT* pLabelInfo);

    GpuTimeStatus OnCmdBeginRenderPass(VkCommandBuffer         commandBuffer,
                                       PFN_vkCmdWriteTimestamp pfnCmdWriteTimestamp);

    GpuTimeStatus OnCmdEndRenderPass(VkCommandBuffer         commandBuffer,
                                     PFN_vkCmdWriteTimestamp pfnCmdWriteTimestamp);

    GpuTimeStatus OnCmdBeginRenderPass2(VkCommandBuffer         commandBuffer,
                                        PFN_vkCmdWriteTimestamp pfnCmdWriteTimestamp);

    GpuTimeStatus OnCmdEndRenderPass2(VkCommandBuffer         commandBuffer,
                                      PFN_vkCmdWriteTimestamp pfnCmdWriteTimestamp);

    struct Stats
    {
        double average = 0.0;
        double median = 0.0;
        double min = std::numeric_limits<double>::max();
        double max = std::numeric_limits<double>::lowest();
        double stddev = 0.0;
    };
    Stats GetFrameTimeStats() const { return m_metrics.GetFrameTimeStats(); }
    Stats GetFrameCmdTimeStats(size_t index) const { return m_metrics.GetFrameCmdTimeStats(index); }
    Stats GetFrameRenderPassTimeStats(size_t index) const
    {
        return m_metrics.GetFrameRenderPassTimeStats(index);
    }
    size_t GetCmdRenderPassCount(size_t index) const
    {
        return m_metrics.GetCmdRenderPassCount(index);
    }
    std::string GetStatsString() const;

private:
    class FrameMetrics
    {
    public:
        static constexpr size_t kInvalidRenderPassCount = static_cast<size_t>(-1);
        FrameMetrics() = default;
        void   AddFrameData(double                     frame_time,
                            const std::vector<double>& cmd_time_vec,
                            const std::vector<double>& renderpass_time_vec,
                            const std::vector<size_t>& cmd_renderpass_count_vec);
        Stats  GetFrameTimeStats() const;
        Stats  GetFrameCmdTimeStats(size_t index) const;
        Stats  GetFrameRenderPassTimeStats(size_t index) const;
        size_t GetFrameCmdCount() const;
        size_t GetFrameRenderPassCount() const;
        size_t GetCmdRenderPassCount(size_t index) const;

    private:
        Stats  GetStatistics(const std::deque<double>& data) const;
        double CalculateAverage(const std::deque<double>& data) const;
        double CalculateMedian(const std::deque<double>& data) const;
        double CalculateStdDev(const std::deque<double>& data, double average) const;
        void   Reset();

        std::deque<double>              m_frame_time;
        std::vector<size_t>             m_cmd_renderpass_count_vec;
        std::vector<std::deque<double>> m_cmd_time_vec;
        std::vector<std::deque<double>> m_renderpass_time_vec;
    };

    class TimeStampSlotAllocator
    {
    public:
        static constexpr uint32_t kSlotsPerBlock = 64;
        static constexpr uint32_t kTotalSlots = kSlotsPerBlock * 4;
        static constexpr uint32_t kNumBlocks = kTotalSlots / kSlotsPerBlock;
        static constexpr uint32_t kInvalidIndex = static_cast<uint32_t>(-1);
        static constexpr uint32_t kFrameMetricsLimit = 1000;

        TimeStampSlotAllocator();
        void     Reset();
        uint32_t AllocateSlot();
        void     FreeSlots(const std::vector<uint32_t>& slots);

    private:
        std::atomic<size_t>   m_masks[kNumBlocks];
        std::atomic<uint32_t> m_cur = 0;
    };

    struct CommandBufferInfo
    {
        void Reset()
        {
            is_frameboundary = false;
            usage_one_submit = false;
            reusable = false;
        }
        const static uint32_t kInvalidTimeStampOffset = static_cast<uint32_t>(-1);

        std::vector<uint32_t> renderpass_slots;
        VkCommandPool         pool = VK_NULL_HANDLE;
        uint32_t              begin_timestamp_offset = kInvalidTimeStampOffset;
        uint32_t              end_timestamp_offset = kInvalidTimeStampOffset;
        bool                  is_frameboundary = false;
        bool                  usage_one_submit = false;
        bool                  reusable = false;
    };

    GpuTimeStatus UpdateFrameMetrics(PFN_vkGetQueryPoolResults pfnGetQueryPoolResults);

    FrameMetrics m_metrics;

    std::set<VkQueue>                                      m_queues;
    std::unordered_map<VkCommandBuffer, CommandBufferInfo> m_cmds;
    std::vector<VkCommandBuffer>                           m_frame_cmds;
    TimeStampSlotAllocator                                 m_timestamp_allocator;

    VkDevice                     m_device = VK_NULL_HANDLE;
    const VkAllocationCallbacks* m_allocator = nullptr;
    VkQueryPool                  m_query_pool = VK_NULL_HANDLE;
    uint64_t                     m_frame_index = 0;
    uint32_t                     m_timestamp_counter = 0;
    float                        m_timestamp_period = 0.0f;
    bool                         m_valid_frame = true;
};

}  // namespace Dive