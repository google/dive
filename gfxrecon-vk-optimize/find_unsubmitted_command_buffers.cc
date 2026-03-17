#include "find_unsubmitted_command_buffers.h"

#include "aliases.h"
#include "generated/generated_vulkan_consumer.h"
#include "process_file.h"
#include "span.h"

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)
namespace
{
class FindUnsubmittedCommandBuffersConsumer : public VulkanConsumer
{
 public:
    void Process_vkAllocateCommandBuffers(
        const ApiCallInfo& call_info, VkResult returnValue, HandleId device,
        StructPointerDecoder<Decoded_VkCommandBufferAllocateInfo>* pAllocateInfo,
        HandlePointerDecoder<VkCommandBuffer>* pCommandBuffers) override
    {
        for (HandleId id : MakeHandleSpan(*pCommandBuffers))
        {
            allocated_command_buffers_.insert(id);
        }
    }

    void Process_vkQueueSubmit(const ApiCallInfo& call_info, VkResult returnValue, HandleId queue,
                               uint32_t submitCount,
                               StructPointerDecoder<Decoded_VkSubmitInfo>* pSubmits,
                               HandleId fence) override
    {
        for (const Decoded_VkSubmitInfo& submit_info : MakeMetaStructSpan(*pSubmits))
        {
            for (const HandleId id : MakeHandleSpan(submit_info.pCommandBuffers))
            {
                submitted_command_buffers_.insert(id);
            }
        }
    }

    HandleSet CalculateUnsubmittedCommandBuffers()
    {
        // TODO: std::set_difference? requires being sorted
        HandleSet unsubmitted_command_buffers;
        for (HandleId allocated : allocated_command_buffers_)
        {
            if (!submitted_command_buffers_.contains(allocated))
            {
                unsubmitted_command_buffers.insert(allocated);
            }
        }
        return unsubmitted_command_buffers;
    }

 private:
    HandleSet allocated_command_buffers_;
    HandleSet submitted_command_buffers_;
};
}  // namespace

absl::StatusOr<HandleSet> FindUnsubmittedCommandBuffers(const std::filesystem::path& capture_file)
{
    FindUnsubmittedCommandBuffersConsumer consumer;
    if (!ProcessFile(capture_file, consumer))
    {
        return absl::UnknownError("A failure has occurred during file processing");
    }

    return consumer.CalculateUnsubmittedCommandBuffers();
}

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)
