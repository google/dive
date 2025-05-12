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

#include "vk_rt_layer_impl.h"

#include <cstdio>
#include <cstdlib>
#if defined(__ANDROID__)
#    include <dlfcn.h>
#endif

#include <vulkan/vulkan_core.h>
#include "capture_service/log.h"

#include <inttypes.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace DiveLayer
{

static bool sEnableDrawcallReport = false;
static bool sEnableDrawcallLimit = false;
static bool sEnableDrawcallFilter = false;
// To use sEnableOpenXRGPUTiming, make sure to
//     - Disable system gpu preemption
//     - Insert "vr-marker,frame_end,type,application" as frame boundary
// Note that the performance will drop due to vkDeviceWaitIdle
// Setting sEnableOpenXRGPUTiming to false will NOT disable adding/removing cmds to/from m_cmds
// This is to support future possible options to enable/disable XR GPU Timing at runtime
static bool sEnableOpenXRGPUTiming = false;
static bool sRemoveImageFlagFDMOffset = false;
static bool sRemoveImageFlagSubSampled = false;
static bool sDisableTimestamp = false;

static uint32_t sDrawcallCounter = 0;
static size_t   sTotalIndexCounter = 0;

static constexpr uint32_t kDrawcallCountLimit = 300;
static constexpr uint32_t kVisibilityMaskIndexCount = 42;
static constexpr uint32_t kQueryCount = 256;

static constexpr uint32_t kFrameMetricsLimit = 1000;

// FrameMetrics
void DiveRuntimeLayer::FrameMetrics::AddFrameTime(double time)
{
    if (m_frame_data.size() == kFrameMetricsLimit)
    {
        m_frame_data.pop_front();
    }
    m_frame_data.push_back(time);
}

DiveRuntimeLayer::FrameMetrics::Stats DiveRuntimeLayer::FrameMetrics::GetStatistics() const
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

void DiveRuntimeLayer::FrameMetrics::PrintStats(const FrameMetrics::Stats& stats)
{
    LOGI("FrameMetrics: ");
    LOGI("  Min: %f ms ", stats.min);
    LOGI("  Max: %f ms ", stats.max);
    LOGI("  Mean: %f ms ", stats.average);
    LOGI("  Median: %f ms ", stats.median);
    LOGI("  Std: %f ms ", stats.stddev);
}

double DiveRuntimeLayer::FrameMetrics::CalculateAverage() const
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

double DiveRuntimeLayer::FrameMetrics::CalculateMedian() const
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

double DiveRuntimeLayer::FrameMetrics::CalculateStdDev(double average) const
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

// DiveRuntimeLayer
DiveRuntimeLayer::DiveRuntimeLayer() :
    m_device_proc_addr(nullptr),
    m_allocator(nullptr),
    m_query_pool(VK_NULL_HANDLE),
    m_device(VK_NULL_HANDLE),
    m_frame_index(0),
    m_timestamp_counter(0),
    m_timestamp_period(0.0f)
{
}

DiveRuntimeLayer::~DiveRuntimeLayer() {}

VkResult DiveRuntimeLayer::QueuePresentKHR(PFN_vkQueuePresentKHR   pfn,
                                           VkQueue                 queue,
                                           const VkPresentInfoKHR* pPresentInfo)
{
    // Be careful, this func is NOT called for OpenXR app!!!
    return pfn(queue, pPresentInfo);
}

VkResult DiveRuntimeLayer::CreateImage(PFN_vkCreateImage            pfn,
                                       VkDevice                     device,
                                       const VkImageCreateInfo*     pCreateInfo,
                                       const VkAllocationCallbacks* pAllocator,
                                       VkImage*                     pImage)
{
    // Remove VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM flag
    if (sRemoveImageFlagFDMOffset &&
        ((pCreateInfo->flags & VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM) != 0))
    {
        LOGI("Image %p CreateImage has the density map offset flag! ", pImage);
        const_cast<VkImageCreateInfo*>(pCreateInfo)
        ->flags &= ~VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM;
    }

    if (sRemoveImageFlagSubSampled &&
        ((pCreateInfo->flags & VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT) != 0))
    {
        LOGI("Image %p CreateImage has the subsampled bit flag! ", pImage);
        const_cast<VkImageCreateInfo*>(pCreateInfo)->flags &= ~VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT;
    }

    return pfn(device, pCreateInfo, pAllocator, pImage);
}

void DiveRuntimeLayer::CmdDrawIndexed(PFN_vkCmdDrawIndexed pfn,
                                      VkCommandBuffer      commandBuffer,
                                      uint32_t             indexCount,
                                      uint32_t             instanceCount,
                                      uint32_t             firstIndex,
                                      int32_t              vertexOffset,
                                      uint32_t             firstInstance)
{
    //  Disable drawcalls with N index count
    //  Specifically for visibility mask:
    //  BiRP is using 2 drawcalls with 42 each, URP is using 1 drawcall with 84,
    if (sEnableDrawcallFilter && ((indexCount == kVisibilityMaskIndexCount) ||
                                  (indexCount == kVisibilityMaskIndexCount * 2)))
    {
        LOGI("Skip drawcalls with index count of %d & %d",
             kVisibilityMaskIndexCount,
             kVisibilityMaskIndexCount * 2);
        return;
    }

    ++sDrawcallCounter;
    sTotalIndexCounter += indexCount;

    if (sEnableDrawcallLimit && (sDrawcallCounter > kDrawcallCountLimit))
    {
        return;
    }

    return pfn(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void DiveRuntimeLayer::CmdResetQueryPool(PFN_vkCmdResetQueryPool pfn,
                                         VkCommandBuffer         commandBuffer,
                                         VkQueryPool             queryPool,
                                         uint32_t                firstQuery,
                                         uint32_t                queryCount)
{
    if (sDisableTimestamp)
    {
        return;
    }
    pfn(commandBuffer, queryPool, firstQuery, queryCount);
}

void DiveRuntimeLayer::CmdWriteTimestamp(PFN_vkCmdWriteTimestamp pfn,
                                         VkCommandBuffer         commandBuffer,
                                         VkPipelineStageFlagBits pipelineStage,
                                         VkQueryPool             queryPool,
                                         uint32_t                query)
{
    if (sDisableTimestamp)
    {
        return;
    }
    pfn(commandBuffer, pipelineStage, queryPool, query);
}

VkResult DiveRuntimeLayer::GetQueryPoolResults(PFN_vkGetQueryPoolResults pfn,
                                               VkDevice                  device,
                                               VkQueryPool               queryPool,
                                               uint32_t                  firstQuery,
                                               uint32_t                  queryCount,
                                               size_t                    dataSize,
                                               void*                     pData,
                                               VkDeviceSize              stride,
                                               VkQueryResultFlags        flags)
{
    if (sDisableTimestamp)
    {
        return VK_SUCCESS;
    }

    return pfn(device, queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);
}

void DiveRuntimeLayer::DestroyCommandPool(PFN_vkDestroyCommandPool     pfn,
                                          VkDevice                     device,
                                          VkCommandPool                commandPool,
                                          const VkAllocationCallbacks* pAllocator)
{
    auto it = m_cmds.begin();
    while (it != m_cmds.end())
    {
        if (it->second.pool == commandPool)
        {
            it = m_cmds.erase(it);  // Erase and update iterator
        }
        else
        {
            ++it;  // Move to the next element
        }
    }
    return pfn(device, commandPool, pAllocator);
}

VkResult DiveRuntimeLayer::AllocateCommandBuffers(PFN_vkAllocateCommandBuffers       pfn,
                                                  VkDevice                           device,
                                                  const VkCommandBufferAllocateInfo* pAllocateInfo,
                                                  VkCommandBuffer* pCommandBuffers)
{
    VkResult result = pfn(device, pAllocateInfo, pCommandBuffers);
    if (result == VK_SUCCESS)
    {
        for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; ++i)
        {
            if (m_cmds.find(pCommandBuffers[i]) != m_cmds.end())
            {
                LOGE("AllocateCommandBuffers %p has been already added!", pCommandBuffers[i]);
            }
            m_cmds.insert(
            { pCommandBuffers[i], { pAllocateInfo->commandPool, m_timestamp_counter, false } });
            // 1 at vkBeginCommandBuffer and 1 at vkEndCommandBuffer
            m_timestamp_counter += 2;
        }
    }

    return result;
}

void DiveRuntimeLayer::FreeCommandBuffers(PFN_vkFreeCommandBuffers pfn,
                                          VkDevice                 device,
                                          VkCommandPool            commandPool,
                                          uint32_t                 commandBufferCount,
                                          const VkCommandBuffer*   pCommandBuffers)
{
    for (uint32_t i = 0; i < commandBufferCount; ++i)
    {
        if (m_cmds.find(pCommandBuffers[i]) == m_cmds.end())
        {
            LOGE("%p is not in the cmd cache!", pCommandBuffers[i]);
        }
        m_cmds.erase(pCommandBuffers[i]);
    }
    return pfn(device, commandPool, commandBufferCount, pCommandBuffers);
}

VkResult DiveRuntimeLayer::ResetCommandBuffer(PFN_vkResetCommandBuffer  pfn,
                                              VkCommandBuffer           commandBuffer,
                                              VkCommandBufferResetFlags flags)
{
    if (m_cmds.find(commandBuffer) == m_cmds.end())
    {
        LOGE("%p is not in the cmd cache!", commandBuffer);
    }
    m_cmds[commandBuffer].Reset();

    return pfn(commandBuffer, flags);
}

VkResult DiveRuntimeLayer::ResetCommandPool(PFN_vkResetCommandPool  pfn,
                                            VkDevice                device,
                                            VkCommandPool           commandPool,
                                            VkCommandPoolResetFlags flags)
{
    for (auto& cmd : m_cmds)
    {
        if (cmd.second.pool == commandPool)
        {
            cmd.second.Reset();
        }
    }

    return pfn(device, commandPool, flags);
}

VkResult DiveRuntimeLayer::BeginCommandBuffer(PFN_vkBeginCommandBuffer        pfn,
                                              VkCommandBuffer                 commandBuffer,
                                              const VkCommandBufferBeginInfo* pBeginInfo)
{
    VkResult result = pfn(commandBuffer, pBeginInfo);
    if (sEnableDrawcallReport)
    {
        LOGI("Drawcall count: %d", sDrawcallCounter);
        LOGI("Total index count: %zd", sTotalIndexCounter);
    }

    sDrawcallCounter = 0;
    sTotalIndexCounter = 0;

    if (sEnableOpenXRGPUTiming)
    {
        if (m_cmds.find(commandBuffer) == m_cmds.end())
        {
            LOGE("%p is not in the cmd cache!", commandBuffer);
        }

        if (m_cmds[commandBuffer].usage_one_submit)
        {
            m_cmds[commandBuffer].Reset();
        }

        if ((pBeginInfo->flags & VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) != 0)
        {
            m_cmds[commandBuffer].usage_one_submit = true;
        }

        PFN_vkCmdWriteTimestamp CmdWriteTimestamp = (PFN_vkCmdWriteTimestamp)
        m_device_proc_addr(m_device, "vkCmdWriteTimestamp");
        CmdWriteTimestamp(commandBuffer,
                          VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                          m_query_pool,
                          m_cmds[commandBuffer].timestamp_offset);
    }
    return result;
}

VkResult DiveRuntimeLayer::EndCommandBuffer(PFN_vkEndCommandBuffer pfn,
                                            VkCommandBuffer        commandBuffer)
{
    if (m_cmds.find(commandBuffer) == m_cmds.end())
    {
        LOGE("%p is not in the cmd cache!", commandBuffer);
    }

    if (sEnableOpenXRGPUTiming)
    {
        PFN_vkCmdWriteTimestamp CmdWriteTimestamp = (PFN_vkCmdWriteTimestamp)
        m_device_proc_addr(m_device, "vkCmdWriteTimestamp");
        CmdWriteTimestamp(commandBuffer,
                          VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                          m_query_pool,
                          m_cmds[commandBuffer].timestamp_offset + 1);
    }
    return pfn(commandBuffer);
}

VkResult DiveRuntimeLayer::CreateDevice(PFN_vkGetDeviceProcAddr      pa,
                                        PFN_vkCreateDevice           pfn,
                                        float                        timestampPeriod,
                                        VkPhysicalDevice             physicalDevice,
                                        const VkDeviceCreateInfo*    pCreateInfo,
                                        const VkAllocationCallbacks* pAllocator,
                                        VkDevice*                    pDevice)
{

    m_allocator = pAllocator;

    VkResult result = pfn(physicalDevice, pCreateInfo, pAllocator, pDevice);
    m_device = *pDevice;

    m_device_proc_addr = pa;

    if (result == VK_SUCCESS)
    {
        PFN_vkCreateQueryPool CreateQueryPool = (PFN_vkCreateQueryPool)pa(m_device,
                                                                          "vkCreateQueryPool");

        // Create a query pool for timestamps
        VkQueryPoolCreateInfo queryPoolInfo{};
        queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queryPoolInfo.queryCount = kQueryCount;

        result = CreateQueryPool(m_device, &queryPoolInfo, m_allocator, &m_query_pool);
        m_timestamp_period = timestampPeriod;

        PFN_vkResetQueryPool ResetQueryPool = (PFN_vkResetQueryPool)
        m_device_proc_addr(m_device, "vkResetQueryPool");
        ResetQueryPool(m_device, m_query_pool, 0, kQueryCount);
    }

    return result;
}

void DiveRuntimeLayer::DestroyDevice(PFN_vkDestroyDevice          pfn,
                                     VkDevice                     device,
                                     const VkAllocationCallbacks* pAllocator)
{
    if (device != m_device)
    {
        LOGI("Wrong device while destroying device!!!");
    }

    if ((m_device != VK_NULL_HANDLE) && (m_query_pool != VK_NULL_HANDLE))
    {
        if (m_device_proc_addr == nullptr)
        {
            LOGI("Device proc addr is nullptr!!!");
            return;
        }

        PFN_vkQueueWaitIdle QueueWaitIdle = (PFN_vkQueueWaitIdle)
        m_device_proc_addr(m_device, "vkQueueWaitIdle");

        if (m_queues.empty())
        {
            LOGI("vk queue is empty!");
        }

        for (auto& q : m_queues)
        {
            QueueWaitIdle(q);
        }
        m_queues.clear();

        PFN_vkDestroyQueryPool DestroyQueryPool = (PFN_vkDestroyQueryPool)
        m_device_proc_addr(m_device, "vkDestroyQueryPool");

        DestroyQueryPool(m_device, m_query_pool, m_allocator);
        m_query_pool = VK_NULL_HANDLE;
        m_allocator = nullptr;
    }

    m_device = VK_NULL_HANDLE;

    pfn(device, pAllocator);
}

void DiveRuntimeLayer::UpdateFrameMetrics(VkDevice device)
{
    // Get the timestamp results
    // *2 for VK_QUERY_RESULT_WITH_AVAILABILITY_BIT
    uint64_t timestamps_with_availability[kQueryCount * 2];

    PFN_vkGetQueryPoolResults GetQueryPoolResults = (PFN_vkGetQueryPoolResults)
    m_device_proc_addr(device, "vkGetQueryPoolResults");
    // VK_QUERY_RESULT_PARTIAL_BIT is used instead of VK_QUERY_RESULT_WAIT_BIT since some of the
    // timestamps may not be finished. we have some pre-recorded cmds that may not be replayed, but
    // the counter slot is reserved VK_QUERY_RESULT_WITH_AVAILABILITY_BIT is used so that we can
    // check for individual ones to make sure the value is valid

    constexpr size_t data_per_query = sizeof(uint64_t);          // For the result itself
    constexpr size_t availability_per_query = sizeof(uint64_t);  // For the availability status
    VkDeviceSize     data_size = m_timestamp_counter * (data_per_query + availability_per_query);
    constexpr VkDeviceSize stride = data_per_query + availability_per_query;

    VkResult r = GetQueryPoolResults(device,
                                     m_query_pool,
                                     0,
                                     m_timestamp_counter,
                                     data_size,
                                     timestamps_with_availability,
                                     stride,
                                     VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_PARTIAL_BIT |
                                     VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);

    if (r == VK_SUCCESS)
    {
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
                availability_begin = timestamps_with_availability[(timestamp_offset + 1) * 2 + 1];
                uint64_t availability_end = timestamps_with_availability[timestamp_offset * 2 + 1];

                if ((availability_begin != 0) && (availability_end != 0))
                {
                    // Calculate the elapsed time in nanoseconds
                    uint64_t
                    elapsed_time = timestamps_with_availability[(timestamp_offset + 1) * 2] -
                                   timestamps_with_availability[timestamp_offset * 2];
                    double elapsed_time_in_ms = elapsed_time * m_timestamp_period * 0.000001;
                    LOGI("Elapsed time: %f ms for cmd %p", elapsed_time_in_ms, cmd);
                    frame_time += elapsed_time_in_ms;
                }
                else
                {
                    frame_time = 0.0;
                    valid_frame_time = false;
                    LOGE("Query result is not available for cmd %p", cmd);
                    break;
                }
            }
        }

        if (valid_frame_time)
        {
            m_metrics.AddFrameTime(frame_time);
        }
        LOGI("Current Frame %" PRIu64 " has %d command buffers with a GPU time: %f ms",
             m_frame_index,
             static_cast<uint32_t>(m_frame_cmds.size()),
             frame_time);
    }

    m_metrics.PrintStats(m_metrics.GetStatistics());
}

