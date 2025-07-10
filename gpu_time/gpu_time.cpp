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

#include "gpu_time.h"

#include <iomanip>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <limits>
#include <sstream>
#include <cstring>

static constexpr uint32_t kQueryCount = 256;
static constexpr uint32_t kFrameMetricsLimit = 1000;

namespace Dive
{

// FrameMetrics
void GPUTime::FrameMetrics::AddFrameTime(double time)
{
    if (m_frame_data.size() == kFrameMetricsLimit)
    {
        m_frame_data.pop_front();
    }
    m_frame_data.push_back(time);
}

GPUTime::Stats GPUTime::FrameMetrics::GetStatistics() const
{
    Stats stats;
    stats.min = std::numeric_limits<double>::max();     // Initialize min to max value
    stats.max = std::numeric_limits<double>::lowest();  // Initialize max to lowest value

    for (const auto& frame : m_frame_data)
    {
        double time = frame;
        stats.min = std::min(stats.min, time);  // Update min
        stats.max = std::max(stats.max, time);  // Update max
    }

    stats.average = CalculateAverage();  // Call CalculateAverage directly
    stats.median = CalculateMedian();
    stats.stddev = CalculateStdDev(stats.average);

    return stats;
}

std::string GPUTime::FrameMetrics::GetStatsString(const Stats& stats) const
{
    std::stringstream ss;
    ss << "FrameMetrics:\n"
       << std::fixed << std::setprecision(2) << "  Min: " << stats.min << " ms\n"
       << "  Max: " << stats.max << " ms\n"
       << "  Mean: " << stats.average << " ms\n"
       << "  Median: " << stats.median << " ms\n"
       << "  Std: " << stats.stddev << " ms";
    return ss.str();
}

double GPUTime::FrameMetrics::CalculateAverage() const
{
    if (m_frame_data.empty())
    {
        return 0.0;
    }

    double sum = 0.0;
    for (const auto& frame : m_frame_data)
    {
        sum += frame;
    }
    return sum / m_frame_data.size();
}

double GPUTime::FrameMetrics::CalculateMedian() const
{
    if (m_frame_data.empty())
    {
        return 0.0;
    }

    // Create a mutable copy of the data to sort it,
    // as the original m_frame_data is const in this const member function.
    std::deque<double> sorted_data = m_frame_data;
    std::sort(sorted_data.begin(), sorted_data.end());

    size_t size = sorted_data.size();
    if (size % 2 == 0)
    {
        // Even number of elements: average of the two middle elements
        double mid1 = sorted_data[size / 2 - 1];
        double mid2 = sorted_data[size / 2];
        return (mid1 + mid2) / 2.0;
    }
    else
    {
        // Odd number of elements: the middle element
        return sorted_data[size / 2];
    }
}

double GPUTime::FrameMetrics::CalculateStdDev(double average) const
{
    if (m_frame_data.size() < 2)
    {
        return 0.0;
    }
    double variance = 0.0;
    for (const auto& frame : m_frame_data)
    {
        double time = frame;
        variance += (time - average) * (time - average);
    }
    variance /= (m_frame_data.size() - 1);
    return std::sqrt(variance);
}

GPUTime::GPUTime() :
    m_device(VK_NULL_HANDLE),
    m_allocator(nullptr),
    m_query_pool(VK_NULL_HANDLE),
    m_frame_index(0),
    m_timestamp_counter(0),
    m_timestamp_period(0.0f)
{
}

GPUTime::~GPUTime() {}

std::string GPUTime::GetStatsString() const
{
    std::string message = "Frame " + std::to_string(m_frame_index) + " processed successfully.\n" +
                          m_metrics.GetStatsString(GetStats());
    return message;
}

GPUTime::GpuTimeStatus GPUTime::OnCreateDevice(VkDevice                     device,
                                               const VkAllocationCallbacks* allocator,
                                               float                        timestampPeriod,
                                               PFN_vkCreateQueryPool        pfnCreateQueryPool,
                                               PFN_vkResetQueryPool         pfnResetQueryPool)
{
    if (device == VK_NULL_HANDLE)
    {
        return GPUTime::GpuTimeStatus{ "Need to pass in a valid device!", false };
    }
    m_allocator = allocator;
    m_device = device;
    m_timestamp_period = timestampPeriod;

    // Create a query pool for timestamps
    VkQueryPoolCreateInfo queryPoolInfo{};
    queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    queryPoolInfo.queryCount = kQueryCount;

    VkResult result = pfnCreateQueryPool(m_device, &queryPoolInfo, m_allocator, &m_query_pool);
    if (result != VK_SUCCESS)
    {
        return GPUTime::GpuTimeStatus{ "vkCreateQueryPool failed with VkResult: " +
                                       std::to_string(static_cast<int>(result)),
                                       false };
    }

    pfnResetQueryPool(m_device, m_query_pool, 0, kQueryCount);
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::OnDestroyDevice(VkDevice               device,
                                                PFN_vkQueueWaitIdle    pfnQueueWaitIdle,
                                                PFN_vkDestroyQueryPool pfnDestroyQueryPool)
{
    if (device != m_device)
    {
        return GPUTime::GpuTimeStatus{ "Not destroying the cached device!" };
    }

    if ((m_device != VK_NULL_HANDLE) && (m_query_pool != VK_NULL_HANDLE))
    {
        if (m_queues.empty())
        {
            return GPUTime::GpuTimeStatus{ "vk queue is empty!" };
        }

        for (auto& q : m_queues)
        {
            pfnQueueWaitIdle(q);
        }
        m_queues.clear();

        pfnDestroyQueryPool(m_device, m_query_pool, m_allocator);
        m_query_pool = VK_NULL_HANDLE;
        m_allocator = nullptr;
    }
    m_device = VK_NULL_HANDLE;
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::OnDestroyCommandPool(VkCommandPool commandPool)
{
    if (commandPool == VK_NULL_HANDLE)
    {
        // it is valid to have null command pool as input
        return GPUTime::GpuTimeStatus();
    }

    auto it = m_cmds.begin();
    while (it != m_cmds.end())
    {
        if (it->second.pool == commandPool)
        {
            it = m_cmds.erase(it);
        }
        else
        {
            ++it;
        }
    }
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::OnAllocateCommandBuffers(
const VkCommandBufferAllocateInfo* pAllocateInfo,
VkCommandBuffer*                   pCommandBuffers)
{
    if (m_timestamp_counter + pAllocateInfo->commandBufferCount * 2 > kQueryCount)
    {
        return GPUTime::GpuTimeStatus{ "Exceeded maximum number of query slots.", false };
    }

    for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; ++i)
    {
        if (m_cmds.find(pCommandBuffers[i]) != m_cmds.end())
        {
            std::stringstream ss;
            ss << static_cast<void*>(pCommandBuffers[i]) << " has been already added!";
            return GPUTime::GpuTimeStatus{ ss.str(), false };
        }
        m_cmds.insert(
        { pCommandBuffers[i], { pAllocateInfo->commandPool, m_timestamp_counter, false } });
        // 1 at vkBeginCommandBuffer and 1 at vkEndCommandBuffer
        m_timestamp_counter += 2;
    }
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::OnFreeCommandBuffers(uint32_t               commandBufferCount,
                                                     const VkCommandBuffer* pCommandBuffers)
{
    for (uint32_t i = 0; i < commandBufferCount; ++i)
    {
        if (m_cmds.find(pCommandBuffers[i]) == m_cmds.end())
        {
            std::stringstream ss;
            ss << static_cast<void*>(pCommandBuffers[i]) << " is not in the cmd cache!";
            return GPUTime::GpuTimeStatus{ ss.str(), false };
        }
        m_cmds.erase(pCommandBuffers[i]);
    }
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::OnResetCommandBuffer(VkCommandBuffer commandBuffer)
{
    if (m_cmds.find(commandBuffer) == m_cmds.end())
    {
        std::stringstream ss;
        ss << static_cast<void*>(commandBuffer) << " is not in the cmd cache!";
        return GPUTime::GpuTimeStatus{ ss.str(), false };
    }
    m_cmds[commandBuffer].Reset();
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::OnResetCommandPool(VkCommandPool commandPool)
{
    for (auto& cmd : m_cmds)
    {
        if (cmd.second.pool == commandPool)
        {
            cmd.second.timestamp_offset = CommandBufferInfo::kInvalidTimeStampOffset;
            cmd.second.Reset();
        }
    }
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::OnBeginCommandBuffer(VkCommandBuffer           commandBuffer,
                                                     VkCommandBufferUsageFlags flags,
                                                     PFN_vkCmdWriteTimestamp   pfnCmdWriteTimestamp)
{
    if (m_cmds.find(commandBuffer) == m_cmds.end())
    {
        std::stringstream ss;
        ss << static_cast<void*>(commandBuffer) << " is not in the cmd cache!";
        return GPUTime::GpuTimeStatus{ ss.str(), false };
    }

    if (m_cmds[commandBuffer].usage_one_submit)
    {
        m_cmds[commandBuffer].Reset();
    }

    if ((flags & VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) != 0)
    {
        m_cmds[commandBuffer].usage_one_submit = true;
    }

    pfnCmdWriteTimestamp(commandBuffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         m_query_pool,
                         m_cmds[commandBuffer].timestamp_offset);
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::OnEndCommandBuffer(VkCommandBuffer         commandBuffer,
                                                   PFN_vkCmdWriteTimestamp pfnCmdWriteTimestamp)
{
    if (m_cmds.find(commandBuffer) == m_cmds.end())
    {
        std::stringstream ss;
        ss << static_cast<void*>(commandBuffer) << " is not in the cmd cache!";
        return GPUTime::GpuTimeStatus{ ss.str(), false };
    }

    pfnCmdWriteTimestamp(commandBuffer,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         m_query_pool,
                         m_cmds[commandBuffer].timestamp_offset + 1);
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::UpdateFrameMetrics(PFN_vkGetQueryPoolResults pfnGetQueryPoolResults)
{
    // Get the timestamp results
    // *2 for VK_QUERY_RESULT_WITH_AVAILABILITY_BIT
    uint64_t timestamps_with_availability[kQueryCount * 2];

    // VK_QUERY_RESULT_PARTIAL_BIT is used instead of VK_QUERY_RESULT_WAIT_BIT since some of the
    // timestamps may not be finished. we have some pre-recorded cmds that may not be replayed, but
    // the counter slot is reserved. VK_QUERY_RESULT_WITH_AVAILABILITY_BIT is used so that we can
    // check for individual ones to make sure the value is valid

    constexpr size_t data_per_query = sizeof(uint64_t);          // For the result itself
    constexpr size_t availability_per_query = sizeof(uint64_t);  // For the availability status
    VkDeviceSize     data_size = m_timestamp_counter * (data_per_query + availability_per_query);
    constexpr VkDeviceSize stride = data_per_query + availability_per_query;

    VkResult result = pfnGetQueryPoolResults(m_device,
                                             m_query_pool,
                                             0,
                                             m_timestamp_counter,
                                             data_size,
                                             timestamps_with_availability,
                                             stride,
                                             VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_PARTIAL_BIT |
                                             VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);

    if (result != VK_SUCCESS)
    {
        return GPUTime::GpuTimeStatus{ "vkGetQueryPoolResults failed with VkResult: " +
                                       std::to_string(static_cast<int>(result)),
                                       false };
    }

    // TODO(wangra): Currently we calculating the frame time by accumulating command buffer time
    // this is NOT correct since the works between command buffers might overlapping with each other
    // (if there is no sync between them)
    // In the future, we could keep command buffer gpu time or render pass gpu time separately
    // but this requires all frames have the same commandbuffers and renderpasses which might
    // not be true for the runtime layer case
    double frame_time = 0.0;
    bool   valid_frame_time = true;

    for (const auto& cmd : m_frame_cmds)
    {
        // cmd may not be in the m_cmds when some cmds got deleted before submitting the frame
        // boundary cmd
        if (m_cmds.find(cmd) != m_cmds.end())
        {
            const uint32_t timestamp_offset = m_cmds[cmd].timestamp_offset;

            uint64_t
            availability_end = timestamps_with_availability[(timestamp_offset + 1) * 2 + 1];
            uint64_t availability_begin = timestamps_with_availability[timestamp_offset * 2 + 1];

            if ((availability_begin != 0) && (availability_end != 0))
            {
                // Calculate the elapsed time in nanoseconds
                uint64_t elapsed_time = timestamps_with_availability[(timestamp_offset + 1) * 2] -
                                        timestamps_with_availability[timestamp_offset * 2];
                double elapsed_time_in_ms = elapsed_time * m_timestamp_period * 0.000001;
                frame_time += elapsed_time_in_ms;
            }
            else
            {
                frame_time = 0.0;
                valid_frame_time = false;
                std::stringstream ss;
                ss << "Query result is not available for cmd " << static_cast<void*>(cmd);
                return GPUTime::GpuTimeStatus{ ss.str(), false };
            }
        }
    }

    if (valid_frame_time)
    {
        m_metrics.AddFrameTime(frame_time);
    }

    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::OnQueueSubmit(uint32_t                  submitCount,
                                              const VkSubmitInfo*       pSubmits,
                                              PFN_vkDeviceWaitIdle      pfnDeviceWaitIdle,
                                              PFN_vkResetQueryPool      pfnResetQueryPool,
                                              PFN_vkGetQueryPoolResults pfnGetQueryPoolResults)
{
    bool is_frame_boundary = false;

    if ((pSubmits != nullptr) && (pSubmits->pCommandBuffers != nullptr))
    {
        for (uint32_t i = 0; i < submitCount; i++)
        {
            uint32_t num_command_buffers = pSubmits->commandBufferCount;
            for (uint32_t c = 0; c < num_command_buffers; ++c)
            {
                const auto& cmd = pSubmits->pCommandBuffers[c];
                if (m_cmds.find(cmd) == m_cmds.end())
                {
                    std::stringstream ss;
                    ss << static_cast<void*>(cmd) << " is not in the cmd cache!";
                    if (is_frame_boundary)
                    {
                        m_frame_cmds.clear();
                    }
                    return GPUTime::GpuTimeStatus{ ss.str(), false };
                }
                if (m_cmds[cmd].is_frameboundary)
                {
                    is_frame_boundary = true;
                }
                m_frame_cmds.push_back(cmd);
            }
        }
    }

    if (is_frame_boundary)
    {
        //  force sync to make sure the gpu is done with this frame
        pfnDeviceWaitIdle(m_device);
        GPUTime::GpuTimeStatus update_status = UpdateFrameMetrics(pfnGetQueryPoolResults);
        if (!update_status.success)
        {
            return update_status;
        }

        m_frame_index++;
        m_frame_cmds.clear();

        pfnResetQueryPool(m_device, m_query_pool, 0, kQueryCount);
    }
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::OnGetDeviceQueue2(VkQueue* pQueue)
{
    m_queues.insert(*pQueue);
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::OnGetDeviceQueue(VkQueue* pQueue)
{
    m_queues.insert(*pQueue);
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::OnCmdInsertDebugUtilsLabelEXT(
VkCommandBuffer             commandBuffer,
const VkDebugUtilsLabelEXT* pLabelInfo)
{
    if (pLabelInfo == nullptr || pLabelInfo->pLabelName == nullptr)
    {
        return GPUTime::GpuTimeStatus{ "pLabelInfo cannot be nullptr!", false };
    }

    if (strcmp(kVulkanVrFrameDelimiterString, pLabelInfo->pLabelName) == 0)
    {
        if (m_cmds.find(commandBuffer) == m_cmds.end())
        {
            std::stringstream ss;
            ss << static_cast<void*>(commandBuffer) << " is not in the cmd cache!";
            return GPUTime::GpuTimeStatus{ ss.str(), false };
        }
        m_cmds[commandBuffer].is_frameboundary = true;
    }
    return GPUTime::GpuTimeStatus();
}

}  // namespace Dive
