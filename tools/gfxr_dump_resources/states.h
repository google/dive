
#pragma once

#include "gfxreconstruct/framework/decode/api_decoder.h"
#include "gfxreconstruct/framework/decode/struct_pointer_decoder.h"
#include "gfxreconstruct/framework/format/format.h"
#include "gfxreconstruct/framework/generated/generated_vulkan_consumer.h"
#include "gfxreconstruct/framework/generated/generated_vulkan_struct_decoders.h"

// forward decl to break recursive includes
class StateMachine;

class LookingForQueueSubmit : public gfxrecon::decode::VulkanConsumer
{
public:
// TODO inject accept function so it's more readable as a possible action?
    LookingForQueueSubmit(StateMachine& parent);

    void Process_vkQueueSubmit(
    const gfxrecon::decode::ApiCallInfo&                                            call_info,
    VkResult                                                                        returnValue,
    gfxrecon::format::HandleId                                                      queue,
    uint32_t                                                                        submitCount,
    gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkSubmitInfo>* pSubmits,
    gfxrecon::format::HandleId                                                      fence) override;

private:
    StateMachine& parent_;
};

class LookingForBegin : public gfxrecon::decode::VulkanConsumer
{
public:
    LookingForBegin(StateMachine& parent, gfxrecon::decode::VulkanConsumer& find_queue_submit);

    void Process_vkBeginCommandBuffer(
    const gfxrecon::decode::ApiCallInfo& call_info,
    VkResult                             returnValue,
    gfxrecon::format::HandleId           commandBuffer,
    gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkCommandBufferBeginInfo>*
    pBeginInfo) override;

private:
    StateMachine&                     parent_;
    gfxrecon::decode::VulkanConsumer& find_queue_submit_;
};