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
#include <vulkan/vulkan_core.h>

#include <deque>
#include <filesystem>
#include <functional>
#include <limits>
#include <mutex>
#include <numeric>
#include <set>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "frame_boundary_detector.h"
#include "gpu_time.h"
#include "network/drawcall_filter_config.h"

namespace DiveLayer
{

class DiveRuntimeLayer
{
 public:
    struct TrackedPSO
    {
        std::string name;
        bool has_alpha_blend;
    };

    DiveRuntimeLayer();
    ~DiveRuntimeLayer();
    VkResult QueuePresentKHR(PFN_vkQueuePresentKHR pfn, VkQueue queue,
                             const VkPresentInfoKHR* pPresentInfo);

    VkResult CreateImage(PFN_vkCreateImage pfn, VkDevice device,
                         const VkImageCreateInfo* pCreateInfo,
                         const VkAllocationCallbacks* pAllocator, VkImage* pImage);

    VkResult CreateGraphicsPipelines(PFN_vkCreateGraphicsPipelines pfn, VkDevice device,
                                     VkPipelineCache pipelineCache, uint32_t createInfoCount,
                                     const VkGraphicsPipelineCreateInfo* pCreateInfos,
                                     const VkAllocationCallbacks* pAllocator,
                                     VkPipeline* pPipelines);

    void DestroyPipeline(PFN_vkDestroyPipeline pfn, VkDevice device, VkPipeline pipeline,
                         const VkAllocationCallbacks* pAllocator);

    void CmdBindPipeline(PFN_vkCmdBindPipeline pfn, VkCommandBuffer commandBuffer,
                         VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline);

    VkResult SetDebugUtilsObjectNameEXT(PFN_vkSetDebugUtilsObjectNameEXT pfn, VkDevice device,
                                        const VkDebugUtilsObjectNameInfoEXT* pNameInfo);

    void CmdDraw(PFN_vkCmdDraw pfn, VkCommandBuffer commandBuffer, uint32_t vertexCount,
                 uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);

    void CmdDrawIndexed(PFN_vkCmdDrawIndexed pfn, VkCommandBuffer commandBuffer,
                        uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                        int32_t vertexOffset, uint32_t firstInstance);

    void CmdDrawIndirect(PFN_vkCmdDrawIndirect pfn, VkCommandBuffer commandBuffer, VkBuffer buffer,
                         VkDeviceSize offset, uint32_t drawCount, uint32_t stride);

    void CmdDrawIndexedIndirect(PFN_vkCmdDrawIndexedIndirect pfn, VkCommandBuffer commandBuffer,
                                VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount,
                                uint32_t stride);

    void CmdDrawIndirectCount(PFN_vkCmdDrawIndirectCount pfn, VkCommandBuffer commandBuffer,
                              VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer,
                              VkDeviceSize countBufferOffset, uint32_t maxDrawCount,
                              uint32_t stride);

    void CmdDrawIndexedIndirectCount(PFN_vkCmdDrawIndexedIndirectCount pfn,
                                     VkCommandBuffer commandBuffer, VkBuffer buffer,
                                     VkDeviceSize offset, VkBuffer countBuffer,
                                     VkDeviceSize countBufferOffset, uint32_t maxDrawCount,
                                     uint32_t stride);

    void CmdDrawMeshTasksEXT(PFN_vkCmdDrawMeshTasksEXT pfn, VkCommandBuffer commandBuffer,
                             uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);

    void CmdDrawMeshTasksIndirectEXT(PFN_vkCmdDrawMeshTasksIndirectEXT pfn,
                                     VkCommandBuffer commandBuffer, VkBuffer buffer,
                                     VkDeviceSize offset, uint32_t drawCount, uint32_t stride);

    void CmdDrawMeshTasksIndirectCountEXT(PFN_vkCmdDrawMeshTasksIndirectCountEXT pfn,
                                          VkCommandBuffer commandBuffer, VkBuffer buffer,
                                          VkDeviceSize offset, VkBuffer countBuffer,
                                          VkDeviceSize countBufferOffset, uint32_t maxDrawCount,
                                          uint32_t stride);

    void CmdResetQueryPool(PFN_vkCmdResetQueryPool pfn, VkCommandBuffer commandBuffer,
                           VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount);

    void CmdWriteTimestamp(PFN_vkCmdWriteTimestamp pfn, VkCommandBuffer commandBuffer,
                           VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool,
                           uint32_t query);

    VkResult GetQueryPoolResults(PFN_vkGetQueryPoolResults pfn, VkDevice device,
                                 VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount,
                                 size_t dataSize, void* pData, VkDeviceSize stride,
                                 VkQueryResultFlags flags);

    void DestroyCommandPool(PFN_vkDestroyCommandPool pfn, VkDevice device,
                            VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator);

    VkResult AllocateCommandBuffers(PFN_vkAllocateCommandBuffers pfn, VkDevice device,
                                    const VkCommandBufferAllocateInfo* pAllocateInfo,
                                    VkCommandBuffer* pCommandBuffers);

    void FreeCommandBuffers(PFN_vkFreeCommandBuffers pfn, VkDevice device,
                            VkCommandPool commandPool, uint32_t commandBufferCount,
                            const VkCommandBuffer* pCommandBuffers);

    VkResult ResetCommandBuffer(PFN_vkResetCommandBuffer pfn, VkCommandBuffer commandBuffer,
                                VkCommandBufferResetFlags flags);

