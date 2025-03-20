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

static bool sEnableDrawcallReport = false;
static bool sEnableDrawcallLimit = false;
static bool sEnableDrawcallFilter = false;
static bool sRemoveImageFlagFDMOffset = false;
static bool sRemoveImageFlagSubSampled = false;

static uint32_t sDrawcallCounter = 0;
static size_t   sTotalIndexCounter = 0;

constexpr uint32_t kDrawcallCountLimit = 300;
constexpr uint32_t kVisibilityMaskIndexCount = 42;

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
    if (sRemoveImageFlagFDMOffset &&
        ((pCreateInfo->flags & VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM) != 0))
    {
        LOGI("Image %p CreateImage has the density map offset flag! \n", pImage);
        const_cast<VkImageCreateInfo*>(pCreateInfo)
        ->flags &= ~VK_IMAGE_CREATE_FRAGMENT_DENSITY_MAP_OFFSET_BIT_QCOM;
    }

    if (sRemoveImageFlagSubSampled &&
        ((pCreateInfo->flags & VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT) != 0))
    {
        LOGI("Image %p CreateImage has the subsampled bit flag! \n", pImage);
        const_cast<VkImageCreateInfo*>(pCreateInfo)->flags &= ~VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT;
    }

    return pfn(device, pCreateInfo, pAllocator, pImage);
}

void CmdDrawIndexed(PFN_vkCmdDrawIndexed pfn,
                    VkCommandBuffer      commandBuffer,
                    uint32_t             indexCount,
                    uint32_t             instanceCount,
                    uint32_t             firstIndex,
                    int32_t              vertexOffset,
                    uint32_t             firstInstance)
{
    // Disable drawcalls with N index count
    // Specifically for visibility mask:
    // BiRP is using 2 drawcalls with 42 each, URP is using 1 drawcall with 84,
    if (sEnableDrawcallFilter && ((indexCount == kVisibilityMaskIndexCount) ||
                                  (indexCount == kVisibilityMaskIndexCount * 2)))
    {
        LOGI("Skip drawcalls with index count of %d & %d\n",
             kVisibilityMaskIndexCount,
             kVisibilityMaskIndexCount * 2);
        return;
    }

    ++sDrawcallCounter;
    sTotalIndexCounter += indexCount;

    if (sEnableDrawcallLimit && (sDrawcallCounter > kDrawcallCountLimit))
    {
        return;
    }

    return pfn(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

VkResult BeginCommandBuffer(PFN_vkBeginCommandBuffer        pfn,
                            VkCommandBuffer                 commandBuffer,
                            const VkCommandBufferBeginInfo* pBeginInfo)
{
    if (sEnableDrawcallReport)
    {
        LOGI("Drawcall count: %d\n", sDrawcallCounter);
        LOGI("Total index count: %zd\n", sTotalIndexCounter);
    }

    sDrawcallCounter = 0;
    sTotalIndexCounter = 0;
    return pfn(commandBuffer, pBeginInfo);
}

VkResult EndCommandBuffer(PFN_vkEndCommandBuffer pfn, VkCommandBuffer commandBuffer)
{
    return pfn(commandBuffer);
}

}  // namespace DiveLayer