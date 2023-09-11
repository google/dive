/*
Copyright 2023 Google Inc.

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

namespace DiveXrLayer
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
    PFN_vkGetDeviceProcAddr pfn_get_device_proc_addr = nullptr;
    PFN_vkQueuePresentKHR   QueuePresentKHR = nullptr;
#ifdef VK_USE_PLATFORM_XCB_KHR
#endif  // VK_USE_PLATFORM_XCB_KHR
};

void InitInstanceDispatchTable(VkInstance                instance,
                               PFN_vkGetInstanceProcAddr pa,
                               InstanceDispatchTable    *dt);
void InitDeviceDispatchTable(VkDevice device, PFN_vkGetDeviceProcAddr pa, DeviceDispatchTable *dt);

}  // namespace DiveXrLayer