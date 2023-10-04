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

#include "vk_dispatch.h"
#include "capture_service/log.h"

namespace DiveLayer {

void InitInstanceDispatchTable(VkInstance instance,
                               PFN_vkGetInstanceProcAddr pa,
                               InstanceDispatchTable *dt) {
  LOGI("InitInstanceDispatchTable");

  dt->pfn_get_instance_proc_addr = pa;
  dt->CreateDevice = (PFN_vkCreateDevice)pa(instance, "vkCreateDevice");
  dt->EnumerateDeviceLayerProperties = (PFN_vkEnumerateDeviceLayerProperties)pa(
      instance, "vkEnumerateDeviceLayerProperties");

  dt->EnumerateDeviceExtensionProperties =
      (PFN_vkEnumerateDeviceExtensionProperties)pa(
          instance, "vkEnumerateDeviceExtensionProperties");
}

void InitDeviceDispatchTable(VkDevice device, PFN_vkGetDeviceProcAddr pa,
                             DeviceDispatchTable *dt) {
  LOGI("InitDeviceDispatchTable");
  dt->pfn_get_device_proc_addr = pa;
  dt->QueuePresentKHR = (PFN_vkQueuePresentKHR)pa(device, "vkQueuePresentKHR");
}

} // namespace DiveLayer