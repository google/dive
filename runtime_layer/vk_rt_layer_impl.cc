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
#include "common/log.h"

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
static bool sEnableOpenXRGPUTiming = false;
static bool sRemoveImageFlagFDMOffset = false;
static bool sRemoveImageFlagSubSampled = false;
static bool sDisableTimestamp = false;

static uint32_t sDrawcallCounter = 0;
static size_t   sTotalIndexCounter = 0;

static constexpr uint32_t kDrawcallCountLimit = 300;
static constexpr uint32_t kVisibilityMaskIndexCount = 42;

// DiveRuntimeLayer
DiveRuntimeLayer::DiveRuntimeLayer() :
    m_device_proc_addr(nullptr)
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
    Dive::GPUTime::GpuTimeStatus status = m_gpu_time.OnDestroyCommandPool(commandPool);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }
    return pfn(device, commandPool, pAllocator);
}

VkResult DiveRuntimeLayer::AllocateCommandBuffers(PFN_vkAllocateCommandBuffers       pfn,
                                                  VkDevice                           device,
                                                  const VkCommandBufferAllocateInfo* pAllocateInfo,
                                                  VkCommandBuffer* pCommandBuffers)
{
    VkResult result = pfn(device, pAllocateInfo, pCommandBuffers);

    if (result != VK_SUCCESS)
    {
        return result;
    }

    Dive::GPUTime::GpuTimeStatus status = m_gpu_time.OnAllocateCommandBuffers(pAllocateInfo,
                                                                              pCommandBuffers);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }

    return result;
}

void DiveRuntimeLayer::FreeCommandBuffers(PFN_vkFreeCommandBuffers pfn,
                                          VkDevice                 device,
                                          VkCommandPool            commandPool,
                                          uint32_t                 commandBufferCount,
                                          const VkCommandBuffer*   pCommandBuffers)
{
    Dive::GPUTime::GpuTimeStatus status = m_gpu_time.OnFreeCommandBuffers(commandBufferCount,
                                                                          pCommandBuffers);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }
    return pfn(device, commandPool, commandBufferCount, pCommandBuffers);
}

VkResult DiveRuntimeLayer::ResetCommandBuffer(PFN_vkResetCommandBuffer  pfn,
                                              VkCommandBuffer           commandBuffer,
                                              VkCommandBufferResetFlags flags)
{
    Dive::GPUTime::GpuTimeStatus status = m_gpu_time.OnResetCommandBuffer(commandBuffer);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }

    return pfn(commandBuffer, flags);
}

VkResult DiveRuntimeLayer::ResetCommandPool(PFN_vkResetCommandPool  pfn,
                                            VkDevice                device,
                                            VkCommandPool           commandPool,
                                            VkCommandPoolResetFlags flags)
{
    Dive::GPUTime::GpuTimeStatus status = m_gpu_time.OnResetCommandPool(commandPool);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
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
        PFN_vkCmdWriteTimestamp CmdWriteTimestamp = reinterpret_cast<PFN_vkCmdWriteTimestamp>(
        m_device_proc_addr(m_gpu_time.GetDevice(), "vkCmdWriteTimestamp"));

        Dive::GPUTime::GpuTimeStatus status = m_gpu_time.OnBeginCommandBuffer(commandBuffer,
                                                                              pBeginInfo->flags,
                                                                              CmdWriteTimestamp);
        if (!status.success)
        {
            LOGE("%s", status.message.c_str());
        }
    }
    return result;
}

VkResult DiveRuntimeLayer::EndCommandBuffer(PFN_vkEndCommandBuffer pfn,
                                            VkCommandBuffer        commandBuffer)
{
    if (sEnableOpenXRGPUTiming)
    {
        PFN_vkCmdWriteTimestamp CmdWriteTimestamp = reinterpret_cast<PFN_vkCmdWriteTimestamp>(
        m_device_proc_addr(m_gpu_time.GetDevice(), "vkCmdWriteTimestamp"));

        Dive::GPUTime::GpuTimeStatus status = m_gpu_time.OnEndCommandBuffer(commandBuffer,
                                                                            CmdWriteTimestamp);
        if (!status.success)
        {
            LOGE("%s", status.message.c_str());
        }
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
    VkResult result = pfn(physicalDevice, pCreateInfo, pAllocator, pDevice);
    m_device_proc_addr = pa;

    if (result != VK_SUCCESS)
    {
        return result;
    }

    PFN_vkCreateQueryPool CreateQueryPool = reinterpret_cast<PFN_vkCreateQueryPool>(
    m_device_proc_addr(*pDevice, "vkCreateQueryPool"));

    PFN_vkResetQueryPool ResetQueryPool = reinterpret_cast<PFN_vkResetQueryPool>(
    m_device_proc_addr(*pDevice, "vkResetQueryPool"));

    Dive::GPUTime::GpuTimeStatus status = m_gpu_time.OnCreateDevice(*pDevice,
                                                                    pAllocator,
                                                                    timestampPeriod,
                                                                    CreateQueryPool,
                                                                    ResetQueryPool);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }

    return result;
}

