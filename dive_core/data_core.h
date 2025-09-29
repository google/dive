
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

#pragma once
#include <map>
#include <vector>
#include "pm4_capture_data.h"
#include "gfxr_capture_data.h"
#include "dive_capture_data.h"
#include "capture_event_info.h"
#include "command_hierarchy.h"
#include "event_state.h"
#include "progress_tracker.h"
#include "dive_command_hierarchy.h"

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
// Metadata that describes information in the capture
struct CaptureMetadata
{
    // Information about the command buffers, represented in a tree hierarchy
    CommandHierarchy m_command_hierarchy;

    // Information about each shader in the capture
    std::vector<Disassembly> m_shaders;

    // Information about each buffer in the capture
    std::vector<BufferInfo> m_buffers;

    // Information about each event in the capture
    std::vector<EventInfo> m_event_info;

    // Register state tracking for each event
    // This is separated from EventInfo to take advantage of code-gen
    EventStateInfo m_event_state;

    // Information about the submits in this capture
    uint64_t m_num_pm4_packets;
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
    std::array<uint64_t, Dive::Stats::kNumStats> stats_list = {};

    std::vector<uint32_t> event_num_indices;

    std::vector<Dive::ShaderReference> shader_ref_set;
    std::vector<Viewport>              viewports;
    std::vector<WindowScissor>         window_scissors;

    uint32_t num_binning_passes = 0;
    uint32_t num_tiling_passes = 0;
};

//--------------------------------------------------------------------------------------------------
// Main container for the capture data as well as associated metadata
class DataCore
{
protected:
    ProgressTracker *m_progress_tracker;

public:
    DataCore() = default;

    DataCore(ProgressTracker *progress_tracker);

    // Load the capture file
    CaptureData::LoadResult LoadDiveCaptureData(const std::string &file_name);
    CaptureData::LoadResult LoadPm4CaptureData(const std::string &file_name);
    CaptureData::LoadResult LoadGfxrCaptureData(const std::string &file_name);

    // Parse the capture to generate info that describes the capture
    bool ParseDiveCaptureData();
    bool ParsePm4CaptureData();
    bool ParseGfxrCaptureData();

    // Create meta data from the captured data
    bool CreateDiveMetaData();
    bool CreatePm4MetaData();

    // Get the dive capture data
    const DiveCaptureData &GetDiveCaptureData() const;

    // Get the pm4 capture data (includes access to raw command buffers and memory blocks)
    const Pm4CaptureData &GetPm4CaptureData() const;
    Pm4CaptureData       &GetMutablePm4CaptureData();

    // Get the gfxr capture data
    const GfxrCaptureData &GetGfxrCaptureData() const;
    GfxrCaptureData       &GetMutableGfxrCaptureData();

    // Get the command-hierarchy, which is a tree view interpretation of the command buffer
    const CommandHierarchy &GetCommandHierarchy() const;

    // Get metadata describing the capture (info obtained by parsing the capture)
    const CaptureMetadata &GetCaptureMetadata() const;

    // Gather various statistics from the capture metadata
    void GatherCaptureStats();

    // Print out the statistics to the provided output stream
    void PrintCaptureStats(std::ostream &ostream);

    // Get the capture statistics
    const CaptureStats &GetCaptureStats() const;

private:
    // Create command hierarchy from the captured data
    bool CreateDiveCommandHierarchy();
    bool CreatePm4CommandHierarchy();
    bool CreateGfxrCommandHierarchy();
    // The relatively raw captured dive data (memory & submit blocks)
    DiveCaptureData m_dive_capture_data;
    // The relatively raw captured pm4 data (memory & submit blocks)
    Pm4CaptureData m_pm4_capture_data;
    // The relatively raw captured gfxr data
    GfxrCaptureData m_gfxr_capture_data;

    // Metadata for the capture data in m_capture_data
    CaptureMetadata m_capture_metadata;

    // Statistics about the capture
    CaptureStats m_capture_stats;
};

#if defined(ENABLE_CAPTURE_BUFFERS)
//--------------------------------------------------------------------------------------------------
// Shader reflector callback class, for use by the CaptureMetadataCreator
class SRDCallbacks : public IShaderReflectorCallbacks
{
public:
    // Callbacks on each SRD table used by the shader. The SRD table is a buffer that contains 1 or
    // more SRDs, each of which might be a different type
    virtual bool OnSRDTable(ShaderStage shader_stage,
                            uint64_t    va_addr,
                            uint64_t    size,
                            void       *user_ptr)
    {
        return true;
    }

    virtual bool OnSRDTable(ShaderStage shader_stage,
                            void       *data_ptr,
                            uint64_t    va_addr,
                            uint64_t    size,
                            void       *user_ptr)
    {
        return true;
    }
};
#endif

//--------------------------------------------------------------------------------------------------
// Handles creation of much of the metadata "info" from the capture
class CaptureMetadataCreator : public EmulateCallbacksBase
{
public:
    CaptureMetadataCreator(CaptureMetadata &capture_metadata);
    ~CaptureMetadataCreator();

    virtual void OnSubmitStart(uint32_t submit_index, const SubmitInfo &submit_info) override;
    virtual void OnSubmitEnd(uint32_t submit_index, const SubmitInfo &submit_info) override;

    const EmulateStateTracker &GetStateTracker() const { return m_state_tracker; }

    // Callbacks
    virtual bool OnIbStart(uint32_t                  submit_index,
                           uint32_t                  ib_index,
                           const IndirectBufferInfo &ib_info,
                           IbType                    type) override;

    virtual bool OnIbEnd(uint32_t                  submit_index,
                         uint32_t                  ib_index,
                         const IndirectBufferInfo &ib_info) override;

    virtual bool OnPacket(const IMemoryManager &mem_manager,
                          uint32_t              submit_index,
                          uint32_t              ib_index,
                          uint64_t              va_addr,
                          Pm4Header             header) override;

private:
    bool HandleShaders(const IMemoryManager &mem_manager, uint32_t submit_index, uint32_t opcode);
    void FillEventStateInfo(EventStateInfo::Iterator event_state_it);
    void FillInputAssemblyState(EventStateInfo::Iterator event_state_it);
    void FillTessellationState(EventStateInfo::Iterator event_state_it);
    void FillViewportState(EventStateInfo::Iterator event_state_it);
    void FillRasterizerState(EventStateInfo::Iterator event_state_it);
    void FillMultisamplingState(EventStateInfo::Iterator event_state_it);
    void FillDepthState(EventStateInfo::Iterator event_state_it);
    void FillColorBlendState(EventStateInfo::Iterator event_state_it);
    void FillHardwareSpecificStates(EventStateInfo::Iterator event_state_it);

    // Map from shader address to shader index (in m_capture_metadata.m_shaders)
    std::map<uint64_t, uint32_t> m_shader_addrs;

    // Map from buffer address to buffer index (in m_capture_metadata.m_buffers)
    std::map<uint64_t, uint32_t> m_buffer_addrs;

    CaptureMetadata &m_capture_metadata;
    RenderModeType   m_current_render_mode = RenderModeType::kUnknown;

#if defined(ENABLE_CAPTURE_BUFFERS)
    // SRDCallbacks is a friend class, since it is essentially doing part of
    // CaptureMetadataCreator's work and is only a separate class due to the callback nature of SRD
    // reflection
    friend class SRDCallbacks;
#endif
};

}  // namespace Dive
