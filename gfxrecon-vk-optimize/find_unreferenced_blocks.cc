#include "find_unreferenced_blocks.h"

#include <cstdint>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "aliases.h"
#include "decode/file_processor.h"
#include "generated/generated_vulkan_consumer.h"
#include "generated/generated_vulkan_decoder.h"
#include "gfxrecon-vk-optimize/span.h"
#include "process_file.h"
#include "span.h"
#include "util/defines.h"

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)
namespace
{

class UnreferencedBlockConsumer : public VulkanConsumer
{
 public:
    // Makes a copy of unreferenced_ids so we mutate it with additional handles that we find.
    // unreferenced_ids currently starts with only resources (image, buffer, framebuffer) and
    // command buffers.
    UnreferencedBlockConsumer(const HandleSet& unreferenced_ids,
                              const FileProcessor& file_processor,
                              BlockIndexSet& unreferenced_block_indices)
        : unreferenced_ids_(unreferenced_ids),
          file_processor_(&file_processor),
          unreferenced_block_indices_(&unreferenced_block_indices)

    {
    }

    void Process_vkAllocateMemory(const ApiCallInfo& call_info, VkResult returnValue,
                                  format::HandleId device,
                                  StructPointerDecoder<Decoded_VkMemoryAllocateInfo>* pAllocateInfo,
                                  StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
                                  HandlePointerDecoder<VkDeviceMemory>* pMemory) override
    {
        if (const auto* dedicated_memory =
                GetPNextMetaStruct<Decoded_VkMemoryDedicatedAllocateInfo>(
                    pAllocateInfo->GetMetaStructPointer()->pNext);
            dedicated_memory != nullptr)
        {
            if (unreferenced_ids_.contains(dedicated_memory->image))
            {
                GFXRECON_LOG_INFO(
                    "Ignoring vkAllocateMemory (%u) since pAllocateInfo->image %u is unreferenced",
                    call_info.index, dedicated_memory->image);
                unreferenced_block_indices_->insert(call_info.index);

                GFXRECON_LOG_INFO("Also marking memory %u as unreferenced", *pMemory->GetPointer());
                unreferenced_ids_.insert(*pMemory->GetPointer());
            }
        }
    }

    void Process_vkAllocateCommandBuffers(
        const ApiCallInfo& call_info, VkResult returnValue, format::HandleId device,
        StructPointerDecoder<Decoded_VkCommandBufferAllocateInfo>* pAllocateInfo,
        HandlePointerDecoder<VkCommandBuffer>* pCommandBuffers) override
    {
        // TODO: need to remove from pCommandBuffers if unused. I don't actually know how to do this
    }

    void Process_vkBeginCommandBuffer(
        const ApiCallInfo& call_info, VkResult returnValue, format::HandleId commandBuffer,
        StructPointerDecoder<Decoded_VkCommandBufferBeginInfo>* pBeginInfo) override
    {
        // For the sake of time. keep this for now so that we don't have to ignore every single
        // vkCmd*

        // if (unreferenced_ids_.contains(commandBuffer))
        // {
        //     GFXRECON_LOG_INFO(
        //         "Ignoring Process_vkBeginCommandBuffer (%u) since commandBuffer %u is
        //         unreferenced", call_info.index, commandBuffer);
        //     unreferenced_block_indices_->insert(call_info.index);
        // }
    }

    void Process_vkBindImageMemory(const ApiCallInfo& call_info, VkResult returnValue,
                                   format::HandleId device, format::HandleId image,
                                   format::HandleId memory, VkDeviceSize memoryOffset) override
    {
        if (unreferenced_ids_.contains(image))
        {
            GFXRECON_LOG_INFO(
                "Ignoring Process_vkBindImageMemory (%u) since image %u is unreferenced",
                call_info.index, image);
            unreferenced_block_indices_->insert(call_info.index);
            return;
        }
    }

    void Process_vkCmdBeginRenderPass2KHR(
        const ApiCallInfo& call_info, format::HandleId commandBuffer,
        StructPointerDecoder<Decoded_VkRenderPassBeginInfo>* pRenderPassBegin,
        StructPointerDecoder<Decoded_VkSubpassBeginInfo>* pSubpassBeginInfo) override
    {
        Decoded_VkRenderPassBeginInfo& decoded_begin_info =
            *pRenderPassBegin->GetMetaStructPointer();
        if (unreferenced_ids_.contains(commandBuffer) ||
            unreferenced_ids_.contains(decoded_begin_info.renderPass) ||
            unreferenced_ids_.contains(decoded_begin_info.framebuffer))
        {
            GFXRECON_LOG_INFO(
                "Ignoring Process_vkCmdBeginRenderPass2KHR (%u) a handle is unreferenced",
                call_info.index);
            unreferenced_block_indices_->insert(call_info.index);
            return;
        }
    }

