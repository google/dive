
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

#include <string.h>  // memcpy

#include "adreno.h"
#include "common.h"
#include "dive_capture_format.h"
#include "dive_core/pm4_capture_data.h"
#include "dive_core/stl_replacement.h"
#include "emulate_pm4.h"
#include "memory_manager_base.h"
#include "pm4_info.h"

#include <stdarg.h>
#include <cerrno>
#include <cstdio>
#include <cstring>

namespace Dive
{

// =================================================================================================
// EmulateStateTracker
// =================================================================================================
EmulateStateTracker::EmulateStateTracker() {}

//--------------------------------------------------------------------------------------------------
bool EmulateStateTracker::OnPacket(const IMemoryManager &mem_manager,
                                   uint32_t              submit_index,
                                   uint32_t              ib_index,
                                   uint64_t              va_addr,
                                   Pm4Header             header)
{
    if (header.type == 7 && header.type7.opcode == CP_SET_MARKER)
    {
        PM4_CP_SET_MARKER packet;
        DIVE_VERIFY(mem_manager.RetrieveMemoryData(&packet, submit_index, va_addr, sizeof(packet)));
        // as mentioned in adreno_pm4.xml, only b0-b3 are considered when b8 is not set
        DIVE_ASSERT((packet.u32All0 & 0x100) == 0);
        a6xx_marker marker = static_cast<a6xx_marker>(packet.u32All0 & 0xf);
        switch (marker)
        {
            // This is emitted at the beginning of the render pass if tiled rendering mode is
            // disabled
        case RM6_DIRECT_RENDER:
            m_shader_enable_bit = ShaderEnableBit::kSYSMEM;
            break;
            // This is emitted at the beginning of the binning pass, although the binning pass
            // could be missing even in tiled rendering mode
        case RM6_BIN_VISIBILITY:
        case RM6_BIN_DIRECT:
            m_shader_enable_bit = ShaderEnableBit::kBINNING;
            break;
            // This is emitted at the beginning of the tiled rendering pass
        case RM6_BIN_RENDER_START:
            m_shader_enable_bit = ShaderEnableBit::kGMEM;
            break;
            // This is emitted at the end of the tiled rendering pass
        case RM6_BIN_END_OF_DRAWS:
            // should be paired with RM6_BIN_RENDER_START only if RM6_BIN_VISIBILITY exist, end of
            // tiled mode
            m_shader_enable_bit = std::nullopt;
            break;
            // This is emitted at the beginning of the resolve pass
        case RM6_BIN_RESOLVE:
            m_shader_enable_bit = ShaderEnableBit::kGMEM;
            break;
            // This is emitted for each dispatch
        case RM6_COMPUTE:
            break;
            // This seems to be the end of Resolve Pass
        case RM6_BIN_RENDER_END:
            // should be paired with RM6_BIN_RESOLVE, end of resolve pass
            m_shader_enable_bit = std::nullopt;
            break;
        case RM6_BLIT2DSCALE:
        case RM6_IB1LIST_START:
        case RM6_IB1LIST_END:
        default:
            break;
        }
    }

    if (header.type == 7 && header.type7.opcode == CP_CONTEXT_REG_BUNCH)
    {
        uint32_t dword = 0;
        while (dword < header.type7.count)
        {
            struct RegPair
            {
                uint32_t m_reg_offset;
                uint32_t m_reg_value;
            };
            RegPair  reg_pair;
            uint64_t pair_addr = va_addr + sizeof(header) + dword * sizeof(uint32_t);
            DIVE_VERIFY(
            mem_manager.RetrieveMemoryData(&reg_pair, submit_index, pair_addr, sizeof(reg_pair)));
            dword += 2;
            SetReg(reg_pair.m_reg_offset, reg_pair.m_reg_value);

            const RegInfo *reg_info_ptr = GetRegInfo(reg_pair.m_reg_offset);
            if (reg_info_ptr && reg_info_ptr->m_is_64_bit)
            {
                RegPair  new_reg_pair;
                uint64_t new_pair_addr = va_addr + sizeof(header) + dword * sizeof(uint32_t);
                DIVE_VERIFY(mem_manager.RetrieveMemoryData(&new_reg_pair,
                                                           submit_index,
                                                           new_pair_addr,
                                                           sizeof(new_reg_pair)));

                // Sometimes the upper 32-bits are not set
                // Probably because they're 0s and there's no need to set it
                if (new_reg_pair.m_reg_offset == reg_pair.m_reg_offset + 1)
                {
                    dword += 2;
                    SetReg(new_reg_pair.m_reg_offset, new_reg_pair.m_reg_value);
                }
            }
        }
    }
    // type 4 is setting register
    else if (header.type == 4)
    {
        uint32_t offset_in_bytes = 0;
        uint32_t dword = 0;
        while (dword < header.type4.count)
        {
            uint64_t reg_va_addr = va_addr + sizeof(header) + offset_in_bytes;
            uint32_t reg_offset = header.type4.offset + dword;
            DIVE_ASSERT(reg_offset < kNumRegs);
            const RegInfo *reg_info_ptr = GetRegInfo(reg_offset);

            constexpr size_t dword_in_bytes = sizeof(uint32_t);
            uint32_t         size_in_dwords = 1;
            if (reg_info_ptr != nullptr)
            {
                if (reg_info_ptr->m_is_64_bit)
                    size_in_dwords = 2;
            }
            offset_in_bytes += size_in_dwords * dword_in_bytes;
            for (uint32_t i = 0; i < size_in_dwords; ++i)
            {
                uint32_t reg_value = 0;
                DIVE_VERIFY(mem_manager.RetrieveMemoryData(&reg_value,
                                                           submit_index,
                                                           reg_va_addr + i * dword_in_bytes,
                                                           dword_in_bytes));
                SetReg(reg_offset + i, reg_value);
            }

            dword += size_in_dwords;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
uint64_t EmulateStateTracker::GetCurShaderAddr(ShaderStage     stage,
                                               ShaderEnableBit shader_enable_bit) const
{
    uint32_t offset = UINT32_MAX;
    switch (stage)
    {
    case ShaderStage::kShaderStageCs:
        offset = 0xa9b4 /* SP_CS_OBJ_START */;
        break;
    case ShaderStage::kShaderStageGs:
        offset = 0xa88d /* SP_GS_OBJ_START */;
        break;
    case ShaderStage::kShaderStageHs:
        offset = 0xa834 /* SP_HS_OBJ_START */;
        break;
    case ShaderStage::kShaderStagePs:
        offset = 0xa983 /* SP_FS_OBJ_START */;
        break;
    case ShaderStage::kShaderStageVs:
        offset = 0xa81c /* SP_VS_OBJ_START */;
        break;
    default:
        return UINT64_MAX;
    }
    if (!IsRegSet(offset, shader_enable_bit))
    {
        return UINT64_MAX;
    }
    return GetReg64Value(offset, shader_enable_bit);
}

//--------------------------------------------------------------------------------------------------
uint64_t EmulateStateTracker::GetBufferAddr() const
{
    // return m_buffer_va;
    return 0;
}

//--------------------------------------------------------------------------------------------------
uint32_t EmulateStateTracker::GetBufferSize() const
{
    // return m_buffer_size;
    return 0;
}

//--------------------------------------------------------------------------------------------------
void EmulateStateTracker::Reset()
{
    memset(m_reg, 0, sizeof(m_reg));
    memset(m_reg_is_set, 0, sizeof(m_reg_is_set));
    m_shader_enable_bit = std::nullopt;
}

//--------------------------------------------------------------------------------------------------
void EmulateStateTracker::PushEnableMask(uint32_t enable_mask)
{
    m_enable_mask_stack.push_back(m_enable_mask);
    m_enable_mask = enable_mask;
}

//--------------------------------------------------------------------------------------------------
void EmulateStateTracker::PopEnableMask()
{
    m_enable_mask = m_enable_mask_stack.back();
    m_enable_mask_stack.pop_back();
}

//--------------------------------------------------------------------------------------------------
uint32_t EmulateStateTracker::GetRegValue(uint32_t offset, ShaderEnableBit shader_enable_bit) const
{
    uint32_t i = static_cast<uint32_t>(shader_enable_bit);
    return m_reg[i][offset];
}

//--------------------------------------------------------------------------------------------------
uint32_t EmulateStateTracker::GetRegValue(uint32_t offset) const
{
    DIVE_ASSERT(m_shader_enable_bit.has_value());
    return GetRegValue(offset, *m_shader_enable_bit);
}

//--------------------------------------------------------------------------------------------------
uint64_t EmulateStateTracker::GetReg64Value(uint32_t        offset,
                                            ShaderEnableBit shader_enable_bit) const
{
    uint32_t i = static_cast<uint32_t>(shader_enable_bit);
    return (static_cast<uint64_t>(m_reg[i][offset])) |
           ((static_cast<uint64_t>(m_reg[i][offset + 1])) << 32);
}

//--------------------------------------------------------------------------------------------------
uint64_t EmulateStateTracker::GetReg64Value(uint32_t offset) const
{
    DIVE_ASSERT(m_shader_enable_bit.has_value());
    return GetRegValue(offset, *m_shader_enable_bit);
}

//--------------------------------------------------------------------------------------------------
void EmulateStateTracker::SetReg(uint32_t offset, uint32_t value)
{
    for (unsigned int i = 0; i < kShaderEnableBitCount; ++i)
    {
        if (m_enable_mask & (1u << i))
        {
            m_reg[i][offset] = value;
            m_reg_is_set[i][offset / 8] |= (1 << (offset % 8));
        }
    }
}

//--------------------------------------------------------------------------------------------------
bool EmulateStateTracker::IsRegSet(uint32_t offset) const
{
    DIVE_ASSERT(m_shader_enable_bit.has_value());
    return IsRegSet(offset, *m_shader_enable_bit);
}

//--------------------------------------------------------------------------------------------------
bool EmulateStateTracker::IsRegSet(uint32_t offset, ShaderEnableBit shader_enable_bit) const
{
    uint32_t index = static_cast<uint32_t>(shader_enable_bit);
    uint8_t  is_reg_set = (m_reg_is_set[index][offset / 8] & (1 << (offset % 8)));
    return static_cast<bool>(is_reg_set);
}

// =================================================================================================
// EmulatePM4
// =================================================================================================
EmulatePM4::EmulatePM4() {}

//--------------------------------------------------------------------------------------------------
bool EmulatePM4::ExecuteSubmit(EmulateCallbacksBase     &callbacks,
                               const IMemoryManager     &mem_manager,
                               uint32_t                  submit_index,
                               uint32_t                  num_ibs,
                               const IndirectBufferInfo *ib_ptr)
{
    // Used to keep track of progress of emulation so far
    EmulateState emu_state;
    emu_state.m_submit_index = submit_index;
    emu_state.m_top_of_stack = IbLevel::kPrimaryRing;
    EmulateState::IbStack *primary_ring = &emu_state.m_ib_stack[IbLevel::kPrimaryRing];
    primary_ring->m_cur_va = UINT64_MAX;
    primary_ring->m_ib_queue_index = primary_ring->m_ib_queue_size = 0;

    // Push all the IBs in this submit onto the primary ring stack entry
    for (uint32_t i = 0; i < num_ibs; ++i)
    {
        const IndirectBufferInfo &ib_info = ib_ptr[i];
        if (!QueueIB(ib_info.m_va_addr,
                     ib_info.m_size_in_dwords,
                     ib_info.m_skip,
                     IbType::kNormal,
                     &emu_state))
        {
            return false;
        }
    }
    if (!AdvanceToQueuedIB(mem_manager, &emu_state, callbacks))
        return false;

    // Could be pointing to an IB that needs to be "skipped"
    // Advance to the next valid IB in that case
    if (!CheckAndAdvanceIB(mem_manager, &emu_state, callbacks))
        return false;

    // Should always be emulating something in an IB. If it's parked at the primary ring,
    // then that means emulation has completed
    while (emu_state.m_top_of_stack != IbLevel::kPrimaryRing)
    {
        // Callbacks + advance
        EmulateState::IbStack *cur_ib_level = &emu_state.m_ib_stack[emu_state.m_top_of_stack];

        Pm4Header header;
        DIVE_VERIFY(mem_manager.RetrieveMemoryData(&header,
                                                   emu_state.m_submit_index,
                                                   cur_ib_level->m_cur_va,
                                                   sizeof(Pm4Header)));

        // Check validity of packet
        if (header.type == 4)
        {
            Pm4Type4Header *type4_header = (Pm4Type4Header *)&header;
            if (type4_header->offset_parity != CalcParity(type4_header->offset))
                return false;
            if (type4_header->count_parity != CalcParity(type4_header->count))
                return false;
        }
        else if (header.type == 7)
        {
            Pm4Type7Header *type7_header = (Pm4Type7Header *)&header;
            if (type7_header->opcode_parity != CalcParity(type7_header->opcode))
                return false;
            if (type7_header->count_parity != CalcParity(type7_header->count))
                return false;
            if (type7_header->zeroes != 0)
                return false;
        }

        if (!callbacks.OnPacket(mem_manager,
                                emu_state.m_submit_index,
                                emu_state.m_ib_index,
                                cur_ib_level->m_cur_va,
                                header))
            return false;
        if (!AdvanceCb(mem_manager, &emu_state, callbacks, header))
            return false;
    }  // while there are packets left in submit
    return true;
}

//--------------------------------------------------------------------------------------------------
bool EmulatePM4::AdvanceCb(const IMemoryManager &mem_manager,
                           EmulateState         *emu_state_ptr,
                           EmulateCallbacksBase &callbacks,
                           Pm4Header             header) const
{
    // Deal with calls and chains
    if (header.type == 7 && (header.type7.opcode == CP_INDIRECT_BUFFER_PFE ||
                             header.type7.opcode == CP_INDIRECT_BUFFER_PFD ||
                             header.type7.opcode == CP_INDIRECT_BUFFER_CHAIN))
    {

        PM4_CP_INDIRECT_BUFFER ib_packet;
        DIVE_VERIFY(mem_manager.RetrieveMemoryData(&ib_packet,
                                                   emu_state_ptr->m_submit_index,
                                                   emu_state_ptr->GetCurIb()->m_cur_va,
                                                   sizeof(PM4_CP_INDIRECT_BUFFER)));
        IbType ib_type = (header.type7.opcode == CP_INDIRECT_BUFFER_CHAIN) ? IbType::kChain :
                                                                             IbType::kCall;
        emu_state_ptr->GetCurIb()->m_ib_queue_index = 0;
        emu_state_ptr->GetCurIb()->m_ib_queue_size = 0;
        if (!QueueIB(ib_packet.IB_BASE,
                     ib_packet.bitfields1.IB_SIZE,
                     false,
                     ib_type,
                     emu_state_ptr))
        {
            return false;
        }
        AdvancePacket(emu_state_ptr, header);
        if (!AdvanceToQueuedIB(mem_manager, emu_state_ptr, callbacks))
            return false;
    }
    // Parse CP_SET_AMBLE (previously CP_SET_CTXSWITCH_IB), since it references implicit IBs
    else if (header.type == 7 && header.type7.opcode == CP_SET_AMBLE)
    {
        // For simplicity sake, treat CP_SET_AMBLE essentially as a
        // CALL (i.e. jump to the next IB level), although the hardware probably
        // doesn't do that.
        PM4_CP_SET_AMBLE packet;
        DIVE_VERIFY(mem_manager.RetrieveMemoryData(&packet,
                                                   emu_state_ptr->m_submit_index,
                                                   emu_state_ptr->GetCurIb()->m_cur_va,
                                                   (header.type7.count + 1) * sizeof(uint32_t)));

        // Sometimes this packet is used for purposes other than to jump to an IB. Check size.
        // Example: When TYPE is SAVE_IB
        AdvancePacket(emu_state_ptr, header);
        if (packet.bitfields1.DWORDS != 0)
        {
            emu_state_ptr->GetCurIb()->m_ib_queue_index = 0;
            emu_state_ptr->GetCurIb()->m_ib_queue_size = 0;
            if (!QueueIB(packet.ADDR,
                         packet.bitfields1.DWORDS,
                         false,
                         IbType::kContextSwitchIb,
                         emu_state_ptr))
            {
                return false;
            }
            if (!AdvanceToQueuedIB(mem_manager, emu_state_ptr, callbacks))
                return false;
        }
    }
    // Parse CP_SET_DRAW_STATE, since it references implicit IBs
    else if (header.type == 7 && header.type7.opcode == CP_SET_DRAW_STATE)
    {
        // For simplicity sake, treat CP_SET_DRAW_STATE essentially as multiple
        // CALLs (i.e. jump to the next IB level), although the hardware probably
        // doesn't do that.
        PM4_CP_SET_DRAW_STATE packet;
        DIVE_VERIFY(mem_manager.RetrieveMemoryData(&packet,
                                                   emu_state_ptr->m_submit_index,
                                                   emu_state_ptr->GetCurIb()->m_cur_va,
                                                   (header.type7.count + 1) * sizeof(uint32_t)));
        uint32_t packet_size = (packet.HEADER.count * sizeof(uint32_t));
        uint32_t array_size = packet_size / sizeof(PM4_CP_SET_DRAW_STATE::ARRAY_ELEMENT);
        DIVE_ASSERT((packet_size % sizeof(PM4_CP_SET_DRAW_STATE::ARRAY_ELEMENT)) == 0);
        bool ib_queued = false;
        emu_state_ptr->GetCurIb()->m_ib_queue_index = 0;
        emu_state_ptr->GetCurIb()->m_ib_queue_size = 0;
        for (uint32_t i = 0; i < array_size; i++)
        {
            if (packet.ARRAY[i].bitfields0.DISABLE_ALL_GROUPS)
                break;
            if (packet.ARRAY[i].bitfields0.DISABLE)
                continue;

            uint32_t enable_mask = packet.ARRAY[i].bitfields0.BINNING |
                                   (packet.ARRAY[i].bitfields0.GMEM << 1) |
                                   (packet.ARRAY[i].bitfields0.SYSMEM << 2);

            ib_queued = true;
            if (!QueueIB(packet.ARRAY[i].ADDR,
                         packet.ARRAY[i].bitfields0.COUNT,
                         false,
                         IbType::kDrawState,
                         emu_state_ptr,
                         enable_mask))
            {
                return false;
            }
        }
        AdvancePacket(emu_state_ptr, header);
        if (ib_queued)
        {
            if (!AdvanceToQueuedIB(mem_manager, emu_state_ptr, callbacks))
                return false;
        }
    }
    else if ((header.type == 7) && (header.type7.opcode == CP_START_BIN))
    {

        PM4_CP_START_BIN packet;
        DIVE_VERIFY(mem_manager.RetrieveMemoryData(&packet,
                                                   emu_state_ptr->m_submit_index,
                                                   emu_state_ptr->GetCurIb()->m_cur_va,
                                                   sizeof(PM4_CP_START_BIN)));

        // The CP_START_BIN & CP_END_BIN are pm4s only availabe at a650+
        // here is the layout:
        //  |CP_START_BIN|
        //  |Prefix_block_0|
        //  |Prefix_block_1|
        //  ...
        //  |Prefix_block_n|
        //  |Common_block|
        //  |CP_END_BIN|
        // All the prefix blocks are in a separate IB
        // Prefix_block contains setup for each tile, like viewport/scissors with fixed block size
        // Common_block contains drawcalls and resolve

        uint64_t cp_start_common_block_va = emu_state_ptr->GetCurIb()->m_cur_va +
                                            GetPacketSize(header) * sizeof(uint32_t);

        // A CP_START_BIN is always paired with a CP_END_BIN within the same command buffer, at the
        // same IB Level. So peek ahead to find where the CP_END_BIN appears so that the size of the
        // common block can be determined. No need to emulate the contents of any intervening IBs,
        // since we know the CP_END_BIN is at the same ib level
        EmulateState::IbStack *cur_ib_level = &emu_state_ptr
                                               ->m_ib_stack[emu_state_ptr->m_top_of_stack];
        uint64_t temp_va = cur_ib_level->m_cur_va;
        uint32_t common_block_dword_size = UINT32_MAX;
        while (true)
        {
            Pm4Header temp_header;
            DIVE_VERIFY(mem_manager.RetrieveMemoryData(&temp_header,
                                                       emu_state_ptr->m_submit_index,
                                                       temp_va,
                                                       sizeof(Pm4Header)));
            if (temp_header.type == 7 && temp_header.type7.opcode == CP_END_BIN)
            {
                uint64_t common_block_size = temp_va - cp_start_common_block_va;
                common_block_dword_size = (uint32_t)(common_block_size / sizeof(uint32_t));
                break;
            }
            uint32_t packet_size = GetPacketSize(temp_header);
            temp_va += packet_size * sizeof(uint32_t);

            // Make sure it doesn't run past the end
            DIVE_ASSERT(temp_va < cur_ib_level->m_cur_ib_addr +
                                  cur_ib_level->m_cur_ib_size_in_dwords * sizeof(uint32_t));
        };

        // Make sure the CP_END_BIN was found
        DIVE_ASSERT(common_block_dword_size != UINT32_MAX);

        // Skip past common section, since it will be processed as ibs in the next loop
        cur_ib_level->m_cur_va = cp_start_common_block_va +
                                 common_block_dword_size * sizeof(uint32_t);

        // Go through each prefix+common pair
        for (uint32_t bin = 0; bin < packet.BIN_COUNT; ++bin)
        {
            if (!QueueIB(packet.PREFIX_ADDR + bin * packet.PREFIX_DWORDS * sizeof(uint32_t),
                         packet.PREFIX_DWORDS,
                         false,
                         IbType::kBinPrefix,
                         emu_state_ptr))
            {
                return false;
            }

            if (!QueueIB(cp_start_common_block_va,
                         common_block_dword_size,
                         false,
                         IbType::kBinCommon,
                         emu_state_ptr))
            {
                return false;
            }
        }
        if (!AdvanceToQueuedIB(mem_manager, emu_state_ptr, callbacks))
            return false;
    }
    else if ((header.type == 7) && (header.type7.opcode == CP_FIXED_STRIDE_DRAW_TABLE))
    {
        // CP_FIXED_STRIDE_DRAW_TABLE only availabe at a7xx+
        // Executes an array of fixed-size command buffers where each buffer is assumed to have one
        // draw call, skipping buffers with non - visible draw calls.
        // if CP_START_BIN/CP_END_BIN are used, and CP_FIXED_STRIDE_DRAW_TABLE is used for
        // drawcalls, it will be in the Common_block
        PM4_CP_FIXED_STRIDE_DRAW_TABLE packet;
        DIVE_VERIFY(mem_manager.RetrieveMemoryData(&packet,
                                                   emu_state_ptr->m_submit_index,
                                                   emu_state_ptr->GetCurIb()->m_cur_va,
                                                   sizeof(PM4_CP_FIXED_STRIDE_DRAW_TABLE)));

        for (uint32_t draw = 0; draw < packet.bitfields2.COUNT; ++draw)
        {
            if (!QueueIB(packet.IB_BASE + draw * packet.bitfields1.STRIDE * sizeof(uint32_t),
                         packet.bitfields1.STRIDE,
                         false,
                         IbType::kFixedStrideDrawTable,
                         emu_state_ptr))
            {
                return false;
            }
        }

        AdvancePacket(emu_state_ptr, header);
        if (!AdvanceToQueuedIB(mem_manager, emu_state_ptr, callbacks))
            return false;
    }
    else
    {
        AdvancePacket(emu_state_ptr, header);
    }

    // Could be pointing to an IB that needs to be "skipped", or reached the end of current IB.
    // Advance to the next valid IB in that case
    if (!CheckAndAdvanceIB(mem_manager, emu_state_ptr, callbacks))
        return false;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool EmulatePM4::QueueIB(uint64_t      ib_addr,
                         uint32_t      ib_size_in_dwords,
                         bool          skip,
                         IbType        ib_type,
                         EmulateState *emu_state,
                         uint32_t      enable_mask) const
{
    // Grab current stack level info
    EmulateState::IbStack *cur_ib_level = emu_state->GetCurIb();

    // Queue up only non-0-sized IBs
    if (ib_size_in_dwords == 0)
        return true;

    if (cur_ib_level->m_ib_queue_size >= kMaxPendingIbs)
    {
        DIVE_ASSERT(false);
        return false;
    }

    // Add to list of IBs to CALL/CHAIN to
    cur_ib_level->m_ib_queue_addrs[cur_ib_level->m_ib_queue_size] = ib_addr;
    cur_ib_level->m_ib_queue_sizes_in_dwords[cur_ib_level->m_ib_queue_size] = ib_size_in_dwords;
    cur_ib_level->m_ib_queue_skip[cur_ib_level->m_ib_queue_size] = skip;
    cur_ib_level->m_ib_queue_enable_mask[cur_ib_level->m_ib_queue_size] = enable_mask;
    cur_ib_level->m_ib_queue_type[cur_ib_level->m_ib_queue_size] = ib_type;
    cur_ib_level->m_ib_queue_size++;
    return true;
}

//--------------------------------------------------------------------------------------------------
bool EmulatePM4::AdvanceToQueuedIB(const IMemoryManager &mem_manager,
                                   EmulateState         *emu_state,
                                   EmulateCallbacksBase &callbacks) const
{
    // Grab current stack level info
    EmulateState::IbStack *prev_ib_level = emu_state->GetCurIb();

    // Check if there are any queued IBs to advance to. If not, early out.
    if (prev_ib_level->m_ib_queue_index >= prev_ib_level->m_ib_queue_size)
        return true;

    // Can't advance to a queued up IB if there are no IBs queued up!
    DIVE_ASSERT(prev_ib_level->m_ib_queue_size != 0);

    uint32_t index = prev_ib_level->m_ib_queue_index;
    if (prev_ib_level->m_ib_queue_type[index] != IbType::kChain)
    {
        DIVE_ASSERT(emu_state->m_top_of_stack < IbLevel::kTotalIbLevels - 1);

        // If it's calling into another IB from the primary ring, then update the ib index.
        // Note that only IBs called from primary ring as labelled as "normal" ibs
        if (prev_ib_level->m_ib_queue_type[index] == IbType::kNormal)
            emu_state->m_ib_index = prev_ib_level->m_ib_queue_index;

        // Enter next IB level (CALL)
        emu_state->m_top_of_stack = (IbLevel)(emu_state->m_top_of_stack + 1);

        EmulateState::IbStack *new_ib_level = emu_state->GetCurIb();
        new_ib_level->m_cur_va = prev_ib_level->m_ib_queue_addrs[index];
        new_ib_level->m_cur_ib_size_in_dwords = prev_ib_level->m_ib_queue_sizes_in_dwords[index];
        new_ib_level->m_cur_ib_skip = prev_ib_level->m_ib_queue_skip[index];
        new_ib_level->m_cur_ib_enable_mask = prev_ib_level->m_ib_queue_enable_mask[index];
        new_ib_level->m_cur_ib_addr = new_ib_level->m_cur_va;
        new_ib_level->m_cur_ib_type = prev_ib_level->m_ib_queue_type[index];
        new_ib_level->m_ib_queue_index = new_ib_level->m_ib_queue_size = 0;
    }
    else
    {
        DIVE_ASSERT(prev_ib_level->m_ib_queue_size == 1);
        DIVE_ASSERT(prev_ib_level->m_ib_queue_index == 0);

        // The CHAIN packet should be the last packet in the current IB. Double check this.
        uint64_t cur_end_addr = prev_ib_level->m_cur_ib_addr +
                                prev_ib_level->m_cur_ib_size_in_dwords * sizeof(uint32_t);
        if (prev_ib_level->m_cur_va != cur_end_addr)
        {
            DIVE_ASSERT(false);
            return false;
        }
        prev_ib_level->m_cur_va = prev_ib_level->m_ib_queue_addrs[0];
        prev_ib_level->m_cur_ib_size_in_dwords = prev_ib_level->m_ib_queue_sizes_in_dwords[0];
        prev_ib_level->m_cur_ib_skip = prev_ib_level->m_ib_queue_skip[0];
        prev_ib_level->m_cur_ib_enable_mask = prev_ib_level->m_ib_queue_enable_mask[0];
        prev_ib_level->m_cur_ib_addr = prev_ib_level->m_cur_va;
    }

    // Advance the queue index to next element in the queue
    prev_ib_level->m_ib_queue_index++;

    EmulateState::IbStack *cur_ib_level = emu_state->GetCurIb();

    // It's possible that an application has already reset/destroyed the command buffer. Check
    // whether the memory is valid (ie. overwritten), and skip it if it isn't
    bool skip_ib = !mem_manager.IsValid(emu_state->m_submit_index,
                                        cur_ib_level->m_cur_va,
                                        cur_ib_level->m_cur_ib_size_in_dwords * sizeof(uint32_t));
    cur_ib_level->m_cur_ib_skip |= skip_ib;

    // Start-Ib Callback
    {
        IndirectBufferInfo call_chain_ib_info;
        call_chain_ib_info.m_va_addr = cur_ib_level->m_cur_va;
        call_chain_ib_info.m_size_in_dwords = cur_ib_level->m_cur_ib_size_in_dwords;
        call_chain_ib_info.m_ib_level = (uint8_t)emu_state->m_top_of_stack;
        call_chain_ib_info.m_skip = cur_ib_level->m_cur_ib_skip;
        call_chain_ib_info.m_enable_mask = cur_ib_level->m_cur_ib_enable_mask;
        if (!callbacks.OnIbStart(emu_state->m_submit_index,
                                 emu_state->m_ib_index,
                                 call_chain_ib_info,
                                 cur_ib_level->m_cur_ib_type))
        {
            return false;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool EmulatePM4::CheckAndAdvanceIB(const IMemoryManager &mem_manager,
                                   EmulateState         *emu_state,
                                   EmulateCallbacksBase &callbacks) const
{
    // Could be pointing to an IB that needs to be "skipped", or reached the end of current IB.
    // Loop until pointing to a "valid" IB.
    // m_cur_va == UINT64_MAX when there are no more IBs to execute (i.e. set for primary ring)
    while (emu_state->GetCurIb()->m_cur_va != UINT64_MAX && emu_state->GetCurIb()->EndOfCurIb())
    {
        // Done with current IB. Jump back to an earlier level.
        if (!AdvanceOutOfIB(emu_state, callbacks))
            return false;

        // Handle any pending queued IBs
        if (!AdvanceToQueuedIB(mem_manager, emu_state, callbacks))
            return false;
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
void EmulatePM4::AdvancePacket(EmulateState *emu_state, Pm4Header header) const
{
    uint32_t packet_size = GetPacketSize(header);
    emu_state->GetCurIb()->m_cur_va += packet_size * sizeof(uint32_t);
}

//--------------------------------------------------------------------------------------------------
bool EmulatePM4::AdvanceOutOfIB(EmulateState *emu_state, EmulateCallbacksBase &callbacks) const
{
    EmulateState::IbStack *cur_ib_level = emu_state->GetCurIb();

    emu_state->m_top_of_stack = (IbLevel)(emu_state->m_top_of_stack - 1);

    IndirectBufferInfo ib_info;
    ib_info.m_va_addr = cur_ib_level->m_cur_ib_addr;
    ib_info.m_size_in_dwords = cur_ib_level->m_cur_ib_size_in_dwords;
    ib_info.m_ib_level = (uint8_t)emu_state->m_top_of_stack;
    ib_info.m_skip = cur_ib_level->m_cur_ib_skip;

    // End-Ib Callback
    if (!callbacks.OnIbEnd(emu_state->m_submit_index, emu_state->m_ib_index, ib_info))
        return false;

    return true;
}

//--------------------------------------------------------------------------------------------------
uint32_t EmulatePM4::CalcParity(uint32_t val)
{
    // See: http://graphics.stanford.edu/~seander/bithacks.html#ParityParallel
    // note that we want odd parity so 0x6996 is inverted.
    val ^= val >> 16;
    val ^= val >> 8;
    val ^= val >> 4;
    val &= 0xf;
    return (~0x6996 >> val) & 1;
}

//--------------------------------------------------------------------------------------------------
bool IsDrawDispatchEventOpcode(uint32_t opcode)
{
    return IsDrawEventOpcode(opcode) || IsDispatchEventOpcode(opcode);
}

//--------------------------------------------------------------------------------------------------
bool IsDispatchEventOpcode(uint32_t opcode)
{
    switch (opcode)
    {
    case CP_EXEC_CS_INDIRECT:
    case CP_EXEC_CS:
        return true;
    };
    return false;
}
//--------------------------------------------------------------------------------------------------
bool IsDrawEventOpcode(uint32_t opcode)
{
    switch (opcode)
    {
    case CP_DRAW_INDX:
    case CP_DRAW_INDX_OFFSET:
    case CP_DRAW_INDIRECT:
    case CP_DRAW_INDX_INDIRECT:
    case CP_DRAW_INDIRECT_MULTI:
    case CP_DRAW_AUTO:
        return true;
    };
    return false;
}

//--------------------------------------------------------------------------------------------------
uint32_t GetPacketSize(Pm4Header header)
{
    // Assumes the packet is valid, so doesn't check the parity bits at all
    switch (header.type)
    {
    case 2:
        return 1;
    case 4:
    {
        return header.type4.count + 1;
    }
    case 7:
        return header.type7.count + 1;
    }
    DIVE_ASSERT(false);
    return 0;
}

// =================================================================================================
// EmulateCallbacksBase
// =================================================================================================

bool EmulateCallbacksBase::ProcessSubmits(const DiveVector<SubmitInfo> &submits,
                                          const IMemoryManager         &mem_manager)
{
    for (uint32_t submit_index = 0; submit_index < submits.size(); ++submit_index)
    {
        const Dive::SubmitInfo &submit_info = submits[submit_index];
        OnSubmitStart(submit_index, submit_info);

        if (submit_info.IsDummySubmit())
        {
            OnSubmitEnd(submit_index, submit_info);
            continue;
        }

        // Only gfx or compute engine types are parsed
        if ((submit_info.GetEngineType() != Dive::EngineType::kUniversal) &&
            (submit_info.GetEngineType() != Dive::EngineType::kCompute))
        {
            OnSubmitEnd(submit_index, submit_info);
            continue;
        }

        EmulatePM4 emu;
        if (!emu.ExecuteSubmit(*this,
                               mem_manager,
                               submit_index,
                               submit_info.GetNumIndirectBuffers(),
                               submit_info.GetIndirectBufferInfoPtr()))
            return false;

        OnSubmitEnd(submit_index, submit_info);
    }
    return true;
}

}  // namespace Dive
