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

#include "shader_disassembly.h"

namespace Dive
{

Disassembly::Disassembly(const IMemoryManager& mem_manager,
                         uint32_t              submit_index,
                         uint64_t              address,
                         ILog*                 log) :
    m_mem_manager(mem_manager),
    m_submit_index(submit_index),
    m_address(address),
    m_log(log)
{
}

void Disassembly::Disassemble() const
{
    m_disassembled_data.m_listing = "Shader disassembly not available.";
}

bool Disassemble(const uint8_t*                             shader_memory,
                 uint64_t                                   shader_address,
                 size_t                                     shader_size,
                 std::vector<ShaderInstruction>*            instructions,
                 std::function<std::string(uint64_t index)> on_emit,
                 std::string&                               output,
                 ILog*                                      log_ptr)
{
    output = "Shader disassembly not available.";
    return false;
}

}  // namespace Dive
