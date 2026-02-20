#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "decode/file_processor.h"
#include "decode/vulkan_pnext_node.h"
#include "format/format.h"
#include "generated/generated_vulkan_consumer.h"
#include "generated/generated_vulkan_decoder.h"
#include "generated/generated_vulkan_enum_to_string.h"
#include "generated/generated_vulkan_struct_decoders_forward.h"
#include "util/logging.h"

// From xlib, likely included by a GFXR header
#undef Status

ABSL_FLAG(std::optional<uint64_t>, block_index, std::nullopt,
          "Only write the graph component that is affected by block_index. If not specified, all "
          "components are written");

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)
namespace
{

struct Node
{
    std::string id;    // internal identifier, e.g. HandleId
    std::string name;  // human readable identifier, e.g. struct name
    std::unordered_map<std::string, std::string> properties;
    std::unordered_set<Node*> children;
    std::unordered_set<Node*> parents;  // Empty if root
};

// from gfxreconstruct/test/android_mock/android/hardware_buffer.h
enum AHardwareBuffer_Format
{
    AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM = 1,
    AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM = 2,
    AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM = 3,
    AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM = 4,
    AHARDWAREBUFFER_FORMAT_R16G16B16A16_FLOAT = 0x16,
    AHARDWAREBUFFER_FORMAT_R10G10B10A2_UNORM = 0x2b,
    AHARDWAREBUFFER_FORMAT_BLOB = 0x21,
    AHARDWAREBUFFER_FORMAT_D16_UNORM = 0x30,
    AHARDWAREBUFFER_FORMAT_D24_UNORM = 0x31,
    AHARDWAREBUFFER_FORMAT_D24_UNORM_S8_UINT = 0x32,
    AHARDWAREBUFFER_FORMAT_D32_FLOAT = 0x33,
    AHARDWAREBUFFER_FORMAT_D32_FLOAT_S8_UINT = 0x34,
    AHARDWAREBUFFER_FORMAT_S8_UINT = 0x35,
    AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420 = 0x23,
    AHARDWAREBUFFER_FORMAT_YCbCr_P010 = 0x36,
    AHARDWAREBUFFER_FORMAT_R8_UNORM = 0x38,
};

enum AHardwareBuffer_UsageFlags
{
    AHARDWAREBUFFER_USAGE_CPU_READ_NEVER = 0UL,
    AHARDWAREBUFFER_USAGE_CPU_READ_RARELY = 2UL,
    AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN = 3UL,
    AHARDWAREBUFFER_USAGE_CPU_READ_MASK = 0xFUL,
    AHARDWAREBUFFER_USAGE_CPU_WRITE_NEVER = 0UL << 4,
    AHARDWAREBUFFER_USAGE_CPU_WRITE_RARELY = 2UL << 4,
    AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN = 3UL << 4,
    AHARDWAREBUFFER_USAGE_CPU_WRITE_MASK = 0xFUL << 4,
    AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE = 1UL << 8,
    AHARDWAREBUFFER_USAGE_GPU_FRAMEBUFFER = 1UL << 9,
    AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT = AHARDWAREBUFFER_USAGE_GPU_FRAMEBUFFER,
    AHARDWAREBUFFER_USAGE_COMPOSER_OVERLAY = 1ULL << 11,
    AHARDWAREBUFFER_USAGE_PROTECTED_CONTENT = 1UL << 14,
    AHARDWAREBUFFER_USAGE_VIDEO_ENCODE = 1UL << 16,
    AHARDWAREBUFFER_USAGE_SENSOR_DIRECT_DATA = 1UL << 23,
    AHARDWAREBUFFER_USAGE_GPU_DATA_BUFFER = 1UL << 24,
    AHARDWAREBUFFER_USAGE_GPU_CUBE_MAP = 1UL << 25,
    AHARDWAREBUFFER_USAGE_GPU_MIPMAP_COMPLETE = 1UL << 26,
    AHARDWAREBUFFER_USAGE_FRONT_BUFFER = 1ULL << 32,
    AHARDWAREBUFFER_USAGE_VENDOR_0 = 1ULL << 28,
    AHARDWAREBUFFER_USAGE_VENDOR_1 = 1ULL << 29,
    AHARDWAREBUFFER_USAGE_VENDOR_2 = 1ULL << 30,
    AHARDWAREBUFFER_USAGE_VENDOR_3 = 1ULL << 31,
    AHARDWAREBUFFER_USAGE_VENDOR_4 = 1ULL << 48,
    AHARDWAREBUFFER_USAGE_VENDOR_5 = 1ULL << 49,
    AHARDWAREBUFFER_USAGE_VENDOR_6 = 1ULL << 50,
    AHARDWAREBUFFER_USAGE_VENDOR_7 = 1ULL << 51,
    AHARDWAREBUFFER_USAGE_VENDOR_8 = 1ULL << 52,
    AHARDWAREBUFFER_USAGE_VENDOR_9 = 1ULL << 53,
    AHARDWAREBUFFER_USAGE_VENDOR_10 = 1ULL << 54,
    AHARDWAREBUFFER_USAGE_VENDOR_11 = 1ULL << 55,
    AHARDWAREBUFFER_USAGE_VENDOR_12 = 1ULL << 56,
    AHARDWAREBUFFER_USAGE_VENDOR_13 = 1ULL << 57,
    AHARDWAREBUFFER_USAGE_VENDOR_14 = 1ULL << 58,
    AHARDWAREBUFFER_USAGE_VENDOR_15 = 1ULL << 59,
    AHARDWAREBUFFER_USAGE_VENDOR_16 = 1ULL << 60,
    AHARDWAREBUFFER_USAGE_VENDOR_17 = 1ULL << 61,
    AHARDWAREBUFFER_USAGE_VENDOR_18 = 1ULL << 62,
    AHARDWAREBUFFER_USAGE_VENDOR_19 = 1ULL << 63,
};

std::string_view ToString(AHardwareBuffer_Format format)
{
    switch (format)
    {
        case AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM:
            return "AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM";
        case AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM:
            return "AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM";
        case AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM:
            return "AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM";
        case AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM:
            return "AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM";
        case AHARDWAREBUFFER_FORMAT_R16G16B16A16_FLOAT:
            return "AHARDWAREBUFFER_FORMAT_R16G16B16A16_FLOAT";
        case AHARDWAREBUFFER_FORMAT_R10G10B10A2_UNORM:
            return "AHARDWAREBUFFER_FORMAT_R10G10B10A2_UNORM";
        case AHARDWAREBUFFER_FORMAT_BLOB:
            return "AHARDWAREBUFFER_FORMAT_BLOB";
        case AHARDWAREBUFFER_FORMAT_D16_UNORM:
            return "AHARDWAREBUFFER_FORMAT_D16_UNORM";
        case AHARDWAREBUFFER_FORMAT_D24_UNORM:
            return "AHARDWAREBUFFER_FORMAT_D24_UNORM";
        case AHARDWAREBUFFER_FORMAT_D24_UNORM_S8_UINT:
            return "AHARDWAREBUFFER_FORMAT_D24_UNORM_S8_UINT";
        case AHARDWAREBUFFER_FORMAT_D32_FLOAT:
            return "AHARDWAREBUFFER_FORMAT_D32_FLOAT";
        case AHARDWAREBUFFER_FORMAT_D32_FLOAT_S8_UINT:
            return "AHARDWAREBUFFER_FORMAT_D32_FLOAT_S8_UINT";
        case AHARDWAREBUFFER_FORMAT_S8_UINT:
            return "AHARDWAREBUFFER_FORMAT_S8_UINT";
        case AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420:
            return "AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420";
        case AHARDWAREBUFFER_FORMAT_YCbCr_P010:
            return "AHARDWAREBUFFER_FORMAT_YCbCr_P010";
        case AHARDWAREBUFFER_FORMAT_R8_UNORM:
            return "AHARDWAREBUFFER_FORMAT_R8_UNORM";
    }

    return "<unknown AHardwareBuffer_Format>";
}

std::string_view UnpackCpuReadUsage(AHardwareBuffer_UsageFlags usage)
{
    switch (usage & AHARDWAREBUFFER_USAGE_CPU_READ_MASK)
    {
        case AHARDWAREBUFFER_USAGE_CPU_READ_NEVER:
            return "CPU_READ_NEVER";
        case AHARDWAREBUFFER_USAGE_CPU_READ_RARELY:
            return "CPU_READ_RARELY";
        case AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN:
            return "CPU_READ_OFTEN";
        default:
            return "<unknown CPU_READ>";
    }
}

std::string_view UnpackCpuWriteUsage(AHardwareBuffer_UsageFlags usage)
{
    switch (usage & AHARDWAREBUFFER_USAGE_CPU_WRITE_MASK)
    {
        case AHARDWAREBUFFER_USAGE_CPU_WRITE_NEVER:
            return "CPU_WRITE_NEVER";
        case AHARDWAREBUFFER_USAGE_CPU_WRITE_RARELY:
            return "CPU_WRITE_RARELY";
        case AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN:
            return "CPU_WRITE_OFTEN";
        default:
            return "<unknown CPU_WRITE>";
    }
}

std::string ToString(AHardwareBuffer_UsageFlags usage)
{
    // Usage is packed into 64bit value and can be subdivided:
    //
    // VVVV VVVV VVVV VVVV AAAA AAAA AAAA AAAA VVVV AAAA AAAA AAAA AAAA AAAA WWWW RRRR
    //
    // R is CPU read mode, one of CPU_READ_NEVER/RARELY/OFTEN
    // W is CPU write mode, one of CPU_WRITE_NEVER/RARELY/OFTEN
    // V are vendor-specific bits
    // A are the other flags that Android needs; not all of them are used.

    std::vector<std::string_view> bit_descriptions;
    bit_descriptions.push_back(UnpackCpuReadUsage(usage));
    bit_descriptions.push_back(UnpackCpuWriteUsage(usage));

    auto describe_bit = [&bit_descriptions, &usage](AHardwareBuffer_UsageFlags bit,
                                                    std::string_view description) {
        if ((usage & bit) == bit)
        {
            bit_descriptions.push_back(description);
        }
    };

    describe_bit(AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE, "GPU_SAMPLED_IMAGE");
    describe_bit(AHARDWAREBUFFER_USAGE_GPU_FRAMEBUFFER, "GPU_FRAMEBUFFER");
    describe_bit(AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT, "GPU_COLOR_OUTPUT");
    describe_bit(AHARDWAREBUFFER_USAGE_COMPOSER_OVERLAY, "COMPOSER_OVERLAY");
    describe_bit(AHARDWAREBUFFER_USAGE_PROTECTED_CONTENT, "PROTECTED_CONTENT");
    describe_bit(AHARDWAREBUFFER_USAGE_VIDEO_ENCODE, "VIDEO_ENCODE");
    describe_bit(AHARDWAREBUFFER_USAGE_SENSOR_DIRECT_DATA, "SENSOR_DIRECT_DATA");
    describe_bit(AHARDWAREBUFFER_USAGE_GPU_DATA_BUFFER, "GPU_DATA_BUFFER");
    describe_bit(AHARDWAREBUFFER_USAGE_GPU_CUBE_MAP, "GPU_CUBE_MAP");
    describe_bit(AHARDWAREBUFFER_USAGE_GPU_MIPMAP_COMPLETE, "GPU_MIPMAP_COMPLETE");
    describe_bit(AHARDWAREBUFFER_USAGE_FRONT_BUFFER, "FRONT_BUFFER");
    describe_bit(AHARDWAREBUFFER_USAGE_VENDOR_0, "VENDOR_0");
    describe_bit(AHARDWAREBUFFER_USAGE_VENDOR_1, "VENDOR_1");
    describe_bit(AHARDWAREBUFFER_USAGE_VENDOR_2, "VENDOR_2");
    describe_bit(AHARDWAREBUFFER_USAGE_VENDOR_3, "VENDOR_3");
    describe_bit(AHARDWAREBUFFER_USAGE_VENDOR_4, "VENDOR_4");
    describe_bit(AHARDWAREBUFFER_USAGE_VENDOR_5, "VENDOR_5");
    describe_bit(AHARDWAREBUFFER_USAGE_VENDOR_6, "VENDOR_6");
    describe_bit(AHARDWAREBUFFER_USAGE_VENDOR_7, "VENDOR_7");
    describe_bit(AHARDWAREBUFFER_USAGE_VENDOR_8, "VENDOR_8");
    describe_bit(AHARDWAREBUFFER_USAGE_VENDOR_9, "VENDOR_9");
    describe_bit(AHARDWAREBUFFER_USAGE_VENDOR_10, "VENDOR_10");
    describe_bit(AHARDWAREBUFFER_USAGE_VENDOR_11, "VENDOR_11");
    describe_bit(AHARDWAREBUFFER_USAGE_VENDOR_12, "VENDOR_12");
    describe_bit(AHARDWAREBUFFER_USAGE_VENDOR_13, "VENDOR_13");
    describe_bit(AHARDWAREBUFFER_USAGE_VENDOR_14, "VENDOR_14");
    describe_bit(AHARDWAREBUFFER_USAGE_VENDOR_15, "VENDOR_15");
    describe_bit(AHARDWAREBUFFER_USAGE_VENDOR_16, "VENDOR_16");
    describe_bit(AHARDWAREBUFFER_USAGE_VENDOR_18, "VENDOR_18");
    describe_bit(AHARDWAREBUFFER_USAGE_VENDOR_19, "VENDOR_19");

    return absl::StrJoin(bit_descriptions, "|");
}

class MyConsumer : public VulkanConsumer
{
 public:
    MyConsumer(std::unordered_map<std::string, Node>& out_graph,
               std::unordered_map<uint64_t, std::unordered_set<Node*>>& out_block_association)
        : graph_(&out_graph), block_association_(&out_block_association)
    {
    }

