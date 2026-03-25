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
#include <dlfcn.h>
#endif

#include <inttypes.h>
#include <vulkan/vulkan_core.h>

#include <algorithm>
#include <cmath>
#include <cstring>

#include "common/log.h"

namespace DiveLayer
{

static bool sEnableDrawcallReport = false;
static bool sEnableDrawcallLimit = false;
static bool sEnableDrawcallFilter = false;

// For OpenXR Apps, this requires enabling the frame delimiter
static bool sEnableGPUTiming = false;
static bool sRemoveImageFlagFDMOffset = false;
static bool sRemoveImageFlagSubSampled = false;
static bool sDisableTimestamp = false;

static uint32_t sDrawcallCounter = 0;
static size_t sTotalIndexCounter = 0;

static constexpr uint32_t kDrawcallCountLimit = 300;
static constexpr uint32_t kVisibilityMaskIndexCount = 42;

// Because Vulkan guarantees that a specific VkCommandBuffer is only recorded by a single CPU thread
// at a time, a thread_local map is mathematically guaranteed to never experience a data race. It
// requires absolutely no mutexes.
// This map exists independently on every CPU core. No locks required!
// Also, this keeps track of the current pipeline alpha state.
static thread_local absl::flat_hash_map<VkCommandBuffer, bool> sCmdBufferCurrentPipelineHasAlpha;

// The maximum number of command buffers that can be tracked simultaneously by the local thread.
// It is used to prevent stale data for sCmdBufferCurrentPipelineHasAlpha.
static constexpr uint32_t kMaxConcurrentCBs = 512;

// DiveRuntimeLayer
DiveRuntimeLayer::DiveRuntimeLayer() : m_device_proc_addr(nullptr) {}

DiveRuntimeLayer::~DiveRuntimeLayer() {}

VkResult DiveRuntimeLayer::QueuePresentKHR(PFN_vkQueuePresentKHR pfn, VkQueue queue,
                                           const VkPresentInfoKHR* pPresentInfo)
{
    // Process frame boundary tasks for non-OpenXR apps.
    ProcessFrameBoundaryTasks();

    // Be careful, this func is NOT called for OpenXR app!!!
    VkResult result = pfn(queue, pPresentInfo);

    if (result != VK_SUCCESS)
    {
        return result;
    }

    auto status = m_gpu_time.OnQueuePresent(m_pfn_vkDeviceWaitIdle, m_pfn_vkResetQueryPool,
                                            m_pfn_vkGetQueryPoolResults);

    if (!status.success)
    {
        LOGE("Frame Boundary!");
        LOGE("%s", status.message.c_str());
    }
    else
    {
        LOGI("%s", m_gpu_time.GetStatsString().c_str());
    }

    return result;
}

VkResult DiveRuntimeLayer::CreateImage(PFN_vkCreateImage pfn, VkDevice device,
                                       const VkImageCreateInfo* pCreateInfo,
                                       const VkAllocationCallbacks* pAllocator, VkImage* pImage)
{
    // Remove VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM flag
    if (sRemoveImageFlagFDMOffset &&
        ((pCreateInfo->flags & VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM) != 0))
    {
        LOGI("Image %p CreateImage has the density map offset flag! ", pImage);
        const_cast<VkImageCreateInfo*>(pCreateInfo)->flags &=
            ~VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM;
    }

    if (sRemoveImageFlagSubSampled &&
        ((pCreateInfo->flags & VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT) != 0))
    {
        LOGI("Image %p CreateImage has the subsampled bit flag! ", pImage);
        const_cast<VkImageCreateInfo*>(pCreateInfo)->flags &= ~VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT;
    }

    return pfn(device, pCreateInfo, pAllocator, pImage);
}

VkResult DiveRuntimeLayer::CreateGraphicsPipelines(PFN_vkCreateGraphicsPipelines pfn,
                                                   VkDevice device, VkPipelineCache pipelineCache,
                                                   uint32_t createInfoCount,
                                                   const VkGraphicsPipelineCreateInfo* pCreateInfos,
                                                   const VkAllocationCallbacks* pAllocator,
                                                   VkPipeline* pPipelines)
{
    VkResult result =
        pfn(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    if (result == VK_SUCCESS)
    {
        std::unique_lock<std::shared_mutex> lock(m_pso_mutex);
        for (uint32_t i = 0; i < createInfoCount; ++i)
        {
            bool has_alpha = false;
            if (pCreateInfos[i].pColorBlendState)
            {
                for (uint32_t j = 0; j < pCreateInfos[i].pColorBlendState->attachmentCount; ++j)
                {
                    if (pCreateInfos[i].pColorBlendState->pAttachments[j].blendEnable == VK_TRUE)
                    {
                        has_alpha = true;
                        break;
                    }
                }
            }
            Network::PSOInfo info{
                .name = has_alpha ? "PSO_alpha_blend_enabled" : "PSO_opaque",
                .pipeline_handle = reinterpret_cast<uint64_t>(pPipelines[i]),
                .has_alpha_blend = has_alpha,
            };
            m_live_psos[pPipelines[i]] = info;
        }
    }
    return result;
}

void DiveRuntimeLayer::DestroyPipeline(PFN_vkDestroyPipeline pfn, VkDevice device,
                                       VkPipeline pipeline, const VkAllocationCallbacks* pAllocator)
{
    {
        std::unique_lock<std::shared_mutex> lock(m_pso_mutex);
        m_live_psos.erase(pipeline);
    }
    pfn(device, pipeline, pAllocator);
}

VkResult DiveRuntimeLayer::SetDebugUtilsObjectNameEXT(
    PFN_vkSetDebugUtilsObjectNameEXT pfn, VkDevice device,
    const VkDebugUtilsObjectNameInfoEXT* pNameInfo)
{
    if (pNameInfo && pNameInfo->objectType == VK_OBJECT_TYPE_PIPELINE && pNameInfo->pObjectName)
    {
        std::unique_lock<std::shared_mutex> lock(m_pso_mutex);
        VkPipeline pipeline = (VkPipeline)pNameInfo->objectHandle;
        if (auto it = m_live_psos.find(pipeline); it != m_live_psos.end())
        {
            it->second.name = pNameInfo->pObjectName;
        }
    }
    if (pfn)
    {
        return pfn(device, pNameInfo);
    }
    return VK_SUCCESS;
}

void DiveRuntimeLayer::CmdBindPipeline(PFN_vkCmdBindPipeline pfn, VkCommandBuffer commandBuffer,
                                       VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
    if (pipelineBindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS)
    {
        bool has_alpha = false;
        {
            // Use a shared (read-only) lock here to allow multithreaded command recording
            // without contention. This populates the lock-free thread_local cache
            // (sCmdBufferCurrentPipelineHasAlpha) used by the ultra-hot CmdDraw paths.
            std::shared_lock<std::shared_mutex> lock(m_pso_mutex);
            if (auto it = m_live_psos.find(pipeline); it != m_live_psos.end())
            {
                has_alpha = it->second.has_alpha_blend;
            }
        }
        sCmdBufferCurrentPipelineHasAlpha[commandBuffer] = has_alpha;
    }
    pfn(commandBuffer, pipelineBindPoint, pipeline);
}

void DiveRuntimeLayer::CmdDraw(PFN_vkCmdDraw pfn, VkCommandBuffer commandBuffer,
                               uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                               uint32_t firstInstance)
{
    if (ShouldFilterDrawCall(commandBuffer, vertexCount, std::nullopt, instanceCount))
    {
        return;
    }
    if (CheckAndIncrementDrawcallCount())
    {
        return;
    }

    pfn(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void DiveRuntimeLayer::CmdDrawIndexed(PFN_vkCmdDrawIndexed pfn, VkCommandBuffer commandBuffer,
                                      uint32_t indexCount, uint32_t instanceCount,
                                      uint32_t firstIndex, int32_t vertexOffset,
                                      uint32_t firstInstance)
{
    if (ShouldFilterDrawCall(commandBuffer, std::nullopt, indexCount, instanceCount))
    {
        return;
    }
    if (CheckAndIncrementDrawcallCount())
    {
        return;
    }

    //  Disable drawcalls with N index count
    //  Specifically for visibility mask:
    //  BiRP is using 2 drawcalls with 42 each, URP is using 1 drawcall with 84,
    if (sEnableDrawcallFilter && ((indexCount == kVisibilityMaskIndexCount) ||
                                  (indexCount == kVisibilityMaskIndexCount * 2)))
    {
        LOGI("Skip drawcalls with index count of %d & %d", kVisibilityMaskIndexCount,
             kVisibilityMaskIndexCount * 2);
        return;
    }

    ++sDrawcallCounter;
    sTotalIndexCounter += indexCount;

    if (sEnableDrawcallLimit && (sDrawcallCounter > kDrawcallCountLimit))
    {
        return;
    }

    pfn(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void DiveRuntimeLayer::CmdDrawIndirect(PFN_vkCmdDrawIndirect pfn, VkCommandBuffer commandBuffer,
                                       VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount,
                                       uint32_t stride)
{
    if (ShouldFilterDrawCall(commandBuffer))
    {
        return;
    }
    if (CheckAndIncrementDrawcallCount())
    {
        return;
    }

    pfn(commandBuffer, buffer, offset, drawCount, stride);
}

void DiveRuntimeLayer::CmdDrawIndexedIndirect(PFN_vkCmdDrawIndexedIndirect pfn,
                                              VkCommandBuffer commandBuffer, VkBuffer buffer,
                                              VkDeviceSize offset, uint32_t drawCount,
                                              uint32_t stride)
{
    if (ShouldFilterDrawCall(commandBuffer))
    {
        return;
    }
    if (CheckAndIncrementDrawcallCount())
    {
        return;
    }

    pfn(commandBuffer, buffer, offset, drawCount, stride);
}

void DiveRuntimeLayer::CmdDrawIndirectCount(PFN_vkCmdDrawIndirectCount pfn,
                                            VkCommandBuffer commandBuffer, VkBuffer buffer,
                                            VkDeviceSize offset, VkBuffer countBuffer,
                                            VkDeviceSize countBufferOffset, uint32_t maxDrawCount,
                                            uint32_t stride)
{
    if (ShouldFilterDrawCall(commandBuffer))
    {
        return;
    }
    if (CheckAndIncrementDrawcallCount())
    {
        return;
    }

    pfn(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

void DiveRuntimeLayer::CmdDrawIndexedIndirectCount(PFN_vkCmdDrawIndexedIndirectCount pfn,
                                                   VkCommandBuffer commandBuffer, VkBuffer buffer,
                                                   VkDeviceSize offset, VkBuffer countBuffer,
                                                   VkDeviceSize countBufferOffset,
                                                   uint32_t maxDrawCount, uint32_t stride)
{
    if (ShouldFilterDrawCall(commandBuffer))
    {
        return;
    }
    if (CheckAndIncrementDrawcallCount())
    {
        return;
    }

    pfn(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

void DiveRuntimeLayer::CmdDrawMeshTasksEXT(PFN_vkCmdDrawMeshTasksEXT pfn,
                                           VkCommandBuffer commandBuffer, uint32_t groupCountX,
                                           uint32_t groupCountY, uint32_t groupCountZ)
{
    if (ShouldFilterDrawCall(commandBuffer))
    {
        return;
    }
    if (CheckAndIncrementDrawcallCount())
    {
        return;
    }

    pfn(commandBuffer, groupCountX, groupCountY, groupCountZ);
}

void DiveRuntimeLayer::CmdDrawMeshTasksIndirectEXT(PFN_vkCmdDrawMeshTasksIndirectEXT pfn,
                                                   VkCommandBuffer commandBuffer, VkBuffer buffer,
                                                   VkDeviceSize offset, uint32_t drawCount,
                                                   uint32_t stride)
{
    if (ShouldFilterDrawCall(commandBuffer))
    {
        return;
    }
    if (CheckAndIncrementDrawcallCount())
    {
        return;
    }

    pfn(commandBuffer, buffer, offset, drawCount, stride);
}

void DiveRuntimeLayer::CmdDrawMeshTasksIndirectCountEXT(PFN_vkCmdDrawMeshTasksIndirectCountEXT pfn,
                                                        VkCommandBuffer commandBuffer,
                                                        VkBuffer buffer, VkDeviceSize offset,
                                                        VkBuffer countBuffer,
                                                        VkDeviceSize countBufferOffset,
                                                        uint32_t maxDrawCount, uint32_t stride)
{
    if (ShouldFilterDrawCall(commandBuffer))
    {
        return;
    }
    if (CheckAndIncrementDrawcallCount())
    {
        return;
    }

    pfn(commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

void DiveRuntimeLayer::CmdResetQueryPool(PFN_vkCmdResetQueryPool pfn, VkCommandBuffer commandBuffer,
                                         VkQueryPool queryPool, uint32_t firstQuery,
                                         uint32_t queryCount)
{
    if (sDisableTimestamp)
    {
        return;
    }
    pfn(commandBuffer, queryPool, firstQuery, queryCount);
}

void DiveRuntimeLayer::CmdWriteTimestamp(PFN_vkCmdWriteTimestamp pfn, VkCommandBuffer commandBuffer,
                                         VkPipelineStageFlagBits pipelineStage,
                                         VkQueryPool queryPool, uint32_t query)
{
    if (sDisableTimestamp)
    {
        return;
    }
    pfn(commandBuffer, pipelineStage, queryPool, query);
}

VkResult DiveRuntimeLayer::GetQueryPoolResults(PFN_vkGetQueryPoolResults pfn, VkDevice device,
                                               VkQueryPool queryPool, uint32_t firstQuery,
                                               uint32_t queryCount, size_t dataSize, void* pData,
                                               VkDeviceSize stride, VkQueryResultFlags flags)
{
    if (sDisableTimestamp)
    {
        return VK_SUCCESS;
    }

    return pfn(device, queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);
}

void DiveRuntimeLayer::DestroyCommandPool(PFN_vkDestroyCommandPool pfn, VkDevice device,
                                          VkCommandPool commandPool,
                                          const VkAllocationCallbacks* pAllocator)
{
    Dive::GPUTime::GpuTimeStatus status = m_gpu_time.OnDestroyCommandPool(commandPool);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }
    return pfn(device, commandPool, pAllocator);
}

VkResult DiveRuntimeLayer::AllocateCommandBuffers(PFN_vkAllocateCommandBuffers pfn, VkDevice device,
                                                  const VkCommandBufferAllocateInfo* pAllocateInfo,
                                                  VkCommandBuffer* pCommandBuffers)
{
    VkResult result = pfn(device, pAllocateInfo, pCommandBuffers);

    if (result != VK_SUCCESS)
    {
        return result;
    }

    m_boundary_detector.OnAllocateCommandBuffers(pAllocateInfo, pCommandBuffers);

    Dive::GPUTime::GpuTimeStatus status =
        m_gpu_time.OnAllocateCommandBuffers(pAllocateInfo, pCommandBuffers);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }

    return result;
}

void DiveRuntimeLayer::FreeCommandBuffers(PFN_vkFreeCommandBuffers pfn, VkDevice device,
                                          VkCommandPool commandPool, uint32_t commandBufferCount,
                                          const VkCommandBuffer* pCommandBuffers)
{
    m_boundary_detector.OnFreeCommandBuffers(commandBufferCount, pCommandBuffers);

    Dive::GPUTime::GpuTimeStatus status =
        m_gpu_time.OnFreeCommandBuffers(commandBufferCount, pCommandBuffers);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }
    pfn(device, commandPool, commandBufferCount, pCommandBuffers);
}

VkResult DiveRuntimeLayer::ResetCommandBuffer(PFN_vkResetCommandBuffer pfn,
                                              VkCommandBuffer commandBuffer,
                                              VkCommandBufferResetFlags flags)
{
    m_boundary_detector.OnResetCommandBuffer(commandBuffer);

    Dive::GPUTime::GpuTimeStatus status = m_gpu_time.OnResetCommandBuffer(commandBuffer);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }

    return pfn(commandBuffer, flags);
}

VkResult DiveRuntimeLayer::ResetCommandPool(PFN_vkResetCommandPool pfn, VkDevice device,
                                            VkCommandPool commandPool,
                                            VkCommandPoolResetFlags flags)
{
    m_boundary_detector.OnResetCommandPool(commandPool);

    Dive::GPUTime::GpuTimeStatus status = m_gpu_time.OnResetCommandPool(commandPool);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }

    return pfn(device, commandPool, flags);
}

VkResult DiveRuntimeLayer::BeginCommandBuffer(PFN_vkBeginCommandBuffer pfn,
                                              VkCommandBuffer commandBuffer,
                                              const VkCommandBufferBeginInfo* pBeginInfo)
{
    if (sCmdBufferCurrentPipelineHasAlpha.size() > kMaxConcurrentCBs)
    {
        sCmdBufferCurrentPipelineHasAlpha.clear();
    }
    sCmdBufferCurrentPipelineHasAlpha[commandBuffer] = false;

    VkResult result = pfn(commandBuffer, pBeginInfo);
    if (sEnableDrawcallReport)
    {
        LOGI("Drawcall count: %d", sDrawcallCounter);
        LOGI("Total index count: %zd", sTotalIndexCounter);
    }

    sDrawcallCounter = 0;
    sTotalIndexCounter = 0;

    Dive::GPUTime::GpuTimeStatus status = m_gpu_time.OnBeginCommandBuffer(
        commandBuffer, pBeginInfo->flags, m_pfn_vkCmdWriteTimestamp);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }

    return result;
}

VkResult DiveRuntimeLayer::EndCommandBuffer(PFN_vkEndCommandBuffer pfn,
                                            VkCommandBuffer commandBuffer)
{
    Dive::GPUTime::GpuTimeStatus status =
        m_gpu_time.OnEndCommandBuffer(commandBuffer, m_pfn_vkCmdWriteTimestamp);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }
    return pfn(commandBuffer);
}

VkResult DiveRuntimeLayer::CreateDevice(PFN_vkGetDeviceProcAddr pa, PFN_vkCreateDevice pfn,
                                        float timestampPeriod, VkPhysicalDevice physicalDevice,
                                        const VkDeviceCreateInfo* pCreateInfo,
                                        const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
    VkResult result = pfn(physicalDevice, pCreateInfo, pAllocator, pDevice);
    m_device_proc_addr = pa;

    if (result != VK_SUCCESS)
    {
        return result;
    }

    m_gpu_time.SetEnable(sEnableGPUTiming);

    // Initialize all vk func pointers
    PFN_vkCreateQueryPool CreateQueryPool =
        reinterpret_cast<PFN_vkCreateQueryPool>(m_device_proc_addr(*pDevice, "vkCreateQueryPool"));

    m_pfn_vkResetQueryPool =
        reinterpret_cast<PFN_vkResetQueryPool>(m_device_proc_addr(*pDevice, "vkResetQueryPool"));
    m_pfn_vkQueueWaitIdle =
        reinterpret_cast<PFN_vkQueueWaitIdle>(m_device_proc_addr(*pDevice, "vkQueueWaitIdle"));
    m_pfn_vkDestroyQueryPool = reinterpret_cast<PFN_vkDestroyQueryPool>(
        m_device_proc_addr(*pDevice, "vkDestroyQueryPool"));

    m_pfn_vkDeviceWaitIdle =
        reinterpret_cast<PFN_vkDeviceWaitIdle>(m_device_proc_addr(*pDevice, "vkDeviceWaitIdle"));

    m_pfn_vkGetQueryPoolResults = reinterpret_cast<PFN_vkGetQueryPoolResults>(
        m_device_proc_addr(*pDevice, "vkGetQueryPoolResults"));

    m_pfn_vkCmdWriteTimestamp = reinterpret_cast<PFN_vkCmdWriteTimestamp>(
        m_device_proc_addr(*pDevice, "vkCmdWriteTimestamp"));

    Dive::GPUTime::GpuTimeStatus status =
        m_gpu_time.OnCreateDevice(*pDevice, pAllocator, timestampPeriod, CreateQueryPool,
                                  m_pfn_vkResetQueryPool, m_pfn_vkDestroyQueryPool);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }

    return result;
}

void DiveRuntimeLayer::DestroyDevice(PFN_vkDestroyDevice pfn, VkDevice device,
                                     const VkAllocationCallbacks* pAllocator)
{
    if (m_device_proc_addr == nullptr)
    {
        LOGI("Device proc addr is nullptr!!!");
        return;
    }

    Dive::GPUTime::GpuTimeStatus status = m_gpu_time.OnDestroyDevice(device, m_pfn_vkQueueWaitIdle);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }

    m_pfn_vkResetQueryPool = nullptr;
    m_pfn_vkQueueWaitIdle = nullptr;
    m_pfn_vkDestroyQueryPool = nullptr;
    m_pfn_vkDeviceWaitIdle = nullptr;
    m_pfn_vkGetQueryPoolResults = nullptr;
    m_pfn_vkCmdWriteTimestamp = nullptr;

    pfn(device, pAllocator);
}

VkResult DiveRuntimeLayer::AcquireNextImageKHR(PFN_vkAcquireNextImageKHR pfn, VkDevice device,
                                               VkSwapchainKHR swapchain, uint64_t timeout,
                                               VkSemaphore semaphore, VkFence fence,
                                               uint32_t* pImageIndex)
{
    // Be careful, this func is NOT called for OpenXR app!!!
    return pfn(device, swapchain, timeout, semaphore, fence, pImageIndex);
}

VkResult DiveRuntimeLayer::QueueSubmit(PFN_vkQueueSubmit pfn, VkQueue queue, uint32_t submitCount,
                                       const VkSubmitInfo* pSubmits, VkFence fence)
{
    VkResult result = pfn(queue, submitCount, pSubmits, fence);

    if (result != VK_SUCCESS)
    {
        return result;
    }

    if (sEnableGPUTiming)
    {
        auto submit_status =
            m_gpu_time.OnQueueSubmit(submitCount, pSubmits, m_pfn_vkDeviceWaitIdle,
                                     m_pfn_vkResetQueryPool, m_pfn_vkGetQueryPoolResults);
        if (!submit_status.gpu_time_status.success)
        {
            if (submit_status.contains_frame_boundary)
            {
                LOGE("Frame Boundary!");
            }
            LOGE("%s", submit_status.gpu_time_status.message.c_str());
        }
        else
        {
            if (submit_status.contains_frame_boundary)
            {
                LOGI("%s", m_gpu_time.GetStatsString().c_str());
            }
        }
    }

    bool is_frame_boundary = m_boundary_detector.ContainsFrameBoundary(submitCount, pSubmits);
    if (is_frame_boundary)
    {
        // Process frame boundary tasks for OpenXR apps.
        ProcessFrameBoundaryTasks();
    }

    return result;
}

void DiveRuntimeLayer::GetDeviceQueue2(PFN_vkGetDeviceQueue2 pfn, VkDevice device,
                                       const VkDeviceQueueInfo2* pQueueInfo, VkQueue* pQueue)
{
    pfn(device, pQueueInfo, pQueue);
    Dive::GPUTime::GpuTimeStatus status = m_gpu_time.OnGetDeviceQueue2(pQueue);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }
}

void DiveRuntimeLayer::GetDeviceQueue(PFN_vkGetDeviceQueue pfn, VkDevice device,
                                      uint32_t queueFamilyIndex, uint32_t queueIndex,
                                      VkQueue* pQueue)
{
    pfn(device, queueFamilyIndex, queueIndex, pQueue);
    Dive::GPUTime::GpuTimeStatus status = m_gpu_time.OnGetDeviceQueue(pQueue);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }
}

void DiveRuntimeLayer::CmdInsertDebugUtilsLabel(PFN_vkCmdInsertDebugUtilsLabelEXT pfn,
                                                VkCommandBuffer commandBuffer,
                                                const VkDebugUtilsLabelEXT* pLabelInfo)
{
    pfn(commandBuffer, pLabelInfo);

    if (Dive::FrameBoundaryDetector::BoundaryStatus status =
            m_boundary_detector.MarkBoundary(commandBuffer, pLabelInfo);
        !status.success)
    {
        LOGE("%s", status.message.c_str());
    }

    Dive::GPUTime::GpuTimeStatus status =
        m_gpu_time.OnCmdInsertDebugUtilsLabelEXT(commandBuffer, pLabelInfo);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }
}

