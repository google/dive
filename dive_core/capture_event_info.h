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
#include <vector>
#include "capture_data.h"
#include "common.h"
#include "shader_disassembly.h"

namespace Dive
{

//--------------------------------------------------------------------------------------------------
struct BufferInfo
{
    uint64_t                      m_addr;
    uint64_t                      m_size;
    Dive::Legacy::BUF_DATA_FORMAT m_data_format;
    Dive::Legacy::BUF_NUM_FORMAT  m_num_format;
    Dive::Legacy::SQ_SEL_XYZW01   m_dst_sel_x;
    Dive::Legacy::SQ_SEL_XYZW01   m_dst_sel_y;
    Dive::Legacy::SQ_SEL_XYZW01   m_dst_sel_z;
    Dive::Legacy::SQ_SEL_XYZW01   m_dst_sel_w;
};

struct ShaderReference
{
    uint32_t    m_shader_index = UINT32_MAX;
    ShaderStage m_stage;
    uint32_t    m_enable_mask;
};

enum class RenderModeType
{
    kDirect,
    kBinning,
    kTiled,
    kResolve,
    kDispatch,
    kUnknown
};

//--------------------------------------------------------------------------------------------------
struct EventInfo
{
    // Indices of each buffer used in the event, for each shader
    std::vector<uint32_t> m_buffer_indices[(uint32_t)ShaderStage::kShaderStageCount];

    // Log entries from parsing the capture metadata and disassembling the shaders.
    // These cannot be directly output to the log because we don't know the eventIds while parsing
    // the metadata.
    DeferredLog m_metadata_log;

    // References of each shader used in the event.
    std::vector<ShaderReference> m_shader_references;

    // Submit that contains this event
    uint32_t m_submit_index;

    // Type of event
    enum class EventType
    {
        kDraw,
        kDispatch,
        kResolve,
        kSync
    };
    EventType m_type;

    RenderModeType m_render_mode = RenderModeType::kUnknown;
    std::string    m_str;
};

//--------------------------------------------------------------------------------------------------
// Helper functions
enum class SyncType
{
    // Map to EVENT_WRITEs (vgt_event_type)
    kEventWriteStart = vgt_event_type::VS_DEALLOC,
    kEventWriteEnd = vgt_event_type::CACHE_INVALIDATE7,

    kWaitMemWrites,
    kWaitForIdle,
    kWaitForMe,

    kNone
};

SyncType GetSyncType(const IMemoryManager &mem_manager,
                     uint32_t              submit_index,
                     uint64_t              addr,
                     uint32_t              opcode);

bool IsDrawDispatchResolveEvent(const IMemoryManager &mem_manager,
                                uint32_t              submit_index,
                                uint64_t              addr,
                                uint32_t              opcode);

bool IsDrawDispatchResolveSyncEvent(const IMemoryManager &mem_manager,
                                    uint32_t              submit_index,
                                    uint64_t              addr,
                                    uint32_t              opcode);
bool IsResolveEvent(const IMemoryManager &mem_manager,
                    uint32_t              submit_index,
                    uint64_t              addr,
                    uint32_t              opcode);

}  // namespace Dive