    void Process_vkCmdCopyBufferToImage(
        const ApiCallInfo& call_info, format::HandleId commandBuffer, format::HandleId srcBuffer,
        format::HandleId dstImage, VkImageLayout dstImageLayout, uint32_t regionCount,
        StructPointerDecoder<Decoded_VkBufferImageCopy>* pRegions) override
    {
        if (unreferenced_ids_.contains(commandBuffer) || unreferenced_ids_.contains(srcBuffer) ||
            unreferenced_ids_.contains(dstImage))
        {
            GFXRECON_LOG_INFO(
                "Ignoring Process_vkCmdCopyBufferToImage (%u) a handle is unreferenced",
                call_info.index);
            unreferenced_block_indices_->insert(call_info.index);
            return;
        }
    }

    void Process_vkCmdDraw(const ApiCallInfo& call_info, format::HandleId commandBuffer,
                           uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
                           uint32_t firstInstance) override
    {
        if (unreferenced_ids_.contains(commandBuffer))
        {
            GFXRECON_LOG_INFO("Ignoring Process_vkCmdDraw (%u) a handle is unreferenced",
                              call_info.index);
            unreferenced_block_indices_->insert(call_info.index);
            return;
        }
    }

    void Process_vkCmdDrawIndexed(const ApiCallInfo& call_info, format::HandleId commandBuffer,
                                  uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
                                  int32_t vertexOffset, uint32_t firstInstance) override
    {
        if (unreferenced_ids_.contains(commandBuffer))
        {
            GFXRECON_LOG_INFO("Ignoring Process_vkCmdDrawIndexed (%u) a handle is unreferenced",
                              call_info.index);
            unreferenced_block_indices_->insert(call_info.index);
            return;
        }
    }

    void Process_vkCmdEndRenderPass2KHR(
        const ApiCallInfo& call_info, format::HandleId commandBuffer,
        StructPointerDecoder<Decoded_VkSubpassEndInfo>* pSubpassEndInfo) override
    {
        if (unreferenced_ids_.contains(commandBuffer))
        {
            GFXRECON_LOG_INFO(
                "Ignoring Process_vkCmdEndRenderPass2KHR (%u) a handle is unreferenced",
                call_info.index);
            unreferenced_block_indices_->insert(call_info.index);
            return;
        }
    }

    void Process_vkCmdPipelineBarrier(
        const ApiCallInfo& call_info, format::HandleId commandBuffer,
        VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
        VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount,
        StructPointerDecoder<Decoded_VkMemoryBarrier>* pMemoryBarriers,
        uint32_t bufferMemoryBarrierCount,
        StructPointerDecoder<Decoded_VkBufferMemoryBarrier>* pBufferMemoryBarriers,
        uint32_t imageMemoryBarrierCount,
        StructPointerDecoder<Decoded_VkImageMemoryBarrier>* pImageMemoryBarriers) override
    {
        if (unreferenced_ids_.contains(commandBuffer))
        {
            GFXRECON_LOG_INFO(
                "Ignoring Process_vkCmdPipelineBarrier (%u) since command buffer %u is "
                "unreferenced",
                call_info.index, commandBuffer);
            unreferenced_block_indices_->insert(call_info.index);
            return;
        }

        std::span<Decoded_VkImageMemoryBarrier> barriers(
            pImageMemoryBarriers->GetMetaStructPointer(), pImageMemoryBarriers->GetLength());
        for (const Decoded_VkImageMemoryBarrier& barrier : barriers)
        {
            if (unreferenced_ids_.contains(barrier.image))
            {
                GFXRECON_LOG_INFO(
                    "Ignoring Process_vkCmdPipelineBarrier (%u) since image %u is unreferenced",
                    call_info.index, barrier.image);
                unreferenced_block_indices_->insert(call_info.index);

                GFXRECON_LOG_INFO("Also marking command buffer %u as unreferenced", commandBuffer);
                unreferenced_ids_.insert(commandBuffer);
                return;
            }
        }
    }

    void Process_vkCreateImage(const ApiCallInfo& call_info, VkResult returnValue,
                               format::HandleId device,
                               StructPointerDecoder<Decoded_VkImageCreateInfo>* pCreateInfo,
                               StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
                               HandlePointerDecoder<VkImage>* pImage) override
    {
        format::HandleId image_handle = *pImage->GetPointer();
        if (unreferenced_ids_.contains(image_handle))
        {
            GFXRECON_LOG_INFO("Ignoring Process_vkCreateImage (%u) since image %u is unreferenced",
                              call_info.index, image_handle);
            unreferenced_block_indices_->insert(call_info.index);
        }
    }

