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

#include <vulkan/vk_layer.h>
#include <vulkan/vulkan.h>

namespace DiveLayer
{

struct InstanceDispatchTable
{
    PFN_vkGetInstanceProcAddr                pfn_get_instance_proc_addr = nullptr;
    PFN_vkCreateDevice                       CreateDevice = nullptr;
    PFN_vkEnumerateDeviceLayerProperties     EnumerateDeviceLayerProperties = nullptr;
    PFN_vkEnumerateDeviceExtensionProperties EnumerateDeviceExtensionProperties = nullptr;
};

struct DeviceDispatchTable
{
    PFN_vkGetDeviceProcAddr           pfn_get_device_proc_addr = nullptr;
    PFN_vkQueuePresentKHR             QueuePresentKHR = nullptr;
    PFN_vkCreateImage                 CreateImage = nullptr;
    PFN_vkCmdDrawIndexed              CmdDrawIndexed = nullptr;
    PFN_vkCmdResetQueryPool           CmdResetQueryPool = nullptr;
    PFN_vkCmdWriteTimestamp           CmdWriteTimestamp = nullptr;
    PFN_vkGetQueryPoolResults         GetQueryPoolResults = nullptr;
    PFN_vkDestroyCommandPool          DestroyCommandPool = nullptr;
    PFN_vkAllocateCommandBuffers      AllocateCommandBuffers = nullptr;
    PFN_vkFreeCommandBuffers          FreeCommandBuffers = nullptr;
    PFN_vkResetCommandBuffer          ResetCommandBuffer = nullptr;
    PFN_vkBeginCommandBuffer          BeginCommandBuffer = nullptr;
    PFN_vkEndCommandBuffer            EndCommandBuffer = nullptr;
    PFN_vkAcquireNextImageKHR         AcquireNextImageKHR = nullptr;
    PFN_vkQueueSubmit                 QueueSubmit = nullptr;
    PFN_vkGetDeviceQueue2             GetDeviceQueue2 = nullptr;
    PFN_vkGetDeviceQueue              GetDeviceQueue = nullptr;
    PFN_vkDestroyDevice               DestroyDevice = nullptr;
    PFN_vkCmdInsertDebugUtilsLabelEXT CmdInsertDebugUtilsLabel = nullptr;
    PFN_vkCmdBeginRenderPass          CmdBeginRenderPass = nullptr;
    PFN_vkCmdEndRenderPass            CmdEndRenderPass = nullptr;
    PFN_vkCmdBeginRenderPass2         CmdBeginRenderPass2 = nullptr;
    PFN_vkCmdEndRenderPass2           CmdEndRenderPass2 = nullptr;
};

void InitInstanceDispatchTable(VkInstance                instance,
                               PFN_vkGetInstanceProcAddr pa,
                               InstanceDispatchTable    *dt);
void InitDeviceDispatchTable(VkDevice device, PFN_vkGetDeviceProcAddr pa, DeviceDispatchTable *dt);

}  // namespace DiveLayer
