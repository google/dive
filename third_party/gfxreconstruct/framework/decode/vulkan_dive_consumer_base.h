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

#ifndef GFXRECON_DECODE_VULKAN_DIVE_CONSUMER_BASE_H
#define GFXRECON_DECODE_VULKAN_DIVE_CONSUMER_BASE_H

#include "util/output_stream.h"
#include "util/defines.h"
#include "annotation_handler.h"
#include "format/platform_types.h"
#include "generated/generated_vulkan_consumer.h"
#include "vulkan/vulkan.h"
#include "util/json_util.h"
#include "util/dive_function_data.h"

#include <cstdint>
#include <cstdio>
#include <string>

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

class VulkanExportDiveConsumerBase : public VulkanConsumer
{
  public:
    VulkanExportDiveConsumerBase() = default;

    virtual ~VulkanExportDiveConsumerBase() override;

    void Initialize(AnnotationHandler* writer);

    void Destroy();

    bool IsValid() const { return true; }

    virtual void
    ProcessSetDeviceMemoryPropertiesCommand(format::HandleId                             physical_device_id,
                                            const std::vector<format::DeviceMemoryType>& memory_types,
                                            const std::vector<format::DeviceMemoryHeap>& memory_heaps) override;

    void Process_vkCmdBuildAccelerationStructuresIndirectKHR(
        const ApiCallInfo&                                                         call_info,
        format::HandleId                                                           commandBuffer,
        uint32_t                                                                   infoCount,
        StructPointerDecoder<Decoded_VkAccelerationStructureBuildGeometryInfoKHR>* pInfos,
        PointerDecoder<VkDeviceAddress>*                                           pIndirectDeviceAddresses,
        PointerDecoder<uint32_t>*                                                  pIndirectStrides,
        PointerDecoder<uint32_t*>*                                                 ppMaxPrimitiveCounts) override;

    virtual void Process_vkCreateShaderModule(
        const gfxrecon::decode::ApiCallInfo&                                                        call_info,
        VkResult                                                                                    returnValue,
        gfxrecon::format::HandleId                                                                  device,
        gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkShaderModuleCreateInfo>* pCreateInfo,
        gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkAllocationCallbacks>*    pAllocator,
        gfxrecon::decode::HandlePointerDecoder<VkShaderModule>* pShaderModule) override;

    virtual void Process_vkGetPipelineCacheData(const ApiCallInfo&       call_info,
                                                VkResult                 returnValue,
                                                format::HandleId         device,
                                                format::HandleId         pipelineCache,
                                                PointerDecoder<size_t>*  pDataSize,
                                                PointerDecoder<uint8_t>* pData) override;

    virtual void Process_vkCreatePipelineCache(const ApiCallInfo&                                       call_info,
                                               VkResult                                                 returnValue,
                                               format::HandleId                                         device,
                                               StructPointerDecoder<Decoded_VkPipelineCacheCreateInfo>* pCreateInfo,
                                               StructPointerDecoder<Decoded_VkAllocationCallbacks>*     pAllocator,
                                               HandlePointerDecoder<VkPipelineCache>* pPipelineCache) override;

    virtual void Process_vkCmdPushConstants(const ApiCallInfo&       call_info,
                                            format::HandleId         commandBuffer,
                                            format::HandleId         layout,
                                            VkShaderStageFlags       stageFlags,
                                            uint32_t                 offset,
                                            uint32_t                 size,
                                            PointerDecoder<uint8_t>* pValues) override;

    void Process_vkUpdateDescriptorSetWithTemplateKHR(const ApiCallInfo&               call_info,
                                                      format::HandleId                 device,
                                                      format::HandleId                 descriptorSet,
                                                      format::HandleId                 descriptorUpdateTemplate,
                                                      DescriptorUpdateTemplateDecoder* pData) override
    {
        Process_vkUpdateDescriptorSetWithTemplate(
            call_info, device, descriptorSet, descriptorUpdateTemplate, pData, true);
    }

    void Process_vkUpdateDescriptorSetWithTemplate(const ApiCallInfo&               call_info,
                                                   format::HandleId                 device,
                                                   format::HandleId                 descriptorSet,
                                                   format::HandleId                 descriptorUpdateTemplate,
                                                   DescriptorUpdateTemplateDecoder* pData) override
    {
        Process_vkUpdateDescriptorSetWithTemplate(
            call_info, device, descriptorSet, descriptorUpdateTemplate, pData, false);
    }

  protected:
    void Process_vkUpdateDescriptorSetWithTemplate(const ApiCallInfo&               call_info,
                                                   format::HandleId                 device,
                                                   format::HandleId                 descriptorSet,
                                                   format::HandleId                 descriptorUpdateTemplate,
                                                   DescriptorUpdateTemplateDecoder* pData,
                                                   bool                             use_KHR_suffix);

    virtual void Process_vkCmdPushDescriptorSetWithTemplateKHR(const ApiCallInfo& call_info,
                                                               format::HandleId   commandBuffer,
                                                               format::HandleId   descriptorUpdateTemplate,
                                                               format::HandleId   layout,
                                                               uint32_t           set,
                                                               DescriptorUpdateTemplateDecoder* pData) override;

    virtual void Process_vkCmdPushDescriptorSetWithTemplate2KHR(
        const ApiCallInfo&                                                 call_info,
        format::HandleId                                                   commandBuffer,
        StructPointerDecoder<Decoded_VkPushDescriptorSetWithTemplateInfo>* pPushDescriptorSetWithTemplateInfo) override;

    void WriteBlockStart() { writer_->WriteBlockStart(); }
                                      
    void WriteBlockEnd(util::DiveFunctionData function_data) { writer_->WriteBlockEnd(function_data); }


    /// A field not present in binary format which identifies the index of each
    /// command within its command buffer.
    /// @todo Make this field optional.
    constexpr const char* NameCommandIndex() const { return "cmd_index"; }
    /// A field not present in binary format which identifies the index of each
    /// submit in the global order of all submits to all queues as recorded in
    /// the binary trace file.
    /// @todo Make this field optional.

    uint32_t GetCommandBufferRecordIndex(format::HandleId command_buffer)
    {
        uint32_t index = ++rec_cmd_index_[command_buffer];
        return index;
    }

    void ResetCommandBufferRecordIndex(format::HandleId command_buffer) { rec_cmd_index_[command_buffer] = 0; }

    uint32_t                                       submit_index_{ 0 }; // index of submissions across the trace
    std::unordered_map<format::HandleId, uint32_t> rec_cmd_index_;
    AnnotationHandler* writer_{ nullptr };
};

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)

#endif // GFXRECON_DECODE_VULKAN_DIVE_CONSUMER_BASE_H