    void Process_vkCreateImageView(const ApiCallInfo& call_info, VkResult returnValue,
                                   format::HandleId device,
                                   StructPointerDecoder<Decoded_VkImageViewCreateInfo>* pCreateInfo,
                                   StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
                                   HandlePointerDecoder<VkImageView>* pView) override
    {
        format::HandleId image_handle = pCreateInfo->GetMetaStructPointer()->image;
        format::HandleId image_view_handle = *pView->GetPointer();
        if (!unreferenced_ids_.contains(image_handle))
        {
            return;
        }

        GFXRECON_LOG_INFO("Ignoring Process_vkCreateImageView (%u) since a handle is unreferenced",
                          call_info.index);
        unreferenced_block_indices_->insert(call_info.index);

        // For some reason, we're not given unreferenced VkImageViews
        GFXRECON_LOG_INFO("Also marking iamge view %u as unreferenced", image_view_handle);
        unreferenced_ids_.insert(image_view_handle);
    }

    void Process_vkCreateFramebuffer(
        const ApiCallInfo& call_info, VkResult returnValue, format::HandleId device,
        StructPointerDecoder<Decoded_VkFramebufferCreateInfo>* pCreateInfo,
        StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
        HandlePointerDecoder<VkFramebuffer>* pFramebuffer) override
    {
        format::HandleId framebuffer_handle = *pFramebuffer->GetPointer();
        if (unreferenced_ids_.contains(framebuffer_handle))
        {
            GFXRECON_LOG_INFO(
                "Ignoring Process_vkCreateFramebuffer (%u) since framebuffer %u is unreferenced",
                call_info.index, framebuffer_handle);
            unreferenced_block_indices_->insert(call_info.index);
            return;
        }

        std::span<const format::HandleId> attachments(
            pCreateInfo->GetMetaStructPointer()->pAttachments.GetPointer(),
            pCreateInfo->GetMetaStructPointer()->pAttachments.GetLength());
        for (const format::HandleId attachment : attachments)
        {
            if (!unreferenced_ids_.contains(attachment))
            {
                continue;
            }
            GFXRECON_LOG_INFO(
                "Ignoring Process_vkCreateFramebuffer (%u) since framebuffer %u uses an "
                "unreferenced attachment %u",
                call_info.index, framebuffer_handle, attachment);
            unreferenced_block_indices_->insert(call_info.index);

            GFXRECON_LOG_INFO("Also marking framebuffer %u as unreferenced", framebuffer_handle);
            unreferenced_ids_.insert(framebuffer_handle);
            return;
        }
    }

    void Process_vkDestroyImage(
        const ApiCallInfo& call_info, format::HandleId device, format::HandleId image,
        StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator) override
    {
        if (!unreferenced_ids_.contains(image))
        {
            return;
        }
        GFXRECON_LOG_INFO("Ignoring Process_vkDestroyImage (%u) since image %u is unreferenced",
                          call_info.index, image);
        unreferenced_block_indices_->insert(call_info.index);
    }

    void Process_vkDestroyImageView(
        const ApiCallInfo& call_info, format::HandleId device, format::HandleId imageView,
        StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator) override
    {
        if (!unreferenced_ids_.contains(imageView))
        {
            return;
        }
        GFXRECON_LOG_INFO(
            "Ignoring Process_vkDestroyImageView (%u) since image view %u is unreferenced",
            call_info.index, imageView);
        unreferenced_block_indices_->insert(call_info.index);
    }

    void Process_vkEndCommandBuffer(const ApiCallInfo& call_info, VkResult returnValue,
                                    format::HandleId commandBuffer) override
    {
        // For the sake of time, allow begin/end so we don't have to ignore every single vkCmd

        // if (unreferenced_ids_.contains(commandBuffer))
        // {
        //     GFXRECON_LOG_INFO(
        //         "Ignoring Process_vkEndCommandBuffer (%u) since commandBuffer %u is
        //         unreferenced", call_info.index, commandBuffer);
        //     unreferenced_block_indices_->insert(call_info.index);
        // }
    }

    void Process_vkGetImageMemoryRequirements(
        const ApiCallInfo& call_info, format::HandleId device, format::HandleId image,
        StructPointerDecoder<Decoded_VkMemoryRequirements>* pMemoryRequirements) override
    {
        if (unreferenced_ids_.contains(image))
        {
            GFXRECON_LOG_INFO(
                "Ignoring Process_vkGetImageMemoryRequirements (%u) since image %u is unreferenced",
                call_info.index, image);
            unreferenced_block_indices_->insert(call_info.index);
            return;
        }
    }

    void Process_vkSetDebugUtilsObjectNameEXT(
        const ApiCallInfo& call_info, VkResult returnValue, format::HandleId device,
        StructPointerDecoder<Decoded_VkDebugUtilsObjectNameInfoEXT>* pNameInfo) override
    {
        HandleId id = pNameInfo->GetMetaStructPointer()->objectHandle;
        if (unreferenced_ids_.contains(id))
        {
            GFXRECON_LOG_INFO(
                "Ignoring Process_vkSetDebugUtilsObjectNameEXT (%u) since object %u is "
                "unreferenced",
                call_info.index, id);
            unreferenced_block_indices_->insert(call_info.index);
        }
    }

