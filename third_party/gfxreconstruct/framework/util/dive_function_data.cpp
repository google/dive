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

#include "dive_function_data.h"

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(util)

DiveFunctionData::DiveFunctionData(const std::string& name, uint32_t cmd_buffer_index, uint64_t block_index, const nlohmann::ordered_json& args) : m_name(name), m_cmd_buffer_index(cmd_buffer_index), m_block_index(block_index), m_args(args){}

const std::string& DiveFunctionData::GetFunctionName() const {
    return m_name;
}
uint32_t DiveFunctionData::GetCmdBufferIndex() const {
    return m_cmd_buffer_index;
}
uint64_t DiveFunctionData::GetBlockIndex() const{
    return m_block_index;
}
const nlohmann::ordered_json DiveFunctionData::GetArgs() const {
    return m_args;
}

GFXRECON_END_NAMESPACE(util)
GFXRECON_END_NAMESPACE(gfxrecon)