    void Process_vkAllocateCommandBuffers(
        const ApiCallInfo& call_info, VkResult returnValue, format::HandleId device,
        StructPointerDecoder<Decoded_VkCommandBufferAllocateInfo>* pAllocateInfo,
        HandlePointerDecoder<VkCommandBuffer>* pCommandBuffers) override
    {
        std::unordered_set<Node*>& association = (*block_association_)[call_info.index];
        auto command_buffers =
            std::span(pCommandBuffers->GetPointer(), pCommandBuffers->GetLength());
        for (const format::HandleId& id : command_buffers)
        {
            association.insert(EmitNode(id, "VkCommandBuffer"));
        }
    }

    void Process_vkAllocateMemory(const ApiCallInfo& call_info, VkResult returnValue,
                                  format::HandleId device,
                                  StructPointerDecoder<Decoded_VkMemoryAllocateInfo>* pAllocateInfo,
                                  StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
                                  HandlePointerDecoder<VkDeviceMemory>* pMemory) override
    {
        std::unordered_set<Node*>& association = (*block_association_)[call_info.index];
        association.insert(EmitNode(*pMemory->GetPointer(), "VkDeviceMemory"));

        if (const auto* import_hardware_buffer =
                GetPNextMetaStruct<Decoded_VkImportAndroidHardwareBufferInfoANDROID>(
                    pAllocateInfo->GetMetaStructPointer()->pNext);
            import_hardware_buffer != nullptr)
        {
            const auto& [memory_node, hardware_buffer_node] =
                EmitEdge(*pMemory->GetPointer(), import_hardware_buffer->buffer);
            association.insert(memory_node);
            association.insert(hardware_buffer_node);
        }

        if (const auto* dedicated_memory =
                GetPNextMetaStruct<Decoded_VkMemoryDedicatedAllocateInfo>(
                    pAllocateInfo->GetMetaStructPointer()->pNext);
            dedicated_memory != nullptr)
        {
            // TODO dedicated_memory->buffer
            const auto& [image_node, memory_node] =
                EmitEdge(dedicated_memory->image, *pMemory->GetPointer());
            association.insert(image_node);
        }
    }

