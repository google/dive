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

#include <array>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include <vulkan/vk_layer.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "capture_service/log.h"
#include "vk_rt_dispatch.h"
#include "vk_rt_layer_impl.h"

namespace DiveLayer
{

DiveRuntimeLayer sDiveRuntimeLayer;
// Declare our per-instance and per-device contexts.
// These are created and initialized in vkCreateInstance and vkCreateDevice.
struct InstanceData
{
    VkInstance            instance;
    InstanceDispatchTable dispatch_table;
};

struct DeviceData
{
    VkDevice            device;
    DeviceDispatchTable dispatch_table;
};

// note this is from vulkan loader:
// https://github.com/KhronosGroup/Vulkan-Loader/blob/main/loader/loader.h#L46
// note this will get the dispatch table as the key
// according to the vulkan loader, vkdevice, vkcmd, vkqueue share the same table
// search `loader_set_dispatch` in
// https://github.com/KhronosGroup/Vulkan-Loader/blob/main/loader/trampoline.c#L1067
inline uintptr_t DataKey(const void *object) { return (uintptr_t)(*(void **)object); }

namespace
{
// Generally we expect to get the same device and instance, so we keep them
// handy
static thread_local InstanceData *last_used_instance_data = nullptr;
static thread_local DeviceData   *last_used_device_data = nullptr;

std::mutex                                                   g_instance_mutex;
std::unordered_map<uintptr_t, std::unique_ptr<InstanceData>> g_instance_data;

std::mutex                                                 g_device_mutex;
std::unordered_map<uintptr_t, std::unique_ptr<DeviceData>> g_device_data;

constexpr VkLayerProperties layer_properties = { "VK_LAYER_Dive",
                                                 VK_MAKE_VERSION(1, 0, VK_HEADER_VERSION),
                                                 1,
                                                 "Dive capture layer for xr." };

static constexpr std::array<VkExtensionProperties, 2> instance_extensions{ {
{ VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_SPEC_VERSION },
{ VK_EXT_DEBUG_UTILS_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_SPEC_VERSION },
} };

static constexpr std::array<VkExtensionProperties, 1> device_extensions{ {
{ VK_EXT_DEBUG_MARKER_EXTENSION_NAME, VK_EXT_DEBUG_MARKER_SPEC_VERSION },
} };

}  // namespace

InstanceData *GetInstanceLayerData(uintptr_t key)
{
    if (last_used_instance_data && DataKey(last_used_instance_data->instance) == key)
    {
        return last_used_instance_data;
    }

    std::lock_guard<std::mutex> lock(g_instance_mutex);
    last_used_instance_data = g_instance_data[key].get();
    return last_used_instance_data;
}

DeviceData *GetDeviceLayerData(uintptr_t key)
{
    if (last_used_device_data && DataKey(last_used_device_data->device) == key)
    {
        return last_used_device_data;
    }

    std::lock_guard<std::mutex> lock(g_device_mutex);
    last_used_device_data = g_device_data[key].get();
    return last_used_device_data;
}

struct VkStruct
{
    VkStructureType sType;
    const void     *pNext;
};

VkStruct *FindOnChain(VkStruct *s, VkStructureType type)
{
    VkStruct *n = (VkStruct *)s->pNext;
    while (n && n->sType != type)
    {
        n = (VkStruct *)n->pNext;
    }
    return n;
}

VkLayerInstanceCreateInfo *GetLoaderInstanceInfo(const VkInstanceCreateInfo *create_info,
                                                 VkLayerFunction             func_type)
{
    VkStruct *n = (VkStruct *)create_info;
    while ((n = FindOnChain(n, VK_STRUCTURE_TYPE_LOADER_INSTANCE_CREATE_INFO)) != nullptr)
    {
        VkLayerInstanceCreateInfo *vci = (VkLayerInstanceCreateInfo *)n;
        if (vci->function == func_type)
        {
            return vci;
        }
    }
    return nullptr;
}

VkLayerDeviceCreateInfo *GetLoaderDeviceInfo(const VkDeviceCreateInfo *create_info,
                                             VkLayerFunction           func_type)
{
    VkStruct *n = (VkStruct *)create_info;
    while ((n = FindOnChain(n, VK_STRUCTURE_TYPE_LOADER_DEVICE_CREATE_INFO)) != nullptr)
    {
        VkLayerDeviceCreateInfo *vdi = (VkLayerDeviceCreateInfo *)n;
        if (vdi->function == func_type)
        {
            return vdi;
        }
    }
    return nullptr;
}

// Intercept functions.
// Device functions
VkResult DiveInterceptQueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo)
{
    PFN_vkQueuePresentKHR pfn = nullptr;

    auto layer_data = GetDeviceLayerData(DataKey(queue));
    pfn = layer_data->dispatch_table.QueuePresentKHR;
    return sDiveRuntimeLayer.QueuePresentKHR(pfn, queue, pPresentInfo);
}

