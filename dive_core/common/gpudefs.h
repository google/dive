/*
 Copyright 2019 Google LLC

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

// Warning: This is a common file that is shared with the Dive GUI tool!
#pragma once
#include <stdint.h>

namespace Dive
{

enum class IbType : uint8_t
{
    kNormal,
    kCall,
    kChain,
};

enum class ShaderStage : uint8_t
{
    kShaderStageCs,
    kShaderStageGs,
    kShaderStageHs,
    kShaderStagePs,
    kShaderStageVs,
    kShaderStageCount
};
const uint32_t kShaderStageCount = (uint32_t)ShaderStage::kShaderStageCount;

// There are a maximum number of 2^kNumEventsBits events possible
// This is set to an absurdly high number
const uint32_t kMaxNumEventsBits = 19;

// This is configured for a Vega64
// TODO: Not bothering to make this configurable yet, because it's possible Dive will be supporting
// a Navi architecture, in which case some of these variables do not mean quite the same thing
constexpr uint32_t kNumSe = 4;
constexpr uint32_t kNumSh = 1;
constexpr uint32_t kNumCusPerSh = 16;
constexpr uint32_t kNumSimdsPerCu = 4;
constexpr uint32_t kNumWavefrontsPerSimd = 10;
constexpr uint32_t kNumHardwareContext = 8;
constexpr double   kClockMhz = 1138.0;
constexpr uint64_t kPixelsPerWavefront = 64;

//--------------------------------------------------------------------------------------------------
inline uint16_t DecodeVGPRs(uint32_t pgm_rsrc1)
{
    // The vgpr value is encoded in blocks of 4 and is 0-based, so need to add 1 and multiply by 4.
    return ((pgm_rsrc1 & 0x3F) + 1) * 4;
}

//--------------------------------------------------------------------------------------------------
inline uint16_t DecodeSGPRs(uint32_t pgm_rsrc1)
{
    // The sgpr value is encoded in blocks of 8 and is 0-based, so need to add 1 and multiply by 8.
    // However, on gfx9, the allocation granularity of sgprs is 16, so therefore need to round up
    // to the nearest multiple of 16.
    // This means the mapping from pgm_rsrc1 field value to actual register value is:
    //  0 - 16, 1 - 16, 2 - 32, 3 - 32, 4 - 48, 5 - 48, etc.
    uint16_t sgprs = (((pgm_rsrc1 >> 6) & 0xF) + 1) * 8;
    sgprs = ((sgprs + 15) & ~15);
    return sgprs;
}

}  // namespace Dive