    void Process_vkBindBufferMemory(const ApiCallInfo& call_info, VkResult returnValue,
                                    format::HandleId device, format::HandleId buffer,
                                    format::HandleId memory, VkDeviceSize memoryOffset) override
    {
        const auto& [buffer_node, memory_node] = EmitEdge(buffer, memory);
        std::unordered_set<Node*>& association = (*block_association_)[call_info.index];
        association.insert(buffer_node);
        association.insert(memory_node);
    }

    void Process_vkBindImageMemory(const ApiCallInfo& call_info, VkResult returnValue,
                                   format::HandleId device, format::HandleId image,
                                   format::HandleId memory, VkDeviceSize memoryOffset) override
    {
        std::unordered_set<Node*>& association = (*block_association_)[call_info.index];
        const auto& [image_node, memory_node] = EmitEdge(image, memory);
        association.insert(image_node);
        association.insert(memory_node);
    }

    void Process_vkCmdBeginRenderPass(
        const ApiCallInfo& call_info, format::HandleId commandBuffer,
        StructPointerDecoder<Decoded_VkRenderPassBeginInfo>* pRenderPassBegin,
        VkSubpassContents contents) override
    {
        std::unordered_set<Node*>& association = (*block_association_)[call_info.index];

        const auto& [command_buffer_node, frame_buffer_node] =
            EmitEdge(commandBuffer, pRenderPassBegin->GetMetaStructPointer()->framebuffer);
        association.insert(command_buffer_node);
        association.insert(frame_buffer_node);

        const auto& [unused, render_pass_node] =
            EmitEdge(commandBuffer, pRenderPassBegin->GetMetaStructPointer()->renderPass);
        association.insert(render_pass_node);
    }

