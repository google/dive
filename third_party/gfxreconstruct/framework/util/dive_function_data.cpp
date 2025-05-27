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

DiveFunctionData::DiveFunctionData(std::string name, uint32_t cmd_buffer_index, uint64_t block_index, nlohmann::ordered_json args) : name_(name), cmd_buffer_index_(cmd_buffer_index), block_index_(block_index), args_(args){}

std::string DiveFunctionData::GetFunctionName(){
    return name_;
}
uint32_t DiveFunctionData::GetCmdBufferIndex(){
    return cmd_buffer_index_;
}
uint64_t DiveFunctionData::GetBlockIndex(){
    return block_index_;
}
nlohmann::ordered_json DiveFunctionData::GetArgs(){
    return args_;
}

GFXRECON_END_NAMESPACE(util)
GFXRECON_END_NAMESPACE(gfxrecon)
