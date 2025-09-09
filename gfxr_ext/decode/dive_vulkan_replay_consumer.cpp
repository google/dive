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

DiveVulkanReplayConsumer::DiveVulkanReplayConsumer(
std::shared_ptr<application::Application> application,
const VulkanReplayOptions&                options) :
    VulkanReplayConsumer(application, options)
{
}

DiveVulkanReplayConsumer::~DiveVulkanReplayConsumer()
{
    // For trimmed captures, all resources that are not released within the frame are released by
    // FreeAllLiveObjects in VulkanReplayConsumerBase::~VulkanReplayConsumerBase()
    // So it is safe to release all resources from the deferred list here (all parent resources
    // should be still alive at this point)
    ReleaseResourcesFromDeferredList();
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

    gpu_time_.SetEnable(enable_gpu_time_);

    VkDevice device = MapHandle<VulkanDeviceInfo>(*(pDevice->GetPointer()),
                                                  &CommonObjectInfoTable::GetVkDeviceInfo);

    PFN_vkCreateQueryPool CreateQueryPool = reinterpret_cast<PFN_vkCreateQueryPool>(
    GetDeviceTable(device)->CreateQueryPool);

    PFN_vkResetQueryPool ResetQueryPool = reinterpret_cast<PFN_vkResetQueryPool>(
    GetDeviceTable(device)->ResetQueryPool);

    auto in_physicalDevice = GetObjectInfoTable().GetVkPhysicalDeviceInfo(physicalDevice);
    VkPhysicalDeviceProperties deviceProperties;
    GetInstanceTable(in_physicalDevice->handle)
    ->GetPhysicalDeviceProperties(in_physicalDevice->handle, &deviceProperties);

    Dive::GPUTime::GpuTimeStatus status = gpu_time_
                                          .OnCreateDevice(device,
                                                          pAllocator->GetPointer(),
                                                          deviceProperties.limits.timestampPeriod,
                                                          CreateQueryPool,
                                                          ResetQueryPool);

    if (!status.success)
    {
        GFXRECON_LOG_ERROR(status.message.c_str());
    }
}