    void Process_vkCmdBeginRenderPass2(
        const ApiCallInfo& call_info, format::HandleId commandBuffer,
        StructPointerDecoder<Decoded_VkRenderPassBeginInfo>* pRenderPassBegin,
        StructPointerDecoder<Decoded_VkSubpassBeginInfo>* pSubpassBeginInfo) override
    {
        std::unordered_set<Node*>& association = (*block_association_)[call_info.index];

        const auto& [command_buffer_node, frame_buffer_node] =
            EmitEdge(commandBuffer, pRenderPassBegin->GetMetaStructPointer()->framebuffer);
        association.insert(command_buffer_node);
        association.insert(frame_buffer_node);

        const auto& [unused, render_pass_node] =
            EmitEdge(commandBuffer, pRenderPassBegin->GetMetaStructPointer()->renderPass);
        association.insert(render_pass_node);
    }

    void Process_vkCmdBeginRenderPass2KHR(
        const ApiCallInfo& call_info, format::HandleId commandBuffer,
        StructPointerDecoder<Decoded_VkRenderPassBeginInfo>* pRenderPassBegin,
        StructPointerDecoder<Decoded_VkSubpassBeginInfo>* pSubpassBeginInfo) override
    {
        std::unordered_set<Node*>& association = (*block_association_)[call_info.index];

        const auto& [command_buffer_node, frame_buffer_node] =
            EmitEdge(commandBuffer, pRenderPassBegin->GetMetaStructPointer()->framebuffer);
        association.insert(command_buffer_node);
        association.insert(frame_buffer_node);

        const auto& [unused, render_pass_node] =
            EmitEdge(commandBuffer, pRenderPassBegin->GetMetaStructPointer()->renderPass);
        association.insert(render_pass_node);
    }

