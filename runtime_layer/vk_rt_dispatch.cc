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

#include "vk_rt_dispatch.h"
#include "common/log.h"

namespace DiveLayer
{

void InitInstanceDispatchTable(VkInstance                instance,
                               PFN_vkGetInstanceProcAddr pa,
                               InstanceDispatchTable    *dt)
{
    LOGI("InitInstanceDispatchTable");

    dt->pfn_get_instance_proc_addr = pa;
    dt->CreateDevice = (PFN_vkCreateDevice)pa(instance, "vkCreateDevice");
    dt->EnumerateDeviceLayerProperties = (PFN_vkEnumerateDeviceLayerProperties)
    pa(instance, "vkEnumerateDeviceLayerProperties");

    dt->EnumerateDeviceExtensionProperties = (PFN_vkEnumerateDeviceExtensionProperties)
    pa(instance, "vkEnumerateDeviceExtensionProperties");
}

void InitDeviceDispatchTable(VkDevice device, PFN_vkGetDeviceProcAddr pa, DeviceDispatchTable *dt)
{
    LOGI("InitDeviceDispatchTable");
    dt->pfn_get_device_proc_addr = pa;
    dt->QueuePresentKHR = (PFN_vkQueuePresentKHR)pa(device, "vkQueuePresentKHR");
    dt->CreateImage = (PFN_vkCreateImage)pa(device, "vkCreateImage");
    dt->CmdDrawIndexed = (PFN_vkCmdDrawIndexed)pa(device, "vkCmdDrawIndexed");
    dt->CmdResetQueryPool = (PFN_vkCmdResetQueryPool)pa(device, "vkCmdResetQueryPool");
    dt->CmdWriteTimestamp = (PFN_vkCmdWriteTimestamp)pa(device, "vkCmdWriteTimestamp");
    dt->GetQueryPoolResults = (PFN_vkGetQueryPoolResults)pa(device, "vkGetQueryPoolResults");
    dt->DestroyCommandPool = (PFN_vkDestroyCommandPool)pa(device, "vkDestroyCommandPool");
    dt->AllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)pa(device,
                                                                  "vkAllocateCommandBuffers");
    dt->FreeCommandBuffers = (PFN_vkFreeCommandBuffers)pa(device, "vkFreeCommandBuffers");
    dt->ResetCommandBuffer = (PFN_vkResetCommandBuffer)pa(device, "vkResetCommandBuffer");
    dt->BeginCommandBuffer = (PFN_vkBeginCommandBuffer)pa(device, "vkBeginCommandBuffer");
    dt->EndCommandBuffer = (PFN_vkEndCommandBuffer)pa(device, "vkEndCommandBuffer");
    dt->AcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)pa(device, "vkAcquireNextImageKHR");
    dt->QueueSubmit = (PFN_vkQueueSubmit)pa(device, "vkQueueSubmit");
    dt->GetDeviceQueue2 = (PFN_vkGetDeviceQueue2)pa(device, "vkGetDeviceQueue2");
    dt->GetDeviceQueue = (PFN_vkGetDeviceQueue)pa(device, "vkGetDeviceQueue");
    dt->DestroyDevice = (PFN_vkDestroyDevice)pa(device, "vkDestroyDevice");
    dt->CmdInsertDebugUtilsLabel = (PFN_vkCmdInsertDebugUtilsLabelEXT)
    pa(device, "vkCmdInsertDebugUtilsLabelEXT");

    dt->CmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)pa(device, "vkCmdBeginRenderPass");
    dt->CmdEndRenderPass = (PFN_vkCmdEndRenderPass)pa(device, "vkCmdEndRenderPass");
    dt->CmdBeginRenderPass2 = (PFN_vkCmdBeginRenderPass2)pa(device, "vkCmdBeginRenderPass2");
    dt->CmdEndRenderPass2 = (PFN_vkCmdEndRenderPass2)pa(device, "vkCmdEndRenderPass2");
}

}  // namespace DiveLayer