/*
** Copyright (c) 2024 LunarG, Inc.
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and associated documentation files (the "Software"),
** to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense,
** and/or sell copies of the Software, and to permit persons to whom the
** Software is furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
** FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
** DEALINGS IN THE SOFTWARE.
*/

#ifndef GFXRECON_GENERATED_VULKAN_REPLAY_DUMP_RESOURCES_COMMON_H
#define GFXRECON_GENERATED_VULKAN_REPLAY_DUMP_RESOURCES_COMMON_H

#include "decode/common_object_info_table.h"
#include "vulkan/vulkan_core.h"
#include "util/defines.h"
#include "util/image_writer.h"
#include "util/options.h"

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

using CommandBufferIterator = std::vector<VkCommandBuffer>::const_iterator;

enum PipelineBindPoints
{
    kBindPoint_graphics = 0,
    kBindPoint_compute,
    kBindPoint_ray_tracing,

    kBindPoint_count
};

template <typename T>
static bool IsInsideRange(const std::vector<T>& vec, T value)
{
    if (!vec.size())
    {
        return false;
    }
    else
    {
        return (value >= *(vec.begin()) && value <= *(vec.end() - 1));
    }
}

PipelineBindPoints VkPipelineBindPointToPipelineBindPoint(VkPipelineBindPoint bind_point);

enum DumpedImageFormat
{
    kFormatBMP,
    KFormatPNG,
    KFormatRaw
};

DumpedImageFormat GetDumpedImageFormat(const VulkanDeviceInfo*            device_info,
                                       const encode::VulkanDeviceTable*   device_table,
                                       const encode::VulkanInstanceTable* instance_table,
                                       VulkanObjectInfoTable&             object_info_table,
                                       VkFormat                           src_format,
                                       VkImageTiling                      src_image_tiling,
                                       VkImageType                        type,
                                       util::ScreenshotFormat             image_file_format,
                                       bool                               dump_raw = false);

const char* ImageFileExtension(DumpedImageFormat image_format);

uint32_t GetMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties& memory_properties,
                            uint32_t                                type_bits,
                            VkMemoryPropertyFlags                   property_flags);

VkResult CloneImage(CommonObjectInfoTable&                  object_info_table,
                    const encode::VulkanDeviceTable*        device_table,
                    const VkPhysicalDeviceMemoryProperties* replay_device_phys_mem_props,
                    const VulkanImageInfo*                  image_info,
                    VkImage*                                new_image,
                    VkDeviceMemory*                         new_image_memory);

VkResult CloneBuffer(CommonObjectInfoTable&                  object_info_table,
                     const encode::VulkanDeviceTable*        device_table,
                     const VkPhysicalDeviceMemoryProperties* replay_device_phys_mem_props,
                     const VulkanBufferInfo*                 buffer_info,
                     VkBuffer*                               new_buffer,
                     VkDeviceMemory*                         new_buffer_memory,
                     VkDeviceSize                            override_size = 0);

uint32_t VkIndexTypeToBytes(VkIndexType type);

std::pair<uint32_t, uint32_t> FindMinMaxVertexIndices(const std::vector<uint8_t>& index_data,
                                                      uint32_t                    index_count,
                                                      uint32_t                    first_index,
                                                      int32_t                     vertex_offset,
                                                      VkIndexType                 type);

VkResult DumpImageToFile(const VulkanImageInfo*             image_info,
                         const VulkanDeviceInfo*            device_info,
                         const encode::VulkanDeviceTable*   device_table,
                         const encode::VulkanInstanceTable* instance_table,
                         CommonObjectInfoTable&             object_info_table,
                         const std::vector<std::string>&    filenames,
                         float                              scale,
                         std::vector<bool>&                 scaling_supported,
                         util::ScreenshotFormat             image_file_format,
                         bool                               dump_all_subresources = false,
                         bool                               dump_image_raw        = false,
                         bool                               dump_separate_alpha   = false,
                         VkImageLayout                      layout                = VK_IMAGE_LAYOUT_MAX_ENUM);

bool CheckDescriptorCompatibility(VkDescriptorType desc_type_a, VkDescriptorType desc_type_b);

std::string ShaderStageToStr(VkShaderStageFlagBits shader_stage);

std::string ImageAspectToStr(VkImageAspectFlagBits aspect);

std::string FormatToStr(VkFormat format);

std::string IndexTypeToStr(VkIndexType type);

VkResult CreateVkBuffer(VkDeviceSize                            size,
                        const encode::VulkanDeviceTable*        device_table,
                        VkDevice                                parent_device,
                        VkBaseInStructure*                      pNext,
                        const VkPhysicalDeviceMemoryProperties* replay_device_phys_mem_props,
                        VkBuffer*                               new_buffer,
                        VkDeviceMemory*                         new_memory);

void GetFormatAspects(VkFormat format, std::vector<VkImageAspectFlagBits>& aspects);

class VulkanDumpResourcesDelegate;
class DefaultVulkanDumpResourcesDelegate;

enum class DumpResourceType : uint32_t
{
    kUnknown,
    kRtv,
    kDsv,
    kVertex,
    kIndex,
    kImageDescriptor,
    kBufferDescriptor,
    kInlineUniformBufferDescriptor,
    kDrawCallInfo,
    kDispatchInfo,
    kTraceRaysIndex,
    kDispatchTraceRaysImage,
    kDispatchTraceRaysBuffer,
    kDispatchTraceRaysImageDescriptor,
    kDispatchTraceRaysBufferDescriptor,
    kDispatchTraceRaysInlineUniformBufferDescriptor,
};

#define DEPTH_ATTACHMENT ~0

GFXRECON_END_NAMESPACE(gfxrecon)
GFXRECON_END_NAMESPACE(decode)

#endif /* GFXRECON_GENERATED_VULKAN_REPLAY_DUMP_RESOURCES_COMMON_H */