    void Process_vkCreateBuffer(const ApiCallInfo& call_info, VkResult returnValue,
                                format::HandleId device,
                                StructPointerDecoder<Decoded_VkBufferCreateInfo>* pCreateInfo,
                                StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
                                HandlePointerDecoder<VkBuffer>* pBuffer) override
    {
        VkBufferCreateInfo& create_info = *pCreateInfo->GetPointer();
        (*block_association_)[call_info.index].insert(
            EmitNode(*pBuffer->GetPointer(), "VkBuffer",
                     {{{"size", absl::StrCat(create_info.size)},
                       {"usage", util::ToString<VkImageUsageFlagBits>(create_info.usage)}}}));
    }

    void Process_vkCreateFramebuffer(
        const ApiCallInfo& call_info, VkResult returnValue, format::HandleId device,
        StructPointerDecoder<Decoded_VkFramebufferCreateInfo>* pCreateInfo,
        StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
        HandlePointerDecoder<VkFramebuffer>* pFramebuffer) override
    {
        std::unordered_set<Node*>& association = (*block_association_)[call_info.index];
        association.insert(EmitNode(*pFramebuffer->GetPointer(), "VkFramebuffer"));

        auto& attachments_decoder = pCreateInfo->GetMetaStructPointer()->pAttachments;
        auto attachments =
            std::span(attachments_decoder.GetPointer(), attachments_decoder.GetLength());
        for (const format::HandleId& id : attachments)
        {
            const auto& [unused, to_node] = EmitEdge(*pFramebuffer->GetPointer(), id);
            association.insert(to_node);
        }
        const auto& [from_node, to_node] =
            EmitEdge(pCreateInfo->GetMetaStructPointer()->renderPass, *pFramebuffer->GetPointer());
        association.insert(from_node);
    }