void DiveRuntimeLayer::CmdBeginRenderPass(PFN_vkCmdBeginRenderPass pfn,
                                          VkCommandBuffer commandBuffer,
                                          const VkRenderPassBeginInfo* pRenderPassBegin,
                                          VkSubpassContents contents)
{
    Dive::GPUTime::GpuTimeStatus status =
        m_gpu_time.OnCmdBeginRenderPass(commandBuffer, m_pfn_vkCmdWriteTimestamp);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }

    pfn(commandBuffer, pRenderPassBegin, contents);
}

void DiveRuntimeLayer::CmdEndRenderPass(PFN_vkCmdEndRenderPass pfn, VkCommandBuffer commandBuffer)
{
    pfn(commandBuffer);

    Dive::GPUTime::GpuTimeStatus status =
        m_gpu_time.OnCmdEndRenderPass(commandBuffer, m_pfn_vkCmdWriteTimestamp);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }
}

void DiveRuntimeLayer::CmdBeginRenderPass2(PFN_vkCmdBeginRenderPass2 pfn,
                                           VkCommandBuffer commandBuffer,
                                           const VkRenderPassBeginInfo* pRenderPassBegin,
                                           const VkSubpassBeginInfo* pSubpassBeginInfo)
{
    Dive::GPUTime::GpuTimeStatus status =
        m_gpu_time.OnCmdBeginRenderPass2(commandBuffer, m_pfn_vkCmdWriteTimestamp);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }
    pfn(commandBuffer, pRenderPassBegin, pSubpassBeginInfo);
}

