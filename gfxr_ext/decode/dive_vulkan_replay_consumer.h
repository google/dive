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

// NOLINT(build/header_guard)
#ifndef GFXRECON_DECODE_VULKAN_DIVE_CONSUMER_H
#define GFXRECON_DECODE_VULKAN_DIVE_CONSUMER_H

#include <set>
#include <unordered_map>
#include <vector>

#include "absl/functional/any_invocable.h"
#include "generated/generated_vulkan_replay_consumer.h"
#include "gpu_time/gpu_time.h"

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

class DiveVulkanReplayConsumer : public VulkanReplayConsumer
{
 public:
    DiveVulkanReplayConsumer(std::shared_ptr<application::Application> application,
                             const VulkanReplayOptions& options);

    ~DiveVulkanReplayConsumer() override;

    void Process_vkCreateInstance(const ApiCallInfo& call_info, VkResult returnValue,
                                  StructPointerDecoder<Decoded_VkInstanceCreateInfo>* pCreateInfo,
                                  StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
                                  HandlePointerDecoder<VkInstance>* pInstance) override;

    void Process_vkCreateDevice(const ApiCallInfo& call_info, VkResult returnValue,
                                format::HandleId physicalDevice,
                                StructPointerDecoder<Decoded_VkDeviceCreateInfo>* pCreateInfo,
                                StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
                                HandlePointerDecoder<VkDevice>* pDevice) override;

    void Process_vkDestroyDevice(
        const ApiCallInfo& call_info, format::HandleId device,
        StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator) override;

    void Process_vkDestroyCommandPool(
        const ApiCallInfo& call_info, format::HandleId device, format::HandleId commandPool,
        StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator) override;

    void Process_vkAllocateCommandBuffers(
        const ApiCallInfo& call_info, VkResult returnValue, format::HandleId device,
        StructPointerDecoder<Decoded_VkCommandBufferAllocateInfo>* pAllocateInfo,
        HandlePointerDecoder<VkCommandBuffer>* pCommandBuffers) override;

    void Process_vkFreeCommandBuffers(
        const ApiCallInfo& call_info, format::HandleId device, format::HandleId commandPool,
        uint32_t commandBufferCount,
        HandlePointerDecoder<VkCommandBuffer>* pCommandBuffers) override;

    void Process_vkResetCommandBuffer(const ApiCallInfo& call_info, VkResult returnValue,
                                      format::HandleId commandBuffer,
                                      VkCommandBufferResetFlags flags) override;

    void Process_vkCreateCommandPool(
        const ApiCallInfo& call_info, VkResult returnValue, format::HandleId device,
        StructPointerDecoder<Decoded_VkCommandPoolCreateInfo>* pCreateInfo,
        StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
        HandlePointerDecoder<VkCommandPool>* pCommandPool) override;

    void Process_vkResetCommandPool(const ApiCallInfo& call_info, VkResult returnValue,
                                    format::HandleId device, format::HandleId commandPool,
                                    VkCommandPoolResetFlags flags) override;

    void Process_vkQueueSubmit(const ApiCallInfo& call_info, VkResult returnValue,
                               format::HandleId queue, uint32_t submitCount,
                               StructPointerDecoder<Decoded_VkSubmitInfo>* pSubmits,
                               format::HandleId fence) override;

    void Process_vkQueuePresentKHR(
        const ApiCallInfo& call_info, VkResult returnValue, format::HandleId queue,
        StructPointerDecoder<Decoded_VkPresentInfoKHR>* pPresentInfo) override;

    void Process_vkBeginCommandBuffer(
        const ApiCallInfo& call_info, VkResult returnValue, format::HandleId commandBuffer,
        StructPointerDecoder<Decoded_VkCommandBufferBeginInfo>* pBeginInfo) override;

    void Process_vkEndCommandBuffer(const ApiCallInfo& call_info, VkResult returnValue,
                                    format::HandleId commandBuffer) override;

    void Process_vkGetDeviceQueue2(const ApiCallInfo& call_info, format::HandleId device,
                                   StructPointerDecoder<Decoded_VkDeviceQueueInfo2>* pQueueInfo,
                                   HandlePointerDecoder<VkQueue>* pQueue) override;

    void Process_vkGetDeviceQueue(const ApiCallInfo& call_info, format::HandleId device,
                                  uint32_t queueFamilyIndex, uint32_t queueIndex,
                                  HandlePointerDecoder<VkQueue>* pQueue) override;

    void Process_vkCmdInsertDebugUtilsLabelEXT(
        const ApiCallInfo& call_info, format::HandleId commandBuffer,
        StructPointerDecoder<Decoded_VkDebugUtilsLabelEXT>* pLabelInfo) override;

