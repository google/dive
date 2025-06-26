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

#include "dive_vulkan_replay_consumer.h"
#include "vulkan/vulkan_core.h"

#include <iostream>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <limits>

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

static constexpr uint32_t kQueryCount = 256;
static constexpr uint32_t kFrameMetricsLimit = 1000;

// FrameMetrics
void FrameMetrics::AddFrameTime(double time)
{
    if (m_frame_data.size() == kFrameMetricsLimit)
    {
        m_frame_data.pop_front();
    }
    m_frame_data.push_back(time);
}

FrameMetrics::Stats FrameMetrics::GetStatistics() const
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

void FrameMetrics::PrintStats(const FrameMetrics::Stats& stats) 
{
    GFXRECON_LOG_INFO("FrameMetrics: ");
    GFXRECON_LOG_INFO("  Min: %f ms ", stats.min);
    GFXRECON_LOG_INFO("  Max: %f ms ", stats.max);
    GFXRECON_LOG_INFO("  Mean: %f ms ", stats.average);
    GFXRECON_LOG_INFO("  Median: %f ms ", stats.median);
    GFXRECON_LOG_INFO("  Std: %f ms ", stats.stddev);
}

double FrameMetrics::CalculateAverage() const
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

double FrameMetrics::CalculateMedian() const
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

double FrameMetrics::CalculateStdDev(double average) const
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

DiveVulkanReplayConsumer::DiveVulkanReplayConsumer(
std::shared_ptr<application::Application> application,
const VulkanReplayOptions&                options) :
    VulkanReplayConsumer(application, options),
    allocator_(nullptr),
    query_pool_(VK_NULL_HANDLE),
    device_(VK_NULL_HANDLE),
    frame_index_(false),
    timestamp_counter_(0),
    timestamp_period_(0.0f)
{
}

DiveVulkanReplayConsumer::~DiveVulkanReplayConsumer()
{
    DestroyQueryPool();
}

void DiveVulkanReplayConsumer::Process_vkDestroyCommandPool(
const ApiCallInfo&                                   call_info,
format::HandleId                                     device,
format::HandleId                                     commandPool,
StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator)
{
    auto          in_commandPool = GetObjectInfoTable().GetVkCommandPoolInfo(commandPool);
    VkCommandPool pool_handle = in_commandPool ? in_commandPool->handle : VK_NULL_HANDLE;
    if (pool_handle == VK_NULL_HANDLE)
    {
        return;
    }

    auto it = cmds_.begin();
    while (it != cmds_.end())
    {
        if (it->second.pool == pool_handle)
        {
            it = cmds_.erase(it);  // Erase and update iterator
        }
        else
        {
            ++it;  // Move to the next element
        }
    }

    VulkanReplayConsumer::Process_vkDestroyCommandPool(call_info, device, commandPool, pAllocator);
}

void DiveVulkanReplayConsumer::Process_vkAllocateCommandBuffers(
const ApiCallInfo&                                         call_info,
VkResult                                                   returnValue,
format::HandleId                                           device,
StructPointerDecoder<Decoded_VkCommandBufferAllocateInfo>* pAllocateInfo,
HandlePointerDecoder<VkCommandBuffer>*                     pCommandBuffers)
{
    VulkanReplayConsumer::Process_vkAllocateCommandBuffers(call_info,
                                                           returnValue,
                                                           device,
                                                           pAllocateInfo,
                                                           pCommandBuffers);

    if (returnValue == VK_SUCCESS)
    {
        for (uint32_t i = 0; i < pAllocateInfo->GetPointer()->commandBufferCount; ++i)
        {
            if (cmds_.find(pCommandBuffers->GetHandlePointer()[i]) != cmds_.end())
            {
                GFXRECON_LOG_ERROR("AllocateCommandBuffers %p has been already added!",
                                   pCommandBuffers->GetHandlePointer()[i]);
            }
            cmds_.insert(
            { pCommandBuffers->GetHandlePointer()[i],
              { pAllocateInfo->GetPointer()->commandPool, timestamp_counter_, false } });
            // 1 at vkBeginCommandBuffer and 1 at vkEndCommandBuffer
            timestamp_counter_ += 2;
        }
    }
}

