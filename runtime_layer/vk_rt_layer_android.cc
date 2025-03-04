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

#include <vulkan/vk_layer.h>
#include <vulkan/vulkan.h>

extern "C"
{
#if defined(__ANDROID__)
    // This is needed since the android vulkan loader requires the layer to implement the
    // instance&device enumeration functions
    VkResult VKAPI_CALL DiveInterceptEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice,
                                                                    uint32_t        *pPropertyCount,
                                                                    VkLayerProperties *pProperties);

    VkResult VKAPI_CALL
    DiveInterceptEnumerateDeviceExtensionProperties(VkPhysicalDevice       physicalDevice,
                                                    const char            *pLayerName,
                                                    uint32_t              *pPropertyCount,
                                                    VkExtensionProperties *pProperties);

    VkResult VKAPI_CALL DiveInterceptEnumerateInstanceExtensionProperties(
    const VkEnumerateInstanceExtensionPropertiesChain *pChain,
    const char                                        *pLayerName,
    uint32_t                                          *pPropertyCount,
    VkExtensionProperties                             *pProperties);

    VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(VkPhysicalDevice   physicalDevice,
                                                         uint32_t          *pPropertyCount,
                                                         VkLayerProperties *pProperties)
    {
        return DiveInterceptEnumerateDeviceLayerProperties(physicalDevice,
                                                           pPropertyCount,
                                                           pProperties);
    }

    VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice       physicalDevice,
                                                             const char            *pLayerName,
                                                             uint32_t              *pPropertyCount,
                                                             VkExtensionProperties *pProperties)
    {
        return DiveInterceptEnumerateDeviceExtensionProperties(physicalDevice,
                                                               pLayerName,
                                                               pPropertyCount,
                                                               pProperties);
    }

    VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t          *pPropertyCount,
                                                           VkLayerProperties *pProperties)
    {
        return DiveInterceptEnumerateDeviceLayerProperties(VK_NULL_HANDLE,
                                                           pPropertyCount,
                                                           pProperties);
    }

    VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char *pLayerName,
                                                               uint32_t   *pPropertyCount,
                                                               VkExtensionProperties *pProperties)
    {
        return DiveInterceptEnumerateInstanceExtensionProperties(NULL,
                                                                 pLayerName,
                                                                 pPropertyCount,
                                                                 pProperties);
    }
#endif
}