    void Process_vkCmdBeginRenderPass(
        const ApiCallInfo& call_info, format::HandleId commandBuffer,
        StructPointerDecoder<Decoded_VkRenderPassBeginInfo>* pRenderPassBegin,
        VkSubpassContents contents) override;

    void Process_vkCmdEndRenderPass(const ApiCallInfo& call_info,
                                    format::HandleId commandBuffer) override;

    void Process_vkCmdBeginRenderPass2(
        const ApiCallInfo& call_info, format::HandleId commandBuffer,
        StructPointerDecoder<Decoded_VkRenderPassBeginInfo>* pRenderPassBegin,
        StructPointerDecoder<Decoded_VkSubpassBeginInfo>* pSubpassBeginInfo) override;

    void Process_vkCmdEndRenderPass2(
        const ApiCallInfo& call_info, format::HandleId commandBuffer,
        StructPointerDecoder<Decoded_VkSubpassEndInfo>* pSubpassEndInfo) override;

    void Process_vkCmdBeginRenderPass2KHR(
        const ApiCallInfo& call_info, format::HandleId commandBuffer,
        StructPointerDecoder<Decoded_VkRenderPassBeginInfo>* pRenderPassBegin,
        StructPointerDecoder<Decoded_VkSubpassBeginInfo>* pSubpassBeginInfo) override;

    void Process_vkCmdEndRenderPass2KHR(
        const ApiCallInfo& call_info, format::HandleId commandBuffer,
        StructPointerDecoder<Decoded_VkSubpassEndInfo>* pSubpassEndInfo) override;

    void Process_vkCreateFence(const ApiCallInfo& call_info, VkResult returnValue,
                               format::HandleId device,
                               StructPointerDecoder<Decoded_VkFenceCreateInfo>* pCreateInfo,
                               StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
                               HandlePointerDecoder<VkFence>* pFence) override;

    void Process_vkGetFenceFdKHR(const ApiCallInfo& call_info, VkResult returnValue,
                                 format::HandleId device,
                                 StructPointerDecoder<Decoded_VkFenceGetFdInfoKHR>* pGetFdInfo,
                                 PointerDecoder<int>* pFd) override;

    void Process_vkDestroyFence(
        const ApiCallInfo& call_info, format::HandleId device, format::HandleId fence,
        StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator) override;

    void Process_vkAllocateMemory(const ApiCallInfo& call_info, VkResult returnValue,
                                  format::HandleId device,
                                  StructPointerDecoder<Decoded_VkMemoryAllocateInfo>* pAllocateInfo,
                                  StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
                                  HandlePointerDecoder<VkDeviceMemory>* pMemory) override;

    void Process_vkGetImageMemoryRequirements(
        const ApiCallInfo& call_info, format::HandleId device, format::HandleId image,
        StructPointerDecoder<Decoded_VkMemoryRequirements>* pMemoryRequirements) override;

    void Process_vkImportFenceFdKHR(
        const ApiCallInfo& call_info, VkResult returnValue, format::HandleId device,
        StructPointerDecoder<Decoded_VkImportFenceFdInfoKHR>* pImportFenceFdInfo) override;

    void Process_vkCreateImage(const ApiCallInfo& call_info, VkResult returnValue,
                               format::HandleId device,
                               StructPointerDecoder<Decoded_VkImageCreateInfo>* pCreateInfo,
                               StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
                               HandlePointerDecoder<VkImage>* pImage) override;

    void Process_vkDestroyImage(
        const ApiCallInfo& call_info, format::HandleId device, format::HandleId image,
        StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator) override;

    void Process_vkCreateImageView(const ApiCallInfo& call_info, VkResult returnValue,
                                   format::HandleId device,
                                   StructPointerDecoder<Decoded_VkImageViewCreateInfo>* pCreateInfo,
                                   StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
                                   HandlePointerDecoder<VkImageView>* pView) override;

    void Process_vkDestroyImageView(
        const ApiCallInfo& call_info, format::HandleId device, format::HandleId imageView,
        StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator) override;

    void Process_vkCreateBuffer(const ApiCallInfo& call_info, VkResult returnValue,
                                format::HandleId device,
                                StructPointerDecoder<Decoded_VkBufferCreateInfo>* pCreateInfo,
                                StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
                                HandlePointerDecoder<VkBuffer>* pBuffer) override;

    void Process_vkDestroyBuffer(
        const ApiCallInfo& call_info, format::HandleId device, format::HandleId buffer,
        StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator) override;

    void Process_vkCreateBufferView(
        const ApiCallInfo& call_info, VkResult returnValue, format::HandleId device,
        StructPointerDecoder<Decoded_VkBufferViewCreateInfo>* pCreateInfo,
        StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
        HandlePointerDecoder<VkBufferView>* pView) override;

