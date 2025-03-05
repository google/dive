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

#include "vk_rt_layer_impl.h"

#include <cstdio>
#include <cstdlib>
#if defined(__ANDROID__)
#    include <dlfcn.h>
#endif

#include <vulkan/vulkan_core.h>
#include "capture_service/log.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
namespace DiveLayer
{

VkResult QueuePresentKHR(PFN_vkQueuePresentKHR   pfn,
                         VkQueue                 queue,
                         const VkPresentInfoKHR* pPresentInfo)
{
    return pfn(queue, pPresentInfo);
}

VkResult CreateImage(PFN_vkCreateImage            pfn,
                     VkDevice                     device,
                     const VkImageCreateInfo*     pCreateInfo,
                     const VkAllocationCallbacks* pAllocator,
                     VkImage*                     pImage)
{
    // Remove VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM flag
    if ((pCreateInfo->flags & VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM) != 0)
    {
        // LOGI("Image %p CreateImage has the density map offset flag! \n", pImage);
        const_cast<VkImageCreateInfo*>(pCreateInfo)
        ->flags &= ~VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM;
    }

    return pfn(device, pCreateInfo, pAllocator, pImage);
}

}  // namespace DiveLayer