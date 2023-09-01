/*
 Copyright 2021 Google LLC

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

bool Disassemble(const uint8_t*                             shader_memory,
                 uint64_t                                   shader_address,
                 size_t                                     shader_max_size,
                 std::vector<ShaderInstruction>*            instructions,
                 std::function<std::string(uint64_t index)> on_emit,
                 std::string&                               output,
                 ILog*                                      log_ptr)
{
    return false;
}

void Disassembly::Init(const uint8_t* data, uint64_t address, size_t max_size, ILog* log_ptr) {}

std::string Disassembly::GetInstructionText(uint32_t index) const
{
    return "";
}

uint64_t Disassembly::GetShaderSize() const
{
    return 0;
}

}  // namespace Dive