void DiveVulkanReplayConsumer::Process_vkFreeCommandBuffers(
const ApiCallInfo&                     call_info,
format::HandleId                       device,
format::HandleId                       commandPool,
uint32_t                               commandBufferCount,
HandlePointerDecoder<VkCommandBuffer>* pCommandBuffers)
{
    for (uint32_t i = 0; i < commandBufferCount; ++i)
    {
        if (cmds_.find(pCommandBuffers->GetHandlePointer()[i]) == cmds_.end())
        {
            GFXRECON_LOG_ERROR("%p is not in the cmd cache!",
                               pCommandBuffers->GetHandlePointer()[i]);
        }
        cmds_.erase(pCommandBuffers->GetHandlePointer()[i]);
    }

    VulkanReplayConsumer::Process_vkFreeCommandBuffers(call_info,
                                                       device,
                                                       commandPool,
                                                       commandBufferCount,
                                                       pCommandBuffers);
}

void DiveVulkanReplayConsumer::Process_vkResetCommandBuffer(const ApiCallInfo&        call_info,
                                                            VkResult                  returnValue,
                                                            format::HandleId          commandBuffer,
                                                            VkCommandBufferResetFlags flags)
{
    auto in_commandBuffer = GetObjectInfoTable().GetVkCommandBufferInfo(commandBuffer);

    if (cmds_.find(in_commandBuffer->handle) == cmds_.end())
    {
        GFXRECON_LOG_ERROR("%p is not in the cmd cache!", in_commandBuffer->handle);
    }
    cmds_[in_commandBuffer->handle].Reset();

    VulkanReplayConsumer::Process_vkResetCommandBuffer(call_info,
                                                       returnValue,
                                                       commandBuffer,
                                                       flags);
}

void DiveVulkanReplayConsumer::Process_vkResetCommandPool(const ApiCallInfo&      call_info,
                                                          VkResult                returnValue,
                                                          format::HandleId        device,
                                                          format::HandleId        commandPool,
                                                          VkCommandPoolResetFlags flags)
{
    auto in_commandPool = GetObjectInfoTable().GetVkCommandPoolInfo(commandPool);
    if (in_commandPool == nullptr)
    {
        return;
    }

    for (auto& cmd : cmds_)
    {
        if (cmd.second.pool == in_commandPool->handle)
        {
            cmd.second.timestamp_offset = CommandBufferInfo::kInvalidTimeStampOffset;
            cmd.second.Reset();
        }
    }
    VulkanReplayConsumer::Process_vkResetCommandPool(call_info,
                                                     returnValue,
                                                     device,
                                                     commandPool,
                                                     flags);
}

void DiveVulkanReplayConsumer::Process_vkBeginCommandBuffer(
const ApiCallInfo&                                      call_info,
VkResult                                                returnValue,
format::HandleId                                        commandBuffer,
StructPointerDecoder<Decoded_VkCommandBufferBeginInfo>* pBeginInfo)
{
    VulkanReplayConsumer::Process_vkBeginCommandBuffer(call_info,
                                                       returnValue,
                                                       commandBuffer,
                                                       pBeginInfo);
    VkCommandBuffer in_commandBuffer = MapHandle<
    VulkanCommandBufferInfo>(commandBuffer, &CommonObjectInfoTable::GetVkCommandBufferInfo);

    if (cmds_.find(in_commandBuffer) == cmds_.end())
    {
        GFXRECON_LOG_ERROR("%p is not in the cmd cache!", in_commandBuffer);
    }

    if (cmds_[in_commandBuffer].usage_one_submit)
    {
        cmds_[in_commandBuffer].Reset();
    }

    if ((pBeginInfo->GetPointer()->flags & VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) != 0)
    {
        cmds_[in_commandBuffer].usage_one_submit = true;
    }

    GetDeviceTable(in_commandBuffer)
    ->CmdWriteTimestamp(in_commandBuffer,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        query_pool_,
                        cmds_[in_commandBuffer].timestamp_offset);
}

