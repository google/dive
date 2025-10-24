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

#include "dive_renderdoc.h"
#include "graphics/vulkan_struct_get_pnext.h"
#include "util/logging.h"
#include "generated/generated_vulkan_struct_handle_mappers.h"
#include "util/to_string.h"

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

DiveVulkanReplayConsumer::DiveVulkanReplayConsumer(
std::shared_ptr<application::Application> application,
const VulkanReplayOptions&                options) :
    VulkanReplayConsumer(application, options)
{
}

DiveVulkanReplayConsumer::~DiveVulkanReplayConsumer() {}

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
    device_ = device;

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
    GetDeviceTable(device_)->QueueWaitIdle);
    PFN_vkDestroyQueryPool DestroyQueryPool = reinterpret_cast<PFN_vkDestroyQueryPool>(
    GetDeviceTable(device_)->DestroyQueryPool);

    Dive::GPUTime::GpuTimeStatus status = gpu_time_.OnDestroyDevice(in_device,
                                                                    QueueWaitIdle,
                                                                    DestroyQueryPool);
    fence_signal_queue_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE;
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

void DiveVulkanReplayConsumer::Process_vkCreateCommandPool(
const ApiCallInfo&                                     call_info,
VkResult                                               returnValue,
format::HandleId                                       device,
StructPointerDecoder<Decoded_VkCommandPoolCreateInfo>* pCreateInfo,
StructPointerDecoder<Decoded_VkAllocationCallbacks>*   pAllocator,
HandlePointerDecoder<VkCommandPool>*                   pCommandPool)
{
    // Forcing all pools to use VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT.
    // This is needed because without this flag, vkBeginCommandBuffer will NOT perform an implicit
    // reset if the command buffer is not in the Initial state. Some of the command buffers are not
    // created with this flag, but for the case of looping frames, calling vkBeginCommandBuffer
    // again on those cmds, and submitting those cmds would cause VK_ERROR_INITIALIZATION_FAILED.
    // Note that there might be performance impact on this which needs more investigation
    // (b/444976541)
    VkCommandPoolCreateInfo* create_info = pCreateInfo->GetPointer();
    create_info->flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VulkanReplayConsumer::Process_vkCreateCommandPool(call_info,
                                                      returnValue,
                                                      device,
                                                      pCreateInfo,
                                                      pAllocator,
                                                      pCommandPool);
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
    GetDeviceTable(device_)->DeviceWaitIdle);

    PFN_vkResetQueryPool ResetQueryPool = reinterpret_cast<PFN_vkResetQueryPool>(
    GetDeviceTable(device_)->ResetQueryPool);

    PFN_vkGetQueryPoolResults GetQueryPoolResults = reinterpret_cast<PFN_vkGetQueryPoolResults>(
    GetDeviceTable(device_)->GetQueryPoolResults);

    const VkSubmitInfo* submit_infos = pSubmits->GetPointer();
    auto                submit_status = gpu_time_.OnQueueSubmit(submitCount,
                                                 submit_infos,
                                                 DeviceWaitIdle,
                                                 ResetQueryPool,
                                                 GetQueryPoolResults);

    auto IsFrameBoundary = [](Decoded_VkSubmitInfo*  submit_info_data,
                              uint32_t               submit_count,
                              CommonObjectInfoTable& object_info_table) -> bool {
        if (submit_info_data == nullptr)
        {
            return false;
        }

        for (uint32_t i = 0; i < submit_count; ++i)
        {
            size_t     command_buffer_count = submit_info_data[i].pCommandBuffers.GetLength();
            const auto command_buffer_ids = submit_info_data[i].pCommandBuffers.GetPointer();

            for (uint32_t j = 0; j < command_buffer_count; ++j)
            {
                auto command_buffer_info = object_info_table.GetVkCommandBufferInfo(
                command_buffer_ids[j]);

                if (command_buffer_info->is_frame_boundary)
                {
                    return true;
                }
            }
        }

        return false;
    };

    bool is_frame_boundary = IsFrameBoundary(pSubmits->GetMetaStructPointer(),
                                             submitCount,
                                             GetObjectInfoTable());

    // vkDeviceWaitIdle is needed since when we loop the frame, we do not double/triple buffer cmds.
    // If the CPU is too fast, it might start to write to cmd while GPU is using it which would
    // cause random crashes. (No need to add device wait idle when gpu time is enabled since it is
    // called in GPUTime::OnQueueSubmit)
    if (!gpu_time_.IsEnabled() && is_frame_boundary)
    {
        DeviceWaitIdle(device_);
    }

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
    VkQueue* queue = pQueue->GetHandlePointer();
    fence_signal_queue_ = *queue;
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
    VkQueue* queue = pQueue->GetHandlePointer();
    fence_signal_queue_ = *queue;
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