    void Process_vkCreateImage(const ApiCallInfo& call_info, VkResult returnValue,
                               format::HandleId device,
                               StructPointerDecoder<Decoded_VkImageCreateInfo>* pCreateInfo,
                               StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
                               HandlePointerDecoder<VkImage>* pImage) override
    {
        std::unordered_set<Node*>& association = (*block_association_)[call_info.index];
        VkImageCreateInfo& create_info = *pCreateInfo->GetPointer();
        association.insert(EmitNode(*pImage->GetPointer(), "VkImage",
                                    {{{"format", util::ToString(create_info.format)},
                                      {"extent.width", absl::StrCat(create_info.extent.width)},
                                      {"extent.height", absl::StrCat(create_info.extent.height)},
                                      {"extent.depth", absl::StrCat(create_info.extent.depth)}}}));
    }

    void Process_vkCreateImageView(const ApiCallInfo& call_info, VkResult returnValue,
                                   format::HandleId device,
                                   StructPointerDecoder<Decoded_VkImageViewCreateInfo>* pCreateInfo,
                                   StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
                                   HandlePointerDecoder<VkImageView>* pView) override
    {
        std::unordered_set<Node*>& association = (*block_association_)[call_info.index];
        association.insert(
            EmitNode(*pView->GetPointer(), "VkImageView",
                     {{{"format", util::ToString(pCreateInfo->GetPointer()->format)}}}));

        const auto& [unused, to_node] =
            EmitEdge(*pView->GetPointer(), pCreateInfo->GetMetaStructPointer()->image);
        association.insert(to_node);
    }

    void Process_vkCreateRenderPass(
        const ApiCallInfo& call_info, VkResult returnValue, format::HandleId device,
        StructPointerDecoder<Decoded_VkRenderPassCreateInfo>* pCreateInfo,
        StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
        HandlePointerDecoder<VkRenderPass>* pRenderPass) override
    {
        (*block_association_)[call_info.index].insert(
            EmitNode(*pRenderPass->GetPointer(), "VkRenderPass"));
    }

    void Process_vkCreateRenderPass2(
        const ApiCallInfo& call_info, VkResult returnValue, format::HandleId device,
        StructPointerDecoder<Decoded_VkRenderPassCreateInfo2>* pCreateInfo,
        StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
        HandlePointerDecoder<VkRenderPass>* pRenderPass) override
    {
        (*block_association_)[call_info.index].insert(
            EmitNode(*pRenderPass->GetPointer(), "VkRenderPass"));
    }

    void Process_vkCreateRenderPass2KHR(
        const ApiCallInfo& call_info, VkResult returnValue, format::HandleId device,
        StructPointerDecoder<Decoded_VkRenderPassCreateInfo2>* pCreateInfo,
        StructPointerDecoder<Decoded_VkAllocationCallbacks>* pAllocator,
        HandlePointerDecoder<VkRenderPass>* pRenderPass) override
    {
        (*block_association_)[call_info.index].insert(
            EmitNode(*pRenderPass->GetPointer(), "VkRenderPass"));
    }

