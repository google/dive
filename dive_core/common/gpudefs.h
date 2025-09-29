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
#include <array>
#include <stdint.h>

namespace Dive
{

enum class IbType : uint8_t
{
    kNormal,
    kCall,
    kChain,
    kContextSwitchIb,
    kDrawState,
    kBinPrefix,
    kBinCommon,
    kFixedStrideDrawTable,
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

//--------------------------------------------------------------------------------------------------
// Trace Statistics
// Number of draw calls in BINNING, DIRECT, and TILED
//  With LRZ enabled, and LRZ Write enabled
//  With depth test enabled
//  With depth write enabled
//  With EarlyZ, LateZ enabled
//  With CullMode enabled
// Number of CLEARs  (???)
// List of different unique viewports used
// Min, Max, Total number of indices rendered
// Number of tiles + size of tiles + location of tiles
// Number of unique shaders, per stage (BINNING vs TILING)
//  Min, Max, and total instruction counts
//  Min, Max, and total GPR count
// Number of CPEventWrites
//  RESOLVE, FLUSH_COLOR, FLUSH_DEPTH, INVALIDATE_COLOR, INVALIDATE_DEPTH
// Number of CpWaitForIdle()
// Number of prefetches
// Number of loads/stores to GMEM
struct Stats
{
    enum Type : uint32_t
    {
        kNumBinnigPasses,
        kNumTilingPasses,
        kBinningDraws,
        kDirectDraws,
        kTiledDraws,
        kDispatches,
        kWaitMemWrites,
        kWaitForIdle,
        kWaitForMe,
        kDepthTestEnabled,
        kDepthWriteEnabled,
        kEarlyZ,
        kLateZ,
        kEarlyLRZLateZ,
        kLrzEnabled,
        kLrzWriteEnabled,
        kCullModeEnabled,
        kTotalIndices,
        kMinIndices,
        kMaxIndices,
        kMedianIndices,
        kShaders,
        kBinningVS,
        kNonBinningVS,
        kNonVS,
        kTotalInstructions,
        kMinInstructions,
        kMaxInstructions,
        kMedianInstructions,
        kTotalGPRs,
        kMinGPRs,
        kMaxGPRs,
        kMedianGPRs,
        kTotalResolves,
        kGmemToSysmemResolves,
        kGmemToSysmemAndClearGmemResolves,
        kClearGmemResolves,
        kSysmemToGmemResolves,
        kNumStats
    };
};

constexpr std::array kStatMap = {
    std::pair(Stats::kNumBinnigPasses, "Num Binning Passes"),
    std::pair(Stats::kNumTilingPasses, "Num Tiling Passes"),
    std::pair(Stats::kBinningDraws, "Num Draws (BINNING)"),
    std::pair(Stats::kDirectDraws, "Num Draws (DIRECT)"),
    std::pair(Stats::kTiledDraws, "Num Draws (TILED)"),
    std::pair(Stats::kDispatches, "Num Dispatches"),
    std::pair(Stats::kWaitMemWrites, "Num WaitMemWrites"),
    std::pair(Stats::kWaitForIdle, "Num WaitForIdle"),
    std::pair(Stats::kWaitForMe, "Num WaitForMe"),
    std::pair(Stats::kDepthTestEnabled, "Num Draws with Depth Test Enabled"),
    std::pair(Stats::kDepthWriteEnabled, "Num Draws with Depth Write Enabled"),
    std::pair(Stats::kEarlyZ, "Num Draws with EarlyZ"),
    std::pair(Stats::kLateZ, "Num Draws with LateZ"),
    std::pair(Stats::kEarlyLRZLateZ, "Num Draws with Early LRZ & LateZ"),
    std::pair(Stats::kLrzEnabled, "Num Draws with LRZ Enabled"),
    std::pair(Stats::kLrzWriteEnabled, "Num Draws with LRZ Write Enabled"),
    std::pair(Stats::kCullModeEnabled, "Num Draws with culling enabled"),
    std::pair(Stats::kTotalIndices, "Total indices in all draws (includes non-indexed draws)"),
    std::pair(Stats::kMinIndices, "\tMin indices in a single draw"),
    std::pair(Stats::kMaxIndices, "\tMax indices in a single draw"),
    std::pair(Stats::kMedianIndices, "\tMedian indices in a single draw"),
    std::pair(Stats::kShaders, "Number of unique shaders"),
    std::pair(Stats::kBinningVS, "\tNumber of BINNING VS"),
    std::pair(Stats::kNonBinningVS, "\tNumber of non-BINNING VS"),
    std::pair(Stats::kNonVS, "\tNumber of non-VS Shaders"),
    std::pair(Stats::kTotalInstructions, "Total instructions in all shaders"),
    std::pair(Stats::kMinInstructions, "\tMin instructions in a single shader"),
    std::pair(Stats::kMaxInstructions, "\tMax instructions in a single shader"),
    std::pair(Stats::kMedianInstructions, "\tMedian instructions in a single shader"),
    std::pair(Stats::kTotalGPRs, "Total GPRs in all shaders"),
    std::pair(Stats::kMinGPRs, "\tMin GPRs in a single shader"),
    std::pair(Stats::kMaxGPRs, "\tMax GPRs in a single shader"),
    std::pair(Stats::kMedianGPRs, "\tMedian GPRs in a single shader"),
    std::pair(Stats::kTotalResolves, "Total resolves"),
    std::pair(Stats::kGmemToSysmemResolves, "\tGmem to SysMem Resolves"),
    std::pair(Stats::kGmemToSysmemAndClearGmemResolves, "\tGmem to SysMem Resolves and Clear Gmem"),
    std::pair(Stats::kClearGmemResolves, "\tGmem Clears"),
    std::pair(Stats::kSysmemToGmemResolves, "\tSysMem to Gmem Resolves"),
};

enum ViewPortStats
{
    kViewport,
    kViewport_x,
    kViewport_y,
    kViewport_width,
    kViewport_height,
    kViewport_minDepth,
    kViewport_maxDepth,
    kNumViewportStats
};

inline const char *viewport_stats_desc[kNumViewportStats] = {
    "Viewports", "x", "y", "width", "height", "minDepth", "maxDepth",
};

enum WindowScissorStats
{
    kWindowScissors,
    kWindowScissors_tl_x,
    kWindowScissors_br_x,
    kWindowScissors_tl_y,
    kWindowScissors_br_y,
    kWindowScissors_Width,
    kWindowScissors_Height,
    kNumWindowScissorStats
};

inline const char *window_scissor_stats_desc[kNumWindowScissorStats] = {
    "Window scissors", "tl_x", "br_x", "tl_y", "br_y", "Width", "Height"
};

}  // namespace Dive