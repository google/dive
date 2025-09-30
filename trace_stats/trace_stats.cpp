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

#include "trace_stats.h"
#include "dive_core/event_state.h"

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

//--------------------------------------------------------------------------------------------------
void TraceStats::GatherTraceStats(const Dive::CaptureMetadata &meta_data)
{
    m_capture_stats = CaptureStats();  // Reset any previous stats

    std::array<uint64_t, Dive::Stats::kNumStats> &stats_list = m_capture_stats.m_stats_list;

    size_t                      event_count = meta_data.m_event_info.size();
    const Dive::EventStateInfo &event_state = meta_data.m_event_state;

    Dive::RenderModeType      cur_type = Dive::RenderModeType::kUnknown;
    [[maybe_unused]] uint32_t num_draws_in_pass = 0;

    for (size_t i = 0; i < event_count; ++i)
    {
        const Dive::EventInfo &info = meta_data.m_event_info[i];

        if (info.m_render_mode != cur_type)
        {
            if (info.m_render_mode == Dive::RenderModeType::kBinning)
                m_capture_stats.m_num_binning_passes++;
            else if (info.m_render_mode == Dive::RenderModeType::kTiled)
                m_capture_stats.m_num_tiling_passes++;

            cur_type = info.m_render_mode;
            num_draws_in_pass = 0;
        }

        const auto GatherResolves = [&](Stats::Type resolve_type) {
            stats_list[Stats::kTotalResolves]++;
            stats_list[resolve_type]++;
        };

        if (info.m_type == Dive::EventInfo::EventType::kDispatch)
            stats_list[Stats::kDispatches]++;
        else if (info.m_type == Dive::EventInfo::EventType::kWaitMemWrites)
            stats_list[Stats::kWaitMemWrites]++;
        else if (info.m_type == Dive::EventInfo::EventType::kWaitForIdle)
            stats_list[Stats::kWaitForIdle]++;
        else if (info.m_type == Dive::EventInfo::EventType::kWaitForMe)
            stats_list[Stats::kWaitForMe]++;
        else if (info.m_type == Dive::EventInfo::EventType::kGmemToSysmemResolve)
            GatherResolves(Stats::kGmemToSysmemResolves);
        else if (info.m_type == Dive::EventInfo::EventType::kGmemToSysMemResolveAndClearGmem)
            GatherResolves(Stats::kGmemToSysmemAndClearGmemResolves);
        else if (info.m_type == Dive::EventInfo::EventType::kClearGmem)
            GatherResolves(Stats::kClearGmemResolves);
        else if (info.m_type == Dive::EventInfo::EventType::kSysmemToGmemResolve)
            GatherResolves(Stats::kSysmemToGmemResolves);
        else if (info.m_type == Dive::EventInfo::EventType::kDraw)
        {
            if (info.m_render_mode == Dive::RenderModeType::kBinning)
                stats_list[Dive::Stats::kBinningDraws]++;
            else if (info.m_render_mode == Dive::RenderModeType::kDirect)
                stats_list[Dive::Stats::kDirectDraws]++;
            else if (info.m_render_mode == Dive::RenderModeType::kTiled)
                stats_list[Dive::Stats::kTiledDraws]++;

            num_draws_in_pass++;
            if (info.m_num_indices != 0)
                m_capture_stats.m_event_num_indices.push_back(info.m_num_indices);

            const uint32_t event_id = static_cast<uint32_t>(i);
            auto event_state_it = event_state.find(static_cast<Dive::EventStateId>(event_id));

            if (info.m_render_mode != Dive::RenderModeType::kBinning)
            {
                CHECK_AND_TRACK_STATE(Dive::Stats::kDepthTestEnabled, DepthTestEnabled);
                CHECK_AND_TRACK_STATE(Dive::Stats::kDepthWriteEnabled,
                                      DepthTestEnabled,
                                      DepthWriteEnabled);
                if (event_state_it->DepthTestEnabled())
                {
                    CHECK_AND_TRACK_STATE_EQUAL(Dive::Stats::kEarlyZ, ZTestMode, A6XX_EARLY_Z);
                    CHECK_AND_TRACK_STATE_EQUAL(Dive::Stats::kLateZ, ZTestMode, A6XX_LATE_Z);
                    CHECK_AND_TRACK_STATE_EQUAL(Dive::Stats::kEarlyLRZLateZ,
                                                ZTestMode,
                                                A6XX_EARLY_LRZ_LATE_Z);
                }
            }
            if ((info.m_render_mode == Dive::RenderModeType::kDirect) ||
                (info.m_render_mode == Dive::RenderModeType::kBinning))
            {
                CHECK_AND_TRACK_STATE(Dive::Stats::kLrzEnabled, DepthTestEnabled, LRZEnabled);
                CHECK_AND_TRACK_STATE(Dive::Stats::kLrzWriteEnabled,
                                      DepthTestEnabled,
                                      DepthWriteEnabled,
                                      LRZWrite);
            }

            CHECK_AND_TRACK_STATE_NOT_EQUAL(Dive::Stats::kCullModeEnabled,
                                            CullMode,
                                            VK_CULL_MODE_NONE);

            for (uint32_t v = 0; v < 16; ++v)
            {
                if (event_state_it->IsViewportSet(v))
                {
                    Viewport viewport;
                    viewport.m_vk_viewport = event_state_it->Viewport(v);
                    m_capture_stats.m_viewports.push_back(viewport);
                }
            }

            if (event_state_it->IsWindowScissorTLXSet() &&
                event_state_it->IsWindowScissorTLYSet() &&
                event_state_it->IsWindowScissorBRXSet() && event_state_it->IsWindowScissorBRYSet())
            {
                WindowScissor window_scissor;
                window_scissor.m_tl_x = event_state_it->WindowScissorTLX();
                window_scissor.m_tl_y = event_state_it->WindowScissorTLY();
                window_scissor.m_br_x = event_state_it->WindowScissorBRX();
                window_scissor.m_br_y = event_state_it->WindowScissorBRY();
                m_capture_stats.m_window_scissors.push_back(window_scissor);
            }
        }

        for (size_t ref = 0; ref < info.m_shader_references.size(); ++ref)
            if (info.m_shader_references[ref].m_shader_index != UINT32_MAX)
                m_capture_stats.m_shader_ref_set.push_back(info.m_shader_references[ref]);
    }

    stats_list[Dive::Stats::kNumBinnigPasses] = m_capture_stats.m_num_binning_passes;
    stats_list[Dive::Stats::kNumTilingPasses] = m_capture_stats.m_num_tiling_passes;

    if (!m_capture_stats.m_event_num_indices.empty())
    {
        GATHER_TOTAL_MIN_MAX_MEDIAN(m_capture_stats.m_event_num_indices, Indices);
    }

    std::vector<size_t>   shaders_num_instructions;
    std::vector<uint32_t> shaders_num_gprs;

    stats_list[Dive::Stats::kShaders] = meta_data.m_shaders.size();

    for (const Dive::ShaderReference &ref : m_capture_stats.m_shader_ref_set)
    {
        if (ref.m_stage == Dive::ShaderStage::kShaderStageVs)
        {
            if (ref.m_enable_mask & (uint32_t)Dive::ShaderEnableBit::kBINNING)
                stats_list[Dive::Stats::kBinningVS]++;
            else
                stats_list[Dive::Stats::kNonBinningVS]++;
        }
        else
            stats_list[Dive::Stats::kNonVS]++;

        const Dive::Disassembly &disass = meta_data.m_shaders[ref.m_shader_index];
        shaders_num_instructions.push_back(disass.GetNumInstructions());
        shaders_num_gprs.push_back(disass.GetGPRCount());
    }

    if (!shaders_num_instructions.empty())
    {
        GATHER_TOTAL_MIN_MAX_MEDIAN(shaders_num_instructions, Instructions);
    }
    if (!shaders_num_gprs.empty())
    {
        GATHER_TOTAL_MIN_MAX_MEDIAN(shaders_num_gprs, GPRs);
    }
}

