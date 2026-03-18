/*
Copyright 2026 Google Inc.

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

#include <vulkan/vulkan_core.h>

#include <string>
#include <unordered_map>

namespace Dive
{

class FrameBoundaryDetector
{
 public:
    static constexpr const char* kVulkanVrFrameDelimiterString =
        "vr-marker,frame_end,type,application";

    struct BoundaryStatus
    {
        std::string message;
        bool success = true;
    };

    void OnAllocateCommandBuffers(const VkCommandBufferAllocateInfo* allocate_info_ptr,
                                  VkCommandBuffer* command_buffers_ptr);

    void OnFreeCommandBuffers(uint32_t command_buffer_count,
                              const VkCommandBuffer* command_buffers_ptr);

    void OnResetCommandBuffer(VkCommandBuffer command_buffer);

    void OnResetCommandPool(VkCommandPool command_pool);

    BoundaryStatus MarkBoundary(VkCommandBuffer command_buffer,
                                const VkDebugUtilsLabelEXT* label_info_ptr);

    bool ContainsFrameBoundary(uint32_t submit_count, const VkSubmitInfo* submits_ptr) const;

    bool IsFrameBoundary(VkCommandBuffer command_buffer) const;

    void ConsumeBoundaries(uint32_t submit_count, const VkSubmitInfo* submits_ptr);

 private:
    struct CommandBufferInfo
    {
        VkCommandPool pool = VK_NULL_HANDLE;
        bool is_frameboundary = false;
    };

    std::unordered_map<VkCommandBuffer, CommandBufferInfo> m_cmds;
};

}  // namespace Dive