VkResult DiveRuntimeLayer::AcquireNextImageKHR(PFN_vkAcquireNextImageKHR pfn,
                                               VkDevice                  device,
                                               VkSwapchainKHR            swapchain,
                                               uint64_t                  timeout,
                                               VkSemaphore               semaphore,
                                               VkFence                   fence,
                                               uint32_t*                 pImageIndex)
{
    // Be careful, this func is NOT called for OpenXR app!!!
    return pfn(device, swapchain, timeout, semaphore, fence, pImageIndex);
}

VkResult DiveRuntimeLayer::QueueSubmit(PFN_vkQueueSubmit   pfn,
                                       VkQueue             queue,
                                       uint32_t            submitCount,
                                       const VkSubmitInfo* pSubmits,
                                       VkFence             fence)
{
    bool is_frame_boundary = false;
    if (sEnableOpenXRGPUTiming)
    {
        for (uint32_t i = 0; i < submitCount; ++i)
        {
            const auto& submit = pSubmits[i];
            for (uint32_t j = 0; j < submit.commandBufferCount; ++j)
            {
                const auto& cmd = submit.pCommandBuffers[j];
                if (m_cmds.find(cmd) == m_cmds.end())
                {
                    LOGE("%p is not in the cmd cache!", cmd);
                }
                if (m_cmds[cmd].is_frameboundary)
                {
                    is_frame_boundary = true;
                }
                m_frame_cmds.push_back(cmd);
            }
        }
    }

    VkResult result = pfn(queue, submitCount, pSubmits, fence);

    if (sEnableOpenXRGPUTiming && is_frame_boundary)
    {
        // force sync to make sure the gpu is done with this frame
        PFN_vkDeviceWaitIdle DeviceWaitIdle = (PFN_vkDeviceWaitIdle)
        m_device_proc_addr(m_device, "vkDeviceWaitIdle");
        DeviceWaitIdle(m_device);
        UpdateFrameMetrics(m_device);
        m_frame_index++;
        m_frame_cmds.clear();

        PFN_vkResetQueryPool ResetQueryPool = (PFN_vkResetQueryPool)
        m_device_proc_addr(m_device, "vkResetQueryPool");
        ResetQueryPool(m_device, m_query_pool, 0, kQueryCount);
    }
    return result;
}

