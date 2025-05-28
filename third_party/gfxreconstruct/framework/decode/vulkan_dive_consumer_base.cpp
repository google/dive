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

#include "decode/vulkan_dive_consumer_base.h"
#include "decode/custom_vulkan_struct_to_json.h"

#include "generated/generated_vulkan_enum_to_json.h"

#include "util/json_util.h"
#include "util/platform.h"
#include "util/file_path.h"
#include "util/to_string.h"

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

using namespace util::platform;

VulkanExportDiveConsumerBase::~VulkanExportDiveConsumerBase()
{
}

void VulkanExportDiveConsumerBase::Initialize(AnnotationHandler* writer)
{
    GFXRECON_ASSERT(writer);
    writer_ = writer;
}

void VulkanExportDiveConsumerBase::ProcessSetDeviceMemoryPropertiesCommand(
    format::HandleId                             physical_device_id,
    const std::vector<format::DeviceMemoryType>& memory_types,
    const std::vector<format::DeviceMemoryHeap>& memory_heaps)
{
}

void VulkanExportDiveConsumerBase::Process_vkCmdBuildAccelerationStructuresIndirectKHR(
    const ApiCallInfo&                                                         call_info,
    format::HandleId                                                           commandBuffer,
    uint32_t                                                                   infoCount,
    StructPointerDecoder<Decoded_VkAccelerationStructureBuildGeometryInfoKHR>* pInfos,
    PointerDecoder<VkDeviceAddress>*                                           pIndirectDeviceAddresses,
    PointerDecoder<uint32_t>*                                                  pIndirectStrides,
    PointerDecoder<uint32_t*>*                                                 ppMaxPrimitiveCounts)
{
    nlohmann::ordered_json dive_data;
    const util::JsonOptions json_options;
    auto& args = dive_data["args"];
    HandleToJson(args["commandBuffer"], commandBuffer, json_options);
    FieldToJson(args["infoCount"], infoCount, json_options);
    FieldToJson(args["pInfos"], pInfos, json_options);
    FieldToJson(args["pIndirectDeviceAddresses"], pIndirectDeviceAddresses, json_options);
    FieldToJson(args["pIndirectStrides"], pIndirectStrides, json_options);

    auto infos                     = pInfos ? pInfos->GetPointer() : nullptr;
    auto max_primitive_counts      = ppMaxPrimitiveCounts ? ppMaxPrimitiveCounts->GetPointer() : nullptr;
    auto max_primitive_counts_json = args["ppMaxPrimitiveCounts"];

    for (uint32_t i = 0; i < infoCount; ++i)
    {
        auto element = max_primitive_counts_json[i];
        FieldToJson(max_primitive_counts_json[i], max_primitive_counts[i], infos[i].geometryCount, json_options);
    }
    util::DiveFunctionData function_data("vkCmdBuildAccelerationStructuresIndirectKHR", GetCommandBufferRecordIndex(commandBuffer), call_info.index, args);
    WriteBlockEnd(function_data);
}

void VulkanExportDiveConsumerBase::Process_vkCreateShaderModule(
    const gfxrecon::decode::ApiCallInfo&                                                        call_info,
    VkResult                                                                                    returnValue,
    gfxrecon::format::HandleId                                                                  device,
    gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkShaderModuleCreateInfo>* pCreateInfo,
    gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkAllocationCallbacks>*    pAllocator,
    gfxrecon::decode::HandlePointerDecoder<VkShaderModule>*                                     pShaderModule)
{
}

void VulkanExportDiveConsumerBase::Process_vkGetPipelineCacheData(const ApiCallInfo&       call_info,
                                                                  VkResult                 returnValue,
                                                                  format::HandleId         device,
                                                                  format::HandleId         pipelineCache,
                                                                  PointerDecoder<size_t>*  pDataSize,
                                                                  PointerDecoder<uint8_t>* pData)
{
}