    void Process_vkQueueSubmit(const ApiCallInfo& call_info, VkResult returnValue,
                               format::HandleId queue, uint32_t submitCount,
                               StructPointerDecoder<Decoded_VkSubmitInfo>* pSubmits,
                               format::HandleId fence) override
    {
        std::unordered_set<Node*>& association = (*block_association_)[call_info.index];

        std::string unique_submit_name = absl::StrCat("vkQueueSubmit_", call_info.index);
        association.insert(EmitNode(unique_submit_name, "vkQueueSubmit",
                                    {{{"block_id", absl::StrCat(call_info.index)}}}));

        std::span<Decoded_VkSubmitInfo> decoded_submit_infos(pSubmits->GetMetaStructPointer(),
                                                             pSubmits->GetLength());
        for (const auto& submit_info : decoded_submit_infos)
        {
            std::span<const format::HandleId> command_buffer_ids(
                submit_info.pCommandBuffers.GetPointer(), submit_info.pCommandBuffers.GetLength());
            for (const auto& command_buffer_id : command_buffer_ids)
            {
                const auto& [submit_node, command_buffer_node] =
                    EmitEdge(unique_submit_name, command_buffer_id);
                association.insert(command_buffer_node);
            }
        }
    }

    void ProcessCreateHardwareBufferCommand(
        format::HandleId device_id, format::HandleId memory_id, uint64_t buffer_id, uint32_t format,
        uint32_t width, uint32_t height, uint32_t stride, uint64_t usage, uint32_t layers,
        const std::vector<format::HardwareBufferPlaneInfo>& plane_info) override
    {
        EmitNode(buffer_id, "AHardwareBuffer",
                 {{{"format", ToString(static_cast<enum AHardwareBuffer_Format>(format))},
                   {"usage", ToString(static_cast<enum AHardwareBuffer_UsageFlags>(usage))}}});
        // Can't perform block association since no ApiCallInfo
    }

 private:
    using KeyValuePair = std::pair<std::string_view, std::string_view>;

    Node* EmitNode(std::string_view name, std::string_view label,
                   std::span<const KeyValuePair> properties = {})
    {
        // Allow default constructing so we don't blow up if the capture contains an asset that
        // hasn't been created.
        Node& node = (*graph_)[std::string(name)];
        node.id = name;
        node.name = absl::StrFormat("%s (%s)", label, name);
        for (const KeyValuePair& property : properties)
        {
            node.properties[std::string(property.first)] = std::string(property.second);
        };
        return &node;
    }

    Node* EmitNode(format::HandleId id, std::string_view label,
                   std::span<const KeyValuePair> properties = {})
    {
        return EmitNode(absl::StrCat(id), label, properties);
    }

    std::pair<Node*, Node*> EmitEdge(std::string_view from_name, format::HandleId to)
    {
        // Allow default constructing so we don't blow up if the capture contains an asset that
        // hasn't been created.
        Node& node_from = (*graph_)[std::string(from_name)];
        Node& node_to = (*graph_)[absl::StrCat(to)];
        node_from.children.insert(&node_to);
        node_to.parents.insert(&node_from);
        return std::make_pair(&node_from, &node_to);
    }

    std::pair<Node*, Node*> EmitEdge(format::HandleId from, format::HandleId to)
    {
        return EmitEdge(absl::StrCat(from), to);
    }