void DiveRuntimeLayer::CmdEndRenderPass2(PFN_vkCmdEndRenderPass2 pfn, VkCommandBuffer commandBuffer,
                                         const VkSubpassEndInfo* pSubpassEndInfo)
{
    pfn(commandBuffer, pSubpassEndInfo);

    Dive::GPUTime::GpuTimeStatus status =
        m_gpu_time.OnCmdEndRenderPass2(commandBuffer, m_pfn_vkCmdWriteTimestamp);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }
}

void DiveRuntimeLayer::EnqueueFrameBoundaryTask(std::function<void()> task)
{
    std::lock_guard<std::mutex> lock(m_task_mutex);
    m_frame_boundary_tasks.push_back(std::move(task));
}

void DiveRuntimeLayer::ProcessFrameBoundaryTasks()
{
    {
        std::lock_guard<std::mutex> lock(m_config_mutex);
        m_active_filter_config = m_pending_filter_config;
    }

    m_global_drawcall_counter.store(0, std::memory_order_relaxed);

    std::vector<std::function<void()>> tasks_to_run;
    {
        std::lock_guard<std::mutex> lock(m_task_mutex);
        if (m_frame_boundary_tasks.empty())
        {
            return;
        }

        tasks_to_run = std::move(m_frame_boundary_tasks);
    }

    for (auto& task : tasks_to_run)
    {
        task();
    }
}

