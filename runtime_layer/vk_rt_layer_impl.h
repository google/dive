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

VkResult QueuePresentKHR(PFN_vkQueuePresentKHR   pfn,
                         VkQueue                 queue,
                         const VkPresentInfoKHR* pPresentInfo);

VkResult CreateImage(PFN_vkCreateImage            pfn,
                     VkDevice                     device,
                     const VkImageCreateInfo*     pCreateInfo,
                     const VkAllocationCallbacks* pAllocator,
                     VkImage*                     pImage);

void CmdDrawIndexed(PFN_vkCmdDrawIndexed pfn,
                    VkCommandBuffer      commandBuffer,
                    uint32_t             indexCount,
                    uint32_t             instanceCount,
                    uint32_t             firstIndex,
                    int32_t              vertexOffset,
                    uint32_t             firstInstance);

VkResult BeginCommandBuffer(PFN_vkBeginCommandBuffer        pfn,
                            VkCommandBuffer                 commandBuffer,
                            const VkCommandBufferBeginInfo* pBeginInfo);

VkResult EndCommandBuffer(PFN_vkEndCommandBuffer pfn, VkCommandBuffer commandBuffer);

}  // namespace DiveLayer