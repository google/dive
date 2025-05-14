
#pragma once

#include "gfxreconstruct/framework/decode/api_decoder.h"
#include "gfxreconstruct/framework/decode/struct_pointer_decoder.h"
#include "gfxreconstruct/framework/format/format.h"
#include "gfxreconstruct/framework/generated/generated_vulkan_consumer.h"
#include "gfxreconstruct/framework/generated/generated_vulkan_struct_decoders.h"

// forward decl to break recursive includes
class StateMachine;

// Have BeginCommandBuffer, BeginRenderPass
// Looking for Draw or EndRenderPass
class LookingForDraw : public gfxrecon::decode::VulkanConsumer
{
public:
    LookingForDraw(StateMachine& parent, gfxrecon::decode::VulkanConsumer& found_end);

    void Process_vkCmdDraw(const gfxrecon::decode::ApiCallInfo& call_info,
                           gfxrecon::format::HandleId           commandBuffer,
                           uint32_t                             vertexCount,
                           uint32_t                             instanceCount,
                           uint32_t                             firstVertex,
                           uint32_t                             firstInstance) override;

    void Process_vkCmdEndRenderPass(const gfxrecon::decode::ApiCallInfo& call_info,
                                    gfxrecon::format::HandleId           commandBuffer) override;

    // TODO subpass

private:
    StateMachine&                     parent_;
    gfxrecon::decode::VulkanConsumer& found_end_;
};

// Have BeginCommandBuffer
// looking for BeginRenderPass or QueueSubmit
class LookingForRenderPass : public gfxrecon::decode::VulkanConsumer
{
public:
    LookingForRenderPass(StateMachine& parent, gfxrecon::decode::VulkanConsumer& found_begin);

    void Process_vkQueueSubmit(
    const gfxrecon::decode::ApiCallInfo&                                            call_info,
    VkResult                                                                        returnValue,
    gfxrecon::format::HandleId                                                      queue,
    uint32_t                                                                        submitCount,
    gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkSubmitInfo>* pSubmits,
    gfxrecon::format::HandleId                                                      fence) override;

    void Process_vkCmdBeginRenderPass(
    const gfxrecon::decode::ApiCallInfo& call_info,
    gfxrecon::format::HandleId           commandBuffer,
    gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkRenderPassBeginInfo>*
                      pRenderPassBegin,
    VkSubpassContents contents) override;

private:
    StateMachine&                     parent_;
    gfxrecon::decode::VulkanConsumer& found_begin_;
};

// A very short-lived state but modeled for completeness.
class LookingForBegin : public gfxrecon::decode::VulkanConsumer
{
public:
    LookingForBegin(StateMachine& parent, gfxrecon::decode::VulkanConsumer& found_begin);

    void Process_vkBeginCommandBuffer(
    const gfxrecon::decode::ApiCallInfo& call_info,
    VkResult                             returnValue,
    gfxrecon::format::HandleId           commandBuffer,
    gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkCommandBufferBeginInfo>*
    pBeginInfo) override;

private:
    StateMachine&                     parent_;
    gfxrecon::decode::VulkanConsumer& found_begin_;
};
