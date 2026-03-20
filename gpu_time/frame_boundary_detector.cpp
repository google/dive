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

#include "frame_boundary_detector.h"

#include <cstring>

namespace Dive
{

void FrameBoundaryDetector::OnAllocateCommandBuffers(
    const VkCommandBufferAllocateInfo* allocate_info_ptr, VkCommandBuffer* command_buffers_ptr)
{
    if (allocate_info_ptr == nullptr || command_buffers_ptr == nullptr)
    {
        return;
    }

    // We only track primary command buffers.
    if (allocate_info_ptr->level == VK_COMMAND_BUFFER_LEVEL_SECONDARY)
    {
        return;
    }

    for (uint32_t i = 0; i < allocate_info_ptr->commandBufferCount; ++i)
    {
        m_cmds[command_buffers_ptr[i]] = {.pool = allocate_info_ptr->commandPool,
                                          .is_frameboundary = false};
    }
}

void FrameBoundaryDetector::OnFreeCommandBuffers(uint32_t command_buffer_count,
                                                 const VkCommandBuffer* command_buffers_ptr)
{
    if (command_buffers_ptr == nullptr)
    {
        return;
    }

    for (uint32_t i = 0; i < command_buffer_count; ++i)
    {
        m_cmds.erase(command_buffers_ptr[i]);
    }
}

void FrameBoundaryDetector::OnResetCommandBuffer(VkCommandBuffer command_buffer)
{
    if (auto it = m_cmds.find(command_buffer); it != m_cmds.end())
    {
        it->second.is_frameboundary = false;
    }
}

void FrameBoundaryDetector::OnResetCommandPool(VkCommandPool command_pool)
{
    for (auto& [cmd, info] : m_cmds)
    {
        if (info.pool == command_pool)
        {
            info.is_frameboundary = false;
        }
    }
}

FrameBoundaryDetector::BoundaryStatus FrameBoundaryDetector::MarkBoundary(
    VkCommandBuffer command_buffer, const VkDebugUtilsLabelEXT* label_info_ptr)
{
    if (label_info_ptr == nullptr || label_info_ptr->pLabelName == nullptr)
    {
        return FrameBoundaryDetector::BoundaryStatus{
            .message = "label_info_ptr or label_info_ptr->pLabelName is null!", .success = false};
    }

    if (strcmp(kVulkanVrFrameDelimiterString, label_info_ptr->pLabelName) == 0)
    {
        auto it = m_cmds.find(command_buffer);
        if (it != m_cmds.end())
        {
            it->second.is_frameboundary = true;
        }
        else
        {
            return FrameBoundaryDetector::BoundaryStatus{
                .message =
                    "command_buffer is not in the cmd cache (a frame boundary should be always in "
                    "a primary command buffer)",
                .success = false};
        }
    }

    return FrameBoundaryDetector::BoundaryStatus();
}

bool FrameBoundaryDetector::ContainsFrameBoundary(uint32_t submit_count,
                                                  const VkSubmitInfo* submits_ptr) const
{
    if (submits_ptr == nullptr)
    {
        return false;
    }

    for (uint32_t i = 0; i < submit_count; ++i)
    {
        if (submits_ptr[i].pCommandBuffers == nullptr)
        {
            continue;
        }

        for (uint32_t c = 0; c < submits_ptr[i].commandBufferCount; ++c)
        {
            if (IsFrameBoundary(submits_ptr[i].pCommandBuffers[c]))
            {
                return true;
            }
        }
    }
    return false;
}

bool FrameBoundaryDetector::IsFrameBoundary(VkCommandBuffer command_buffer) const
{
    if (auto it = m_cmds.find(command_buffer); it != m_cmds.end())
    {
        return it->second.is_frameboundary;
    }
    return false;
}

void FrameBoundaryDetector::ConsumeBoundaries(uint32_t submit_count,
                                              const VkSubmitInfo* submits_ptr)
{
    if (submits_ptr == nullptr)
    {
        return;
    }

    for (uint32_t i = 0; i < submit_count; ++i)
    {
        if (submits_ptr[i].pCommandBuffers == nullptr)
        {
            continue;
        }

        for (uint32_t c = 0; c < submits_ptr[i].commandBufferCount; ++c)
        {
            if (auto it = m_cmds.find(submits_ptr[i].pCommandBuffers[c]);
                it != m_cmds.end() && it->second.is_frameboundary)
            {
                it->second.is_frameboundary = false;
            }
        }
    }
}

}  // namespace Dive
