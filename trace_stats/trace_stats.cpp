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

#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>

#include "dive_core/event_state.h"

namespace Dive
{
namespace
{

class ThreadPool
{
public:
    ThreadPool() = default;
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;
    ~ThreadPool() { Stop(); }

    void Run(std::function<void()> &&func)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_tasks.push_back(std::move(func));
        }
        m_condition_variable.notify_one();
    }

    void Start(unsigned int num_workers = 0)
    {
        num_workers = (num_workers > 0 ? num_workers : GetDefaultThreadCount());

        std::unique_lock<std::mutex> lock(m_mutex);
        m_running = true;
        for (unsigned int i = static_cast<unsigned int>(m_workers.size()); i < num_workers; ++i)
        {
            m_workers.emplace_back([this]() { this->WorkerImpl(); });
        }
    }

    void Stop()
    {
        std::deque<std::thread> workers;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (!m_running)
            {
                return;
            }
            m_running = false;
            std::swap(workers, m_workers);
        }
        m_condition_variable.notify_all();
        for (std::thread &worker : workers)
        {
            worker.join();
        }
    }

    static unsigned int SuggestedNumberOfWorkers(unsigned int task_count)
    {
        // We are still bottlenecked by the slowest disassembly task.
        // 4x less worker than disassembly tasks seems be the point of diminishing return.
        constexpr unsigned int kLoadFactor = 4;
        return std::min<unsigned int>((task_count + kLoadFactor - 1) / kLoadFactor,
                                      GetDefaultThreadCount());
    }

