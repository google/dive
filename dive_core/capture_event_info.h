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
struct ShaderInfo
{
    uint64_t    m_addr;
    uint64_t    m_size;
    Disassembly m_disassembly;
};

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

//--------------------------------------------------------------------------------------------------
struct EventInfo
{
    // Indices of each buffer used in the event, for each shader
    std::vector<uint32_t> m_buffer_indices[(uint32_t)ShaderStage::kShaderStageCount];

    // Log entries from parsing the capture metadata and disassembling the shaders.
    // These cannot be directly output to the log because we don't know the eventIds while parsing
    // the metadata.
    DeferredLog m_metadata_log;

    // Indices of each shader used in the event. Set to UINT32_MAX if not applicable
    uint32_t m_shader_indices[(uint32_t)ShaderStage::kShaderStageCount] = { UINT32_MAX };

    // Submit that contains this event
    uint32_t m_submit_index;

    // Type of event
    enum class EventType
    {
        kDraw,
        kDispatch,
        kBlit,
        kSync
    };
    EventType m_type;

    // Texture, Buffer indices
};

//--------------------------------------------------------------------------------------------------
// Helper functions
enum class SyncType
{
    kNone,
    kAcquireMem,
    kWaitRegMem,
    kReleaseMem,
    kPfpSyncMe,

    // The following are all EVENT_WRITEs
    kEventWriteStart,
    kFlushInvCbMeta = kEventWriteStart,
    kFlushInvCbPixelData,
    kCacheFlushInvEvent,  // Flush/inv all CB/DB caches
    kVsPartialFlush,
    kPsPartialFlush,
    kCsPartialFlush,
    kDbCacheFlushAndInv,
    kVgtFlush,
    kEventWriteEnd = kVgtFlush + 1,
};

SyncType GetSyncType(const IMemoryManager &mem_manager,
                     uint32_t              submit_index,
                     uint64_t              addr,
                     uint32_t              opcode);
bool     IsDrawDispatchBlitSyncEvent(const IMemoryManager &mem_manager,
                                     uint32_t              submit_index,
                                     uint64_t              addr,
                                     uint32_t              opcode);
bool     IsBlitEvent(const IMemoryManager &mem_manager,
                     uint32_t              submit_index,
                     uint64_t              addr,
                     uint32_t              opcode);

}  // namespace Dive