void DiveVulkanReplayConsumer::Process_vkEndCommandBuffer(const ApiCallInfo& call_info,
                                                          VkResult           returnValue,
                                                          format::HandleId   commandBuffer)
{
    VkCommandBuffer in_commandBuffer = MapHandle<
    VulkanCommandBufferInfo>(commandBuffer, &CommonObjectInfoTable::GetVkCommandBufferInfo);

    if (cmds_.find(in_commandBuffer) == cmds_.end())
    {
        GFXRECON_LOG_ERROR("%p is not in the cmd cache!", in_commandBuffer);
    }

    GetDeviceTable(in_commandBuffer)
    ->CmdWriteTimestamp(in_commandBuffer,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        query_pool_,
                        cmds_[in_commandBuffer].timestamp_offset + 1);

    VulkanReplayConsumer::Process_vkEndCommandBuffer(call_info, returnValue, commandBuffer);
}

void DiveVulkanReplayConsumer::Process_vkCreateDevice(
const ApiCallInfo&                                   call_info,
VkResult                                             returnValue,
format::HandleId                                     physicalDevice,
StructPointerDecoder<Decoded_VkDeviceCreateInfo>*    pCreateInfo,
StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
HandlePointerDecoder<VkDevice>*                      pDevice)
{
    VulkanReplayConsumer::Process_vkCreateDevice(call_info,
                                                 returnValue,
                                                 physicalDevice,
                                                 pCreateInfo,
                                                 pAllocator,
                                                 pDevice);

    if (returnValue != VK_SUCCESS)
    {
        return;
    }

    // Create a query pool for timestamps
    VkQueryPoolCreateInfo queryPoolInfo{};
    queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    queryPoolInfo.queryCount = kQueryCount;

    device_ = MapHandle<VulkanDeviceInfo>(*(pDevice->GetPointer()),
                                          &CommonObjectInfoTable::GetVkDeviceInfo);
    allocator_ = pAllocator->GetPointer();
    VkResult result = GetDeviceTable(device_)->CreateQueryPool(device_,
                                                               &queryPoolInfo,
                                                               allocator_,
                                                               &query_pool_);
    auto     in_physicalDevice = GetObjectInfoTable().GetVkPhysicalDeviceInfo(physicalDevice);
    VkPhysicalDeviceProperties deviceProperties;
    GetInstanceTable(in_physicalDevice->handle)
    ->GetPhysicalDeviceProperties(in_physicalDevice->handle, &deviceProperties);
    timestamp_period_ = deviceProperties.limits.timestampPeriod;
    std::cout << "Device Name: " << deviceProperties.deviceName << std::endl;
    std::cout << "Device Type: "
              << (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ?
                  "Discrete GPU" :
                  deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ?
                  "Integrated GPU" :
                  deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU ?
                  "Virtual GPU" :
                  deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU ? "CPU" :
                                                                               "Other")
              << std::endl;
    std::cout << "API Version: " << VK_VERSION_MAJOR(deviceProperties.apiVersion) << "."
              << VK_VERSION_MINOR(deviceProperties.apiVersion) << "."
              << VK_VERSION_PATCH(deviceProperties.apiVersion) << std::endl;

    GetDeviceTable(device_)->ResetQueryPool(device_, query_pool_, 0, kQueryCount);
}

void DiveVulkanReplayConsumer::Process_vkDestroyDevice(
const ApiCallInfo&                                   call_info,
format::HandleId                                     device,
StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator)
{
    VkDevice in_device = MapHandle<VulkanDeviceInfo>(device,
                                                     &CommonObjectInfoTable::GetVkDeviceInfo);
    assert(in_device == device_);

    DestroyQueryPool();

    device_ = VK_NULL_HANDLE;
    VulkanReplayConsumer::Process_vkDestroyDevice(call_info, device, pAllocator);
}

