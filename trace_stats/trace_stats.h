
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

#include "vulkan/vulkan_core.h"
#include <array>
#include <set>
#include <vector>
#include "dive_core/capture_event_info.h"
#include "dive_core/data_core.h"

namespace Dive
{

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
        kNumBinningPasses,
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
    std::pair(Stats::kNumBinningPasses, "Num Binning Passes"),
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

struct Viewport
{
    VkViewport m_vk_viewport;
    bool       operator<(const Viewport &other) const
    {
        if (m_vk_viewport.x != other.m_vk_viewport.x)
            return m_vk_viewport.x < other.m_vk_viewport.x;
        if (m_vk_viewport.y != other.m_vk_viewport.y)
            return m_vk_viewport.y < other.m_vk_viewport.y;
        if (m_vk_viewport.width != other.m_vk_viewport.width)
            return m_vk_viewport.width < other.m_vk_viewport.width;
        if (m_vk_viewport.height != other.m_vk_viewport.height)
            return m_vk_viewport.height < other.m_vk_viewport.height;
        if (m_vk_viewport.minDepth != other.m_vk_viewport.minDepth)
            return m_vk_viewport.minDepth < other.m_vk_viewport.minDepth;
        return m_vk_viewport.maxDepth < other.m_vk_viewport.maxDepth;
    }
};

struct WindowScissor
{
    uint32_t m_tl_x;
    uint32_t m_tl_y;
    uint32_t m_br_x;
    uint32_t m_br_y;
    bool     operator<(const WindowScissor &other) const
    {
        uint32_t area = (m_br_x - m_tl_x) * (m_br_y - m_tl_y);
        uint32_t other_area = (other.m_br_x - other.m_tl_x) * (other.m_br_y - other.m_tl_y);
        if (area != other_area)
            return area < other_area;
        if (m_tl_y != other.m_tl_y)
            return m_tl_y < other.m_tl_y;
        if (m_tl_x != other.m_tl_x)
            return m_tl_x < other.m_tl_x;
        if (m_br_y != other.m_br_y)
            return m_br_y < other.m_br_y;
        return m_br_x < other.m_br_x;
    }
};

// ---------------------------------------------------------------------
// Statistics Container
// ---------------------------------------------------------------------

struct CaptureStats
{
    std::array<uint64_t, Dive::Stats::kNumStats> m_stats_list = {};

    std::vector<uint32_t> m_event_num_indices;

    std::set<Dive::ShaderReference> m_shader_ref_set;
    std::set<Viewport>              m_viewports;
    std::set<WindowScissor>         m_window_scissors;

    uint32_t m_num_binning_passes = 0;
    uint32_t m_num_tiling_passes = 0;
};

class TraceStats
{
public:
    TraceStats() = default;
    ~TraceStats() = default;

    // Gather the trace statistics from the metadata
    void GatherTraceStats(const Dive::CaptureMetadata &meta_data, CaptureStats &capture_stats);

    // Print the capture statistics to the output stream
    void PrintTraceStats(const CaptureStats &capture_stats, std::ostream &ostream);
};

}  // namespace Dive