    VkResult ResetCommandPool(PFN_vkResetCommandPool pfn, VkDevice device,
                              VkCommandPool commandPool, VkCommandPoolResetFlags flags);

    VkResult BeginCommandBuffer(PFN_vkBeginCommandBuffer pfn, VkCommandBuffer commandBuffer,
                                const VkCommandBufferBeginInfo* pBeginInfo);

    VkResult EndCommandBuffer(PFN_vkEndCommandBuffer pfn, VkCommandBuffer commandBuffer);

    VkResult CreateDevice(PFN_vkGetDeviceProcAddr pa, PFN_vkCreateDevice pfn, float timestampPeriod,
                          VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo,
                          const VkAllocationCallbacks* pAllocator, VkDevice* pDevice);

    void DestroyDevice(PFN_vkDestroyDevice pfn, VkDevice device,
                       const VkAllocationCallbacks* pAllocator);

    VkResult AcquireNextImageKHR(PFN_vkAcquireNextImageKHR pfn, VkDevice device,
                                 VkSwapchainKHR swapchain, uint64_t timeout, VkSemaphore semaphore,
                                 VkFence fence, uint32_t* pImageIndex);

    VkResult QueueSubmit(PFN_vkQueueSubmit pfn, VkQueue queue, uint32_t submitCount,
                         const VkSubmitInfo* pSubmits, VkFence fence);

    void GetDeviceQueue2(PFN_vkGetDeviceQueue2 pfn, VkDevice device,
                         const VkDeviceQueueInfo2* pQueueInfo, VkQueue* pQueue);

    void GetDeviceQueue(PFN_vkGetDeviceQueue pfn, VkDevice device, uint32_t queueFamilyIndex,
                        uint32_t queueIndex, VkQueue* pQueue);

    void CmdInsertDebugUtilsLabel(PFN_vkCmdInsertDebugUtilsLabelEXT pfn,
                                  VkCommandBuffer commandBuffer,
                                  const VkDebugUtilsLabelEXT* pLabelInfo);

    void CmdBeginRenderPass(PFN_vkCmdBeginRenderPass pfn, VkCommandBuffer commandBuffer,
                            const VkRenderPassBeginInfo* pRenderPassBegin,
                            VkSubpassContents contents);

    void CmdEndRenderPass(PFN_vkCmdEndRenderPass pfn, VkCommandBuffer commandBuffer);

    void CmdBeginRenderPass2(PFN_vkCmdBeginRenderPass2 pfn, VkCommandBuffer commandBuffer,
                             const VkRenderPassBeginInfo* pRenderPassBegin,
                             const VkSubpassBeginInfo* pSubpassBeginInfo);

    void CmdEndRenderPass2(PFN_vkCmdEndRenderPass2 pfn, VkCommandBuffer commandBuffer,
                           const VkSubpassEndInfo* pSubpassEndInfo);

    void UpdateFilterConfig(const Network::DrawcallFilterConfig& config)
    {
        std::unique_lock lock(m_config_mutex);
        m_pending_filter_config = config;
    }

    void EnqueueFrameBoundaryTask(std::function<void()> task);

    void ProcessFrameBoundaryTasks();

    std::vector<Network::PSOInfo> GetLivePSOs();

 private:
    bool CheckAndIncrementDrawcallCount();

    template <bool HasVertex, bool HasIndex, bool HasInstance>
    bool ShouldFilterDrawCall(VkCommandBuffer command_buffer, uint32_t vertex_count = 0,
                              uint32_t index_count = 0, uint32_t instance_count = 0) const;

    Dive::GPUTime m_gpu_time;
    Dive::FrameBoundaryDetector m_boundary_detector;

    PFN_vkGetDeviceProcAddr m_device_proc_addr = nullptr;

    // Cache all vk function pointers
    PFN_vkResetQueryPool m_pfn_vkResetQueryPool = nullptr;
    PFN_vkQueueWaitIdle m_pfn_vkQueueWaitIdle = nullptr;
    PFN_vkDestroyQueryPool m_pfn_vkDestroyQueryPool = nullptr;
    PFN_vkDeviceWaitIdle m_pfn_vkDeviceWaitIdle = nullptr;
    PFN_vkGetQueryPoolResults m_pfn_vkGetQueryPoolResults = nullptr;
    PFN_vkCmdWriteTimestamp m_pfn_vkCmdWriteTimestamp = nullptr;

    // Configuration for drawcall filtering.
    std::mutex m_config_mutex;
    Network::DrawcallFilterConfig m_pending_filter_config;
    Network::DrawcallFilterConfig m_active_filter_config;

    // Frame boundary tasks to be executed at the end of a frame.
    std::mutex m_task_mutex;
    std::vector<std::function<void()>> m_frame_boundary_tasks;

    // Global drawcall counter.
    std::atomic<uint32_t> m_global_drawcall_counter{0};

    // Pipeline State Object (PSO) state tracking.
    // Performance Note: std::unique_lock (exclusive write) is strictly limited to
    // infrequent pipeline lifecycle events (CreateGraphicsPipelines, DestroyPipeline,
    // and SetDebugUtilsObjectNameEXT).
    // Command recording paths (CmdBindPipeline) use std::shared_lock (concurrent read)
    // to prevent blocking multiple threads recording command buffers simultaneously.
    // The hottest paths (CmdDraw*) do not touch this mutex at all.
    std::shared_mutex m_pso_mutex;
    absl::flat_hash_map<VkPipeline, TrackedPSO> m_live_psos;
};

}  // namespace DiveLayer
