
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

#include "vulkan/vulkan_core.h"
#include <array>
#include <vector>
#include "dive_core/capture_event_info.h"
#include "dive_core/data_core.h"

namespace Dive
{

#define CHECK_AND_TRACK_STATE_1(stats_enum, state)                   \
    if (event_state_it->Is##state##Set() && event_state_it->state()) \
        stats_list[stats_enum]++;

#define CHECK_AND_TRACK_STATE_2(stats_enum, state1, state2)                       \
    if (event_state_it->Is##state1##Set() && event_state_it->Is##state2##Set() && \
        event_state_it->state1() && event_state_it->state2())                     \
        stats_list[stats_enum]++;

#define CHECK_AND_TRACK_STATE_3(stats_enum, state1, state2, state3)               \
    if (event_state_it->Is##state1##Set() && event_state_it->Is##state2##Set() && \
        event_state_it->Is##state3##Set() && event_state_it->state1() &&          \
        event_state_it->state2() && event_state_it->state3())                     \
        stats_list[stats_enum]++;

#define CHECK_AND_TRACK_STATE_EQUAL(stats_enum, state, state_value) \
    if (event_state_it->Is##state##Set())                           \
        if (event_state_it->state() == state_value)                 \
            stats_list[stats_enum]++;

#define CHECK_AND_TRACK_STATE_NOT_EQUAL(stats_enum, state, state_value) \
    if (event_state_it->Is##state##Set())                               \
        if (event_state_it->state() != state_value)                     \
            stats_list[stats_enum]++;

#define FUNC_CHOOSER(_f1, _f2, _f3, _f4, ...) _f4
#define MSVC_WORKAROUND(argsWithParentheses) FUNC_CHOOSER argsWithParentheses

// Trailing , is workaround for GCC/CLANG, suppressing false positive error "ISO C99 requires rest
// arguments to be used" MSVC_WORKAROUND is workaround for MSVC for counting # of arguments
// correctly
#define CHECK_AND_TRACK_STATE_N(...) \
    MSVC_WORKAROUND(                 \
    (__VA_ARGS__, CHECK_AND_TRACK_STATE_3, CHECK_AND_TRACK_STATE_2, CHECK_AND_TRACK_STATE_1, ))
#define CHECK_AND_TRACK_STATE(stats_enum, ...) \
    CHECK_AND_TRACK_STATE_N(__VA_ARGS__)(stats_enum, __VA_ARGS__)

#define GATHER_TOTAL_MIN_MAX_MEDIAN(array_name, type)                                         \
    {                                                                                         \
        std::sort(array_name.begin(), array_name.end());                                      \
        size_t n = array_name.size();                                                         \
        if (n % 2 != 0)                                                                       \
        {                                                                                     \
            stats_list[Dive::Stats::kMedian##type] = array_name[n / 2];                       \
        }                                                                                     \
        else                                                                                  \
        {                                                                                     \
            auto mid1 = array_name[n / 2 - 1];                                                \
            auto mid2 = array_name[n / 2];                                                    \
            stats_list[Dive::Stats::kMedian##type] = (uint64_t)((float)(mid1 + mid2) / 2.0f); \
        }                                                                                     \
        stats_list[Dive::Stats::kMin##type] = *std::min_element(array_name.begin(),           \
                                                                array_name.end());            \
        stats_list[Dive::Stats::kMax##type] = *std::max_element(array_name.begin(),           \
                                                                array_name.end());            \
        stats_list[Dive::Stats::kTotal##type] = std::accumulate(array_name.begin(),           \
                                                                array_name.end(),             \
                                                                (uint64_t)0);                 \
    }

#define GATHER_RESOLVES(type)                         \
    do                                                \
    {                                                 \
        stats_list[Dive::Stats::kTotalResolves]++;    \
        stats_list[Dive::Stats::k##type##Resolves]++; \
    } while (0)

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
    std::array<uint64_t, Dive::Stats::kNumStats> stats_list = {};

    std::vector<uint32_t> event_num_indices;

    std::vector<Dive::ShaderReference> shader_ref_set;
    std::vector<Viewport>              viewports;
    std::vector<WindowScissor>         window_scissors;

    uint32_t num_binning_passes = 0;
    uint32_t num_tiling_passes = 0;
};

class TraceStats
{
public:
    TraceStats() = default;
    ~TraceStats() = default;

    // Gather the trace statistics from the metadata
    void GatherTraceStats(const Dive::CaptureMetadata &meta_data);

    // Print the capture statistics to the output stream
    void PrintTraceStats(std::ostream &ostream);

    // Get the capture statistics
    const CaptureStats &GetCaptureStats() const { return m_capture_stats; };

private:
    CaptureStats m_capture_stats;
};

}  // namespace Dive