void DiveVulkanReplayConsumer::DestroyQueryPool()
{
    if ((device_ != VK_NULL_HANDLE) && (query_pool_ != VK_NULL_HANDLE))
    {
        assert(!queues_.empty());
        for (auto& q : queues_)
        {
            GetDeviceTable(device_)->QueueWaitIdle(q);
        }
        queues_.clear();

        for (auto& q : queues_)
        {
            GetDeviceTable(device_)->QueueWaitIdle(q);
        }
        GetDeviceTable(device_)->DestroyQueryPool(device_, query_pool_, allocator_);
        query_pool_ = VK_NULL_HANDLE;
        allocator_ = nullptr;
    }
}

void DiveVulkanReplayConsumer::UpdateFrameMetrics(VkDevice device)
{
    // Get the timestamp results
    // *2 for VK_QUERY_RESULT_WITH_AVAILABILITY_BIT
    uint64_t timestamps_with_availability[kQueryCount * 2];

    // VK_QUERY_RESULT_PARTIAL_BIT is used instead of VK_QUERY_RESULT_WAIT_BIT since some of the
    // timestamps may not be finished. we have some pre-recorded cmds that may not be replayed, but
    // the counter slot is reserved VK_QUERY_RESULT_WITH_AVAILABILITY_BIT is used so that we can
    // check for individual ones to make sure the value is valid

    constexpr size_t data_per_query = sizeof(uint64_t);          // For the result itself
    constexpr size_t availability_per_query = sizeof(uint64_t);  // For the availability status
    VkDeviceSize     data_size = timestamp_counter_ * (data_per_query + availability_per_query);
    constexpr VkDeviceSize stride = data_per_query + availability_per_query;

    VkResult r = GetDeviceTable(device_)
                 ->GetQueryPoolResults(device,
                                       query_pool_,
                                       0,
                                       timestamp_counter_,
                                       data_size,
                                       timestamps_with_availability,
                                       stride,
                                       VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_PARTIAL_BIT |
                                       VK_QUERY_RESULT_WITH_AVAILABILITY_BIT);

    if (r == VK_SUCCESS)
    {
        double frame_time = 0.0;
        bool   valid_frame_time = true;

        for (const auto& cmd : frame_cmds_)
        {
            // cmd may not be in the m_cmds when some cmds got deleted before submitting the frame
            // boundary cmd
            if (cmds_.find(cmd) != cmds_.end())
            {
                const uint32_t timestamp_offset = cmds_[cmd].timestamp_offset;

                uint64_t
                availability_end = timestamps_with_availability[(timestamp_offset + 1) * 2 + 1];
                uint64_t
                availability_begin = timestamps_with_availability[timestamp_offset * 2 + 1];

                if ((availability_begin != 0) && (availability_end != 0))
                {
                    // Calculate the elapsed time in nanoseconds
                    uint64_t
                    elapsed_time = timestamps_with_availability[(timestamp_offset + 1) * 2] -
                                   timestamps_with_availability[timestamp_offset * 2];
                    double elapsed_time_in_ms = elapsed_time * timestamp_period_ * 0.000001;
                    GFXRECON_LOG_INFO("Elapsed time: %f ms for cmd %p", elapsed_time_in_ms, cmd);
                    frame_time += elapsed_time_in_ms;
                }
                else
                {
                    frame_time = 0.0;
                    valid_frame_time = false;
                    GFXRECON_LOG_ERROR("Query result is not available for cmd %p", cmd);
                    break;
                }
            }
        }

        if (valid_frame_time)
        {
            metrics_.AddFrameTime(frame_time);
        }
        GFXRECON_LOG_INFO("Current Frame %" PRIu64
                           " has %d command buffers with a GPU time: %f ms",
             frame_index_,
             static_cast<uint32_t>(frame_cmds_.size()),
             frame_time);
    }

    metrics_.PrintStats(metrics_.GetStatistics());
}