VkResult DiveInterceptCreateImage(VkDevice                     device,
                                  const VkImageCreateInfo     *pCreateInfo,
                                  const VkAllocationCallbacks *pAllocator,
                                  VkImage                     *pImage)
{
    PFN_vkCreateImage pfn = nullptr;

    auto layer_data = GetDeviceLayerData(DataKey(device));

    pfn = layer_data->dispatch_table.CreateImage;
    return sDiveRuntimeLayer.CreateImage(pfn, device, pCreateInfo, pAllocator, pImage);
}

void DiveInterceptCmdDrawIndexed(VkCommandBuffer commandBuffer,
                                 uint32_t        indexCount,
                                 uint32_t        instanceCount,
                                 uint32_t        firstIndex,
                                 int32_t         vertexOffset,
                                 uint32_t        firstInstance)
{
    PFN_vkCmdDrawIndexed pfn = nullptr;

    auto layer_data = GetDeviceLayerData(DataKey(commandBuffer));

    pfn = layer_data->dispatch_table.CmdDrawIndexed;
    return sDiveRuntimeLayer.CmdDrawIndexed(pfn,
                                            commandBuffer,
                                            indexCount,
                                            instanceCount,
                                            firstIndex,
                                            vertexOffset,
                                            firstInstance);
}

void DiveInterceptCmdResetQueryPool(VkCommandBuffer commandBuffer,
                                    VkQueryPool     queryPool,
                                    uint32_t        firstQuery,
                                    uint32_t        queryCount)
{
    PFN_vkCmdResetQueryPool pfn = nullptr;

    auto layer_data = GetDeviceLayerData(DataKey(commandBuffer));

    pfn = layer_data->dispatch_table.CmdResetQueryPool;
    sDiveRuntimeLayer.CmdResetQueryPool(pfn, commandBuffer, queryPool, firstQuery, queryCount);
}

void DiveInterceptCmdWriteTimestamp(VkCommandBuffer         commandBuffer,
                                    VkPipelineStageFlagBits pipelineStage,
                                    VkQueryPool             queryPool,
                                    uint32_t                query)
{
    PFN_vkCmdWriteTimestamp pfn = nullptr;

    auto layer_data = GetDeviceLayerData(DataKey(commandBuffer));

    pfn = layer_data->dispatch_table.CmdWriteTimestamp;
    sDiveRuntimeLayer.CmdWriteTimestamp(pfn, commandBuffer, pipelineStage, queryPool, query);
}

