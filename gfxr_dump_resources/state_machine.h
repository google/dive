/*
 Copyright 2025 Google LLC

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

#pragma once

#include <functional>

#include "third_party/gfxreconstruct/framework/format/format.h"
#include "third_party/gfxreconstruct/framework/generated/generated_vulkan_consumer.h"

#include "dump_entry.h"
#include "states.h"

namespace Dive::gfxr
{

// A state machine that, given a sequence of Vulkan calls for a particular command buffer, validates
// the sequence and accumulates relevant dumpable information. Since we're looking for certain
// functions in a certain order with certain constraints it made sense to model this as a state
// machine.
//
// The actual command handling is deferred to the current state of the state machine. See states.h.
//
// Each command buffer ought to have its own state machine to avoid problems that could arise if two
// command buffers are interleaved in the capture. This is handled by DumpResourcesBuilderConsumer.
//
// States:
//
// - LookingForBeginCommandBuffer:
//   - Start here.
//   - When vkBeginCommandBuffer is found, record the block index and transition to
//   LookingForBeginRenderPass.
//
// - LookingForBeginRenderPass:
//   - when vkCmdBeginRenderPass is found, record the block index and transition to LookingForDraw.
//   - When vkQueueSubmit is found, record the block index. Then, either accept or reject depending
//   on whether the dumpable is complete
//
// - LookingForDraw:
//    - When a vkCmdDraw* call is found, record the block index. Stay in this state to accumulate
//    more draw calls.
//    - When vkCmdEndRenderPass is found, record the block index and transition to
//    LookingForBeginRenderPass
//
// The state machine presents its results via one of two functions:
//
// 1. Accept: the dumpable is complete and can be used.
// 2. Reject: the dumpable is incomplete and should be discard.
class StateMachine
{
public:
    using RejectingFunction = std::function<void()>;
    using AcceptingFunction = std::function<void(DumpEntry)>;

    // `command_buffer` is the GFXR ID for the command buffer that is being filtering for.
    //
    // `accept` will be run when a complete dumpable is found. `reject` will be run when the current
    // state of this command buffer should be discard.
    StateMachine(gfxrecon::format::HandleId command_buffer,
                 AcceptingFunction          accept,
                 RejectingFunction          reject);

    // Get the current state machine state.
    gfxrecon::decode::VulkanConsumer& state();

private:
    // States are friends to avoid certain methods being public unnecessarily.
    friend class LookingForDraw;
    friend class LookingForBeginRenderPass;
    friend class LookingForBeginCommandBuffer;

    // Change the state of the state machine.
    void Transition(gfxrecon::decode::VulkanConsumer& new_state);
    // Call accept for reject depending on whether the dumpable is complete.
    void Done();
    // Get the working state for the dumpable.
    DumpEntry& dump_entry();
    // Get the GFXR command buffer that we're looking for.
    const gfxrecon::format::HandleId& command_buffer();

    // For filtering Vulkan calls for relevant info.
    gfxrecon::format::HandleId command_buffer_;
    // Something went wrong; halt the state machine throw away state so far.
    RejectingFunction reject_;
    // DumpEntry is complete and should be propagated.
    AcceptingFunction accept_;

    // States of the state machine
    LookingForDraw               find_draw_;
    LookingForBeginRenderPass    find_render_pass_;
    LookingForBeginCommandBuffer begin_state_;

    // Partial struct that will be filled as info is parsed. Once complete, accept_();
    DumpEntry dump_entry_{};

    // Current state of the state machine
    gfxrecon::decode::VulkanConsumer* state_ = nullptr;
};

}  // namespace Dive::gfxr