void DiveVulkanReplayConsumer::Process_vkDestroyDevice(
const ApiCallInfo&                                   call_info,
format::HandleId                                     device,
StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator)
{
    VkDevice            in_device = MapHandle<VulkanDeviceInfo>(device,
                                                     &CommonObjectInfoTable::GetVkDeviceInfo);
    PFN_vkQueueWaitIdle QueueWaitIdle = reinterpret_cast<PFN_vkQueueWaitIdle>(
    GetDeviceTable(gpu_time_.GetDevice())->QueueWaitIdle);
    PFN_vkDestroyQueryPool DestroyQueryPool = reinterpret_cast<PFN_vkDestroyQueryPool>(
    GetDeviceTable(gpu_time_.GetDevice())->DestroyQueryPool);

    Dive::GPUTime::GpuTimeStatus status = gpu_time_.OnDestroyDevice(in_device,
                                                                    QueueWaitIdle,
                                                                    DestroyQueryPool);
    if (!status.success)
    {
        GFXRECON_LOG_ERROR(status.message.c_str());
    }
    VulkanReplayConsumer::Process_vkDestroyDevice(call_info, device, pAllocator);
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

    Dive::GPUTime::GpuTimeStatus status = gpu_time_.OnDestroyCommandPool(pool_handle);
    if (!status.success)
    {
        GFXRECON_LOG_ERROR(status.message.c_str());
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

    if (returnValue != VK_SUCCESS)
    {
        return;
    }

    Dive::GPUTime::GpuTimeStatus
    status = gpu_time_.OnAllocateCommandBuffers(pAllocateInfo->GetPointer(),
                                                pCommandBuffers->GetHandlePointer());
    if (!status.success)
    {
        GFXRECON_LOG_ERROR(status.message.c_str());
    }
}

void DiveVulkanReplayConsumer::Process_vkFreeCommandBuffers(
const ApiCallInfo&                     call_info,
format::HandleId                       device,
format::HandleId                       commandPool,
uint32_t                               commandBufferCount,
HandlePointerDecoder<VkCommandBuffer>* pCommandBuffers)
{
    Dive::GPUTime::GpuTimeStatus status = gpu_time_.OnFreeCommandBuffers(commandBufferCount,
                                                                         pCommandBuffers
                                                                         ->GetHandlePointer());
    if (!status.success)
    {
        GFXRECON_LOG_ERROR(status.message.c_str());
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
    auto in_commandBuffer = GetObjectInfoTable().GetVkCommandBufferInfo(commandBuffer)->handle;

    Dive::GPUTime::GpuTimeStatus status = gpu_time_.OnResetCommandBuffer(in_commandBuffer);
    if (!status.success)
    {
        GFXRECON_LOG_ERROR(status.message.c_str());
    }

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
    auto in_commandPool = GetObjectInfoTable().GetVkCommandPoolInfo(commandPool)->handle;
    if (in_commandPool == VK_NULL_HANDLE)
    {
        return;
    }

    Dive::GPUTime::GpuTimeStatus status = gpu_time_.OnResetCommandPool(in_commandPool);
    if (!status.success)
    {
        GFXRECON_LOG_ERROR(status.message.c_str());
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

    PFN_vkCmdWriteTimestamp CmdWriteTimestamp = reinterpret_cast<PFN_vkCmdWriteTimestamp>(
    GetDeviceTable(in_commandBuffer)->CmdWriteTimestamp);

    Dive::GPUTime::GpuTimeStatus status = gpu_time_
                                          .OnBeginCommandBuffer(in_commandBuffer,
                                                                pBeginInfo->GetPointer()->flags,
                                                                CmdWriteTimestamp);
    if (!status.success)
    {
        GFXRECON_LOG_ERROR(status.message.c_str());
    }
}

void DiveVulkanReplayConsumer::Process_vkEndCommandBuffer(const ApiCallInfo& call_info,
                                                          VkResult           returnValue,
                                                          format::HandleId   commandBuffer)
{
    VkCommandBuffer in_commandBuffer = MapHandle<
    VulkanCommandBufferInfo>(commandBuffer, &CommonObjectInfoTable::GetVkCommandBufferInfo);

    PFN_vkCmdWriteTimestamp CmdWriteTimestamp = reinterpret_cast<PFN_vkCmdWriteTimestamp>(
    GetDeviceTable(in_commandBuffer)->CmdWriteTimestamp);

    Dive::GPUTime::GpuTimeStatus status = gpu_time_.OnEndCommandBuffer(in_commandBuffer,
                                                                       CmdWriteTimestamp);
    if (!status.success)
    {
        GFXRECON_LOG_ERROR(status.message.c_str());
    }

    VulkanReplayConsumer::Process_vkEndCommandBuffer(call_info, returnValue, commandBuffer);
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

    PFN_vkDeviceWaitIdle DeviceWaitIdle = reinterpret_cast<PFN_vkDeviceWaitIdle>(
    GetDeviceTable(gpu_time_.GetDevice())->DeviceWaitIdle);

    PFN_vkResetQueryPool ResetQueryPool = reinterpret_cast<PFN_vkResetQueryPool>(
    GetDeviceTable(gpu_time_.GetDevice())->ResetQueryPool);

    PFN_vkGetQueryPoolResults GetQueryPoolResults = reinterpret_cast<PFN_vkGetQueryPoolResults>(
    GetDeviceTable(gpu_time_.GetDevice())->GetQueryPoolResults);

    const VkSubmitInfo* submit_infos = pSubmits->GetPointer();
    auto                submit_status = gpu_time_.OnQueueSubmit(submitCount,
                                                 submit_infos,
                                                 DeviceWaitIdle,
                                                 ResetQueryPool,
                                                 GetQueryPoolResults);

    if (!submit_status.gpu_time_status.success)
    {
        if (submit_status.contains_frame_boundary)
        {
            GFXRECON_LOG_ERROR("Frame Boundary!");
        }
        GFXRECON_LOG_ERROR(submit_status.gpu_time_status.message.c_str());
    }
    else
    {
        if (submit_status.contains_frame_boundary)
        {
            GFXRECON_LOG_INFO(gpu_time_.GetStatsString().c_str());
            gpu_time_stats_csv_str_ = gpu_time_.GetStatsCSVString();
        }
    }
}

void DiveVulkanReplayConsumer::Process_vkGetDeviceQueue2(
const ApiCallInfo&                                call_info,
format::HandleId                                  device,
StructPointerDecoder<Decoded_VkDeviceQueueInfo2>* pQueueInfo,
HandlePointerDecoder<VkQueue>*                    pQueue)
{
    VulkanReplayConsumer::Process_vkGetDeviceQueue2(call_info, device, pQueueInfo, pQueue);
    VkQueue*                     queue = pQueue->GetHandlePointer();
    Dive::GPUTime::GpuTimeStatus status = gpu_time_.OnGetDeviceQueue2(queue);
    if (!status.success)
    {
        GFXRECON_LOG_ERROR(status.message.c_str());
    }
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
    VkQueue*                     queue = pQueue->GetHandlePointer();
    Dive::GPUTime::GpuTimeStatus status = gpu_time_.OnGetDeviceQueue(queue);
    if (!status.success)
    {
        GFXRECON_LOG_ERROR(status.message.c_str());
    }
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

    Dive::GPUTime::GpuTimeStatus status = gpu_time_.OnCmdInsertDebugUtilsLabelEXT(in_commandBuffer,
                                                                                  in_pLabelInfo);
    if (!status.success)
    {
        GFXRECON_LOG_ERROR(status.message.c_str());
    }
}

void DiveVulkanReplayConsumer::Process_vkCmdBeginRenderPass(
const ApiCallInfo&                                   call_info,
format::HandleId                                     commandBuffer,
StructPointerDecoder<Decoded_VkRenderPassBeginInfo>* pRenderPassBegin,
VkSubpassContents                                    contents)
{
    VkCommandBuffer in_commandBuffer = MapHandle<
    VulkanCommandBufferInfo>(commandBuffer, &CommonObjectInfoTable::GetVkCommandBufferInfo);

    PFN_vkCmdWriteTimestamp CmdWriteTimestamp = reinterpret_cast<PFN_vkCmdWriteTimestamp>(
    GetDeviceTable(in_commandBuffer)->CmdWriteTimestamp);

    Dive::GPUTime::GpuTimeStatus status = gpu_time_.OnCmdBeginRenderPass(in_commandBuffer,
                                                                         CmdWriteTimestamp);
    if (!status.success)
    {
        GFXRECON_LOG_ERROR(status.message.c_str());
    }
    VulkanReplayConsumer::Process_vkCmdBeginRenderPass(call_info,
                                                       commandBuffer,
                                                       pRenderPassBegin,
                                                       contents);
}

void DiveVulkanReplayConsumer::Process_vkCmdEndRenderPass(const ApiCallInfo& call_info,
                                                          format::HandleId   commandBuffer)
{
    VulkanReplayConsumer::Process_vkCmdEndRenderPass(call_info, commandBuffer);

    VkCommandBuffer in_commandBuffer = MapHandle<
    VulkanCommandBufferInfo>(commandBuffer, &CommonObjectInfoTable::GetVkCommandBufferInfo);

    PFN_vkCmdWriteTimestamp CmdWriteTimestamp = reinterpret_cast<PFN_vkCmdWriteTimestamp>(
    GetDeviceTable(in_commandBuffer)->CmdWriteTimestamp);

    Dive::GPUTime::GpuTimeStatus status = gpu_time_.OnCmdEndRenderPass(in_commandBuffer,
                                                                       CmdWriteTimestamp);
    if (!status.success)
    {
        GFXRECON_LOG_ERROR(status.message.c_str());
    }
}
void DiveVulkanReplayConsumer::Process_vkCmdBeginRenderPass2(
const ApiCallInfo&                                   call_info,
format::HandleId                                     commandBuffer,
StructPointerDecoder<Decoded_VkRenderPassBeginInfo>* pRenderPassBegin,
StructPointerDecoder<Decoded_VkSubpassBeginInfo>*    pSubpassBeginInfo)
{
    VkCommandBuffer in_commandBuffer = MapHandle<
    VulkanCommandBufferInfo>(commandBuffer, &CommonObjectInfoTable::GetVkCommandBufferInfo);

    PFN_vkCmdWriteTimestamp CmdWriteTimestamp = reinterpret_cast<PFN_vkCmdWriteTimestamp>(
    GetDeviceTable(in_commandBuffer)->CmdWriteTimestamp);

    Dive::GPUTime::GpuTimeStatus status = gpu_time_.OnCmdBeginRenderPass2(in_commandBuffer,
                                                                          CmdWriteTimestamp);
    if (!status.success)
    {
        GFXRECON_LOG_ERROR(status.message.c_str());
    }

    VulkanReplayConsumer::Process_vkCmdBeginRenderPass2(call_info,
                                                        commandBuffer,
                                                        pRenderPassBegin,
                                                        pSubpassBeginInfo);
}

void DiveVulkanReplayConsumer::Process_vkCmdEndRenderPass2(
const ApiCallInfo&                              call_info,
format::HandleId                                commandBuffer,
StructPointerDecoder<Decoded_VkSubpassEndInfo>* pSubpassEndInfo)
{
    VulkanReplayConsumer::Process_vkCmdEndRenderPass2(call_info, commandBuffer, pSubpassEndInfo);

    VkCommandBuffer in_commandBuffer = MapHandle<
    VulkanCommandBufferInfo>(commandBuffer, &CommonObjectInfoTable::GetVkCommandBufferInfo);

    PFN_vkCmdWriteTimestamp CmdWriteTimestamp = reinterpret_cast<PFN_vkCmdWriteTimestamp>(
    GetDeviceTable(in_commandBuffer)->CmdWriteTimestamp);

    Dive::GPUTime::GpuTimeStatus status = gpu_time_.OnCmdEndRenderPass2(in_commandBuffer,
                                                                        CmdWriteTimestamp);
    if (!status.success)
    {
        GFXRECON_LOG_ERROR(status.message.c_str());
    }
}

void DiveVulkanReplayConsumer::Process_vkCreateFence(
const ApiCallInfo&                                   call_info,
VkResult                                             returnValue,
format::HandleId                                     device,
StructPointerDecoder<Decoded_VkFenceCreateInfo>*     pCreateInfo,
StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
HandlePointerDecoder<VkFence>*                       pFence)
{
    VulkanReplayConsumer::Process_vkCreateFence(call_info,
                                                returnValue,
                                                device,
                                                pCreateInfo,
                                                pAllocator,
                                                pFence);

    if (returnValue != VK_SUCCESS)
    {
        return;
    }

    if (!setup_finished_)
    {
        deferred_release_fences_[*(pFence->GetPointer())] = { device, pAllocator };
    }
}

void DiveVulkanReplayConsumer::Process_vkDestroyFence(
const ApiCallInfo&                                   call_info,
format::HandleId                                     device,
format::HandleId                                     fence,
StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator)
{
    // We only destroy the fence if it is not in the deferred release list
    if (deferred_release_fences_.find(fence) == deferred_release_fences_.end())
    {
        VulkanReplayConsumer::Process_vkDestroyFence(call_info, device, fence, pAllocator);
    }
}

void DiveVulkanReplayConsumer::ProcessStateEndMarker(uint64_t frame_number)
{
    gpu_time_.ClearFrameCache();
    setup_finished_ = true;
}

void DiveVulkanReplayConsumer::ReleaseResourcesFromDeferredList()
{
    GFXRECON_LOG_INFO("Release Resources From the DeferredList!");
    // - We only defer release fences for now, may need to add other resources here
    for (const auto& [fence_handle, fence_release_info] : deferred_release_fences_)
    {
        VulkanReplayConsumer::Process_vkDestroyFence(ApiCallInfo{},
                                                     fence_release_info.device,
                                                     fence_handle,
                                                     fence_release_info.pAllocator);
    }
    deferred_release_fences_.clear();
}

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)