private:
    std::function<void()> NextTask()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition_variable.wait(lock, [this] { return !m_running || !m_tasks.empty(); });
        if (!m_running || m_tasks.empty())
        {
            return {};
        }
        std::function<void()> result = m_tasks.front();
        m_tasks.pop_front();
        return result;
    }

    static unsigned int GetDefaultThreadCount()
    {
        unsigned int count = std::thread::hardware_concurrency();
        return (count > 1 ? count - 1 : 1);
    }

    void WorkerImpl()
    {
        while (auto task = NextTask())
        {
            task();
        }
    }

    bool                              m_running;
    std::mutex                        m_mutex;
    std::deque<std::thread>           m_workers;
    std::deque<std::function<void()>> m_tasks;
    std::condition_variable           m_condition_variable;
};

}  // namespace

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
void TraceStats::GatherTraceStats(const Dive::Context         &context,
                                  const Dive::CaptureMetadata &meta_data,
                                  CaptureStats                &capture_stats)
{
    capture_stats = CaptureStats();  // Reset any previous stats

    std::array<uint64_t, Dive::Stats::kNumStats> &stats_list = capture_stats.m_stats_list;

    size_t                      event_count = meta_data.m_event_info.size();
    const Dive::EventStateInfo &event_state = meta_data.m_event_state;

    Dive::RenderModeType cur_type = Dive::RenderModeType::kUnknown;
    for (size_t i = 0; i < event_count; ++i)
    {
        if (context.Cancelled())
        {
            capture_stats = CaptureStats();
            return;
        }
        const Dive::EventInfo &info = meta_data.m_event_info[i];

        if (info.m_render_mode != cur_type)
        {
            if (info.m_render_mode == Dive::RenderModeType::kBinningVis ||
                info.m_render_mode == Dive::RenderModeType::kBinningDirect)
                capture_stats.m_num_binning_passes++;
            else if (info.m_render_mode == Dive::RenderModeType::kTiled)
                capture_stats.m_num_tiling_passes++;

            cur_type = info.m_render_mode;
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
        else if (info.m_type == Dive::EventInfo::EventType::kColorSysMemToGmemResolve)
            GatherResolves(Stats::kColorSysMemToGmemResolves);
        else if (info.m_type == Dive::EventInfo::EventType::kColorGmemToSysMemResolve ||
                 info.m_type == Dive::EventInfo::EventType::kColorGmemToSysMemResolveAndClear)
            GatherResolves(Stats::kColorGmemToSysMemResolves);
        else if (info.m_type == Dive::EventInfo::EventType::kDepthSysMemToGmemResolve)
            GatherResolves(Stats::kDepthSysMemToGmemResolves);
        else if (info.m_type == Dive::EventInfo::EventType::kDepthGmemToSysMemResolve ||
                 info.m_type == Dive::EventInfo::EventType::kDepthGmemToSysMemResolveAndClear)
            GatherResolves(Stats::kDepthGmemToSysMemResolves);
        else if (info.m_type == Dive::EventInfo::EventType::kColorClearGmem ||
                 info.m_type == Dive::EventInfo::EventType::kColorGmemToSysMemResolveAndClear)
            GatherResolves(Stats::kColorClearGmemResolves);
        else if (info.m_type == Dive::EventInfo::EventType::kDepthClearGmem ||
                 info.m_type == Dive::EventInfo::EventType::kDepthGmemToSysMemResolveAndClear)
            GatherResolves(Stats::kDepthClearGmemResolves);
        else if (info.m_type == Dive::EventInfo::EventType::kDraw)
        {
            if (info.m_render_mode == Dive::RenderModeType::kBinningVis ||
                info.m_render_mode == Dive::RenderModeType::kBinningDirect)
                stats_list[Dive::Stats::kBinningDraws]++;
            else if (info.m_render_mode == Dive::RenderModeType::kDirect)
                stats_list[Dive::Stats::kDirectDraws]++;
            else if (info.m_render_mode == Dive::RenderModeType::kTiled)
                stats_list[Dive::Stats::kTiledDraws]++;

            if (info.m_num_indices != 0)
                capture_stats.m_event_num_indices.push_back(info.m_num_indices);

            const uint32_t event_id = static_cast<uint32_t>(i);
            auto event_state_it = event_state.find(static_cast<Dive::EventStateId>(event_id));

            if (info.m_render_mode == Dive::RenderModeType::kBinningVis ||
                info.m_render_mode == Dive::RenderModeType::kBinningDirect)
            {
                CHECK_AND_TRACK_STATE(Dive::Stats::kDepthTestEnabled, DepthTestEnabled);
                CHECK_AND_TRACK_STATE(Dive::Stats::kDepthWriteEnabled,
                                      DepthTestEnabled,
                                      DepthWriteEnabled);
                if (event_state_it->DepthTestEnabled())
                {
                    CHECK_AND_TRACK_STATE_EQUAL(Dive::Stats::kEarlyZ, ZTestMode, A6XX_EARLY_Z);
                    CHECK_AND_TRACK_STATE_EQUAL(Dive::Stats::kLateZ, ZTestMode, A6XX_LATE_Z);
                    CHECK_AND_TRACK_STATE_EQUAL(Dive::Stats::kEarlyZLateZ,
                                                ZTestMode,
                                                A6XX_EARLY_Z_LATE_Z);
                }
            }
            if (info.m_render_mode == Dive::RenderModeType::kDirect ||
                info.m_render_mode == Dive::RenderModeType::kBinningVis ||
                info.m_render_mode == Dive::RenderModeType::kBinningDirect)
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
                    capture_stats.m_viewports.insert(viewport);
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
                capture_stats.m_window_scissors.insert(window_scissor);
            }
        }

        for (size_t ref = 0; ref < info.m_shader_references.size(); ++ref)
            if (info.m_shader_references[ref].m_shader_index != UINT32_MAX)
                capture_stats.m_shader_ref_set.insert(info.m_shader_references[ref]);
    }

    stats_list[Dive::Stats::kNumBinningPasses] = capture_stats.m_num_binning_passes;
    stats_list[Dive::Stats::kNumTilingPasses] = capture_stats.m_num_tiling_passes;

    if (!capture_stats.m_event_num_indices.empty())
    {
        GATHER_TOTAL_MIN_MAX_MEDIAN(capture_stats.m_event_num_indices, Indices);
    }

    std::vector<size_t>   shaders_num_instructions;
    std::vector<uint32_t> shaders_num_gprs;

    stats_list[Dive::Stats::kShaders] = meta_data.m_shaders.size();

    ThreadPool thread_pool;
    if (meta_data.m_shaders.size() > 0)
    {
        auto task_count = static_cast<unsigned int>(meta_data.m_shaders.size());
        thread_pool.Start(thread_pool.SuggestedNumberOfWorkers(task_count));
        for (const Dive::Disassembly &disassembly : meta_data.m_shaders)
        {
            thread_pool.Run([&context, &disassembly]() {
                if (context.Cancelled())
                {
                    return;
                }
                disassembly.EagerEval();
            });
        }
    }

    for (const Dive::ShaderReference &ref : capture_stats.m_shader_ref_set)
    {
        if (context.Cancelled())
        {
            capture_stats = CaptureStats();
            return;
        }
        if (ref.m_stage == Dive::ShaderStage::kShaderStageVs)
        {
            if (ref.m_enable_mask & static_cast<uint32_t>(Dive::ShaderEnableBitMask::kBINNING))
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
void TraceStats::PrintTraceStats(const CaptureStats &capture_stats, std::ostream &ostream)
{
    const std::array<uint64_t, Dive::Stats::kNumStats> &stats_list = capture_stats.m_stats_list;

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
        if (i != Stats::kNumBinningPasses && i != Stats::kNumTilingPasses)
        {
            ostream << kStatDescriptions[i] << ": " << stats_list[i] << "\n";
        }
    }

    ostream << viewport_stats_desc[kViewport] << ":\n";

    auto print_field = [&ostream](std::string_view name, auto value, bool is_last_item) {
        std::ostringstream string_stream;
        string_stream << name << ": " << std::fixed << std::setprecision(1) << value;

        if (!is_last_item)
        {
            string_stream << ",";
            ostream << std::setw(17);
        }
        ostream << std::left << string_stream.str();
    };

    for (const Viewport &vp : capture_stats.m_viewports)
    {
        ostream << "\t";
        print_field(viewport_stats_desc[kViewport_x], vp.m_vk_viewport.x, false);
        print_field(viewport_stats_desc[kViewport_y], vp.m_vk_viewport.y, false);
        print_field(viewport_stats_desc[kViewport_width], vp.m_vk_viewport.width, false);
        print_field(viewport_stats_desc[kViewport_height], vp.m_vk_viewport.height, false);
        print_field(viewport_stats_desc[kViewport_minDepth], vp.m_vk_viewport.minDepth, false);
        print_field(viewport_stats_desc[kViewport_maxDepth], vp.m_vk_viewport.maxDepth, true);
        ostream << std::endl;
    }

    ostream << window_scissor_stats_desc[kWindowScissors] << ":\n";
    ostream << "\t" << kStatDescriptions[Stats::kNumBinningPasses] << ": "
            << stats_list[Stats::kNumBinningPasses] << "\n";
    ostream << "\t" << kStatDescriptions[Stats::kNumTilingPasses] << ": "
            << stats_list[Stats::kNumTilingPasses] << "\n";

    uint32_t count = 0;
    for (const WindowScissor &ws : capture_stats.m_window_scissors)
    {
        ostream << "\t" << count++ << "\t";
        print_field(window_scissor_stats_desc[kWindowScissors_tl_x], ws.m_tl_x, false);
        print_field(window_scissor_stats_desc[kWindowScissors_br_x], ws.m_br_x, false);
        print_field(window_scissor_stats_desc[kWindowScissors_tl_y], ws.m_tl_y, false);
        print_field(window_scissor_stats_desc[kWindowScissors_br_y], ws.m_br_y, false);
        print_field(window_scissor_stats_desc[kWindowScissors_Width],
                    (ws.m_br_x - ws.m_tl_x + 1),
                    false);
        print_field(window_scissor_stats_desc[kWindowScissors_Height],
                    (ws.m_br_y - ws.m_tl_y + 1),
                    true);
        ostream << std::endl;
    }
}

}  // namespace Dive
