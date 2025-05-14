#pragma once

#include <functional>

#include "gfxreconstruct/framework/decode/api_decoder.h"
#include "gfxreconstruct/framework/decode/struct_pointer_decoder.h"
#include "gfxreconstruct/framework/format/format.h"
#include "gfxreconstruct/framework/generated/generated_vulkan_consumer.h"
#include "gfxreconstruct/framework/generated/generated_vulkan_struct_decoders.h"

#include "dump_entry.h"
#include "states.h"

class StateMachine : public gfxrecon::decode::VulkanConsumer
{
public:
    using RejectingFunction = std::function<void()>;
    using AcceptingFunction = std::function<void(DumpEntry)>;

    StateMachine(gfxrecon::format::HandleId command_buffer,
                 AcceptingFunction          accept,
                 RejectingFunction          reject);

    // Hooks for states
    void                        Transition(gfxrecon::decode::VulkanConsumer& new_state);
    DumpEntry&                  dump_entry();
    gfxrecon::format::HandleId& command_buffer();
    void                        Done();

    void Process_vkBeginCommandBuffer(
    const gfxrecon::decode::ApiCallInfo& call_info,
    VkResult                             returnValue,
    gfxrecon::format::HandleId           commandBuffer,
    gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkCommandBufferBeginInfo>*
    pBeginInfo) override;

    void Process_vkCmdBeginRenderPass(
    const gfxrecon::decode::ApiCallInfo& call_info,
    gfxrecon::format::HandleId           commandBuffer,
    gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkRenderPassBeginInfo>*
                      pRenderPassBegin,
    VkSubpassContents contents) override;

    void Process_vkCmdDraw(const gfxrecon::decode::ApiCallInfo& call_info,
                           gfxrecon::format::HandleId           commandBuffer,
                           uint32_t                             vertexCount,
                           uint32_t                             instanceCount,
                           uint32_t                             firstVertex,
                           uint32_t                             firstInstance) override;

    void Process_vkCmdEndRenderPass(const gfxrecon::decode::ApiCallInfo& call_info,
                                    gfxrecon::format::HandleId           commandBuffer) override;

    void Process_vkQueueSubmit(
    const gfxrecon::decode::ApiCallInfo&                                            call_info,
    VkResult                                                                        returnValue,
    gfxrecon::format::HandleId                                                      queue,
    uint32_t                                                                        submitCount,
    gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkSubmitInfo>* pSubmits,
    gfxrecon::format::HandleId                                                      fence) override;

private:
    // For filtering Vulkan calls for relevant info
    gfxrecon::format::HandleId command_buffer_;
    // Something went wrong; throw away all state
    RejectingFunction reject_;
    // Complete DumpEntry; propagate
    AcceptingFunction accept_;

    LookingForDraw        find_draw_;
    LookingForRenderPass  find_render_pass_;
    LookingForBegin       begin_state_;

    // Partial struct that will be filled as info is parsed. Once complete, accept_();
    DumpEntry dump_entry_{};

    gfxrecon::decode::VulkanConsumer* state_ = nullptr;
};