    // key is node name, typically the HandleId of the resource
    std::unordered_map<std::string, Node>* graph_;
    // key is ApiCallInfo block index
    std::unordered_map<uint64_t, std::unordered_set<Node*>>* block_association_;
};

absl::Status ConstructGraph(
    const std::filesystem::path& gfxr_file, std::unordered_map<std::string, Node>& out_graph,
    std::unordered_map<uint64_t, std::unordered_set<Node*>>& out_block_association)
{
    // Pointer stability is required so make the client pass in a graph
    FileProcessor file_processor;
    if (!file_processor.Initialize(gfxr_file))
    {
        return absl::UnknownError(absl::StrFormat("Failed to open capture file: %s", gfxr_file));
    }

    VulkanDecoder vulkan_decoder;
    file_processor.AddDecoder(&vulkan_decoder);

    MyConsumer consumer(out_graph, out_block_association);
    vulkan_decoder.AddConsumer(&consumer);

    file_processor.ProcessAllFrames();

    return absl::OkStatus();
}

void WriteNode(const Node& node, std::ofstream& dot_file)
{
    dot_file << node.id << " [label=\"" << node.name;
    if (!node.properties.empty())
    {
        dot_file << "\n" << absl::StrJoin(node.properties, "\n", absl::PairFormatter("="));
    }
    dot_file << "\"];\n";
}

void WriteEdge(const Node& parent, const Node& child, std::ofstream& dot_file)
{
    dot_file << parent.id << " -> " << child.id << ";\n";
}

void DumpGraph(const std::unordered_map<std::string, Node>& graph, std::ofstream& dot_file)
{
    dot_file << "digraph test {\n";
    for (const auto& [id, node] : graph)
    {
        WriteNode(node, dot_file);
        for (const Node* child : node.children)
        {
            WriteEdge(node, *child, dot_file);
        }
    }
    dot_file << "}\n";
}

std::unordered_set<const Node*> FlattenGraph(const Node& starting_node)
{
    std::unordered_set<const Node*> to_process = {&starting_node};
    std::unordered_set<const Node*> processed;

    while (!to_process.empty())
    {
        auto node_handle = to_process.extract(to_process.begin());
        for (const Node* child : node_handle.value()->children)
        {
            if (processed.contains(child))
            {
                continue;
            }
            to_process.insert(child);
        }
        for (const Node* parent : node_handle.value()->parents)
        {
            if (processed.contains(parent))
            {
                continue;
            }
            to_process.insert(parent);
        }
        processed.insert(std::move(node_handle));
    }

    return processed;
}

void DumpComponent(const Node& starting_node, std::ofstream& dot_file)
{
    dot_file << "digraph test {\n";

    // Flatten to a unique list of nodes so diamond patterns don't generate redundant edges.
    for (const Node* node : FlattenGraph(starting_node))
    {
        WriteNode(*node, dot_file);
        for (const Node* child : node->children)
        {
            WriteEdge(*node, *child, dot_file);
        }
    }

    dot_file << "}\n";
}

absl::Status Run(const std::filesystem::path& gfxr_file, const std::filesystem::path& dot_file)
{
    std::unordered_map<std::string, Node> graph;
    std::unordered_map<uint64_t, std::unordered_set<Node*>> block_association;

    if (absl::Status status = ConstructGraph(gfxr_file, graph, block_association); !status.ok())
    {
        return status;
    }

    std::ofstream out_dot_file(dot_file);
    if (!out_dot_file.is_open())
    {
        return absl::UnknownError(absl::StrFormat("Failed to output file: %s", dot_file));
    }

    std::optional<uint64_t> block_index = absl::GetFlag(FLAGS_block_index);
    if (block_index.has_value())
    {
        const auto iter = block_association.find(*block_index);
        if (iter == block_association.cend())
        {
            return absl::UnknownError("No graph for block filter");
        }

        std::unordered_set<Node*>& associations = iter->second;
        if (associations.empty())
        {
            return absl::UnknownError("No assocations for block filter");
        }

        // TODO: figure out if all associations are the same graph
        DumpComponent(**associations.begin(), out_dot_file);
    }
    else
    {
        DumpGraph(graph, out_dot_file);
    }

    return absl::OkStatus();
}

}  // namespace
GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)

int main(int argc, char** argv)
{
    absl::SetProgramUsageMessage("Usage: gfxrecon-explorer FILE.GFXR OUTPUT.DOT");
    std::vector<char*> positional_args = absl::ParseCommandLine(argc, argv);
    if (positional_args.size() != 3)
    {
        std::cerr << absl::ProgramUsageMessage() << '\n';
        return 1;
    }

    gfxrecon::util::Log::Init(gfxrecon::util::Log::kDebugSeverity);
    if (absl::Status status = gfxrecon::decode::Run(positional_args[1], positional_args[2]);
        !status.ok())
    {
        GFXRECON_LOG_ERROR("%s", std::string(status.message()).c_str());
        return 1;
    }

    return 0;
}
