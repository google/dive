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

#include <vulkan/vk_layer.h>
#include <vulkan/vulkan.h>

#include <array>
#include <cassert>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include <cstdio>
#include <iostream>
#include <vulkan/vulkan_core.h>

#include "dispatch.h"
#include "layer_impl.h"
#include "log.h"

namespace DiveLayer {

bool is_libwrap_loaded() {
    bool  loaded = false;
#if defined(__ANDROID__)
    FILE *maps = fopen("/proc/self/maps", "r");
    if (!maps)
    {
        return loaded;
    }

  char *line = NULL;
  size_t size = 0;
  while (getline(&line, &size, maps) > 0) {
    if (strstr(line, "libwrap.so")) {
        loaded = true;
        break;
    }
  }
  free(line);
  fclose(maps);
#endif
  return loaded;
}

// Declare our per-instance and per-device contexts.
// These are created and initialized in vkCreateInstance and vkCreateDevice.
struct InstanceData {
  VkInstance instance;
  InstanceDispatchTable dispatch_table;
};

struct DeviceData {
  VkDevice device;
  DeviceDispatchTable dispatch_table;
};

namespace {
// Generally we expect to get the same device and instance, so we keep them
// handy
static thread_local InstanceData *last_used_instance_data = nullptr;
static thread_local DeviceData *last_used_device_data = nullptr;

std::mutex g_instance_mutex;
std::unordered_map<uintptr_t, std::unique_ptr<InstanceData>> g_instance_data;

std::mutex g_device_mutex;
std::unordered_map<uintptr_t, std::unique_ptr<DeviceData>> g_device_data;

constexpr VkLayerProperties layer_properties = {
    "VK_LAYER_Dive", VK_MAKE_VERSION(1, 0, VK_HEADER_VERSION), 1,
    "Dive capture layer for xr."};

static constexpr std::array<VkExtensionProperties, 2> instance_extensions{{
    {VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_SPEC_VERSION},
    {VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_SPEC_VERSION},
}};

static constexpr std::array<VkExtensionProperties, 1> device_extensions{{
    {VK_EXT_DEBUG_MARKER_EXTENSION_NAME, VK_EXT_DEBUG_MARKER_SPEC_VERSION},
}};

} // namespace

uintptr_t DataKey(const void *object) { return (uintptr_t)(*(void **)object); }

InstanceData *GetInstanceLayerData(uintptr_t key) {
  if (last_used_instance_data &&
      DataKey(last_used_instance_data->instance) == key) {
    return last_used_instance_data;
  }

  std::lock_guard<std::mutex> lock(g_instance_mutex);
  last_used_instance_data = g_instance_data[key].get();
  return last_used_instance_data;
}

DeviceData *GetDeviceLayerData(uintptr_t key) {
  if (last_used_device_data && DataKey(last_used_device_data->device) == key) {
    return last_used_device_data;
  }

  std::lock_guard<std::mutex> lock(g_device_mutex);
  last_used_device_data = g_device_data[key].get();
  return last_used_device_data;
}

struct VkStruct {
  VkStructureType sType;
  const void *pNext;
};

VkStruct *FindOnChain(VkStruct *s, VkStructureType type) {
  VkStruct *n = (VkStruct *)s->pNext;
  while (n && n->sType != type) {
    n = (VkStruct *)n->pNext;
  }
  return n;
}

VkLayerInstanceCreateInfo *
GetLoaderInstanceInfo(const VkInstanceCreateInfo *create_info,
                      VkLayerFunction func_type) {
  VkStruct *n = (VkStruct *)create_info;
  while ((n = FindOnChain(n, VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO)) != nullptr) {
    VkLayerInstanceCreateInfo *vci = (VkLayerInstanceCreateInfo *)n;
    if (vci->function == func_type) {
      return vci;
    }
  }
  return nullptr;
}

VkLayerDeviceCreateInfo *
GetLoaderDeviceInfo(const VkDeviceCreateInfo *create_info,
                    VkLayerFunction func_type) {
  VkStruct *n = (VkStruct *)create_info;
  while ((n = FindOnChain(n, VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO)) != nullptr) {
    VkLayerDeviceCreateInfo *vdi = (VkLayerDeviceCreateInfo *)n;
    if (vdi->function == func_type) {
      return vdi;
    }
  }
  return nullptr;
}

// Intercept functions.
VkResult DiveInterceptQueuePresentKHR(VkQueue queue,
                                  const VkPresentInfoKHR *pPresentInfo) {
  PFN_vkQueuePresentKHR pfn = nullptr;

  auto layer_data = GetDeviceLayerData(DataKey(queue));
  pfn = layer_data->dispatch_table.QueuePresentKHR;
  return QueuePresentKHR(pfn, queue, pPresentInfo);
}

// Create instance needs a special implementaiton for layer
VkResult DiveInterceptCreateInstance(const VkInstanceCreateInfo *pCreateInfo,
                                 const VkAllocationCallbacks *pAllocator,
                                 VkInstance *pInstance) {
  LOGI("DiveInterceptCreateInstance");
  // Find the create info
  VkLayerInstanceCreateInfo *layer_create_info =
      GetLoaderInstanceInfo(pCreateInfo, VK_LAYER_LINK_INFO);

  if (layer_create_info == NULL) {
    // No loader instance create info
    return VK_ERROR_INITIALIZATION_FAILED;
  }

  PFN_vkGetInstanceProcAddr pfn_get_instance_proc_addr =
      layer_create_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
  // Move chain on for the next layer.
  layer_create_info->u.pLayerInfo = layer_create_info->u.pLayerInfo->pNext;

  PFN_vkCreateInstance pfn_create_instance =
      (PFN_vkCreateInstance)pfn_get_instance_proc_addr(NULL,
                                                       "vkCreateInstance");

  auto result = pfn_create_instance(pCreateInfo, pAllocator, pInstance);
  if (VK_SUCCESS != result) {
    return result;
  }

  LOGI("Created\n");
  auto id = std::make_unique<InstanceData>();
  id->instance = *pInstance;
  InitInstanceDispatchTable(*pInstance, pfn_get_instance_proc_addr,
                            &id->dispatch_table);

  {
    std::lock_guard<std::mutex> lock(g_instance_mutex);
    auto key = (uintptr_t)(*(void **)(*pInstance));
    g_instance_data[key] = std::move(id);
  }

  return result;
}

VkResult DiveInterceptCreateDevice(VkPhysicalDevice gpu,
                               const VkDeviceCreateInfo *pCreateInfo,
                               const VkAllocationCallbacks *pAllocator,
                               VkDevice *pDevice) {
  VkLayerDeviceCreateInfo *layer_create_info =
      GetLoaderDeviceInfo(pCreateInfo, VK_LAYER_LINK_INFO);
  LOGI("DCI %p\n", layer_create_info);

  // Get the instance data.
  auto instance_data = GetInstanceLayerData(DataKey(gpu));

  // Get the proc addr pointers for this layer and update the chain for the next
  // layer.
  PFN_vkGetInstanceProcAddr pfn_next_instance_proc_addr =
      layer_create_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
  PFN_vkGetDeviceProcAddr pfn_next_device_proc_addr =
      layer_create_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
  PFN_vkCreateDevice pfn_create_device =
      (PFN_vkCreateDevice)pfn_next_instance_proc_addr(instance_data->instance,
                                                      "vkCreateDevice");
  layer_create_info->u.pLayerInfo = layer_create_info->u.pLayerInfo->pNext;

  VkResult result = pfn_create_device(gpu, pCreateInfo, pAllocator, pDevice);
  if (VK_SUCCESS != result) {
    return result;
  }

  LOGI("Created\n");
  auto dd = std::make_unique<DeviceData>();
  dd->device = *pDevice;
  InitDeviceDispatchTable(*pDevice, pfn_next_device_proc_addr,
                          &dd->dispatch_table);

  {
    std::lock_guard<std::mutex> lock(g_device_mutex);
    auto key = (uintptr_t)(*(void **)(*pDevice));
    g_device_data[key] = std::move(dd);
  }

  return result;
}

extern "C" {

VKAPI_ATTR VkResult VKAPI_CALL DiveInterceptEnumerateInstanceLayerProperties(
    uint32_t *pPropertyCount, VkLayerProperties *pProperties) {
  LOGI("DiveInterceptEnumerateInstanceLayerProperties");

  VkResult result = VK_SUCCESS;

  if (pProperties == nullptr) {
    if (pPropertyCount != nullptr) {
      *pPropertyCount = 1;
    }
  } else {
    if ((pPropertyCount != nullptr) && (*pPropertyCount >= 1)) {
      memcpy(pProperties, &layer_properties, sizeof(*pProperties));
      *pPropertyCount = 1;
    } else {
      result = VK_INCOMPLETE;
    }
  }

  return result;
}

VKAPI_ATTR VkResult VKAPI_CALL DiveInterceptEnumerateInstanceExtensionProperties(
    const VkEnumerateInstanceExtensionPropertiesChain *pChain, char *pLayerName,
    uint32_t *pPropertyCount, VkExtensionProperties *pProperties) {

  LOGI("DiveInterceptEnumerateInstanceExtensionProperties");
  VkResult result = VK_SUCCESS;
  if (pLayerName && !strcmp(pLayerName, layer_properties.layerName)) {
    if (pPropertyCount != nullptr) {
      uint32_t extension_count =
          static_cast<uint32_t>(instance_extensions.size());

      if (pProperties == nullptr) {
        *pPropertyCount = extension_count;
      } else {
        if ((*pPropertyCount) < extension_count) {
          result = VK_INCOMPLETE;
          extension_count = *pPropertyCount;
        } else if ((*pPropertyCount) > extension_count) {
          *pPropertyCount = extension_count;
        }

        for (uint32_t i = 0; i < extension_count; ++i) {
          pProperties[i] = instance_extensions[i];
        }
      }
    }
    return result;
  }

  // When not called with this layer's name, call down to retrive the
  // properties.
  uint32_t downstream_ext_count = 0;
  ;
  result = pChain->CallDown(pLayerName, &downstream_ext_count, NULL);
  if (result != VK_SUCCESS)
    return result;
  std::vector<VkExtensionProperties> exts;
  exts.resize(downstream_ext_count);
  result = pChain->CallDown(pLayerName, &downstream_ext_count, &exts[0]);

  if (result != VK_SUCCESS)
    return result;

  exts.insert(exts.end(), instance_extensions.begin(),
              instance_extensions.end());
  if (nullptr == pProperties) {
    *pPropertyCount = downstream_ext_count + 2;
    return VK_SUCCESS;
  } else if (*pPropertyCount > 0) {
    *pPropertyCount = downstream_ext_count + 2;
    memcpy(pProperties, exts.data(),
           exts.size() * sizeof(VkExtensionProperties));
  }

  return result;
}

VKAPI_ATTR VkResult VKAPI_CALL DiveInterceptEnumerateDeviceLayerProperties(
    VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount,
    VkLayerProperties *pProperties) {
  LOGI("DiveInterceptEnumerateDeviceLayerProperties");
  return DiveInterceptEnumerateInstanceLayerProperties(pPropertyCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL DiveInterceptEnumerateDeviceExtensionProperties(
    VkPhysicalDevice physicalDevice, char *pLayerName, uint32_t *pPropertyCount,
    VkExtensionProperties *pProperties) {

  VkResult result = VK_SUCCESS;

  if (nullptr != pLayerName &&
      strcmp(pLayerName, layer_properties.layerName) == 0) {
    if (pPropertyCount != nullptr) {
      uint32_t extension_count =
          static_cast<uint32_t>(device_extensions.size());

      if (pProperties == nullptr) {
        *pPropertyCount = extension_count;
      } else {
        if ((*pPropertyCount) < extension_count) {
          result = VK_INCOMPLETE;
          extension_count = *pPropertyCount;
        } else if ((*pPropertyCount) > extension_count) {
          *pPropertyCount = extension_count;
        }

        for (uint32_t i = 0; i < extension_count; ++i) {
          pProperties[i] = device_extensions[i];
        }
      }
    }
    return result;
  }

  // If not called with this layer's name, call down to get the properties and
  // append our extensions, removing duplicates.
  InstanceData *instance_data = GetInstanceLayerData(DataKey(physicalDevice));

  uint32_t num_other_extensions = 0;
  result = instance_data->dispatch_table.EnumerateDeviceExtensionProperties(
      physicalDevice, nullptr, &num_other_extensions, nullptr);
  if (result != VK_SUCCESS) {
    return result;
  }
  // call down to get other device properties
  std::vector<VkExtensionProperties> extensions(num_other_extensions);
  result = instance_data->dispatch_table.EnumerateDeviceExtensionProperties(
      physicalDevice, pLayerName, &num_other_extensions, &extensions[0]);

  // add our extensions if we have any and requested
  if (result != VK_SUCCESS) {
    return result;
  }

  // not just our layer, we expose all our extensions
  uint32_t max_extensions = *pPropertyCount;

  // set and copy base extensions
  *pPropertyCount = num_other_extensions;

  // find our unique extensions that need to be added
  uint32_t num_additional_extensions = 0;
  auto num_device_extensions = device_extensions.size();
  std::vector<const VkExtensionProperties *> additional_extensions(
      num_device_extensions);

  for (size_t i = 0; i < num_device_extensions; ++i) {
    bool is_unique_extension = true;

    for (size_t j = 0; j < num_other_extensions; ++j) {
      if (0 == strcmp(extensions[j].extensionName,
                      device_extensions[i].extensionName)) {
        is_unique_extension = false;
        break;
      }
    }

    if (is_unique_extension) {
      additional_extensions[num_additional_extensions++] =
          &device_extensions[i];
    }
  }

  // null properties, just count total extensions
  if (nullptr == pProperties) {
    *pPropertyCount += num_additional_extensions;
  } else {
    uint32_t numExtensions = std::min(num_other_extensions, max_extensions);
    memcpy(pProperties, &extensions[0],
           numExtensions * sizeof(VkExtensionProperties));

    for (size_t i = 0;
         i < num_additional_extensions && numExtensions < max_extensions; ++i) {
      pProperties[numExtensions++] = *additional_extensions[i];
    }

    *pPropertyCount = numExtensions;

    // not enough space for all extensions
    if (num_other_extensions + num_additional_extensions > max_extensions) {
      result = VK_INCOMPLETE;
    }
  }

  return result;
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
VK_LAYER_DiveGetDeviceProcAddr(VkDevice dev, const char *func) {
  LOGI("GetDeviceProcAddr %s\n", func);

  if (!strcmp(func, "vkGetDeviceProcAddr"))
    return (PFN_vkVoidFunction)&VK_LAYER_DiveGetDeviceProcAddr;
  if (!strcmp(func, "vkCreateDevice"))
    return (PFN_vkVoidFunction)&DiveInterceptCreateDevice;
  if (0 == strcmp(func, "vkQueuePresentKHR"))
    return (PFN_vkVoidFunction)DiveInterceptQueuePresentKHR;
  auto layer_data = GetDeviceLayerData(DataKey(dev));
  return layer_data->dispatch_table.pfn_get_device_proc_addr(dev, func);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
VK_LAYER_DiveGetInstanceProcAddr(VkInstance inst, const char *func) {
  LOGI("GetInstanceProcAddr %s\n", func);

  if (0 == strcmp(func, "vkGetInstanceProcAddr"))
    return (PFN_vkVoidFunction)&VK_LAYER_DiveGetInstanceProcAddr;
  if (0 == strcmp(func, "vkEnumerateInstanceExtensionProperties"))
    return (PFN_vkVoidFunction)&DiveInterceptEnumerateInstanceExtensionProperties;
  if (0 == strcmp(func, "vkCreateInstance"))
    return (PFN_vkVoidFunction)&DiveInterceptCreateInstance;
  if (inst == VK_NULL_HANDLE)
    return NULL;

  if (0 == strcmp(func, "vkEnumerateDeviceLayerProperties"))
    return (PFN_vkVoidFunction)DiveInterceptEnumerateDeviceLayerProperties;
  if (0 == strcmp(func, "vkEnumerateDeviceExtensionProperties"))
    return (PFN_vkVoidFunction)DiveInterceptEnumerateDeviceExtensionProperties;
  if (!strcmp(func, "vkGetDeviceProcAddr"))
    return (PFN_vkVoidFunction)&VK_LAYER_DiveGetDeviceProcAddr;

  if (0 == strcmp(func, "vkEnumerateInstanceLayerProperties"))
    return (PFN_vkVoidFunction)&DiveInterceptEnumerateInstanceLayerProperties;
  if (0 == strcmp(func, "vkCreateDevice"))
    return (PFN_vkVoidFunction)&DiveInterceptCreateDevice;
  auto instance_data = GetInstanceLayerData(DataKey(inst));
  return instance_data->dispatch_table.pfn_get_instance_proc_addr(inst, func);
}

#if defined(WIN32)
__declspec(dllexport)
#endif
    VKAPI_ATTR VkResult VKAPI_CALL
    VK_LAYER_DiveNegotiateLoaderLayerInterfaceVersion(
        VkNegotiateLayerInterface *pVersionStruct) {
  LOGI("VkNegotiateLayerInterface\n");

  assert(pVersionStruct != NULL);
  assert(pVersionStruct->sType == LAYER_NEGOTIATE_INTERFACE_STRUCT);

  if (pVersionStruct->loaderLayerInterfaceVersion >= 2) {
    pVersionStruct->pfnGetInstanceProcAddr = VK_LAYER_DiveGetInstanceProcAddr;
    pVersionStruct->pfnGetDeviceProcAddr = VK_LAYER_DiveGetDeviceProcAddr;
    pVersionStruct->pfnGetPhysicalDeviceProcAddr = nullptr;
  }
  if (pVersionStruct->loaderLayerInterfaceVersion > 2) {
    pVersionStruct->loaderLayerInterfaceVersion = 2;
  }
  return VK_SUCCESS;
}
}

} // namespace DiveLayer