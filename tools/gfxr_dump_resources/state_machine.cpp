#include "state_machine.h"

#include <functional>

StateMachine::StateMachine(gfxrecon::format::HandleId command_buffer,
                           AcceptingFunction          accept,
                           RejectingFunction          reject) :
    command_buffer_(command_buffer),
    accept_(accept),
    reject_(reject),
    find_queue_submit_(*this),
    begin_state_(*this, find_queue_submit_),
    state_(&begin_state_)
{
}

void StateMachine::Transition(gfxrecon::decode::VulkanConsumer& new_state)
{
    state_ = &new_state;
    // TODO call TransitionTo
}

DumpEntry& StateMachine::dump_entry()
{
    return dump_entry_;
}

gfxrecon::format::HandleId& StateMachine::command_buffer()
{
    return command_buffer_;
}

void StateMachine::Accept()
{
    // todo assert complete
    accept_(std::move(dump_entry_));
}

void StateMachine::Reject()
{
    reject_();
}

void StateMachine::Process_vkBeginCommandBuffer(
const gfxrecon::decode::ApiCallInfo& call_info,
VkResult                             returnValue,
gfxrecon::format::HandleId           commandBuffer,
gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkCommandBufferBeginInfo>*
pBeginInfo)
{
    // Consumer will never be called if state != begin_state_
    assert(state_ == &begin_state_);
    state_->Process_vkBeginCommandBuffer(call_info, returnValue, commandBuffer, pBeginInfo);
}

void StateMachine::Process_vkQueueSubmit(
const gfxrecon::decode::ApiCallInfo&                                            call_info,
VkResult                                                                        returnValue,
gfxrecon::format::HandleId                                                      queue,
uint32_t                                                                        submitCount,
gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkSubmitInfo>* pSubmits,
gfxrecon::format::HandleId                                                      fence)
{
    state_->Process_vkQueueSubmit(call_info, returnValue, queue, submitCount, pSubmits, fence);
}
