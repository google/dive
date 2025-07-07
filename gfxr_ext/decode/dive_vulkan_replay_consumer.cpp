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

    VkDevice device = MapHandle<VulkanDeviceInfo>(*(pDevice->GetPointer()),
                                                  &CommonObjectInfoTable::GetVkDeviceInfo);

    PFN_vkCreateQueryPool CreateQueryPool = reinterpret_cast<PFN_vkCreateQueryPool>(GetDeviceTable(device)
                                                                    ->CreateQueryPool);

    PFN_vkResetQueryPool ResetQueryPool = reinterpret_cast<PFN_vkResetQueryPool>(GetDeviceTable(device)
                                                                 ->ResetQueryPool);

    auto in_physicalDevice = GetObjectInfoTable().GetVkPhysicalDeviceInfo(physicalDevice);
    VkPhysicalDeviceProperties deviceProperties;
    GetInstanceTable(in_physicalDevice->handle)
    ->GetPhysicalDeviceProperties(in_physicalDevice->handle, &deviceProperties);

    Dive::GPUTime::Status
    status = m_gpu_time.OnCreateDevice(device,
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
    VkDevice in_device = MapHandle<VulkanDeviceInfo>(device,
                                                     &CommonObjectInfoTable::GetVkDeviceInfo);
    PFN_vkQueueWaitIdle QueueWaitIdle = reinterpret_cast<PFN_vkQueueWaitIdle>(
    GetDeviceTable(m_gpu_time.GetDevice())->QueueWaitIdle);
    PFN_vkDestroyQueryPool DestroyQueryPool = reinterpret_cast<PFN_vkDestroyQueryPool>(
    GetDeviceTable(m_gpu_time.GetDevice())->DestroyQueryPool);

    Dive::GPUTime::Status status = m_gpu_time.OnDestroyDevice(in_device, QueueWaitIdle, DestroyQueryPool);
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

    Dive::GPUTime::Status status = m_gpu_time.OnDestroyCommandPool(pool_handle);
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

    Dive::GPUTime::Status status = m_gpu_time.OnAllocateCommandBuffers(pAllocateInfo->GetPointer(),
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
    Dive::GPUTime::Status status = m_gpu_time.OnFreeCommandBuffers(commandBufferCount,
                                                          pCommandBuffers->GetHandlePointer());
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

    Dive::GPUTime::Status status = m_gpu_time.OnResetCommandBuffer(in_commandBuffer);
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

    Dive::GPUTime::Status status = m_gpu_time.OnResetCommandPool(in_commandPool);
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

    Dive::GPUTime::Status status = m_gpu_time.OnBeginCommandBuffer(in_commandBuffer,
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

    Dive::GPUTime::Status status = m_gpu_time.OnEndCommandBuffer(in_commandBuffer, CmdWriteTimestamp);
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
    GetDeviceTable(m_gpu_time.GetDevice())->DeviceWaitIdle);

    PFN_vkResetQueryPool ResetQueryPool = reinterpret_cast<PFN_vkResetQueryPool>(
    GetDeviceTable(m_gpu_time.GetDevice())->ResetQueryPool);

    PFN_vkGetQueryPoolResults GetQueryPoolResults = reinterpret_cast<PFN_vkGetQueryPoolResults>(
    GetDeviceTable(m_gpu_time.GetDevice())->GetQueryPoolResults);

    const VkSubmitInfo* submit_infos = pSubmits->GetPointer();
    Dive::GPUTime::Status        status = m_gpu_time.OnQueueSubmit(submitCount,
                                                   submit_infos,
                                                   DeviceWaitIdle,
                                                   ResetQueryPool,
                                                   GetQueryPoolResults);
    if (!status.success)
    {
        GFXRECON_LOG_ERROR(status.message.c_str());
    }
    else
    {
        GFXRECON_LOG_INFO(m_gpu_time.GetStatsString().c_str());
    }
}

void DiveVulkanReplayConsumer::Process_vkGetDeviceQueue2(
const ApiCallInfo&                                call_info,
format::HandleId                                  device,
StructPointerDecoder<Decoded_VkDeviceQueueInfo2>* pQueueInfo,
HandlePointerDecoder<VkQueue>*                    pQueue)
{
    VulkanReplayConsumer::Process_vkGetDeviceQueue2(call_info, device, pQueueInfo, pQueue);
    VkQueue* queue = pQueue->GetHandlePointer();
    Dive::GPUTime::Status status = m_gpu_time.OnGetDeviceQueue2(queue);
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
    VkQueue*     queue = pQueue->GetHandlePointer();
    Dive::GPUTime::Status status = m_gpu_time.OnGetDeviceQueue(queue);
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

    Dive::GPUTime::Status status = m_gpu_time.OnCmdInsertDebugUtilsLabelEXT(in_commandBuffer, in_pLabelInfo);
    if (!status.success)
    {
        GFXRECON_LOG_ERROR(status.message.c_str());
    }
}

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)