    void Process_vkUpdateDescriptorSets(
        const ApiCallInfo& call_info, format::HandleId device, uint32_t descriptorWriteCount,
        StructPointerDecoder<Decoded_VkWriteDescriptorSet>* pDescriptorWrites,
        uint32_t descriptorCopyCount,
        StructPointerDecoder<Decoded_VkCopyDescriptorSet>* pDescriptorCopies) override
    {
        // TODO: Will this work for descriptorWriteCount > 1? Need to modify the call in that
        // case
        for (const Decoded_VkWriteDescriptorSet& descriptor_write :
             MakeMetaStructSpan(*pDescriptorWrites))
        {
            if (descriptor_write.decoded_value->descriptorType !=
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
            {
                continue;
            }

            for (const Decoded_VkDescriptorImageInfo& image_info :
                 MakeMetaStructSpan(*descriptor_write.pImageInfo))
            {
                HandleId image_view = image_info.imageView;
                if (!unreferenced_ids_.contains(image_view))
                {
                    continue;
                }

                GFXRECON_LOG_INFO(
                    "Ignoring Process_vkUpdateDescriptorSets (%u) since object %u is "
                    "unreferenced",
                    call_info.index, image_view);
                unreferenced_block_indices_->insert(call_info.index);
            }
        }
    }

    void ProcessCreateHardwareBufferCommand(
        format::HandleId device_id, format::HandleId memory_id, uint64_t buffer_id, uint32_t format,
        uint32_t width, uint32_t height, uint32_t stride, uint64_t usage, uint32_t layers,
        const std::vector<format::HardwareBufferPlaneInfo>& plane_info) override
    {
        if (unreferenced_ids_.contains(buffer_id))
        {
            GFXRECON_LOG_INFO(
                "Ignoring ProcessCreateHardwareBufferCommand (%u) since object %u is "
                "unreferenced",
                file_processor_->GetCurrentBlockIndex(), buffer_id);
            unreferenced_block_indices_->insert(file_processor_->GetCurrentBlockIndex());
            unreferenced_ids_.insert(memory_id);
        }
    }

    void ProcessFillMemoryCommand(uint64_t memory_id, uint64_t offset, uint64_t size,
                                  const uint8_t* data) override
    {
        if (unreferenced_ids_.contains(memory_id))
        {
            GFXRECON_LOG_INFO(
                "Ignoring ProcessFillMemoryCommand (%u) since object %u is "
                "unreferenced",
                file_processor_->GetCurrentBlockIndex(), memory_id);
            unreferenced_block_indices_->insert(file_processor_->GetCurrentBlockIndex());
        }
    }

    void ProcessInitImageCommand(format::HandleId device_id, format::HandleId image_id,
                                 uint64_t data_size, uint32_t aspect, uint32_t layout,
                                 const std::vector<uint64_t>& level_sizes,
                                 const uint8_t* data) override
    {
        if (!unreferenced_ids_.contains(image_id))
        {
            return;
        }

        GFXRECON_LOG_INFO(
            "Ignoring ProcessInitImageCommand (%u) since object %u is "
            "unreferenced",
            file_processor_->GetCurrentBlockIndex(), image_id);
        unreferenced_block_indices_->insert(file_processor_->GetCurrentBlockIndex());
    }

 private:
    HandleSet unreferenced_ids_;
    // Needed to get block index for meta commands
    const FileProcessor* file_processor_ = nullptr;
    BlockIndexSet* unreferenced_block_indices_ = nullptr;
};
}  // namespace

absl::StatusOr<BlockIndexSet> FindUnreferencedBlocks(const std::filesystem::path& capture_file,
                                                     const HandleSet& unreferenced_ids)
{
    FileProcessor file_processor;
    if (!file_processor.Initialize(capture_file))
    {
        GFXRECON_LOG_INFO("Failed to open capture file: %s", capture_file.string().c_str());
        return absl::UnknownError("Failed to open capture file");
    }

    VulkanDecoder decoder;
    file_processor.AddDecoder(&decoder);

    BlockIndexSet unreferenced_block_indices;
    UnreferencedBlockConsumer consumer(unreferenced_ids, file_processor,
                                       unreferenced_block_indices);
    decoder.AddConsumer(&consumer);

    if (!file_processor.ProcessAllFrames())
    {
        GFXRECON_LOG_INFO("A failure has occurred during file processing: %s",
                          capture_file.string().c_str());
        return absl::UnknownError("A failure has occurred during file processing");
    }

    return unreferenced_block_indices;
}

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)