VkResult DiveInterceptGetQueryPoolResults(VkDevice           device,
                                          VkQueryPool        queryPool,
                                          uint32_t           firstQuery,
                                          uint32_t           queryCount,
                                          size_t             dataSize,
                                          void              *pData,
                                          VkDeviceSize       stride,
                                          VkQueryResultFlags flags)
{
    PFN_vkGetQueryPoolResults pfn = nullptr;

    auto layer_data = GetDeviceLayerData(DataKey(device));

    pfn = layer_data->dispatch_table.GetQueryPoolResults;
    return sDiveRuntimeLayer.GetQueryPoolResults(pfn,
                                                 device,
                                                 queryPool,
                                                 firstQuery,
                                                 queryCount,
                                                 dataSize,
                                                 pData,
                                                 stride,
                                                 flags);
}

void DiveInterceptDestroyCommandPool(VkDevice                     device,
                                     VkCommandPool                commandPool,
                                     const VkAllocationCallbacks *pAllocator)
{
    PFN_vkDestroyCommandPool pfn = nullptr;

    auto layer_data = GetDeviceLayerData(DataKey(device));

    pfn = layer_data->dispatch_table.DestroyCommandPool;
    return sDiveRuntimeLayer.DestroyCommandPool(pfn, device, commandPool, pAllocator);
}

VkResult DiveInterceptAllocateCommandBuffers(VkDevice                           device,
                                             const VkCommandBufferAllocateInfo *pAllocateInfo,
                                             VkCommandBuffer                   *pCommandBuffers)
{
    PFN_vkAllocateCommandBuffers pfn = nullptr;

    auto layer_data = GetDeviceLayerData(DataKey(device));

    pfn = layer_data->dispatch_table.AllocateCommandBuffers;
    return sDiveRuntimeLayer.AllocateCommandBuffers(pfn, device, pAllocateInfo, pCommandBuffers);
}

void DiveInterceptFreeCommandBuffers(VkDevice               device,
                                     VkCommandPool          commandPool,
                                     uint32_t               commandBufferCount,
                                     const VkCommandBuffer *pCommandBuffers)
{
    PFN_vkFreeCommandBuffers pfn = nullptr;

    auto layer_data = GetDeviceLayerData(DataKey(device));

    pfn = layer_data->dispatch_table.FreeCommandBuffers;
    return sDiveRuntimeLayer.FreeCommandBuffers(pfn,
                                                device,
                                                commandPool,
                                                commandBufferCount,
                                                pCommandBuffers);
}

VkResult DiveInterceptResetCommandBuffer(VkCommandBuffer           commandBuffer,
                                         VkCommandBufferResetFlags flags)
{
    PFN_vkResetCommandBuffer pfn = nullptr;

    auto layer_data = GetDeviceLayerData(DataKey(commandBuffer));

    pfn = layer_data->dispatch_table.ResetCommandBuffer;
    return sDiveRuntimeLayer.ResetCommandBuffer(pfn, commandBuffer, flags);
}

VkResult DiveInterceptBeginCommandBuffer(VkCommandBuffer                 commandBuffer,
                                         const VkCommandBufferBeginInfo *pBeginInfo)
{
    PFN_vkBeginCommandBuffer pfn = nullptr;

    auto layer_data = GetDeviceLayerData(DataKey(commandBuffer));

    pfn = layer_data->dispatch_table.BeginCommandBuffer;
    return sDiveRuntimeLayer.BeginCommandBuffer(pfn, commandBuffer, pBeginInfo);
}

VkResult DiveInterceptEndCommandBuffer(VkCommandBuffer commandBuffer)
{
    PFN_vkEndCommandBuffer pfn = nullptr;

    auto layer_data = GetDeviceLayerData(DataKey(commandBuffer));

    pfn = layer_data->dispatch_table.EndCommandBuffer;
    return sDiveRuntimeLayer.EndCommandBuffer(pfn, commandBuffer);
}

void DiveInterceptGetDeviceQueue2(VkDevice                  device,
                                  const VkDeviceQueueInfo2 *pQueueInfo,
                                  VkQueue                  *pQueue)
{
    PFN_vkGetDeviceQueue2 pfn = nullptr;

    auto layer_data = GetDeviceLayerData(DataKey(device));
    pfn = layer_data->dispatch_table.GetDeviceQueue2;
    return sDiveRuntimeLayer.GetDeviceQueue2(pfn, device, pQueueInfo, pQueue);
}

