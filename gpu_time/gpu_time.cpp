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
#include <optional>
#include <thread>
#include <chrono>

namespace Dive
{

GPUTime::TimeStampSlotAllocator::TimeStampSlotAllocator()
{
    Reset();
}

void GPUTime::TimeStampSlotAllocator::Reset()
{
    for (uint32_t i = 0; i < kNumBlocks; ++i)
    {
        m_masks[i].store(0, std::memory_order_relaxed);
    }
    m_cur.store(0, std::memory_order_relaxed);
}

uint32_t GPUTime::TimeStampSlotAllocator::AllocateSlot()
{
    uint32_t current_idx = m_cur.load(std::memory_order_relaxed);

    for (uint32_t i = 0; i < kTotalSlots; ++i)
    {
        const uint32_t slot_idx = (current_idx + i) % kTotalSlots;
        const uint32_t block_idx = slot_idx / kSlotsPerBlock;
        const uint32_t bit_idx = slot_idx % kSlotsPerBlock;
        const size_t   mask = (static_cast<size_t>(1) << bit_idx);

        size_t old_mask = m_masks[block_idx].load(std::memory_order_relaxed);

        // If the m_masks[block_idx] == old_mask, we set the mask with old_mask | mask
        // if Another thread modifies old_mask, old_mask is updated with the new value
        // the condition fails and we loop again, if the mask is already set, we stop the loop
        while ((old_mask & mask) == 0)
        {
            if (m_masks[block_idx].compare_exchange_weak(old_mask,
                                                         old_mask | mask,
                                                         std::memory_order_release,
                                                         std::memory_order_relaxed))
            {
                m_cur.store((slot_idx + 1) % kTotalSlots, std::memory_order_relaxed);
                return slot_idx;
            }
        }
    }
    return kInvalidIndex;
}

void GPUTime::TimeStampSlotAllocator::FreeSlots(const std::vector<uint32_t>& slots)
{
    for (const auto& slot : slots)
    {
        const uint32_t block_idx = slot / kSlotsPerBlock;
        const uint32_t bit_idx = slot % kSlotsPerBlock;
        const size_t   mask = (static_cast<size_t>(1) << bit_idx);

        size_t old_mask = m_masks[block_idx].load(std::memory_order_relaxed);

        while (true)
        {
            // If the bit is already clear, another thread beat us to it. We're done.
            if ((old_mask & mask) == 0)
            {
                break;
            }

            // If the m_masks[block_idx] == old_mask, we clear the mask with old_mask & ~mask
            // if Another thread modifies old_mask, old_mask is updated with the new value
            // the condition fails and we loop again
            if (m_masks[block_idx].compare_exchange_weak(old_mask,
                                                         old_mask & ~mask,
                                                         std::memory_order_release,
                                                         std::memory_order_relaxed))
            {
                break;
            }
        }
    }
}

void GPUTime::FrameMetrics::AddFrameData(double                     frame_time,
                                         const std::vector<double>& cmd_time_vec,
                                         const std::vector<double>& renderpass_time_vec,
                                         const std::vector<size_t>& cmd_renderpass_count_vec)
{
    // TODO(wangra): reset when there is a difference in number of cmds per frame
    // maybe we should expose the Reset and let the app decide when to reset
    size_t new_frame_cmd_count = cmd_time_vec.size();
    size_t new_frame_renderpass_count = renderpass_time_vec.size();
    if ((m_cmd_time_vec.size() != new_frame_cmd_count) ||
        (m_renderpass_time_vec.size() != new_frame_renderpass_count))
    {
        Reset();
        m_cmd_time_vec.resize(new_frame_cmd_count);
        m_renderpass_time_vec.resize(new_frame_renderpass_count);
        m_cmd_renderpass_count_vec = cmd_renderpass_count_vec;
    }

    if (m_frame_time.size() == TimeStampSlotAllocator::kFrameMetricsLimit)
    {
        m_frame_time.pop_front();

        for (auto& c : m_cmd_time_vec)
        {
            c.pop_front();
        }

        for (auto& r : m_renderpass_time_vec)
        {
            r.pop_front();
        }
    }
    m_frame_time.push_back(frame_time);
    for (size_t i = 0; i < new_frame_cmd_count; ++i)
    {
        m_cmd_time_vec[i].push_back(cmd_time_vec[i]);
    }
    for (size_t i = 0; i < new_frame_renderpass_count; ++i)
    {
        m_renderpass_time_vec[i].push_back(renderpass_time_vec[i]);
    }
}

GPUTime::Stats GPUTime::FrameMetrics::GetStatistics(const std::deque<double>& data) const
{
    Stats stats;
    stats.min = std::numeric_limits<double>::max();
    stats.max = std::numeric_limits<double>::lowest();

    for (const auto& d : data)
    {
        stats.min = std::min(stats.min, d);
        stats.max = std::max(stats.max, d);
    }

    stats.average = CalculateAverage(data);
    stats.median = CalculateMedian(data);
    stats.stddev = CalculateStdDev(data, stats.average);

    return stats;
}

double GPUTime::FrameMetrics::CalculateAverage(const std::deque<double>& data) const
{
    if (data.empty())
    {
        return 0.0;
    }

    double sum = 0.0;
    for (const auto& d : data)
    {
        sum += d;
    }
    return sum / data.size();
}

double GPUTime::FrameMetrics::CalculateMedian(const std::deque<double>& data) const
{
    if (data.empty())
    {
        return 0.0;
    }

    // Create a mutable copy of the data to sort it,
    // as the original m_frame_data is const in this const member function.
    std::deque<double> sorted_data = data;
    std::sort(sorted_data.begin(), sorted_data.end());

    size_t size = sorted_data.size();
    if (size % 2 == 0)
    {
        // Even number of elements: average of the two middle elements
        double mid1 = sorted_data[(size / 2) - 1];
        double mid2 = sorted_data[size / 2];
        return (mid1 + mid2) / 2.0;
    }
    else
    {
        // Odd number of elements: the middle element
        return sorted_data[size / 2];
    }
}

double GPUTime::FrameMetrics::CalculateStdDev(const std::deque<double>& data, double average) const
{
    if (data.size() < 2)
    {
        return 0.0;
    }
    double variance = 0.0;
    for (const auto& d : data)
    {
        variance += (d - average) * (d - average);
    }
    variance /= (data.size() - 1);
    return std::sqrt(variance);
}

void GPUTime::FrameMetrics::Reset()
{
    m_frame_time.clear();
    m_cmd_time_vec.clear();
}

GPUTime::Stats GPUTime::FrameMetrics::GetFrameTimeStats() const
{
    return GetStatistics(m_frame_time);
}

GPUTime::Stats GPUTime::FrameMetrics::GetFrameCmdTimeStats(size_t index) const
{
    if (index >= m_cmd_time_vec.size())
    {
        return GPUTime::Stats();
    }
    return GetStatistics(m_cmd_time_vec[index]);
}

GPUTime::Stats GPUTime::FrameMetrics::GetFrameRenderPassTimeStats(size_t index) const
{
    if (index >= m_renderpass_time_vec.size())
    {
        return GPUTime::Stats();
    }
    return GetStatistics(m_renderpass_time_vec[index]);
}

size_t GPUTime::FrameMetrics::GetFrameCmdCount() const
{
    return m_cmd_time_vec.size();
}

size_t GPUTime::FrameMetrics::GetFrameRenderPassCount() const
{
    return m_renderpass_time_vec.size();
}

size_t GPUTime::FrameMetrics::GetCmdRenderPassCount(size_t index) const
{
    if (index >= m_cmd_renderpass_count_vec.size())
    {
        return kInvalidRenderPassCount;
    }
    return m_cmd_renderpass_count_vec[index];
}

std::string GPUTime::GetStatsString() const
{
    const Stats&      stats = GetFrameTimeStats();
    std::stringstream ss;
    ss << "FrameMetrics:\n";

    auto PopulateStatsString = [&](std::stringstream& ss, const Stats& stats, int nLevel) {
        std::string indent(nLevel, '\t');
        ss << std::fixed << std::setprecision(2) << indent << "  Mean: " << stats.average << " ms\n"
           << indent << "  Median: " << stats.median << " ms\n";
    };
    PopulateStatsString(ss, stats, 0);

    size_t renderpass_index = 0;
    ss << "Command Buffer Metrics:\n";
    size_t cmd_count = m_metrics.GetFrameCmdCount();
    for (size_t i = 0; i < cmd_count; ++i)
    {
        const Stats& cmd_stats = GetFrameCmdTimeStats(i);
        ss << "\tCommandBuffer" << i << ": \n";
        PopulateStatsString(ss, cmd_stats, 1);

        size_t renderpass_count = GetCmdRenderPassCount(i);
        for (size_t j = 0; j < renderpass_count; ++j)
        {
            const Stats& renderpass_stats = GetFrameRenderPassTimeStats(renderpass_index);
            ss << "\t\tRenderPass" << renderpass_index << ": \n";
            PopulateStatsString(ss, renderpass_stats, 2);
        }
        renderpass_index += renderpass_count;
    }

    std::string message = "Frame " + std::to_string(m_frame_index) + " processed successfully.\n" +
                          ss.str();
    return message;
}

GPUTime::GpuTimeStatus GPUTime::OnCreateDevice(VkDevice                     device,
                                               const VkAllocationCallbacks* allocator_ptr,
                                               float                        timestamp_period,
                                               PFN_vkCreateQueryPool        pfn_create_query_pool,
                                               PFN_vkResetQueryPool         pfn_reset_query_pool)
{
    if (device == VK_NULL_HANDLE)
    {
        m_valid_frame = false;
        return GPUTime::GpuTimeStatus{ "Need to pass in a valid device!", false };
    }
    m_allocator = allocator_ptr;
    m_device = device;
    m_timestamp_period = timestamp_period;

    // Create a query pool for timestamps
    VkQueryPoolCreateInfo queryPoolInfo{};
    queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    queryPoolInfo.queryCount = TimeStampSlotAllocator::kTotalSlots;

    VkResult result = pfn_create_query_pool(m_device, &queryPoolInfo, m_allocator, &m_query_pool);
    if (result != VK_SUCCESS)
    {
        m_valid_frame = false;
        return GPUTime::GpuTimeStatus{ "vkCreateQueryPool failed with VkResult: " +
                                       std::to_string(static_cast<int>(result)),
                                       false };
    }

    pfn_reset_query_pool(m_device, m_query_pool, 0, TimeStampSlotAllocator::kTotalSlots);
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::OnDestroyDevice(VkDevice               device,
                                                PFN_vkQueueWaitIdle    pfn_queue_wait_idle,
                                                PFN_vkDestroyQueryPool pfn_destroy_query_pool)
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
            pfn_queue_wait_idle(q);
        }
        m_queues.clear();

