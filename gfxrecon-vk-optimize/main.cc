#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_set>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_format.h"
#include "decode/file_processor.h"
#include "dive_core/capture_data.h"
#include "dive_core/gfxr_capture_data.h"
#include "format/format.h"
#include "generated/generated_vulkan_decoder.h"
#include "generated/generated_vulkan_referenced_resource_consumer.h"
#include "generated/generated_vulkan_struct_decoders.h"
#include "gfxr_ext/decode/dive_block_data.h"
#include "util/logging.h"

// From xlib, likely included by a GFXR header
#undef Status

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)
namespace
{

using HandleSet = std::unordered_set<format::HandleId>;
using BlockIndexSet = std::unordered_set<uint64_t>;

class UnreferencedBlockConsumer : public VulkanConsumer
{
 public:
    UnreferencedBlockConsumer(const HandleSet& unreferenced_ids,
                              BlockIndexSet& unreferenced_block_indices)
        : unreferenced_ids_(&unreferenced_ids),
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
            if (IsUnreferenced(dedicated_memory->image))
            {
                GFXRECON_LOG_INFO(
                    "Ignoring vkAllocateMemory (%u) since pAllocateInfo->image %u is unreferenced",
                    call_info.index, dedicated_memory->image);
                unreferenced_block_indices_->insert(call_info.index);

                // TODO this should be part of the other consumer
                GFXRECON_LOG_INFO("Also marking memory %u as unreferenced", *pMemory->GetPointer());
                newly_unreferenced_ids_.insert(*pMemory->GetPointer());
            }
        }
    }

    void Process_vkBindImageMemory(const ApiCallInfo& call_info, VkResult returnValue,
                                   format::HandleId device, format::HandleId image,
                                   format::HandleId memory, VkDeviceSize memoryOffset) override
    {
        if (IsUnreferenced(image))
        {
            GFXRECON_LOG_INFO(
                "Ignoring Process_vkBindImageMemory (%u) since image %u is unreferenced",
                call_info.index, image);
            unreferenced_block_indices_->insert(call_info.index);
        }
    }

    void Process_vkCreateImage(const ApiCallInfo& call_info, VkResult returnValue,
                               format::HandleId device,
                               StructPointerDecoder<Decoded_VkImageCreateInfo>* pCreateInfo,
                               StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
                               HandlePointerDecoder<VkImage>* pImage) override
    {
        format::HandleId image_handle = *pImage->GetPointer();
        if (IsUnreferenced(image_handle))
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
        if (IsUnreferenced(image_handle))
        {
            GFXRECON_LOG_INFO(
                "Ignoring Process_vkCreateImageView (%u) since image %u is unreferenced",
                call_info.index, image_handle);
            unreferenced_block_indices_->insert(call_info.index);

            // TODO this should be part of the other consumer
            GFXRECON_LOG_INFO("Also marking image view %u as unreferenced", image_view_handle);
            newly_unreferenced_ids_.insert(image_view_handle);
        }
    }

    void Process_vkGetImageMemoryRequirements(
        const ApiCallInfo& call_info, format::HandleId device, format::HandleId image,
        StructPointerDecoder<Decoded_VkMemoryRequirements>* pMemoryRequirements) override
    {
        if (IsUnreferenced(image))
        {
            GFXRECON_LOG_INFO(
                "Ignoring Process_vkGetImageMemoryRequirements (%u) since image %u is unreferenced",
                call_info.index, image);
            unreferenced_block_indices_->insert(call_info.index);
        }
    }

    void Process_vkCreateFramebuffer(
        const ApiCallInfo& call_info, VkResult returnValue, format::HandleId device,
        StructPointerDecoder<Decoded_VkFramebufferCreateInfo>* pCreateInfo,
        StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
        HandlePointerDecoder<VkFramebuffer>* pFramebuffer) override
    {
        format::HandleId framebuffer_handle = *pFramebuffer->GetPointer();
        if (IsUnreferenced(framebuffer_handle))
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
            if (IsUnreferenced(attachment))
            {
                GFXRECON_LOG_INFO(
                    "Ignoring Process_vkCreateFramebuffer (%u) since framebuffer %u uses an "
                    "unreferenced attachment %u",
                    call_info.index, framebuffer_handle, attachment);
                unreferenced_block_indices_->insert(call_info.index);

                // TODO this should be part of the other consumer
                GFXRECON_LOG_INFO("Also marking framebuffer %u as unreferenced",
                                  framebuffer_handle);
                newly_unreferenced_ids_.insert(framebuffer_handle);
                return;
            }
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
        if (IsUnreferenced(commandBuffer))
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
            if (IsUnreferenced(barrier.image))
            {
                GFXRECON_LOG_INFO(
                    "Ignoring Process_vkCmdPipelineBarrier (%u) since image %u is unreferenced",
                    call_info.index, barrier.image);
                unreferenced_block_indices_->insert(call_info.index);

                GFXRECON_LOG_INFO("Also marking command buffer %u as unreferenced", commandBuffer);
                newly_unreferenced_ids_.insert(commandBuffer);
                return;
            }
        }
    }

    void Process_vkCmdCopyBufferToImage(
        const ApiCallInfo& call_info, format::HandleId commandBuffer, format::HandleId srcBuffer,
        format::HandleId dstImage, VkImageLayout dstImageLayout, uint32_t regionCount,
        StructPointerDecoder<Decoded_VkBufferImageCopy>* pRegions) override
    {
        if (IsUnreferenced(commandBuffer) || IsUnreferenced(srcBuffer) || IsUnreferenced(dstImage))
        {
            GFXRECON_LOG_INFO(
                "Ignoring Process_vkCmdCopyBufferToImage (%u) a handle is unreferenced",
                call_info.index);
            unreferenced_block_indices_->insert(call_info.index);
            return;
        }
    }

 private:
    bool IsUnreferenced(format::HandleId handle_id) const
    {
        return unreferenced_ids_->contains(handle_id) ||
               newly_unreferenced_ids_.contains(handle_id);
    }

    const HandleSet* unreferenced_ids_ = nullptr;
    BlockIndexSet* unreferenced_block_indices_ = nullptr;
    HandleSet newly_unreferenced_ids_;
};