void DiveInterceptGetDeviceQueue(VkDevice device,
                                 uint32_t queueFamilyIndex,
                                 uint32_t queueIndex,
                                 VkQueue *pQueue)
{
    PFN_vkGetDeviceQueue pfn = nullptr;

    auto layer_data = GetDeviceLayerData(DataKey(device));
    pfn = layer_data->dispatch_table.GetDeviceQueue;
    return sDiveRuntimeLayer.GetDeviceQueue(pfn, device, queueFamilyIndex, queueIndex, pQueue);
}

VkResult DiveInterceptAcquireNextImageKHR(VkDevice       device,
                                          VkSwapchainKHR swapchain,
                                          uint64_t       timeout,
                                          VkSemaphore    semaphore,
                                          VkFence        fence,
                                          uint32_t      *pImageIndex)
{
    PFN_vkAcquireNextImageKHR pfn = nullptr;

    auto layer_data = GetDeviceLayerData(DataKey(device));
    pfn = layer_data->dispatch_table.AcquireNextImageKHR;
    return sDiveRuntimeLayer
    .AcquireNextImageKHR(pfn, device, swapchain, timeout, semaphore, fence, pImageIndex);
}

VkResult DiveInterceptQueueSubmit(VkQueue             queue,
                                  uint32_t            submitCount,
                                  const VkSubmitInfo *pSubmits,
                                  VkFence             fence)
{
    PFN_vkQueueSubmit pfn = nullptr;

    auto layer_data = GetDeviceLayerData(DataKey(queue));
    pfn = layer_data->dispatch_table.QueueSubmit;
    return sDiveRuntimeLayer.QueueSubmit(pfn, queue, submitCount, pSubmits, fence);
}

