/*
 Copyright 2020 Google LLC

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
#include <string>
#include <vector>

#include "log.h"

namespace Dive
{
class ShaderInstruction
{
public:
    uint32_t GetOpcode() const { return m_opcode; }
    void     SetOpcode(uint32_t opcode) { m_opcode = opcode; }

    uint64_t GetFlags() const { return m_flags; }
    void     SetFlags(uint64_t flags) { m_flags = flags; }

    uint64_t GetAddress() const { return m_address; }
    void     SetAddress(uint64_t address) { m_address = address; }

    uint32_t GetImmediate() const { return m_immediate; }
    void     SetImmediate(uint32_t value) { m_immediate = value; }

private:
    // The raw opcode from the instruction
    uint32_t m_opcode;
    // NOTE: For now we're not including operands, because we have no use
    // for them, and there are different kinds to be kept track of. But
    // we do need the immediate value in case of a branch.
    uint64_t m_flags;
    uint32_t m_immediate = UINT32_MAX;

    uint64_t m_address;
};

class Disassembly
{
public:
    void Init(const uint8_t* data, uint64_t address, size_t max_size, ILog* log = nullptr);

    std::string       GetListing() const { return m_listing; }
    ShaderInstruction GetInstruction(int32_t index) const { return m_instructions[index]; };
    size_t            GetNumInstructions() const { return m_instructions.size(); }
    std::string       GetInstructionText(uint32_t index) const;
    uint64_t          GetShaderSize() const;

private:
    std::string                    m_listing;
    std::vector<ShaderInstruction> m_instructions;
};

bool Disassemble(const uint8_t*                             shader_memory,
                 uint64_t                                   shader_address,
                 size_t                                     shader_size,
                 std::vector<ShaderInstruction>*            instructions,
                 std::function<std::string(uint64_t index)> on_emit,
                 std::string&                               output,
                 ILog*                                      log_ptr = nullptr);
}  // namespace Dive