void VulkanExportDiveConsumerBase::Process_vkCreatePipelineCache(
    const ApiCallInfo&                                       call_info,
    VkResult                                                 returnValue,
    format::HandleId                                         device,
    StructPointerDecoder<Decoded_VkPipelineCacheCreateInfo>* pCreateInfo,
    StructPointerDecoder<Decoded_VkAllocationCallbacks>*     pAllocator,
    HandlePointerDecoder<VkPipelineCache>*                   pPipelineCache)
{
}

void VulkanExportDiveConsumerBase::Process_vkCmdPushConstants(const ApiCallInfo&       call_info,
                                                              format::HandleId         commandBuffer,
                                                              format::HandleId         layout,
                                                              VkShaderStageFlags       stageFlags,
                                                              uint32_t                 offset,
                                                              uint32_t                 size,
                                                              PointerDecoder<uint8_t>* pValues)
{
    nlohmann::ordered_json dive_data;
    const util::JsonOptions json_options;
    auto& args = dive_data["args"];
    HandleToJson(args["commandBuffer"], commandBuffer, json_options);
    HandleToJson(args["layout"], layout, json_options);
    FieldToJson(VkShaderStageFlags_t(), args["stageFlags"], stageFlags, json_options);
    FieldToJson(args["offset"], offset, json_options);
    FieldToJson(args["size"], size, json_options);
    FieldToJson(args["pValues"], pValues, json_options);
    util::DiveFunctionData function_data("vkCmdPushConstants", GetCommandBufferRecordIndex(commandBuffer), call_info.index, args);
    WriteBlockEnd(function_data);
}

void VulkanExportDiveConsumerBase::Process_vkUpdateDescriptorSetWithTemplate(const ApiCallInfo& call_info,
                                                                             format::HandleId   device,
                                                                             format::HandleId   descriptorSet,
                                                                             format::HandleId descriptorUpdateTemplate,
                                                                             DescriptorUpdateTemplateDecoder* pData,
                                                                             bool use_KHR_suffix)
{
}

void VulkanExportDiveConsumerBase::Process_vkCmdPushDescriptorSetWithTemplateKHR(
    const ApiCallInfo&               call_info,
    format::HandleId                 commandBuffer,
    format::HandleId                 descriptorUpdateTemplate,
    format::HandleId                 layout,
    uint32_t                         set,
    DescriptorUpdateTemplateDecoder* pData)
{
    nlohmann::ordered_json dive_data;
    const util::JsonOptions json_options;
    auto& args = dive_data["args"];
    HandleToJson(args["commandBuffer"], commandBuffer, json_options);
    HandleToJson(args["descriptorUpdateTemplate"], descriptorUpdateTemplate, json_options);
    HandleToJson(args["layout"], layout, json_options);
    FieldToJson(args["set"], set, json_options);
    FieldToJson(args["pData"], pData, json_options);

    util::DiveFunctionData function_data("vkCmdPushDescriptorSetWithTemplateKHR", GetCommandBufferRecordIndex(commandBuffer), call_info.index, args);
    WriteBlockEnd(function_data);
}

void VulkanExportDiveConsumerBase::Process_vkCmdPushDescriptorSetWithTemplate2KHR(
    const ApiCallInfo&                                                 call_info,
    format::HandleId                                                   commandBuffer,
    StructPointerDecoder<Decoded_VkPushDescriptorSetWithTemplateInfo>* pPushDescriptorSetWithTemplateInfo)
{
    nlohmann::ordered_json dive_data;
    const util::JsonOptions json_options;
    auto& args = dive_data["args"];
    const StructPointerDecoder<Decoded_VkPushDescriptorSetWithTemplateInfo>* info = pPushDescriptorSetWithTemplateInfo;

    HandleToJson(args["commandBuffer"], commandBuffer, json_options);
    FieldToJson(args["pPushDescriptorSetWithTemplateInfo"], info, json_options);

    util::DiveFunctionData function_data("vkCmdPushDescriptorSetWithTemplate2KHR", GetCommandBufferRecordIndex(commandBuffer), call_info.index, args);
    WriteBlockEnd(function_data);
}

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)