void DiveVulkanReplayConsumer::Process_vkCmdBeginRenderPass2KHR(
const ApiCallInfo&                                   call_info,
format::HandleId                                     commandBuffer,
StructPointerDecoder<Decoded_VkRenderPassBeginInfo>* pRenderPassBegin,
StructPointerDecoder<Decoded_VkSubpassBeginInfo>*    pSubpassBeginInfo)
{
    Process_vkCmdBeginRenderPass2(call_info, commandBuffer, pRenderPassBegin, pSubpassBeginInfo);
}

void DiveVulkanReplayConsumer::Process_vkCmdEndRenderPass2KHR(
const ApiCallInfo&                              call_info,
format::HandleId                                commandBuffer,
StructPointerDecoder<Decoded_VkSubpassEndInfo>* pSubpassEndInfo)
{
    Process_vkCmdEndRenderPass2(call_info, commandBuffer, pSubpassEndInfo);
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
        deferred_release_list_.push_back(*(pFence->GetPointer()));
        fence_initial_status_[*(pFence->GetHandlePointer())] = FenceStatus::kUnsignaled;
    }
}

void DiveVulkanReplayConsumer::Process_vkDestroyFence(
const ApiCallInfo&                                   call_info,
format::HandleId                                     device,
format::HandleId                                     fence,
StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator)
{
    // We only destroy the fence if it is not in the deferred release list
    auto it = std::find(deferred_release_list_.begin(), deferred_release_list_.end(), fence);
    if (it == deferred_release_list_.end())
    {
        VulkanReplayConsumer::Process_vkDestroyFence(call_info, device, fence, pAllocator);
    }
}

void DiveVulkanReplayConsumer::Process_vkAllocateMemory(
const ApiCallInfo&                                   call_info,
VkResult                                             returnValue,
format::HandleId                                     device,
StructPointerDecoder<Decoded_VkMemoryAllocateInfo>*  pAllocateInfo,
StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
HandlePointerDecoder<VkDeviceMemory>*                pMemory)
{
    if (GetRenderDocApi() != nullptr)
    {
        // RenderDoc replaces external memory with non-external memory when replaying a .RDC file.
        // If the memory type for the external memory does not match the non-external memory then
        // RenderDoc throws an error. To avoid this, if we detect external memory, allocate what we
        // think RenderDoc wants instead of what's in the .GFXR file.
        //
        // Changing memoryTypeIndex here is brittle since the superclass implementation could
        // overwrite memoryTypeIndex. However, it reduces the number of modifications made to GFXR;
        // this should reduce the number of merge conflicts when we perform the next subtree pull.
        VkMemoryAllocateInfo* allocate_info = pAllocateInfo->GetPointer();
        bool is_importing_android_hardware_buffer = graphics::vulkan_struct_get_pnext<
                                                    VkImportAndroidHardwareBufferInfoANDROID>(
                                                    allocate_info) != nullptr;
        bool using_unsupported_mem_type = (allocate_info->memoryTypeIndex == 8) ||
                                          is_importing_android_hardware_buffer;
        if (using_unsupported_mem_type)
        {
            // Based on testing, imported AHardwareBuffers use memoryTypeIndex 1
            // Some other cases, the app could use mem type 8
            // but RenderDoc only supports 0. We can't call vkGetImageMemoryRequirements since
            // (apart from dedicated memory) we don't know which image this memory will be bound to.
            GFXRECON_LOG_INFO("Unsupported detected during RenderDoc capture! Overriding "
                              "memoryTypeIndex from %u to 0 for memory handle %lu",
                              allocate_info->memoryTypeIndex,
                              *pMemory->GetPointer());
            allocate_info->memoryTypeIndex = 0;
        }
    }

    VulkanReplayConsumer::Process_vkAllocateMemory(call_info,
                                                   returnValue,
                                                   device,
                                                   pAllocateInfo,
                                                   pAllocator,
                                                   pMemory);
}

void DiveVulkanReplayConsumer::ProcessStateEndMarker(uint64_t frame_number)
{
    gpu_time_.ClearFrameCache();
    setup_finished_ = true;

    PFN_vkGetFenceStatus GetFenceStatus = GetDeviceTable(device_)->GetFenceStatus;

    // We keep the inital status of all fences that are created in the setup phase before the 1st
    // frame starts. ProcessStateEndMarker is called only once when the setup phase is finished
    for (auto& [fence, initial_status] : fence_initial_status_)
    {
        const VkResult status = GetFenceStatus(device_, fence);
        GFXRECON_ASSERT((status == VK_NOT_READY) || (status == VK_SUCCESS));
        initial_status = (status == VK_SUCCESS) ? FenceStatus::kSignaled : FenceStatus::kUnsignaled;
    }
}