        pfn_destroy_query_pool(m_device, m_query_pool, m_allocator);
        m_query_pool = VK_NULL_HANDLE;
        m_allocator = nullptr;
    }
    m_device = VK_NULL_HANDLE;
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::OnDestroyCommandPool(VkCommandPool command_pool)
{
    if (command_pool == VK_NULL_HANDLE)
    {
        // it is valid to have null command pool as input
        return GPUTime::GpuTimeStatus();
    }

    auto it = m_cmds.begin();
    while (it != m_cmds.end())
    {
        if (it->second.pool == command_pool)
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
const VkCommandBufferAllocateInfo* allocate_info_ptr,
VkCommandBuffer*                   command_buffers_ptr)
{
    // The cache should not contain secondary command buffers
    if (allocate_info_ptr->level == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
    {
        return GPUTime::GpuTimeStatus();
    }

    for (uint32_t i = 0; i < allocate_info_ptr->commandBufferCount; ++i)
    {
        if (m_cmds.find(command_buffers_ptr[i]) != m_cmds.end())
        {
            m_valid_frame = false;
            std::stringstream ss;
            ss << static_cast<void*>(command_buffers_ptr[i]) << " has been already added!";
            return GPUTime::GpuTimeStatus{ ss.str(), false };
        }

        uint32_t begin_slot = m_timestamp_allocator.AllocateSlot();
        uint32_t end_slot = m_timestamp_allocator.AllocateSlot();

        if ((begin_slot == TimeStampSlotAllocator::kInvalidIndex) ||
            (end_slot == TimeStampSlotAllocator::kInvalidIndex))
        {
            return GPUTime::GpuTimeStatus{ "Exceeded maximum number of query slots.", false };
        }

        m_cmds.insert(
        { command_buffers_ptr[i],
          { {}, allocate_info_ptr->commandPool, begin_slot, end_slot, false, false, false } });
    }
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::OnFreeCommandBuffers(uint32_t               command_buffer_count,
                                                     const VkCommandBuffer* command_buffers_ptr)
{
    for (uint32_t i = 0; i < command_buffer_count; ++i)
    {
        if (m_cmds.find(command_buffers_ptr[i]) == m_cmds.end())
        {
            // The cache doesn't contain secondary command buffers
            continue;
        }
        m_timestamp_allocator.FreeSlots({ m_cmds[command_buffers_ptr[i]].begin_timestamp_offset,
                                          m_cmds[command_buffers_ptr[i]].end_timestamp_offset });
        RemoveCmdFromFrameCache(command_buffers_ptr[i]);
        m_cmds.erase(command_buffers_ptr[i]);
    }
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::OnResetCommandBuffer(VkCommandBuffer command_buffer)
{
    if (m_cmds.find(command_buffer) == m_cmds.end())
    {
        // The cache doesn't contain secondary command buffers
        return GPUTime::GpuTimeStatus();
    }
    RemoveCmdFromFrameCache(command_buffer);
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::OnResetCommandPool(VkCommandPool command_pool)
{
    for (auto& cmd : m_cmds)
    {
        if (cmd.second.pool == command_pool)
        {
            RemoveCmdFromFrameCache(cmd.first);
        }
    }
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::OnBeginCommandBuffer(
VkCommandBuffer           command_buffer,
VkCommandBufferUsageFlags flags,
PFN_vkCmdWriteTimestamp   pfn_cmd_write_timestamp)
{
    if (!m_enable)
    {
        return GPUTime::GpuTimeStatus();
    }
    m_timestamp_allocator.FreeSlots(m_cmds[command_buffer].renderpass_slots);
    m_cmds[command_buffer].renderpass_slots.clear();

    if (m_cmds.find(command_buffer) == m_cmds.end())
    {
        // We do not insert timestamps into secondary command buffers
        return GPUTime::GpuTimeStatus();
    }

    if (m_cmds[command_buffer].usage_one_submit)
    {
        m_cmds[command_buffer].Reset();
    }

    m_cmds[command_buffer].usage_one_submit = ((flags &
                                                VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) != 0);

    m_cmds[command_buffer].reusable = ((flags & VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT) != 0);

    pfn_cmd_write_timestamp(command_buffer,
                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                            m_query_pool,
                            m_cmds[command_buffer].begin_timestamp_offset);
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::OnEndCommandBuffer(VkCommandBuffer         command_buffer,
                                                   PFN_vkCmdWriteTimestamp pfn_cmd_write_timestamp)
{
    if (!m_enable)
    {
        return GPUTime::GpuTimeStatus();
    }

    if (m_cmds.find(command_buffer) == m_cmds.end())
    {
        // We do not insert timestamps into secondary command buffers
        return GPUTime::GpuTimeStatus();
    }

    pfn_cmd_write_timestamp(command_buffer,
                            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                            m_query_pool,
                            m_cmds[command_buffer].end_timestamp_offset);
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::UpdateFrameMetrics(
PFN_vkGetQueryPoolResults pfn_get_query_pool_results)
{

    constexpr size_t data_per_query = sizeof(uint64_t);          // For the result itself
    constexpr size_t availability_per_query = sizeof(uint64_t);  // For the availability status
    VkDeviceSize     data_size = TimeStampSlotAllocator::kTotalSlots *
                             (data_per_query + availability_per_query);
    constexpr VkDeviceSize stride = data_per_query + availability_per_query;

    VkResult result = pfn_get_query_pool_results(m_device,
                                                 m_query_pool,
                                                 0,
                                                 TimeStampSlotAllocator::kTotalSlots,
                                                 data_size,
                                                 m_timestamps_with_availability,
                                                 stride,
                                                 VK_QUERY_RESULT_64_BIT |
                                                 VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);

    if (result != VK_SUCCESS)
    {
        if (result == VK_NOT_READY)
        {
            // Make sure all timestamps are available
            // vkDeviceWaitIdle is used to force gpu to finish all tasks
            // But there is still a delay on the data being transfered from register to mem
            // Some timestamp might still not be available even after vkDeviceWaitIdle is called
            // result could also be VK_NOT_READY since we have a ring buffer to keep all timestamps
            // We allocate slot to all allocated cmd buffers, and it is possible that some cmds
            // are allocated but never submitted in current frame
            // Since we dont know how much the delay is, use kMaxQueryCount to wait for 5 frames to
            // make sure all results are transfered back to mem
            constexpr uint32_t kMaxQueryCount = 5;
            uint32_t           query_count = 0;
            bool               all_timestamp_available = true;
            do
            {
                all_timestamp_available = true;
                for (const auto& cmd : m_frame_cmds)
                {
                    // cmd may not be in the m_cmds when some cmds got deleted before submitting the
                    // frame boundary cmd
                    if (m_cmds.find(cmd) != m_cmds.end())
                    {
                        const uint32_t begin_timestamp_offset = m_cmds[cmd].begin_timestamp_offset;
                        const uint32_t end_timestamp_offset = m_cmds[cmd].end_timestamp_offset;
                        uint64_t
                        availability_end = m_timestamps_with_availability[end_timestamp_offset * 2 +
                                                                          1];
                        uint64_t availability_begin = m_timestamps_with_availability
                        [begin_timestamp_offset * 2 + 1];

                        if ((availability_begin == 0) || (availability_end == 0))
                        {
                            all_timestamp_available = false;
                            // sleep for 14ms (assume 72fps, so ~14ms per frame)
                            // and hope the result would be available
                            std::this_thread::sleep_for(std::chrono::milliseconds(14));
                            result =
                            pfn_get_query_pool_results(m_device,
                                                       m_query_pool,
                                                       0,
                                                       TimeStampSlotAllocator::kTotalSlots,
                                                       data_size,
                                                       m_timestamps_with_availability,
                                                       stride,
                                                       VK_QUERY_RESULT_64_BIT |
                                                       VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);
                            ++query_count;
                            break;
                        }
                    }
                }
            } while (!all_timestamp_available && query_count < kMaxQueryCount);

            if (!all_timestamp_available && query_count >= kMaxQueryCount)
            {
                m_valid_frame = false;
                // Keep only the last 4 digits of each handle since there seems to be a limit amount
                // of chars logcat could output
                auto ToHexString = [](VkCommandBuffer cmd) -> std::string {
                    std::stringstream ss;
                    ss << std::hex << std::setw(4) << std::setfill('0')
                       << (reinterpret_cast<uintptr_t>(cmd) & 0xFFFF);
                    return ss.str();
                };

                std::stringstream ss;
                ss << std::to_string(m_frame_cmds.size()) << " cmds:" << std::endl;
                size_t cmd_index = 0;
                for (const auto& cmd : m_frame_cmds)
                {
                    const uint32_t begin_timestamp_offset = m_cmds[cmd].begin_timestamp_offset;
                    const uint32_t end_timestamp_offset = m_cmds[cmd].end_timestamp_offset;

                    uint64_t
                    availability_end = m_timestamps_with_availability[end_timestamp_offset * 2 + 1];
                    uint64_t
                    availability_begin = m_timestamps_with_availability[begin_timestamp_offset * 2 +
                                                                        1];
                    ss << std::to_string(cmd_index);
                    if ((availability_begin == 0) || (availability_end == 0))
                    {
                        ss << "_NA: ";
                    }
                    else
                    {
                        ss << "_A : ";
                    }

                    ss << "0x" << ToHexString(cmd) << " : S"
                       << std::to_string(static_cast<uint32_t>(begin_timestamp_offset)) << " : E"
                       << std::to_string(static_cast<uint32_t>(end_timestamp_offset)) << std::endl;
                    ++cmd_index;
                }

                return GPUTime::GpuTimeStatus{ ss.str(), false };
            }
        }
        else
        {
            m_valid_frame = false;
            return GPUTime::GpuTimeStatus{ "vkGetQueryPoolResults failed with VkResult: " +
                                           std::to_string(static_cast<int>(result)),
                                           false };
        }
    }

    double              frame_time = 0.0;
    std::vector<double> cmds_time;
    std::vector<double> renderpasses_time;
    std::vector<size_t> cmd_renderpass_count_vec;

    auto GetTimeDuration = [&](uint32_t begin_offset,
                               uint32_t end_offset,
                               uint64_t timestamps_with_availability[]) -> std::optional<double> {
        uint64_t availability_end = timestamps_with_availability[end_offset * 2 + 1];
        uint64_t availability_begin = timestamps_with_availability[begin_offset * 2 + 1];

        if ((availability_begin == 0) || (availability_end == 0))
        {
            // Return an empty optional to signal an invalid result
            return std::nullopt;
        }

        // Calculate the elapsed time in nanoseconds
        uint64_t elapsed_timestamp_increments = timestamps_with_availability[end_offset * 2] -
                                                timestamps_with_availability[begin_offset * 2];
        // m_timestamp_period is the number of nanoseconds per timestamp increment.
        const double kNanoToMilli = 1.0 / 1000000.0;
        double       elapsed_time_in_ms = static_cast<double>(elapsed_timestamp_increments) *
                                    m_timestamp_period * kNanoToMilli;

        return elapsed_time_in_ms;
    };

    for (const auto& cmd : m_frame_cmds)
    {
        // cmd may not be in the m_cmds when some cmds got deleted before submitting the frame
        // boundary cmd
        if (m_cmds.find(cmd) != m_cmds.end())
        {
            const uint32_t begin_timestamp_offset = m_cmds[cmd].begin_timestamp_offset;
            const uint32_t end_timestamp_offset = m_cmds[cmd].end_timestamp_offset;

            auto elapsed_time_in_ms = GetTimeDuration(begin_timestamp_offset,
                                                      end_timestamp_offset,
                                                      m_timestamps_with_availability);

            if (!elapsed_time_in_ms)
            {
                frame_time = 0.0;
                m_valid_frame = false;
                std::stringstream ss;
                ss << "Query result is not available for cmd " << static_cast<void*>(cmd)
                   << " Begin Offset:" << begin_timestamp_offset
                   << " End Offset:" << end_timestamp_offset;
                return GPUTime::GpuTimeStatus{ ss.str(), false };
            }

            cmds_time.push_back(elapsed_time_in_ms.value());
            frame_time += elapsed_time_in_ms.value();

            const size_t renderpass_count = m_cmds[cmd].renderpass_slots.size();
            cmd_renderpass_count_vec.push_back(renderpass_count / 2);
            for (size_t r = 0; r < renderpass_count; r = r + 2)
            {
                const uint32_t renderpass_begin_timestamp_offset = m_cmds[cmd].renderpass_slots[r];
                const uint32_t renderpass_end_timestamp_offset = m_cmds[cmd]
                                                                 .renderpass_slots[r + 1];

                auto
                renderpass_elapsed_time_in_ms = GetTimeDuration(renderpass_begin_timestamp_offset,
                                                                renderpass_end_timestamp_offset,
                                                                m_timestamps_with_availability);

                if (!renderpass_elapsed_time_in_ms)
                {
                    frame_time = 0.0;
                    m_valid_frame = false;
                    std::stringstream ss;
                    ss << "Query result is not available for renderpass " << r << " in the cmd "
                       << static_cast<void*>(cmd) << " Begin Offset:" << begin_timestamp_offset
                       << " End Offset:" << end_timestamp_offset;
                    return GPUTime::GpuTimeStatus{ ss.str(), false };
                }
                renderpasses_time.push_back(renderpass_elapsed_time_in_ms.value());
            }
        }
    }

    if (m_valid_frame)
    {
        m_metrics.AddFrameData(frame_time, cmds_time, renderpasses_time, cmd_renderpass_count_vec);
    }

    return GPUTime::GpuTimeStatus();
}

void GPUTime::RemoveCmdFromFrameCache(VkCommandBuffer cmd)
{
    // Free any slots that were used for render pass timings within this command buffer
    m_timestamp_allocator.FreeSlots(m_cmds[cmd].renderpass_slots);
    m_cmds[cmd].renderpass_slots.clear();
    m_cmds[cmd].Reset();
    auto& vec = m_frame_cmds;
    vec.erase(std::remove(vec.begin(), vec.end(), cmd), vec.end());
}

GPUTime::SubmitStatus GPUTime::OnQueueSubmit(uint32_t                  submit_count,
                                             const VkSubmitInfo*       submits_ptr,
                                             PFN_vkDeviceWaitIdle      pfn_device_wait_idle,
                                             PFN_vkResetQueryPool      pfn_reset_query_pool,
                                             PFN_vkGetQueryPoolResults pfn_get_query_pool_results)
{
    if (!m_enable)
    {
        return { GPUTime::GpuTimeStatus(), false };
    }

    bool is_frame_boundary = false;

    if ((submits_ptr != nullptr) && (submits_ptr->pCommandBuffers != nullptr))
    {
        for (uint32_t i = 0; i < submit_count; i++)
        {
            uint32_t num_command_buffers = submits_ptr[i].commandBufferCount;
            for (uint32_t c = 0; c < num_command_buffers; ++c)
            {
                const auto& cmd = submits_ptr[i].pCommandBuffers[c];
                if (m_cmds.find(cmd) == m_cmds.end())
                {
                    // We do not submit secondary command buffer
                    // All primary command buffers should be in the cache
                    m_valid_frame = false;
                    std::stringstream ss;
                    ss << static_cast<void*>(cmd) << " is not in the cmd cache!";
                    return { GPUTime::GpuTimeStatus{ ss.str(), false }, false };
                }

                if (m_cmds[cmd].reusable)
                {
                    m_valid_frame = false;
                    std::stringstream ss;
                    ss << static_cast<void*>(cmd) << " Reusable cmd is not supported!";
                    return { GPUTime::GpuTimeStatus{ ss.str(), false },
                             m_cmds[cmd].is_frameboundary };
                }

                if (m_cmds[cmd].is_frameboundary)
                {
                    is_frame_boundary = true;
                }

                // Check the case where the same primary cmd buffer is reused within a frame
                auto it = std::find(m_frame_cmds.begin(), m_frame_cmds.end(), cmd);
                if (it == m_frame_cmds.end())
                {
                    m_frame_cmds.push_back(cmd);
                }
            }
        }
    }

    if (is_frame_boundary)
    {
        //  force sync to make sure the gpu is done with this frame
        pfn_device_wait_idle(m_device);

        GPUTime::GpuTimeStatus update_status;
        if (m_valid_frame)
        {
            update_status = UpdateFrameMetrics(pfn_get_query_pool_results);
        }

        m_frame_index++;
        m_frame_cmds.clear();

        pfn_reset_query_pool(m_device, m_query_pool, 0, TimeStampSlotAllocator::kTotalSlots);
        m_valid_frame = true;
        if (!update_status.success)
        {
            return { update_status, true };
        }
    }
    return { GPUTime::GpuTimeStatus(), is_frame_boundary };
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
VkCommandBuffer             command_buffer,
const VkDebugUtilsLabelEXT* label_info_ptr)
{
    if (label_info_ptr == nullptr || label_info_ptr->pLabelName == nullptr)
    {
        return GPUTime::GpuTimeStatus{ "label_info_ptr cannot be nullptr!", false };
    }

    if (strcmp(kVulkanVrFrameDelimiterString, label_info_ptr->pLabelName) == 0)
    {
        // the Frame boundary should be always in a primary command buffer
        if (m_cmds.find(command_buffer) == m_cmds.end())
        {
            m_valid_frame = false;
            std::stringstream ss;
            ss << static_cast<void*>(command_buffer) << " is not in the cmd cache!";
            return GPUTime::GpuTimeStatus{ ss.str(), false };
        }
        m_cmds[command_buffer].is_frameboundary = true;
    }
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::OnCmdBeginRenderPass(
VkCommandBuffer         command_buffer,
PFN_vkCmdWriteTimestamp pfn_cmd_write_timestamp)
{
    if (!m_enable)
    {
        return GPUTime::GpuTimeStatus();
    }
    uint32_t slot = m_timestamp_allocator.AllocateSlot();
    m_cmds[command_buffer].renderpass_slots.push_back(slot);
    pfn_cmd_write_timestamp(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_query_pool, slot);
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::OnCmdEndRenderPass(VkCommandBuffer         command_buffer,
                                                   PFN_vkCmdWriteTimestamp pfn_cmd_write_timestamp)
{
    if (!m_enable)
    {
        return GPUTime::GpuTimeStatus();
    }
    uint32_t slot = m_timestamp_allocator.AllocateSlot();
    m_cmds[command_buffer].renderpass_slots.push_back(slot);
    pfn_cmd_write_timestamp(command_buffer,
                            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                            m_query_pool,
                            slot);
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::OnCmdBeginRenderPass2(
VkCommandBuffer         command_buffer,
PFN_vkCmdWriteTimestamp pfn_cmd_write_timestamp)
{
    if (!m_enable)
    {
        return GPUTime::GpuTimeStatus();
    }
    uint32_t slot = m_timestamp_allocator.AllocateSlot();
    m_cmds[command_buffer].renderpass_slots.push_back(slot);
    pfn_cmd_write_timestamp(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, m_query_pool, slot);
    return GPUTime::GpuTimeStatus();
}

GPUTime::GpuTimeStatus GPUTime::OnCmdEndRenderPass2(VkCommandBuffer         command_buffer,
                                                    PFN_vkCmdWriteTimestamp pfn_cmd_write_timestamp)
{
    if (!m_enable)
    {
        return GPUTime::GpuTimeStatus();
    }
    uint32_t slot = m_timestamp_allocator.AllocateSlot();
    m_cmds[command_buffer].renderpass_slots.push_back(slot);
    pfn_cmd_write_timestamp(command_buffer,
                            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                            m_query_pool,
                            slot);
    return GPUTime::GpuTimeStatus();
}

void Dive::GPUTime::ClearFrameCache()
{
    m_frame_cmds.clear();
}

}  // namespace Dive