    void Process_vkDestroyBufferView(
        const ApiCallInfo& call_info, format::HandleId device, format::HandleId bufferView,
        StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator) override;

    void ProcessStateEndMarker(uint64_t frame_number) override;
    void ProcessFrameEndMarker(uint64_t frame_number) override;

    void ProcessCreateHardwareBufferCommand(
        format::HandleId device_id, format::HandleId memory_id, uint64_t buffer_id, uint32_t format,
        uint32_t width, uint32_t height, uint32_t stride, uint64_t usage, uint32_t layers,
        const std::vector<format::HardwareBufferPlaneInfo>& plane_info) override;

    void Process_vkCmdPipelineBarrier(
        const ApiCallInfo& call_info, format::HandleId commandBuffer,
        VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
        VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount,
        StructPointerDecoder<Decoded_VkMemoryBarrier>* pMemoryBarriers,
        uint32_t bufferMemoryBarrierCount,
        StructPointerDecoder<Decoded_VkBufferMemoryBarrier>* pBufferMemoryBarriers,
        uint32_t imageMemoryBarrierCount,
        StructPointerDecoder<Decoded_VkImageMemoryBarrier>* pImageMemoryBarriers) override;

    void SetEnableGPUTime(bool enable) { enable_gpu_time_ = enable; }

    std::string GetGPUTimeStatsCSVStr() const
    {
        return gpu_time_stats_csv_header_str_ + gpu_time_stats_csv_str_;
    }

 private:
    // Keeps the fences status after setup phase
    enum class FenceStatus
    {
        kUnsignaled,
        kSignaled
    };
    std::unordered_map<VkFence, FenceStatus> fence_initial_status_ = {};
    // This queue is only used for signaling the fences, could be any queue
    VkQueue fence_signal_queue_ = VK_NULL_HANDLE;
    // The deferred release list keeps resources that are created in the "setup phase"
    // Those resources should not be released in the mid of a frame since we may loop frame.
    // For trimmed captures, all resources that are not released within the frame are released by
    // FreeAllLiveObjects in VulkanReplayConsumerBase::~VulkanReplayConsumerBase()
    // So there is no need to manually release those resources
    std::vector<format::HandleId> deferred_release_list_ = {};
    // Fences that have been created with VkExportFenceCreateInfo.
    std::unordered_set<format::HandleId> exportable_fences_;
    // File descriptors made during the frame loop that should be closed at the end of the frame to
    // avoid leaking. The key is the FD stored in the capture file. The value is the FD made during
    // replay. We need to track both since vkImportFenceFdKHR uses the former.
    std::unordered_map<int, int> fds_to_close_at_frame_end_;
    // Track operations that should be performed at the end of a frame loop to prevent unbalanced
    // calls. E.g. a create without a destroy, a begin without an end. Based on how the application
    // manages rendering and how GFXR capture works, we might get captures that straddle frames.
    // This typically results in vkCreate calls that lack vkDestroy calls. For these cases, we can
    // track and free objects so that they are not leaked.
    std::unordered_map<format::HandleId, absl::AnyInvocable<void()>> frame_end_actions_;
    Dive::GPUTime gpu_time_ = {};
    std::string gpu_time_stats_csv_header_str_ = "Type,Id,Mean [ms],Median [ms]\n";
    std::string gpu_time_stats_csv_str_ = "";
    VkDevice device_ = VK_NULL_HANDLE;
    // Cache all vk function pointers
    PFN_vkResetQueryPool pfn_vkResetQueryPool_ = nullptr;
    PFN_vkQueueWaitIdle pfn_vkQueueWaitIdle_ = nullptr;
    PFN_vkDestroyQueryPool pfn_vkDestroyQueryPool_ = nullptr;
    PFN_vkDeviceWaitIdle pfn_vkDeviceWaitIdle_ = nullptr;
    PFN_vkGetQueryPoolResults pfn_vkGetQueryPoolResults_ = nullptr;
    PFN_vkCmdWriteTimestamp pfn_vkCmdWriteTimestamp_ = nullptr;
    PFN_vkGetFenceStatus pfn_vkGetFenceStatus_ = nullptr;
    PFN_vkQueueSubmit pfn_vkQueueSubmit_ = nullptr;
    PFN_vkResetFences pfn_vkResetFences_ = nullptr;
    PFN_vkGetFenceFdKHR pfn_vkGetFenceFdKHR_ = nullptr;
    bool enable_gpu_time_ = false;
    // This is a flag that indicates if the Setup Phase is finised or not for gfx Replay
    // The Setup Phase is done when StateEndMarker is triggered
    bool setup_finished_ = false;
    uint32_t api_version_ = 0;
};

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)

#endif  // GFXRECON_DECODE_VULKAN_REPLAY_CONSUMER_BASE_H
