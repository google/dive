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

#include "state_machine.h"

#include <functional>
#include <iostream>

namespace Dive::gfxr
{

StateMachine::StateMachine(gfxrecon::format::HandleId command_buffer,
                           AcceptingFunction          accept,
                           RejectingFunction          reject) :
    command_buffer_(command_buffer),
    accept_(accept),
    reject_(reject),
    find_draw_(*this, find_render_pass_),
    find_render_pass_(*this, find_draw_),
    begin_state_(*this, find_render_pass_),
    state_(&begin_state_)
{
}

void StateMachine::Transition(gfxrecon::decode::VulkanConsumer& new_state)
{
    state_ = &new_state;
}

DumpEntry& StateMachine::dump_entry()
{
    return dump_entry_;
}

const gfxrecon::format::HandleId& StateMachine::command_buffer()
{
    return command_buffer_;
}

void StateMachine::Done()
{
    if (dump_entry_.IsComplete())
    {
        std::cerr << "Accept! ID=" << command_buffer_ << '\n';
        accept_(std::move(dump_entry_));
    }
    else
    {
        std::cerr << "Reject! ID=" << command_buffer_ << '\n';
        reject_();
    }
}

gfxrecon::decode::VulkanConsumer& StateMachine::state()
{
    return *state_;
}

}  // namespace Dive::gfxr