void DiveVulkanReplayConsumer::ProcessFrameEndMarker(uint64_t frame_number)
{
    PFN_vkGetFenceStatus GetFenceStatus = GetDeviceTable(device_)->GetFenceStatus;

    PFN_vkQueueSubmit QueueSubmit = GetDeviceTable(device_)->QueueSubmit;

    std::vector<VkFence> reset_fence_list = {};

    // We try to bring back the initial status for all fences at the end of each loop
    for (const auto& [fence, initial_status] : fence_initial_status_)
    {
        const VkResult status = GetFenceStatus(device_, fence);
        GFXRECON_ASSERT((status == VK_NOT_READY) || (status == VK_SUCCESS));
        const FenceStatus current_status = (status == VK_SUCCESS) ? FenceStatus::kSignaled :
                                                                    FenceStatus::kUnsignaled;
        if (current_status == initial_status)
        {
            continue;
        }

        switch (initial_status)
        {
        case FenceStatus::kSignaled:
            // vkQueueSubmit here is only for signaling the fence.
            // There will be some CPU overhead, but GPU cost is negligible. It doesn't matter which
            // type of queue it is if we submit 0 cmd
            QueueSubmit(fence_signal_queue_, 0, nullptr, fence);
            break;
        case FenceStatus::kUnsignaled:
            reset_fence_list.push_back(fence);
            break;
        }
    }

    if (!reset_fence_list.empty())
    {
        PFN_vkResetFences ResetFences = GetDeviceTable(device_)->ResetFences;
        VkResult          result = ResetFences(device_,
                                      static_cast<uint32_t>(reset_fence_list.size()),
                                      reset_fence_list.data());
        GFXRECON_ASSERT(result == VK_SUCCESS);
    }
}

void DiveVulkanReplayConsumer::Process_vkGetFenceFdKHR(
const ApiCallInfo&                                 call_info,
VkResult                                           returnValue,
format::HandleId                                   device,
StructPointerDecoder<Decoded_VkFenceGetFdInfoKHR>* pGetFdInfo,
PointerDecoder<int>*                               pFd)
{
    // Workaround for compositor captures. A fence may not have been created with the exportable
    // flag, causing vkGetFenceFdKHR to fail on replay. Since the file descriptor is not used, we
    // can ignore the error and allow replay to continue. The workaround tries with the call and if
    // it failed, then ignore the error and continue the replay.
    auto in_device = GetObjectInfoTable().GetVkDeviceInfo(device);
    MapStructHandles(pGetFdInfo->GetMetaStructPointer(), GetObjectInfoTable());
    auto in_pGetFdInfo = pGetFdInfo->GetPointer();
    int* out_pFd = pFd->IsNull() ? nullptr : pFd->AllocateOutputData(1, -1);

    VkResult replay_result = GetDeviceTable(in_device->handle)
                             ->GetFenceFdKHR(in_device->handle, in_pGetFdInfo, out_pFd);

    if (replay_result != VK_SUCCESS)
    {
        GFXRECON_LOG_WARNING("vkGetFenceFdKHR failed with error %s. Ignoring error and continuing "
                             "replay.",
                             util::ToString(replay_result).c_str());
        replay_result = VK_SUCCESS;
        return;
    }
}

void DiveVulkanReplayConsumer::ProcessCreateHardwareBufferCommand(
format::HandleId                                    device_id,
format::HandleId                                    memory_id,
uint64_t                                            buffer_id,
uint32_t                                            format,
uint32_t                                            width,
uint32_t                                            height,
uint32_t                                            stride,
uint64_t                                            usage,
uint32_t                                            layers,
const std::vector<format::HardwareBufferPlaneInfo>& plane_info)
{
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    // Workaround to replay compositor capture
    if (format > AHARDWAREBUFFER_FORMAT_S8_UINT || (format == AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420))
    {
        GFXRECON_LOG_INFO("AHB format %d, height %d, width %d, layers %d, usage %d, ",
                          format,
                          height,
                          width,
                          layers,
                          usage);
        format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
    }
#endif
    VulkanReplayConsumer::ProcessCreateHardwareBufferCommand(device_id,
                                                             memory_id,
                                                             buffer_id,
                                                             format,
                                                             width,
                                                             height,
                                                             stride,
                                                             usage,
                                                             layers,
                                                             plane_info);
}

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)