// Instance functions
// Create instance needs a special implementation for layer
VkResult DiveInterceptCreateInstance(const VkInstanceCreateInfo  *pCreateInfo,
                                     const VkAllocationCallbacks *pAllocator,
                                     VkInstance                  *pInstance)
{
    LOGI("DiveInterceptCreateInstance");
    // Find the create info
    VkLayerInstanceCreateInfo *layer_create_info = GetLoaderInstanceInfo(pCreateInfo,
                                                                         VK_LAYER_LINK_INFO);

    if (layer_create_info == NULL)
    {
        // No loader instance create info
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    PFN_vkGetInstanceProcAddr pfn_get_instance_proc_addr = layer_create_info->u.pLayerInfo
                                                           ->pfnNextGetInstanceProcAddr;
    // Move chain on for the next layer.
    layer_create_info->u.pLayerInfo = layer_create_info->u.pLayerInfo->pNext;

    PFN_vkCreateInstance pfn_create_instance = (PFN_vkCreateInstance)
    pfn_get_instance_proc_addr(NULL, "vkCreateInstance");

    auto result = pfn_create_instance(pCreateInfo, pAllocator, pInstance);
    if (VK_SUCCESS != result)
    {
        return result;
    }

    LOGI("vk Instance Created\n");
    auto id = std::make_unique<InstanceData>();
    id->instance = *pInstance;
    InitInstanceDispatchTable(*pInstance, pfn_get_instance_proc_addr, &id->dispatch_table);

    {
        std::lock_guard<std::mutex> lock(g_instance_mutex);
        auto                        key = (uintptr_t)(*(void **)(*pInstance));
        g_instance_data[key] = std::move(id);
    }

    return result;
}

VkResult DiveInterceptCreateDevice(VkPhysicalDevice             gpu,
                                   const VkDeviceCreateInfo    *pCreateInfo,
                                   const VkAllocationCallbacks *pAllocator,
                                   VkDevice                    *pDevice)
{
    VkLayerDeviceCreateInfo *layer_create_info = GetLoaderDeviceInfo(pCreateInfo,
                                                                     VK_LAYER_LINK_INFO);
    LOGI("DCI %p\n", layer_create_info);

    // Get the instance data.
    auto instance_data = GetInstanceLayerData(DataKey(gpu));

    // Get the proc addr pointers for this layer and update the chain for the next
    // layer.
    PFN_vkGetInstanceProcAddr pfn_next_instance_proc_addr = layer_create_info->u.pLayerInfo
                                                            ->pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr pfn_next_device_proc_addr = layer_create_info->u.pLayerInfo
                                                        ->pfnNextGetDeviceProcAddr;
    PFN_vkCreateDevice pfn_create_device = (PFN_vkCreateDevice)
    pfn_next_instance_proc_addr(instance_data->instance, "vkCreateDevice");
    layer_create_info->u.pLayerInfo = layer_create_info->u.pLayerInfo->pNext;

    PFN_vkGetPhysicalDeviceProperties
    GetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)
    pfn_next_instance_proc_addr(instance_data->instance, "vkGetPhysicalDeviceProperties");

    VkPhysicalDeviceProperties deviceProperties;
    GetPhysicalDeviceProperties(gpu, &deviceProperties);
    LOGI("Device Name: %s\n", deviceProperties.deviceName);
    LOGI("Device Type: %s\n",
         (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU   ? "Discrete GPU" :
          deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? "Integrated GPU" :
          deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU    ? "Virtual GPU" :
          deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU            ? "CPU" :
                                                                                  "Other"));
    LOGI("API Version: %d.%d.%d",
         VK_VERSION_MAJOR(deviceProperties.apiVersion),
         VK_VERSION_MINOR(deviceProperties.apiVersion),
         VK_VERSION_PATCH(deviceProperties.apiVersion));

    VkResult result = sDiveRuntimeLayer.CreateDevice(pfn_next_device_proc_addr,
                                                     pfn_create_device,
                                                     deviceProperties.limits.timestampPeriod,
                                                     gpu,
                                                     pCreateInfo,
                                                     pAllocator,
                                                     pDevice);

    if (VK_SUCCESS != result)
    {
        return result;
    }

    LOGI("vk Device Created\n");
    auto dd = std::make_unique<DeviceData>();
    dd->device = *pDevice;
    InitDeviceDispatchTable(*pDevice, pfn_next_device_proc_addr, &dd->dispatch_table);

    {
        std::lock_guard<std::mutex> lock(g_device_mutex);
        auto                        key = (uintptr_t)(*(void **)(*pDevice));
        g_device_data[key] = std::move(dd);
    }

    return result;
}

void DiveInterceptDestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator)
{
    PFN_vkDestroyDevice pfn = nullptr;

    auto layer_data = GetDeviceLayerData(DataKey(device));
    pfn = layer_data->dispatch_table.DestroyDevice;
    return sDiveRuntimeLayer.DestroyDevice(pfn, device, pAllocator);
}

void DiveInterceptCmdInsertDebugUtilsLabel(VkCommandBuffer             commandBuffer,
                                           const VkDebugUtilsLabelEXT *pLabelInfo)
{
    PFN_vkCmdInsertDebugUtilsLabelEXT pfn = nullptr;

    auto layer_data = GetDeviceLayerData(DataKey(commandBuffer));
    pfn = layer_data->dispatch_table.CmdInsertDebugUtilsLabel;
    return sDiveRuntimeLayer.CmdInsertDebugUtilsLabel(pfn, commandBuffer, pLabelInfo);
}