absl::StatusOr<HandleSet> FindUnreferencedResources(const std::filesystem::path& capture_file)
{
    FileProcessor file_processor;
    if (!file_processor.Initialize(capture_file))
    {
        return absl::UnknownError(absl::StrFormat("Failed to open capture file: %s", capture_file));
    }

    VulkanDecoder decoder;
    file_processor.AddDecoder(&decoder);

    VulkanReferencedResourceConsumer resref_consumer;
    decoder.AddConsumer(&resref_consumer);

    file_processor.ProcessAllFrames();

    if (resref_consumer.WasNotOptimizable())
    {
        return absl::UnknownError(
            "File did not contain trim state setup - no optimization was performed");
    }

    if ((file_processor.GetCurrentFrameNumber() > 0) &&
        (file_processor.GetErrorState() == gfxrecon::decode::BlockIOError::kErrorNone))
    {
        // Get the list of resources that were included in a command buffer submission during
        // replay.
        HandleSet unreferenced_ids;
        resref_consumer.GetReferencedResourceIds(nullptr, &unreferenced_ids);
        return unreferenced_ids;
    }

    if (file_processor.GetErrorState() != gfxrecon::decode::BlockIOError::kErrorNone)
    {
        return absl::UnknownError("A failure has occurred during file processing");
    }

    return absl::UnknownError("File did not contain any frames");
}

absl::StatusOr<BlockIndexSet> FindUnreferencedBlocks(const std::filesystem::path& capture_file,
                                                     const HandleSet& unreferenced_ids)
{
    FileProcessor file_processor;
    if (!file_processor.Initialize(capture_file))
    {
        return absl::UnknownError(absl::StrFormat("Failed to open capture file: %s", capture_file));
    }

    VulkanDecoder decoder;
    file_processor.AddDecoder(&decoder);

    BlockIndexSet unreferenced_block_indices;
    UnreferencedBlockConsumer consumer(unreferenced_ids, unreferenced_block_indices);
    decoder.AddConsumer(&consumer);

    file_processor.ProcessAllFrames();

    return unreferenced_block_indices;
}

absl::Status RemoveUnreferencedResources(const std::filesystem::path& capture_file,
                                         const std::filesystem::path& output_file)
{
    GFXRECON_WRITE_CONSOLE("Scanning Vulkan file %s for unreferenced resources.",
                           capture_file.c_str());
    absl::StatusOr<HandleSet> unreferenced_ids = FindUnreferencedResources(capture_file);
    if (!unreferenced_ids.ok())
    {
        return unreferenced_ids.status();
    }

    if (unreferenced_ids->empty())
    {
        return absl::UnknownError("No unused resources detected.  A new file will not be created.");
    }

    GFXRECON_WRITE_CONSOLE("Scanning for unused blocks.");
    absl::StatusOr<BlockIndexSet> unreferenced_block_indices =
        FindUnreferencedBlocks(capture_file, *unreferenced_ids);
    if (!unreferenced_block_indices.ok())
    {
        return unreferenced_block_indices.status();
    }

    if (unreferenced_block_indices->empty())
    {
        return absl::UnknownError("No unused blocks detected.");
    }

    // TODO this could be optimized by adding DiveBlockData to any other use of FileProcessor
    Dive::GfxrCaptureData gfxr_capture_data;
    if (gfxr_capture_data.LoadCaptureFile(capture_file) != Dive::CaptureData::LoadResult::kSuccess)
    {
        return absl::UnknownError("LoadCaptureFile failed");
    }
    std::shared_ptr<gfxrecon::decode::DiveBlockData> block_data =
        gfxr_capture_data.GetMutableGfxrData();

    for (uint64_t block_index : *unreferenced_block_indices)
    {
        block_data->AddModification(block_index, 0, nullptr);
    }

    if (!gfxr_capture_data.WriteModifiedGfxrFile(output_file.c_str()))
    {
        return absl::UnknownError("WriteModifiedGfxrFile failed");
    }

    return absl::OkStatus();
}

}  // namespace
GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)

int main(int argc, char** argv)
{
    absl::SetProgramUsageMessage("Usage: gfxrecon-vk-optimizer FILE.GFXR OUT.GFXR");
    std::vector<char*> positional_args = absl::ParseCommandLine(argc, argv);
    if (positional_args.size() != 3)
    {
        std::cerr << absl::ProgramUsageMessage() << '\n';
        return 1;
    }

    gfxrecon::util::Log::Init(gfxrecon::util::Log::kDebugSeverity);
    if (absl::Status status =
            gfxrecon::decode::RemoveUnreferencedResources(positional_args[1], positional_args[2]);
        !status.ok())
    {
        GFXRECON_LOG_ERROR("%s", std::string(status.message()).c_str());
        return 1;
    }

    return 0;
}