//--------------------------------------------------------------------------------------------------
void TraceStats::PrintTraceStats(std::ostream &ostream)
{
    std::array<uint64_t, Dive::Stats::kNumStats> &stats_list = m_capture_stats.m_stats_list;

    DIVE_ASSERT(kStatMap.size() == Stats::kNumStats);

    constexpr std::array<const char *, Stats::kNumStats> kStatDescriptions = [&] {
        std::array<const char *, Stats::kNumStats> arr{};
        for (const auto &[stat, description] : kStatMap)
        {
            arr[stat] = description;
        }
        return arr;
    }();

    // Set output stream format (left alignment)
    ostream << std::left;

    for (uint32_t i = 0; i < Dive::Stats::kNumStats; ++i)
    {
        ostream << kStatDescriptions[i] << ": " << stats_list[i] << "\n";
    }

    ostream << viewport_stats_desc[kViewport] << ":\n";

    for (const Viewport &vp : m_capture_stats.m_viewports)
    {
        ostream << viewport_stats_desc[kViewport_x] << ": " << (int)vp.m_vk_viewport.x << ", "
                << viewport_stats_desc[kViewport_y] << ": " << (int)vp.m_vk_viewport.y << ", "
                << viewport_stats_desc[kViewport_width] << ": " << (int)vp.m_vk_viewport.width
                << ", " << viewport_stats_desc[kViewport_height] << ": "
                << (int)vp.m_vk_viewport.height << ", " << viewport_stats_desc[kViewport_minDepth]
                << ": " << std::fixed << std::setprecision(1) << vp.m_vk_viewport.minDepth << ", "
                << viewport_stats_desc[kViewport_maxDepth] << ": " << std::fixed
                << std::setprecision(1) << vp.m_vk_viewport.maxDepth << "\n";
    }

    ostream << window_scissor_stats_desc[kWindowScissors] << ":\n";

    for (const WindowScissor &ws : m_capture_stats.m_window_scissors)
    {
        ostream << window_scissor_stats_desc[kWindowScissors_tl_x] << ": " << ws.m_tl_x << ", "
                << window_scissor_stats_desc[kWindowScissors_br_x] << ": " << ws.m_br_x << ", "
                << window_scissor_stats_desc[kWindowScissors_tl_y] << ": " << ws.m_tl_y << ", "
                << window_scissor_stats_desc[kWindowScissors_br_y] << ": " << ws.m_br_y << ", "
                << window_scissor_stats_desc[kWindowScissors_Width] << ": "
                << (ws.m_br_x - ws.m_tl_x + 1) << ", "
                << window_scissor_stats_desc[kWindowScissors_Height] << ": "
                << (ws.m_br_y - ws.m_tl_y + 1) << "\n";
    }
}

}  // namespace Dive