extern "C"
{

    VKAPI_ATTR VkResult VKAPI_CALL
    DiveInterceptEnumerateInstanceLayerProperties(uint32_t          *pPropertyCount,
                                                  VkLayerProperties *pProperties)
    {
        LOGI("DiveInterceptEnumerateInstanceLayerProperties");

        VkResult result = VK_SUCCESS;

        if (pProperties == nullptr)
        {
            if (pPropertyCount != nullptr)
            {
                *pPropertyCount = 1;
            }
        }
        else
        {
            if ((pPropertyCount != nullptr) && (*pPropertyCount >= 1))
            {
                memcpy(pProperties, &layer_properties, sizeof(*pProperties));
                *pPropertyCount = 1;
            }
            else
            {
                result = VK_INCOMPLETE;
            }
        }

        return result;
    }

    VKAPI_ATTR VkResult VKAPI_CALL DiveInterceptEnumerateInstanceExtensionProperties(
    const VkEnumerateInstanceExtensionPropertiesChain *pChain,
    char                                              *pLayerName,
    uint32_t                                          *pPropertyCount,
    VkExtensionProperties                             *pProperties)
    {

        LOGI("DiveInterceptEnumerateInstanceExtensionProperties");
        VkResult result = VK_SUCCESS;
        if (pLayerName && !strcmp(pLayerName, layer_properties.layerName))
        {
            if (pPropertyCount != nullptr)
            {
                uint32_t extension_count = static_cast<uint32_t>(instance_extensions.size());

                if (pProperties == nullptr)
                {
                    *pPropertyCount = extension_count;
                }
                else
                {
                    if ((*pPropertyCount) < extension_count)
                    {
                        result = VK_INCOMPLETE;
                        extension_count = *pPropertyCount;
                    }
                    else if ((*pPropertyCount) > extension_count)
                    {
                        *pPropertyCount = extension_count;
                    }

                    for (uint32_t i = 0; i < extension_count; ++i)
                    {
                        pProperties[i] = instance_extensions[i];
                    }
                }
            }
            return result;
        }

        // When not called with this layer's name, call down to retrieve the
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

        exts.insert(exts.end(), instance_extensions.begin(), instance_extensions.end());
        if (nullptr == pProperties)
        {
            *pPropertyCount = downstream_ext_count + 2;
            return VK_SUCCESS;
        }
        else if (*pPropertyCount > 0)
        {
            *pPropertyCount = downstream_ext_count + 2;
            memcpy(pProperties, exts.data(), exts.size() * sizeof(VkExtensionProperties));
        }

        return result;
    }

    VKAPI_ATTR VkResult VKAPI_CALL
    DiveInterceptEnumerateDeviceLayerProperties(VkPhysicalDevice   physicalDevice,
                                                uint32_t          *pPropertyCount,
                                                VkLayerProperties *pProperties)
    {
        LOGI("DiveInterceptEnumerateDeviceLayerProperties");
        return DiveInterceptEnumerateInstanceLayerProperties(pPropertyCount, pProperties);
    }

    VKAPI_ATTR VkResult VKAPI_CALL
    DiveInterceptEnumerateDeviceExtensionProperties(VkPhysicalDevice       physicalDevice,
                                                    char                  *pLayerName,
                                                    uint32_t              *pPropertyCount,
                                                    VkExtensionProperties *pProperties)
    {

        VkResult result = VK_SUCCESS;

        if (nullptr != pLayerName && strcmp(pLayerName, layer_properties.layerName) == 0)
        {
            if (pPropertyCount != nullptr)
            {
                uint32_t extension_count = static_cast<uint32_t>(device_extensions.size());

                if (pProperties == nullptr)
                {
                    *pPropertyCount = extension_count;
                }
                else
                {
                    if ((*pPropertyCount) < extension_count)
                    {
                        result = VK_INCOMPLETE;
                        extension_count = *pPropertyCount;
                    }
                    else if ((*pPropertyCount) > extension_count)
                    {
                        *pPropertyCount = extension_count;
                    }

                    for (uint32_t i = 0; i < extension_count; ++i)
                    {
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
        result = instance_data->dispatch_table
                 .EnumerateDeviceExtensionProperties(physicalDevice,
                                                     nullptr,
                                                     &num_other_extensions,
                                                     nullptr);
        if (result != VK_SUCCESS)
        {
            return result;
        }
        // call down to get other device properties
        std::vector<VkExtensionProperties> extensions(num_other_extensions);
        result = instance_data->dispatch_table
                 .EnumerateDeviceExtensionProperties(physicalDevice,
                                                     pLayerName,
                                                     &num_other_extensions,
                                                     &extensions[0]);

        // add our extensions if we have any and requested
        if (result != VK_SUCCESS)
        {
            return result;
        }

        // not just our layer, we expose all our extensions
        uint32_t max_extensions = *pPropertyCount;

        // set and copy base extensions
        *pPropertyCount = num_other_extensions;

        // find our unique extensions that need to be added
        uint32_t                                   num_additional_extensions = 0;
        auto                                       num_device_extensions = device_extensions.size();
        std::vector<const VkExtensionProperties *> additional_extensions(num_device_extensions);

        for (size_t i = 0; i < num_device_extensions; ++i)
        {
            bool is_unique_extension = true;

            for (size_t j = 0; j < num_other_extensions; ++j)
            {
                if (0 == strcmp(extensions[j].extensionName, device_extensions[i].extensionName))
                {
                    is_unique_extension = false;
                    break;
                }
            }

            if (is_unique_extension)
            {
                additional_extensions[num_additional_extensions++] = &device_extensions[i];
            }
        }

        // null properties, just count total extensions
        if (nullptr == pProperties)
        {
            *pPropertyCount += num_additional_extensions;
        }
        else
        {
            uint32_t numExtensions = std::min(num_other_extensions, max_extensions);
            memcpy(pProperties, &extensions[0], numExtensions * sizeof(VkExtensionProperties));

            for (size_t i = 0; i < num_additional_extensions && numExtensions < max_extensions; ++i)
            {
                pProperties[numExtensions++] = *additional_extensions[i];
            }

            *pPropertyCount = numExtensions;

            // not enough space for all extensions
            if (num_other_extensions + num_additional_extensions > max_extensions)
            {
                result = VK_INCOMPLETE;
            }
        }

        return result;
    }

    VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL VK_LAYER_DiveGetDeviceProcAddr(VkDevice    dev,
                                                                            const char *func)
    {
        if (!strcmp(func, "vkGetDeviceProcAddr"))
            return (PFN_vkVoidFunction)&VK_LAYER_DiveGetDeviceProcAddr;
        if (0 == strcmp(func, "vkQueuePresentKHR"))
            return (PFN_vkVoidFunction)DiveInterceptQueuePresentKHR;
        if (0 == strcmp(func, "vkCreateImage"))
            return (PFN_vkVoidFunction)DiveInterceptCreateImage;
        if (0 == strcmp(func, "vkCmdDrawIndexed"))
            return (PFN_vkVoidFunction)DiveInterceptCmdDrawIndexed;
        if (0 == strcmp(func, "vkCmdResetQueryPool"))
            return (PFN_vkVoidFunction)DiveInterceptCmdResetQueryPool;
        if (0 == strcmp(func, "vkCmdWriteTimestamp"))
            return (PFN_vkVoidFunction)DiveInterceptCmdWriteTimestamp;
        if (0 == strcmp(func, "vkGetQueryPoolResults"))
            return (PFN_vkVoidFunction)DiveInterceptGetQueryPoolResults;
        if (0 == strcmp(func, "vkDestroyCommandPool"))
            return (PFN_vkVoidFunction)DiveInterceptDestroyCommandPool;
        if (0 == strcmp(func, "vkAllocateCommandBuffers"))
            return (PFN_vkVoidFunction)DiveInterceptAllocateCommandBuffers;
        if (0 == strcmp(func, "vkFreeCommandBuffers"))
            return (PFN_vkVoidFunction)DiveInterceptFreeCommandBuffers;
        if (0 == strcmp(func, "vkBeginCommandBuffer"))
            return (PFN_vkVoidFunction)DiveInterceptBeginCommandBuffer;
        if (0 == strcmp(func, "vkResetCommandBuffer"))
            return (PFN_vkVoidFunction)DiveInterceptResetCommandBuffer;
        if (0 == strcmp(func, "vkEndCommandBuffer"))
            return (PFN_vkVoidFunction)DiveInterceptEndCommandBuffer;
        if (0 == strcmp(func, "vkAcquireNextImageKHR"))
            return (PFN_vkVoidFunction)DiveInterceptAcquireNextImageKHR;
        if (0 == strcmp(func, "vkQueueSubmit"))
            return (PFN_vkVoidFunction)DiveInterceptQueueSubmit;
        if (0 == strcmp(func, "vkGetDeviceQueue2"))
            return (PFN_vkVoidFunction)DiveInterceptGetDeviceQueue2;
        if (0 == strcmp(func, "vkEndGetDeviceQueue"))
            return (PFN_vkVoidFunction)DiveInterceptGetDeviceQueue;
        if (0 == strcmp(func, "vkDestroyDevice"))
            return (PFN_vkVoidFunction)&DiveInterceptDestroyDevice;
        if (0 == strcmp(func, "vkCmdInsertDebugUtilsLabelEXT"))
            return (PFN_vkVoidFunction)&DiveInterceptCmdInsertDebugUtilsLabel;
        auto layer_data = GetDeviceLayerData(DataKey(dev));
        return layer_data->dispatch_table.pfn_get_device_proc_addr(dev, func);
    }

    VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL VK_LAYER_DiveGetInstanceProcAddr(VkInstance  inst,
                                                                              const char *func)
    {
        if (0 == strcmp(func, "vkGetInstanceProcAddr"))
            return (PFN_vkVoidFunction)&VK_LAYER_DiveGetInstanceProcAddr;
        if (0 == strcmp(func, "vkEnumerateInstanceExtensionProperties"))
            return (PFN_vkVoidFunction)&DiveInterceptEnumerateInstanceExtensionProperties;
        if (0 == strcmp(func, "vkCreateInstance"))
            return (PFN_vkVoidFunction)&DiveInterceptCreateInstance;
        if (inst == VK_NULL_HANDLE)
            return NULL;

        // This is required since sometimes vkGetInstanceProcAddr is called for
        // vkCmdInsertDebugUtilsLabelEXT even it is a device func
        if (0 == strcmp(func, "vkCmdInsertDebugUtilsLabelEXT"))
        {
            return (PFN_vkVoidFunction)&DiveInterceptCmdInsertDebugUtilsLabel;
        }

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
    VK_LAYER_DiveNegotiateLoaderLayerInterfaceVersion(VkNegotiateLayerInterface *pVersionStruct)
    {
        LOGI("VkNegotiateLayerInterface\n");

        if (pVersionStruct->sType != LAYER_NEGOTIATE_INTERFACE_STRUCT)
        {
            LOGE("pVersionStruct must have the type of LAYER_NEGOTIATE_INTERFACE_STRUCT!");
        }

        if (pVersionStruct->loaderLayerInterfaceVersion >= 2)
        {
            pVersionStruct->pfnGetInstanceProcAddr = VK_LAYER_DiveGetInstanceProcAddr;
            pVersionStruct->pfnGetDeviceProcAddr = VK_LAYER_DiveGetDeviceProcAddr;
            pVersionStruct->pfnGetPhysicalDeviceProcAddr = nullptr;
        }
        if (pVersionStruct->loaderLayerInterfaceVersion > 2)
        {
            pVersionStruct->loaderLayerInterfaceVersion = 2;
        }
        return VK_SUCCESS;
    }
}

}  // namespace DiveLayer