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
#include <iostream>
#include <optional>
#include <vector>
#include <sstream>
#include <numeric>
#include <set>
#include <array>

#include "dive_core/data_core.h"
#include "pm4_info.h"

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

#define GATHER_TOTAL_MIN_MAX_MEDIAN(array_name, type)                                            \
    {                                                                                            \
        std::sort(array_name.begin(), array_name.end());                                         \
        size_t n = array_name.size();                                                            \
        if (n % 2 != 0)                                                                          \
        {                                                                                        \
            stats_list[Stats::kMedian##type] = array_name[n / 2];                                \
        }                                                                                        \
        else                                                                                     \
        {                                                                                        \
            auto mid1 = array_name[n / 2 - 1];                                                   \
            auto mid2 = array_name[n / 2];                                                       \
            stats_list[Stats::kMedian##type] = (uint64_t)((float)(mid1 + mid2) / 2.0f);          \
        }                                                                                        \
        stats_list[Stats::kMin##type] = *std::min_element(array_name.begin(), array_name.end()); \
        stats_list[Stats::kMax##type] = *std::max_element(array_name.begin(), array_name.end()); \
        stats_list[Stats::kTotal##type] = std::accumulate(array_name.begin(),                    \
                                                          array_name.end(),                      \
                                                          (uint64_t)0);                          \
    }

#define PRINT_FIELD(name, value, last_item)                                          \
    {                                                                                \
        std::ostringstream string_stream;                                            \
        string_stream << name << " " << std::fixed << std::setprecision(1) << value; \
        if (!last_item)                                                              \
        {                                                                            \
            string_stream << ",";                                                    \
            ostream << std::setw(17);                                                \
        }                                                                            \
        ostream << std::left << string_stream.str();                                 \
    }

void GatherAndPrintStats(const Dive::CaptureMetadata &meta_data, std::ostream &ostream)
{
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
        std::pair(Stats::kGmemToSysmemAndClearGmemResolves,
                  "\tGmem to SysMem Resolves and Clear Gmem"),
        std::pair(Stats::kClearGmemResolves, "\tGmem Clears"),
        std::pair(Stats::kSysmemToGmemResolves, "\tSysMem to Gmem Resolves"),
    };

    static_assert(kStatMap.size() == Stats::kNumStats,
                  "ERROR: The 'Stat' enum and the 'kStatMap' descriptions are out of sync!");

    constexpr std::array<const char *, Stats::kNumStats> kStatDescriptions = [&] {
        std::array<const char *, Stats::kNumStats> arr{};
        for (const auto &[stat, description] : kStatMap)
        {
            arr[stat] = description;
        }
        return arr;
    }();

    // Wrappers with an operator< so that they can be stored in an std::set
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

    uint64_t stats_list[Stats::kNumStats] = {};

    size_t                          event_count = meta_data.m_event_info.size();
    const Dive::EventStateInfo     &event_state = meta_data.m_event_state;
    std::vector<uint32_t>           event_num_indices;
    std::set<Dive::ShaderReference> shader_ref_set;
    std::set<Viewport>              viewports;
    std::set<WindowScissor>         window_scissors;

    Dive::RenderModeType      cur_type = Dive::RenderModeType::kUnknown;
    uint32_t                  num_binning_passes = 0, num_tiling_passes = 0;
    [[maybe_unused]] uint32_t num_draws_in_pass = 0;

    for (size_t i = 0; i < event_count; ++i)
    {
        const Dive::EventInfo &info = meta_data.m_event_info[i];
        if (info.m_render_mode != cur_type)
        {
#ifndef NDEBUG
            if (cur_type == Dive::RenderModeType::kBinning)
                ostream << "Binning pass " << num_binning_passes << ": " << num_draws_in_pass
                        << std::endl;
            else if (cur_type == Dive::RenderModeType::kTiled)
                ostream << "Tiling pass " << num_tiling_passes << ": " << num_draws_in_pass
                        << std::endl;
#endif
            if (info.m_render_mode == Dive::RenderModeType::kBinning)
                num_binning_passes++;
            else if (info.m_render_mode == Dive::RenderModeType::kTiled)
                num_tiling_passes++;

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
                stats_list[Stats::kBinningDraws]++;
            else if (info.m_render_mode == Dive::RenderModeType::kDirect)
                stats_list[Stats::kDirectDraws]++;
            else if (info.m_render_mode == Dive::RenderModeType::kTiled)
                stats_list[Stats::kTiledDraws]++;

            num_draws_in_pass++;
            if (info.m_num_indices != 0)
                event_num_indices.push_back(info.m_num_indices);

            const uint32_t event_id = static_cast<uint32_t>(i);
            auto event_state_it = event_state.find(static_cast<Dive::EventStateId>(event_id));

            if (info.m_render_mode != Dive::RenderModeType::kBinning)
            {
                CHECK_AND_TRACK_STATE(Stats::kDepthTestEnabled, DepthTestEnabled);
                CHECK_AND_TRACK_STATE(Stats::kDepthWriteEnabled,
                                      DepthTestEnabled,
                                      DepthWriteEnabled);
                if (event_state_it->DepthTestEnabled())
                {
                    CHECK_AND_TRACK_STATE_EQUAL(Stats::kEarlyZ, ZTestMode, A6XX_EARLY_Z);
                    CHECK_AND_TRACK_STATE_EQUAL(Stats::kLateZ, ZTestMode, A6XX_LATE_Z);
                    CHECK_AND_TRACK_STATE_EQUAL(Stats::kEarlyLRZLateZ,
                                                ZTestMode,
                                                A6XX_EARLY_LRZ_LATE_Z);
                }
            }
            if ((info.m_render_mode == Dive::RenderModeType::kDirect) ||
                (info.m_render_mode == Dive::RenderModeType::kBinning))
            {
                CHECK_AND_TRACK_STATE(Stats::kLrzEnabled, DepthTestEnabled, LRZEnabled);
                CHECK_AND_TRACK_STATE(Stats::kLrzWriteEnabled,
                                      DepthTestEnabled,
                                      DepthWriteEnabled,
                                      LRZWrite);
            }

            CHECK_AND_TRACK_STATE_NOT_EQUAL(Stats::kCullModeEnabled, CullMode, VK_CULL_MODE_NONE);

            for (uint32_t v = 0; v < 16; ++v)
            {
                if (event_state_it->IsViewportSet(v))
                {
                    Viewport viewport;
                    viewport.m_vk_viewport = event_state_it->Viewport(v);
                    viewports.insert(viewport);
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
                window_scissors.insert(window_scissor);
            }
        }

        for (size_t ref = 0; ref < info.m_shader_references.size(); ++ref)
            if (info.m_shader_references[ref].m_shader_index != UINT32_MAX)
                shader_ref_set.insert(info.m_shader_references[ref]);
    }

    GATHER_TOTAL_MIN_MAX_MEDIAN(event_num_indices, Indices);

    {
        std::vector<size_t>   shaders_num_instructions;
        std::vector<uint32_t> shaders_num_gprs;
        stats_list[Stats::kShaders] = meta_data.m_shaders.size();
        for (const Dive::ShaderReference &ref : shader_ref_set)
        {
            if (ref.m_stage == Dive::ShaderStage::kShaderStageVs)
            {
                if (ref.m_enable_mask & (uint32_t)Dive::ShaderEnableBit::kBINNING)
                    stats_list[Stats::kBinningVS]++;
                else
                    stats_list[Stats::kNonBinningVS]++;
            }
            else
                stats_list[Stats::kNonVS]++;

            const Dive::Disassembly &disass = meta_data.m_shaders[ref.m_shader_index];
            shaders_num_instructions.push_back(disass.GetNumInstructions());
            shaders_num_gprs.push_back(disass.GetGPRCount());
        }

        GATHER_TOTAL_MIN_MAX_MEDIAN(shaders_num_instructions, Instructions);
        GATHER_TOTAL_MIN_MAX_MEDIAN(shaders_num_gprs, GPRs);
    }

    for (uint32_t i = 0; i < Stats::kNumStats; ++i)
    {
        ostream << kStatDescriptions[i] << ": " << stats_list[i] << std::endl;
    }

    // Print out all unique viewports
    {
        ostream << "Viewports:" << std::endl;
        for (const Viewport &viewport : viewports)
        {
            ostream << "\t";
            PRINT_FIELD("x:", viewport.m_vk_viewport.x, false);
            PRINT_FIELD("y:", viewport.m_vk_viewport.y, false);
            PRINT_FIELD("width:", viewport.m_vk_viewport.width, false);
            PRINT_FIELD("height:", viewport.m_vk_viewport.height, false);
            PRINT_FIELD("minDepth:", viewport.m_vk_viewport.minDepth, false);
            PRINT_FIELD("maxDepth:", viewport.m_vk_viewport.maxDepth, true);
            ostream << std::endl;
        }
    }
    // Print out all unique window scissors (i.e. tiles)
    {
        ostream << "Window scissors:" << std::endl;
        ostream << "\tNum binning passes: " << num_binning_passes << std::endl;
        ostream << "\tNum tiling passes: " << num_tiling_passes << std::endl;
        uint32_t count = 0;
        for (const WindowScissor &window_scissor : window_scissors)
        {
            ostream << "\t" << count++ << "\t";
            PRINT_FIELD("tl_x:", window_scissor.m_tl_x, false);
            PRINT_FIELD("br_x:", window_scissor.m_br_x, false);
            PRINT_FIELD("tl_y:", window_scissor.m_tl_y, false);
            PRINT_FIELD("br_y:", window_scissor.m_br_y, false);
            PRINT_FIELD("Width:", window_scissor.m_br_x - window_scissor.m_tl_x + 1, false);
            PRINT_FIELD("Height:", window_scissor.m_br_y - window_scissor.m_tl_y + 1, true);
            ostream << std::endl;
        }
    }
}

int main(int argc, char **argv)
{
    Pm4InfoInit();

    // Handle args
    if ((argc != 2) && (argc != 3))
    {
        std::cout << "You need to call: lrz_validator <input_file_name.rd> "
                     "<output_details_file_name.txt>(optional)";
        return 0;
    }
    char *input_file_name = argv[1];

    std::string output_file_name = "";
    if (argc == 3)
    {
        output_file_name = argv[2];
    }

    // Load capture
    std::unique_ptr<Dive::DataCore> data_core = std::make_unique<Dive::DataCore>();
    Dive::CaptureData::LoadResult   load_res = data_core->LoadPm4CaptureData(input_file_name);
    if (load_res != Dive::CaptureData::LoadResult::kSuccess)
    {
        std::cout << "Loading capture \"" << input_file_name << "\" failed!";
        return 0;
    }
    std::cout << "Capture file \"" << input_file_name << "\" is loaded!\n";

    // Create meta data
    if (!data_core->CreatePm4MetaData())
    {
        std::cout << "Failed to create meta data!";
        return 0;
    }
    std::cout << "Gathering Stats...\n";

    std::ostream *ostream = &std::cout;
    std::ofstream ofstream;
    if (!output_file_name.empty())
    {
        std::cout << "Output details to \"" << output_file_name << "\"" << std::endl;
        ofstream.open(output_file_name);
        ostream = &ofstream;
    }

    // Gather Stats...
    const Dive::CaptureMetadata &meta_data = data_core->GetCaptureMetadata();
    GatherAndPrintStats(meta_data, *ostream);

    return 1;
}
