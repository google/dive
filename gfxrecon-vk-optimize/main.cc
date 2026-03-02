#include <filesystem>
#include <iostream>
#include <iterator>
#include <memory>
#include <string_view>
#include <unordered_set>

#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "aliases.h"
#include "dive_core/capture_data.h"
#include "dive_core/gfxr_capture_data.h"
#include "find_unreferenced_blocks.h"
#include "find_unsubmitted_command_buffers.h"
#include "generated/generated_vulkan_consumer.h"
#include "generated/generated_vulkan_referenced_resource_consumer.h"
#include "generated/generated_vulkan_struct_decoders_forward.h"
#include "gfxr_ext/decode/dive_block_data.h"
#include "process_file.h"
#include "span.h"
#include "util/defines.h"
#include "util/logging.h"

// From xlib, likely included by a GFXR header
#undef Status

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)
namespace
{

class FindUnreferencedAndroidHardwareBuffersConsumer : public VulkanConsumer
{
 public:
    explicit FindUnreferencedAndroidHardwareBuffersConsumer(const HandleSet& unreferenced_ids)
        : unreferenced_ids_(&unreferenced_ids)
    {
    }

    void Process_vkAllocateMemory(const ApiCallInfo& call_info, VkResult returnValue,
                                  format::HandleId device,
                                  StructPointerDecoder<Decoded_VkMemoryAllocateInfo>* pAllocateInfo,
                                  StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
                                  HandlePointerDecoder<VkDeviceMemory>* pMemory) override
    {
        const auto* dedicated_memory = GetPNextMetaStruct<Decoded_VkMemoryDedicatedAllocateInfo>(
            pAllocateInfo->GetMetaStructPointer()->pNext);
        const auto* import_hardware_buffer =
            GetPNextMetaStruct<Decoded_VkImportAndroidHardwareBufferInfoANDROID>(
                pAllocateInfo->GetMetaStructPointer()->pNext);
        if (dedicated_memory && import_hardware_buffer &&
            unreferenced_ids_->contains(dedicated_memory->image))
        {
            GFXRECON_LOG_INFO(
                "Ignoring AHardwareBuffer (%u) since pAllocateInfo->image %u is unreferenced",
                import_hardware_buffer->buffer, dedicated_memory->image);
            unreferenced_hardware_buffers_.insert(import_hardware_buffer->buffer);
        }
    }

    const HandleSet& GetUnreferencedAndroidHardwareBuffers() const
    {
        return unreferenced_hardware_buffers_;
    }

 private:
    const HandleSet* unreferenced_ids_;
    HandleSet unreferenced_hardware_buffers_;
};

// Resources include: VkBuffer, VkBufferView, VkImage, VkImageView, VkFramebuffer
absl::StatusOr<HandleSet> FindUnreferencedResources(const std::filesystem::path& capture_file)
{
    // Reuse existing GFXR class
    VulkanReferencedResourceConsumer consumer;
    if (!ProcessFile(capture_file, consumer))
    {
        return absl::UnknownError("A failure has occurred during file processing");
    }

    if (consumer.WasNotOptimizable())
    {
        return absl::UnknownError(
            "File did not contain trim state setup - no optimization was performed");
    }

    HandleSet unreferenced_ids;
    consumer.GetReferencedResourceIds(nullptr, &unreferenced_ids);
    return unreferenced_ids;
}

absl::StatusOr<HandleSet> FindUnreferencedAndroidHardwareBuffers(
    const std::filesystem::path& capture_file, const HandleSet& unreferenced_ids)
{
    // Reuse existing GFXR class
    FindUnreferencedAndroidHardwareBuffersConsumer consumer(unreferenced_ids);
    if (!ProcessFile(capture_file, consumer))
    {
        return absl::UnknownError("A failure has occurred during file processing");
    }

    return consumer.GetUnreferencedAndroidHardwareBuffers();
}

absl::StatusOr<HandleSet> FindUnreferencedIds(const std::filesystem::path& capture_file)
{
    GFXRECON_WRITE_CONSOLE("Scanning for unreferenced resources.");
    absl::StatusOr<HandleSet> unreferenced_resource_ids = FindUnreferencedResources(capture_file);
    if (!unreferenced_resource_ids.ok())
    {
        return unreferenced_resource_ids.status();
    }

    if (unreferenced_resource_ids->empty())
    {
        // This is fatal since why are you running the program?
        return absl::UnknownError("No unused resources detected.");
    }

    // FindUnreferencedResources essentially only finds VkImage. To find everything, need to do
    // additional passes. Some of these could be combined into one pass to optimize.

    GFXRECON_WRITE_CONSOLE("Searching for unsubmitted command buffers", capture_file.c_str());
    absl::StatusOr<HandleSet> unsubmitted_command_buffers =
        FindUnsubmittedCommandBuffers(capture_file);
    if (!unsubmitted_command_buffers.ok())
    {
        return unsubmitted_command_buffers.status();
    }

    if (unsubmitted_command_buffers->empty())
    {
        // While one may legitimately want to replay something without command buffer submissions
        // (to test resource creation ordering, etc), what we largely care about is rendered
        // application output.
        return absl::UnknownError("No command buffers submitted.");
    }

    GFXRECON_WRITE_CONSOLE("Scanning for unreferenced AHardwareBuffer.");
    absl::StatusOr<HandleSet> unreferenced_hardware_buffer =
        FindUnreferencedAndroidHardwareBuffers(capture_file, *unreferenced_resource_ids);
    if (!unreferenced_hardware_buffer.ok())
    {
        return unreferenced_hardware_buffer.status();
    }

    HandleSet unreferenced_ids;
    std::copy(unsubmitted_command_buffers->begin(), unsubmitted_command_buffers->end(),
              std::inserter(unreferenced_ids, unreferenced_ids.end()));
    std::copy(unreferenced_resource_ids->begin(), unreferenced_resource_ids->end(),
              std::inserter(unreferenced_ids, unreferenced_ids.end()));
    std::copy(unreferenced_hardware_buffer->begin(), unreferenced_hardware_buffer->end(),
              std::inserter(unreferenced_ids, unreferenced_ids.end()));

    return unreferenced_ids;
}

absl::Status RemoveUnreferencedIds(const std::filesystem::path& capture_file,
                                   const std::filesystem::path& output_file)
{
    GFXRECON_WRITE_CONSOLE("Scanning for unreferenced resources.");
    absl::StatusOr<HandleSet> unreferenced_ids = FindUnreferencedIds(capture_file);
    if (!unreferenced_ids.ok())
    {
        return unreferenced_ids.status();
    }

    if (unreferenced_ids->empty())
    {
        return absl::UnknownError("No unused resources detected.");
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
            gfxrecon::decode::RemoveUnreferencedIds(positional_args[1], positional_args[2]);
        !status.ok())
    {
        GFXRECON_LOG_ERROR("%s", std::string(status.message()).c_str());
        return 1;
    }

    return 0;
}
