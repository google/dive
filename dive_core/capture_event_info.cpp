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

#include <assert.h>

#include "capture_event_info.h"
#include "dive_core/common/memory_manager_base.h"
#include "dive_core/common/pm4_packets/me_pm4_packets.h"
#include "pm4_info.h"

namespace Dive
{
// =================================================================================================
// Util
// =================================================================================================
bool Util::IsEvent(const IMemoryManager &mem_manager,
                   uint32_t              submit_index,
                   uint64_t              addr,
                   uint32_t              opcode,
                   EmulateStateTracker  &state_tracker)
{
    if (IsDrawDispatchEventOpcode(opcode))
        return true;

    if (opcode == CP_BLIT)
        return true;

    SyncType sync_type = Util::GetSyncType(mem_manager, submit_index, addr, opcode, state_tracker);
    if (sync_type != SyncType::kNone)
        return true;

    return false;
}

//--------------------------------------------------------------------------------------------------
SyncType Util::GetSyncType(const IMemoryManager &mem_manager,
                           uint32_t              submit_index,
                           uint64_t              addr,
                           uint32_t              opcode,
                           EmulateStateTracker  &state_tracker)
{
    // 6xx uses CP_EVENT_WRITE packet, which maps to same opcode as CP_EVENT_WRITE7
    // The event field is in the same location with either packet type
    if (opcode == CP_EVENT_WRITE7)
    {
        PM4_CP_EVENT_WRITE packet;
        DIVE_VERIFY(mem_manager.RetrieveMemoryData(&packet, submit_index, addr, sizeof(packet)));
        SyncType type = SyncType::kNone;

        // Be more specific with certain ccu operations
        if (packet.bitfields0.EVENT == vgt_event_type::CCU_RESOLVE)
        {
            uint32_t rb_blit_info_offset = GetRegOffsetByName("RB_BLIT_INFO");
            if (state_tracker.IsRegSet(rb_blit_info_offset))
            {
                RB_BLIT_INFO rb_blit_info;
                rb_blit_info.u32All = state_tracker.GetRegValue(rb_blit_info_offset);
                if (rb_blit_info.bitfields.CLEAR_MASK)
                {
                    // TODO(wangra): update mesa code to handle this correctly
                    // We have encountered the case where rb_blit_info.bitfields.GMEM == 0 but
                    // rb_blit_info.bitfields.UNK0 == 1 The register 0x88e3 has been updated
                    // https://gitlab.freedesktop.org/mesa/mesa/-/blob/main/src/freedreno/registers/adreno/a6xx.xml?ref_type=heads#L1712
                    // The 1st 2 bits are actually TYPE
                    // If rb_blit_info.bitfields.GMEM bit is set, it means either BLIT_EVENT_CLEAR
                    // or BLIT_EVENT_LOAD In my case, the value means BLIT_EVENT_STORE_AND_CLEAR
                    DIVE_ASSERT(rb_blit_info.bitfields.GMEM || rb_blit_info.bitfields.UNK0);
                    type = SyncType::kClearGmem;
                }
                else
                {
                    if (rb_blit_info.bitfields.GMEM)
                        type = SyncType::kSysMemToGmemResolve;
                    else
                        type = SyncType::kGmemToSysMemResolve;
                }
            }
        }
        return type;
    }
    else if (opcode == CP_WAIT_MEM_WRITES)
    {
        return SyncType::kWaitMemWrites;
    }
    else if (opcode == CP_WAIT_FOR_IDLE)
    {
        return SyncType::kWaitForIdle;
    }
    else if (opcode == CP_WAIT_FOR_ME)
    {
        return SyncType::kWaitForMe;
    }
    return SyncType::kNone;
}

//--------------------------------------------------------------------------------------------------
std::string Util::GetEventString(const IMemoryManager &mem_manager,
                                 uint32_t              submit_index,
                                 uint64_t              va_addr,
                                 Pm4Type7Header        header,
                                 EmulateStateTracker  &state_tracker)
{
    uint32_t opcode = header.opcode;

    std::ostringstream string_stream;
    DIVE_ASSERT(IsEvent(mem_manager, submit_index, va_addr, opcode, state_tracker));

    if (opcode == CP_DRAW_INDX)
    {
        PM4_CP_DRAW_INDX packet;
        DIVE_VERIFY(mem_manager.RetrieveMemoryData(&packet, submit_index, va_addr, sizeof(packet)))
        string_stream << "DrawIndexOffset(NumIndices:" << packet.bitfields2.NUM_INDICES << ")";
    }
    else if (opcode == CP_DRAW_INDX)
    {
        PM4_CP_DRAW_INDX packet;
        DIVE_VERIFY(mem_manager.RetrieveMemoryData(&packet, submit_index, va_addr, sizeof(packet)))
        string_stream << "DrawIndexOffset(NumIndices:" << packet.bitfields2.NUM_INDICES << ")";
    }
    else if (opcode == CP_DRAW_INDX_OFFSET)
    {
        // This packet is used for indexed and non-indexed draws.
        // Non-indexed draws do not need to fill out entire packet
        // Note: header.count only includes payload and doesn't include header, hence the + 1
        PM4_CP_DRAW_INDX_OFFSET packet;
        uint32_t                header_and_body_dword_count = header.count + 1;
        DIVE_VERIFY(mem_manager.RetrieveMemoryData(&packet,
                                                   submit_index,
                                                   va_addr,
                                                   header_and_body_dword_count * sizeof(uint32_t)));
        string_stream << "DrawIndexOffset(";
        if (packet.bitfields0.SOURCE_SELECT == DI_SRC_SEL_AUTO_INDEX)  // No indices provided
        {
            string_stream << "AutoIndex,"
                          << "NumInstances:" << packet.bitfields1.NUM_INSTANCES << ","
                          << "NumIndices:" << packet.bitfields2.NUM_INDICES << ")";
        }
        else if (packet.bitfields0.SOURCE_SELECT == DI_SRC_SEL_DMA)  // Indexed draw
        {
            uint64_t addr = ((uint64_t)packet.bitfields5.INDX_BASE_HI << 32) |
                            (uint64_t)packet.bitfields4.INDX_BASE_LO;
            string_stream << "NumInstances:" << packet.bitfields1.NUM_INSTANCES << ","
                          << "NumIndices:" << packet.bitfields2.NUM_INDICES << ","
                          << "IndexBase:" << std::hex << "0x" << addr << ")";
        }
        else
        {
            // Immediate mode not supported AFAIK
            DIVE_ASSERT(false);
            string_stream << ")";
        }
    }
    else if (opcode == CP_DRAW_INDIRECT)
    {
        PM4_CP_DRAW_INDIRECT packet;
        DIVE_VERIFY(mem_manager.RetrieveMemoryData(&packet, submit_index, va_addr, sizeof(packet)))
        uint64_t addr = ((uint64_t)packet.bitfields2.INDIRECT_HI << 32) |
                        (uint64_t)packet.bitfields1.INDIRECT_LO;
        string_stream << "DrawIndirect("
                      << "Indirect:" << "0x" << addr << std::dec << ")";
    }
    else if (opcode == CP_DRAW_INDX_INDIRECT)
    {
        PM4_CP_DRAW_INDX_INDIRECT packet;
        DIVE_VERIFY(mem_manager.RetrieveMemoryData(&packet, submit_index, va_addr, sizeof(packet)))
        uint64_t index_base_addr = ((uint64_t)packet.bitfields2.INDX_BASE_HI << 32) |
                                   (uint64_t)packet.bitfields1.INDX_BASE_LO;
        uint64_t indirect_addr = ((uint64_t)packet.bitfields5.INDIRECT_HI << 32) |
                                 (uint64_t)packet.bitfields4.INDIRECT_LO;
        string_stream << "DrawIndexIndirect("
                      << "IndexBase:" << std::hex << "0x" << index_base_addr << std::dec << ","
                      << "MaxIndices:" << packet.bitfields3.MAX_INDICES << ","
                      << "Indirect:" << std::hex << "0x" << indirect_addr << ")";
    }
    else if (opcode == CP_DRAW_INDIRECT_MULTI)
    {
        PM4_CP_DRAW_INDIRECT_MULTI_INDIRECT_OP_NORMAL base_packet;
        DIVE_VERIFY(
        mem_manager.RetrieveMemoryData(&base_packet, submit_index, va_addr, sizeof(base_packet)));
        if (base_packet.bitfields1.OPCODE == INDIRECT_OP_NORMAL)
        {
            string_stream << "DrawIndirectMulti("
                          << "DrawCount:" << base_packet.DRAW_COUNT << ","
                          << "Indirect:" << std::hex << "0x" << base_packet.INDIRECT << std::dec
                          << ","
                          << "Stride:" << base_packet.STRIDE << ","
                          << "DstOff:" << base_packet.bitfields1.DST_OFF << ")";
        }
        else if (base_packet.bitfields1.OPCODE == INDIRECT_OP_INDEXED)
        {
            PM4_CP_DRAW_INDIRECT_MULTI_INDEXED packet;
            DIVE_VERIFY(
            mem_manager.RetrieveMemoryData(&packet, submit_index, va_addr, sizeof(packet)));
            string_stream << "DrawIndirectMultiIndexed("
                          << "DrawCount:" << packet.DRAW_COUNT << ","
                          << "Index:" << std::hex << "0x" << packet.INDEX << std::dec << ","
                          << "MaxIndices:" << packet.MAX_INDICES << ","
                          << "Indirect:" << std::hex << "0x" << packet.INDIRECT << std::dec << ","
                          << "Stride:" << packet.STRIDE << ","
                          << "DstOff:" << packet.bitfields1.DST_OFF << ")";
        }
        else if (base_packet.bitfields1.OPCODE == INDIRECT_OP_INDIRECT_COUNT)
        {
            PM4_CP_DRAW_INDIRECT_MULTI_INDIRECT packet;
            DIVE_VERIFY(
            mem_manager.RetrieveMemoryData(&packet, submit_index, va_addr, sizeof(packet)));
            string_stream << "DrawIndirectMultiIndirect("
                          << "DrawCount:" << packet.DRAW_COUNT << ","
                          << "Indirect:" << std::hex << "0x" << packet.INDIRECT << std::dec << ","
                          << "IndirectCount:" << packet.INDIRECT_COUNT << ","
                          << "Stride:" << packet.STRIDE << ","
                          << "DstOff:" << packet.bitfields1.DST_OFF << ")";
        }
        else if (base_packet.bitfields1.OPCODE == INDIRECT_OP_INDIRECT_COUNT_INDEXED)
        {
            PM4_CP_DRAW_INDIRECT_MULTI_INDIRECT_INDEXED packet;
            DIVE_VERIFY(
            mem_manager.RetrieveMemoryData(&packet, submit_index, va_addr, sizeof(packet)));
            string_stream << "DrawIndirectMultiIndirectIndexed("
                          << "DrawCount:" << packet.DRAW_COUNT << ","
                          << "Index:" << std::hex << "0x" << packet.INDEX << std::dec << ","
                          << "MaxIndices:" << packet.MAX_INDICES << ","
                          << "Indirect:" << std::hex << "0x" << packet.INDIRECT << std::dec << ","
                          << "IndirectCount:" << packet.INDIRECT_COUNT << ","
                          << "Stride:" << packet.STRIDE << ","
                          << "DstOff:" << packet.bitfields1.DST_OFF << ")";
        }
    }
    else if (opcode == CP_DRAW_AUTO)
    {
        // vkCmdDrawIndirectByteCountEXT
        PM4_CP_DRAW_AUTO packet;
        DIVE_VERIFY(mem_manager.RetrieveMemoryData(&packet, submit_index, va_addr, sizeof(packet)))
        string_stream << "DrawAuto("
                      << "NumInstances:" << packet.bitfields1.NUM_INSTANCES << ","
                      << "CounterBuffer:" << packet.NUM_VERTICES_BASE << ")";
    }
    else if (opcode == CP_EXEC_CS_INDIRECT)
    {
        PM4_CP_EXEC_CS_INDIRECT packet;
        DIVE_VERIFY(mem_manager.RetrieveMemoryData(&packet, submit_index, va_addr, sizeof(packet)));
        uint64_t addr = ((uint64_t)packet.bitfields2.ADDR_HI << 32) |
                        (uint64_t)packet.bitfields1.ADDR_LO;
        string_stream << "ExecCsIndirect(x:" << packet.bitfields3.LOCALSIZEX << ","
                      << "y:" << packet.bitfields3.LOCALSIZEY << ","
                      << "z:" << packet.bitfields3.LOCALSIZEZ << ","
                      << "Addr:" << std::hex << "0x" << addr << std::dec << ")";
    }
    else if (opcode == CP_EXEC_CS)
    {
        PM4_CP_EXEC_CS packet;
        DIVE_VERIFY(mem_manager.RetrieveMemoryData(&packet, submit_index, va_addr, sizeof(packet)));
        string_stream << "ExecCsIndirect(x:" << packet.bitfields1.NGROUPS_X << ","
                      << "y:" << packet.bitfields2.NGROUPS_Y << ","
                      << "z:" << packet.bitfields3.NGROUPS_Z << ")";
    }
    else if (opcode == CP_BLIT)
    {
        PM4_CP_BLIT packet;
        DIVE_VERIFY(mem_manager.RetrieveMemoryData(&packet,
                                                   submit_index,
                                                   va_addr,
                                                   (header.count + 1) * sizeof(uint32_t)));
        std::string op;
        switch (packet.bitfields0.OP)
        {
        case BLIT_OP_FILL:
            op = "BLIT_OP_FILL";
            break;
        case BLIT_OP_COPY:
            op = "BLIT_OP_COPY";
            break;
        case BLIT_OP_SCALE:
            op = "BLIT_OP_SCALE";
            break;
        }
        // Some packets (e.g. BLIT_OP_SCALE) only have header+dword total
        string_stream << "CpBlit(op:" << op;
        DIVE_ASSERT(header.count == 1 ||
                    ((header.count + 1) * sizeof(uint32_t) == sizeof(PM4_CP_BLIT)));
        if (header.count > 2)
        {
            string_stream << ",srcX1:" << packet.bitfields1.SRC_X1 << ","
                          << "srcY1:" << packet.bitfields1.SRC_Y1 << ","
                          << "srcX2:" << packet.bitfields2.SRC_X2 << ","
                          << "srcY2:" << packet.bitfields2.SRC_Y2 << ","
                          << "dstX1:" << packet.bitfields3.DST_X1 << ","
                          << "dstX1:" << packet.bitfields3.DST_Y1 << ","
                          << "dstX2:" << packet.bitfields4.DST_X2 << ","
                          << "dstX2:" << packet.bitfields4.DST_Y2;
        }
        string_stream << ")";
    }
    else if (opcode == CP_EVENT_WRITE7)
    {
        SyncType sync_type = Util::GetSyncType(mem_manager,
                                               submit_index,
                                               va_addr,
                                               opcode,
                                               state_tracker);
        if (sync_type == SyncType::kSysMemToGmemResolve)
        {
            string_stream << "Resolve(SysMem-To-Gmem)";
        }
        else if (sync_type == SyncType::kGmemToSysMemResolve)
        {
            string_stream << "Resolve(GMem-To-Sysmem)";
        }
        else if (sync_type == SyncType::kClearGmem)
        {
            string_stream << "Resolve(ClearGmem)";
        }
        else
        {
            // Note: For CP_EVENT_WRITEs, sync_type maps to a vgt_event_type
            const PacketInfo *packet_info_ptr = GetPacketInfo(opcode);
            DIVE_ASSERT(packet_info_ptr != nullptr);
            DIVE_ASSERT(packet_info_ptr->m_fields.size() > 1);
            DIVE_ASSERT(strcmp(packet_info_ptr->m_fields[0].m_name, "EVENT") == 0);
            const char *enum_str = GetEnumString(packet_info_ptr->m_fields[0].m_enum_handle,
                                                 (uint32_t)sync_type);
            string_stream << "CpEventWrite(type:" << enum_str << ")";
        }
    }
    else if (opcode == CP_WAIT_MEM_WRITES)
    {
        string_stream << "CpWaitMemWrites()";
    }
    else if (opcode == CP_WAIT_FOR_IDLE)
    {
        string_stream << "CpWaitForIdle()";
    }
    else if (opcode == CP_WAIT_FOR_ME)
    {
        string_stream << "CpWaitForMe()";
    }
    else
    {
        DIVE_ASSERT(false);
    }
    return string_stream.str();
}

//--------------------------------------------------------------------------------------------------
uint32_t Util::GetIndexCount(const IMemoryManager &mem_manager,
                             uint32_t              submit_index,
                             uint64_t              va_addr,
                             Pm4Type7Header        header)
{
    uint32_t index_count = 0;

    // Not handling indirect draws:
    //  CP_DRAW_INDIRECT
    //  CP_DRAW_INDX_INDIRECT
    //  CP_DRAW_INDIRECT_MULTI

    if (header.opcode == CP_DRAW_INDX)
    {
        PM4_CP_DRAW_INDX packet;
        DIVE_VERIFY(mem_manager.RetrieveMemoryData(&packet, submit_index, va_addr, sizeof(packet)))
        index_count = packet.bitfields2.NUM_INDICES;
    }
    else if (header.opcode == CP_DRAW_INDX_OFFSET)
    {
        // This packet is used for indexed and non-indexed draws.
        // Non-indexed draws do not need to fill out entire packet
        // Note: header.count only includes payload and doesn't include header, hence the + 1
        PM4_CP_DRAW_INDX_OFFSET packet;
        uint32_t                header_and_body_dword_count = header.count + 1;
        DIVE_VERIFY(mem_manager.RetrieveMemoryData(&packet,
                                                   submit_index,
                                                   va_addr,
                                                   header_and_body_dword_count * sizeof(uint32_t)));
        index_count = packet.bitfields2.NUM_INDICES;
    }
    return index_count;
}

}  // namespace Dive