std::vector<Network::PSOInfo> DiveRuntimeLayer::GetLivePSOs()
{
    std::vector<Network::PSOInfo> result;
    std::shared_lock<std::shared_mutex> lock(m_pso_mutex);
    result.reserve(m_live_psos.size());
    for (const auto& [pipeline, data] : m_live_psos)
    {
        result.push_back(data);
    }
    return result;
}

bool DiveRuntimeLayer::CheckAndIncrementDrawcallCount()
{
    if (!m_active_filter_config.enable_drawcall_limit)
    {
        return false;
    }
    if (m_global_drawcall_counter.fetch_add(1, std::memory_order_relaxed) >=
        m_active_filter_config.max_drawcalls)
    {
        return true;
    }
    return false;
}

bool DiveRuntimeLayer::ShouldFilterDrawCall(VkCommandBuffer command_buffer,
                                            std::optional<uint32_t> vertex_count,
                                            std::optional<uint32_t> index_count,
                                            std::optional<uint32_t> instance_count) const
{
    if (vertex_count.has_value() && m_active_filter_config.filter_by_vertex_count &&
        vertex_count.value() == m_active_filter_config.target_vertex_count)
    {
        return true;
    }
    if (index_count.has_value() && m_active_filter_config.filter_by_index_count &&
        index_count.value() == m_active_filter_config.target_index_count)
    {
        return true;
    }
    if (instance_count.has_value() && m_active_filter_config.filter_by_instance_count &&
        instance_count.value() == m_active_filter_config.target_instance_count)
    {
        return true;
    }

    if (m_active_filter_config.filter_by_alpha_blended)
    {
        if (auto it = sCmdBufferCurrentPipelineHasAlpha.find(command_buffer);
            it != sCmdBufferCurrentPipelineHasAlpha.end() && it->second)
        {
            return true;
        }
    }

    return false;
}

}  // namespace DiveLayer