void DiveRuntimeLayer::GetDeviceQueue2(PFN_vkGetDeviceQueue2     pfn,
                                       VkDevice                  device,
                                       const VkDeviceQueueInfo2* pQueueInfo,
                                       VkQueue*                  pQueue)
{
    pfn(device, pQueueInfo, pQueue);
    if (pQueue != nullptr)
    {
        m_queues.insert(*pQueue);
    }
}

void DiveRuntimeLayer::GetDeviceQueue(PFN_vkGetDeviceQueue pfn,
                                      VkDevice             device,
                                      uint32_t             queueFamilyIndex,
                                      uint32_t             queueIndex,
                                      VkQueue*             pQueue)
{
    pfn(device, queueFamilyIndex, queueIndex, pQueue);
    if (pQueue != nullptr)
    {
        m_queues.insert(*pQueue);
    }
}

void DiveRuntimeLayer::CmdInsertDebugUtilsLabel(PFN_vkCmdInsertDebugUtilsLabelEXT pfn,
                                                VkCommandBuffer                   commandBuffer,
                                                const VkDebugUtilsLabelEXT*       pLabelInfo)
{
    pfn(commandBuffer, pLabelInfo);

    if (strcmp("vr-marker,frame_end,type,application", pLabelInfo->pLabelName) == 0)
    {
        if (m_cmds.find(commandBuffer) == m_cmds.end())
        {
            LOGE("%p is not in the cmd cache!", commandBuffer);
        }
        m_cmds[commandBuffer].is_frameboundary = true;
    }
}

}  // namespace DiveLayer