void DiveVulkanReplayConsumer::Process_vkQueueSubmit(
const ApiCallInfo&                          call_info,
VkResult                                    returnValue,
format::HandleId                            queue,
uint32_t                                    submitCount,
StructPointerDecoder<Decoded_VkSubmitInfo>* pSubmits,
format::HandleId                            fence)
{
    VulkanReplayConsumer::Process_vkQueueSubmit(call_info,
                                                returnValue,
                                                queue,
                                                submitCount,
                                                pSubmits,
                                                fence);

    bool is_frame_boundary = false;

    const VkSubmitInfo* submit_infos = pSubmits->GetPointer();
    if ((submit_infos != nullptr) && (submit_infos->pCommandBuffers != nullptr))
    {
        for (uint32_t i = 0; i < submitCount; i++)
        {
            uint32_t num_command_buffers = submit_infos->commandBufferCount;
            for (uint32_t c = 0; c < num_command_buffers; ++c)
            {
                const auto& cmd = submit_infos->pCommandBuffers[c];
                if (cmds_.find(cmd) == cmds_.end())
                {
                    GFXRECON_LOG_ERROR("%p is not in the cmd cache!", cmd);
                }
                if (cmds_[cmd].is_frameboundary)
                {
                    is_frame_boundary = true;
                }
                frame_cmds_.push_back(cmd);
            }
        }
    }

    if (is_frame_boundary)
    {
        GFXRECON_LOG_INFO("is_frame_boundary!!!");
        // force sync to make sure the gpu is done with this frame
        GetDeviceTable(device_)->DeviceWaitIdle(device_);
        UpdateFrameMetrics(device_);
        frame_index_++;
        frame_cmds_.clear();

        GetDeviceTable(device_)->ResetQueryPool(device_, query_pool_, 0, kQueryCount);
    }
}

void DiveVulkanReplayConsumer::Process_vkGetDeviceQueue2(
const ApiCallInfo&                                call_info,
format::HandleId                                  device,
StructPointerDecoder<Decoded_VkDeviceQueueInfo2>* pQueueInfo,
HandlePointerDecoder<VkQueue>*                    pQueue)
{
    VulkanReplayConsumer::Process_vkGetDeviceQueue2(call_info, device, pQueueInfo, pQueue);
    VkQueue queue = *(pQueue->GetHandlePointer());
    queues_.insert(queue);
}

void DiveVulkanReplayConsumer::Process_vkGetDeviceQueue(const ApiCallInfo& call_info,
                                                        format::HandleId   device,
                                                        uint32_t           queueFamilyIndex,
                                                        uint32_t           queueIndex,
                                                        HandlePointerDecoder<VkQueue>* pQueue)
{
    VulkanReplayConsumer::Process_vkGetDeviceQueue(call_info,
                                                   device,
                                                   queueFamilyIndex,
                                                   queueIndex,
                                                   pQueue);
    VkQueue queue = *(pQueue->GetHandlePointer());
    queues_.insert(queue);
}

void DiveVulkanReplayConsumer::Process_vkCmdInsertDebugUtilsLabelEXT(
const ApiCallInfo&                                  call_info,
format::HandleId                                    commandBuffer,
StructPointerDecoder<Decoded_VkDebugUtilsLabelEXT>* pLabelInfo)
{
    VulkanReplayConsumer::Process_vkCmdInsertDebugUtilsLabelEXT(call_info,
                                                                commandBuffer,
                                                                pLabelInfo);

    VkCommandBuffer in_commandBuffer = MapHandle<
    VulkanCommandBufferInfo>(commandBuffer, &CommonObjectInfoTable::GetVkCommandBufferInfo);

    const VkDebugUtilsLabelEXT* in_pLabelInfo = pLabelInfo->GetPointer();

    if (strcmp("vr-marker,frame_end,type,application", pLabelInfo->GetPointer()->pLabelName) == 0)
    {
        GFXRECON_LOG_INFO("Detect Boundary!!! %p", in_commandBuffer);
        if (cmds_.find(in_commandBuffer) == cmds_.end())
        {
            GFXRECON_LOG_ERROR("%p is not in the cmd cache!", in_commandBuffer);
        }
        cmds_[in_commandBuffer].is_frameboundary = true;
    }
}

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)