void DiveRuntimeLayer::DestroyDevice(PFN_vkDestroyDevice          pfn,
                                     VkDevice                     device,
                                     const VkAllocationCallbacks* pAllocator)
{
    if (m_device_proc_addr == nullptr)
    {
        LOGI("Device proc addr is nullptr!!!");
        return;
    }

    PFN_vkQueueWaitIdle QueueWaitIdle = reinterpret_cast<PFN_vkQueueWaitIdle>(
    m_device_proc_addr(device, "vkQueueWaitIdle"));
    PFN_vkDestroyQueryPool DestroyQueryPool = reinterpret_cast<PFN_vkDestroyQueryPool>(
    m_device_proc_addr(device, "vkDestroyQueryPool"));

    Dive::GPUTime::GpuTimeStatus status = m_gpu_time.OnDestroyDevice(device,
                                                                     QueueWaitIdle,
                                                                     DestroyQueryPool);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }
    pfn(device, pAllocator);
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
    VkResult result = pfn(queue, submitCount, pSubmits, fence);

    if (!sEnableOpenXRGPUTiming || (result != VK_SUCCESS))
    {
        return result;
    }

    PFN_vkDeviceWaitIdle DeviceWaitIdle = reinterpret_cast<PFN_vkDeviceWaitIdle>(
    m_device_proc_addr(m_gpu_time.GetDevice(), "vkDeviceWaitIdle"));

    PFN_vkResetQueryPool ResetQueryPool = reinterpret_cast<PFN_vkResetQueryPool>(
    m_device_proc_addr(m_gpu_time.GetDevice(), "vkResetQueryPool"));

    PFN_vkGetQueryPoolResults GetQueryPoolResults = reinterpret_cast<PFN_vkGetQueryPoolResults>(
    m_device_proc_addr(m_gpu_time.GetDevice(), "vkGetQueryPoolResults"));

    auto submit_status = m_gpu_time.OnQueueSubmit(submitCount,
                                                  pSubmits,
                                                  DeviceWaitIdle,
                                                  ResetQueryPool,
                                                  GetQueryPoolResults);
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

    return result;
}

void DiveRuntimeLayer::GetDeviceQueue2(PFN_vkGetDeviceQueue2     pfn,
                                       VkDevice                  device,
                                       const VkDeviceQueueInfo2* pQueueInfo,
                                       VkQueue*                  pQueue)
{
    pfn(device, pQueueInfo, pQueue);
    Dive::GPUTime::GpuTimeStatus status = m_gpu_time.OnGetDeviceQueue2(pQueue);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }
}

void DiveRuntimeLayer::GetDeviceQueue(PFN_vkGetDeviceQueue pfn,
                                      VkDevice             device,
                                      uint32_t             queueFamilyIndex,
                                      uint32_t             queueIndex,
                                      VkQueue*             pQueue)
{
    pfn(device, queueFamilyIndex, queueIndex, pQueue);
    Dive::GPUTime::GpuTimeStatus status = m_gpu_time.OnGetDeviceQueue(pQueue);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }
}

void DiveRuntimeLayer::CmdInsertDebugUtilsLabel(PFN_vkCmdInsertDebugUtilsLabelEXT pfn,
                                                VkCommandBuffer                   commandBuffer,
                                                const VkDebugUtilsLabelEXT*       pLabelInfo)
{
    pfn(commandBuffer, pLabelInfo);

    Dive::GPUTime::GpuTimeStatus status = m_gpu_time.OnCmdInsertDebugUtilsLabelEXT(commandBuffer,
                                                                                   pLabelInfo);
    if (!status.success)
    {
        LOGE("%s", status.message.c_str());
    }
}

}  // namespace DiveLayer