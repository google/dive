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

#ifndef DIVE_FUNCTION_DATA_H
#define DIVE_FUNCTION_DATA_H

#include "util/defines.h"
#include "../../external/nlohmann/include/nlohmann/json.hpp"
#include <cstdint>
#include <string>


GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(util)

class DiveFunctionData
{
  public:
  DiveFunctionData(std::string name, uint32_t cmd_buffer_index, uint64_t block_index, nlohmann::ordered_json args);

  std::string GetFunctionName();
  uint32_t GetCmdBufferIndex();
  uint64_t GetBlockIndex();
  nlohmann::ordered_json GetArgs();

  std::string name_;
  uint32_t cmd_buffer_index_;
  uint64_t block_index_;
  nlohmann::ordered_json args_;
};

GFXRECON_END_NAMESPACE(util)
GFXRECON_END_NAMESPACE(gfxrecon)
#endif // DIVE_FUNCTION_DATA_HH
