
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
#include "emulate_pm4.h"
#include "memory_manager_base.h"
#include "pm4_info.h"

#include <stdarg.h>
#include <cerrno>
#include <cstdio>
#include <cstring>

namespace Dive
{

union Pm4Header
{
    struct
    {
        uint32_t offset_parity : 28;
        uint32_t type : 4;
    };
    uint32_t u32All;
};

// =================================================================================================
// EmulateStateTracker
// =================================================================================================
EmulateStateTracker::EmulateStateTracker() {}

//--------------------------------------------------------------------------------------------------
bool EmulateStateTracker::OnPacket(const IMemoryManager &mem_manager,
                                   uint32_t              submit_index,
                                   uint32_t              ib_index,
                                   uint64_t              va_addr,
                                   Pm4Type               type,
                                   uint32_t              header)
{
    // type 4 is setting register
    if (type != Pm4Type::kType4)
        return true;

    Pm4Type4Header *type4_header = (Pm4Type4Header *)&header;

    uint32_t offset_in_bytes = 0;
    uint32_t dword = 0;
    while (dword < type4_header->count)
    {
        uint64_t reg_va_addr = va_addr + sizeof(header) + offset_in_bytes;
        uint32_t reg_offset = type4_header->offset + dword;
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
            DIVE_VERIFY(mem_manager.CopyMemory(&reg_value,
                                               submit_index,
                                               reg_va_addr + i * dword_in_bytes,
                                               dword_in_bytes));
            SetReg(reg_offset + i, reg_value);
        }

        dword += size_in_dwords;
    }

    return true;
}
/*
//--------------------------------------------------------------------------------------------------
bool EmulateStateTracker::HandleLoadUConfigRegPacket(const IMemoryManager &mem_manager,
                                                     uint32_t              submit_index,
                                                     uint64_t              va_addr)
{
    PM4_ME_LOAD_UCONFIG_REG packet;
    if (!mem_manager.CopyMemory(&packet, submit_index, va_addr, sizeof(PM4_ME_LOAD_SH_REG)))
        return false;

    // The LOAD_UCONFIG_REG is only processed if the corresponding bit in CONTEXT_CONTROL packet is
    // set (Can also be enabled by the RLC, but Dive does not have visibility there)
    if (!m_load_global_uconfig)
        return true;

    uint64_t lo = packet.bitfields2.base_address_lo;
    uint64_t hi = packet.base_address_hi;
    uint64_t addr = ((hi << 32) | (lo << 2));

    // How many pairs is determined by packet size, with the first pair embedded in packet
    struct PairInfo
    {
        uint32_t m_reg_offset;
        uint32_t m_num_dwords;
    };
    static_assert(sizeof(PairInfo) == 2 * sizeof(uint32_t), "Incorrect size!");
    uint32_t packet_size_dwords = sizeof(PM4_ME_LOAD_UCONFIG_REG) / sizeof(uint32_t);
    uint32_t extra_dwords = packet.header.count + 2 - packet_size_dwords;
    uint32_t num_pairs = 1;  // One pair included in the packet
    num_pairs += extra_dwords / (sizeof(PairInfo) / sizeof(uint32_t));
    DIVE_ASSERT(extra_dwords % (sizeof(PairInfo) / sizeof(uint32_t)) == 0);

    // First pair included at the end of the packet
    uint32_t end_offset_byte = 0;
    uint64_t cur_pair_addr = va_addr + sizeof(PM4_ME_LOAD_UCONFIG_REG) - sizeof(PairInfo);
    for (uint16_t pair = 0; pair < num_pairs; ++pair)
    {
        PairInfo pair_info;
        if (!mem_manager.CopyMemory(&pair_info, submit_index, cur_pair_addr, sizeof(PairInfo)))
            return false;

        uint32_t cur_offset_dword = (pair_info.m_reg_offset + pair_info.m_num_dwords);
        uint32_t cur_offset_byte = cur_offset_dword * sizeof(uint32_t);
        if (cur_offset_byte > end_offset_byte)
            end_offset_byte = cur_offset_byte;

        // Pass m_num_dwords worth of registers to HandleSetShRegister()
        // Do not update tracking because this packet is used as an initialization pass
        if (!HandleSetUConfigRegister(mem_manager,
                                      submit_index,
                                      (uint16_t)pair_info.m_reg_offset,
                                      addr + pair_info.m_reg_offset * sizeof(uint32_t),
                                      (uint16_t)pair_info.m_num_dwords,
                                      false))
        {
            return false;
        }
        cur_pair_addr += sizeof(PairInfo);
    }

    // Set m_buffer_size to the last offset byte loaded from
    m_buffer_va = addr;
    m_buffer_size = end_offset_byte;
    return true;
}

//--------------------------------------------------------------------------------------------------
bool EmulateStateTracker::HandleSetUConfigRegister(const IMemoryManager &mem_manager,
                                                   uint32_t              submit_index,
                                                   uint16_t              reg_offset,
                                                   uint64_t              reg_data_va,
                                                   uint16_t              reg_count,
                                                   bool                  update_tracking)
{
    for (uint16_t reg = 0; reg < reg_count; ++reg)
    {
        uint16_t cur_reg_offset = reg_offset + reg;
        uint64_t cur_reg_addr = reg_data_va + reg * sizeof(uint32_t);

        uint32_t cur_reg_data;
        if (!mem_manager.CopyMemory(&cur_reg_data, submit_index, cur_reg_addr, sizeof(uint32_t)))
            return false;

        m_uconfig_data[reg_offset] = cur_reg_data;

        if (update_tracking)
            m_uconfig_is_set[cur_reg_offset / 8] |= (1 << (cur_reg_offset % 8));
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
bool EmulateStateTracker::HandleLoadShRegPacket(const IMemoryManager &mem_manager,
                                                uint32_t              submit_index,
                                                uint64_t              va_addr)
{
    PM4_ME_LOAD_SH_REG packet;
    if (!mem_manager.CopyMemory(&packet, submit_index, va_addr, sizeof(PM4_ME_LOAD_SH_REG)))
        return false;

    // The LOAD_SH_REG is only processed if the corresponding bit in CONTEXT_CONTROL packet is set
    // (Can also be enabled by the RLC, but Dive does not have visibility there)
    if (!m_load_gfx_sh_regs && packet.header.shaderType == 0)
        return true;
    if (!m_load_cs_sh_regs && packet.header.shaderType == 1)
        return true;

    // One of the uses of the LoadShReg packet is to set the SH_REG_BASE for gfx and compute
    // shader types. However, this packet is not used in this way by the ICD. So assert here to
    // make sure it will not be used this way - because otherwise the state tracker would have
    // to keep track of the SH_REG_BASE as well.
    // Note: This packet is used to set SH_REG_BASE when both word4 & word5 are 0 (sufficient to
    // check just the num_dwords) and the packet size is only big enough for the PM4_ME_LOAD_SH_REG
    DIVE_ASSERT(packet.header.count != (sizeof(PM4_ME_LOAD_SH_REG) / sizeof(uint32_t) - 2) ||
                packet.bitfields5.num_dword != 0);

    uint64_t lo = packet.bitfields2.base_address_lo;
    uint64_t hi = packet.base_address_hi;
    uint64_t addr = ((hi << 32) | (lo << 2));

    // How many pairs is determined by packet size, with the first pair embedded in packet
    struct PairInfo
    {
        uint32_t m_reg_offset : 16;
        uint32_t : 16;
        uint32_t m_num_dwords : 14;
        uint32_t : 18;
    };
    static_assert(sizeof(PairInfo) == 2 * sizeof(uint32_t), "Incorrect size!");
    uint32_t packet_size_dwords = sizeof(PM4_ME_LOAD_SH_REG) / sizeof(uint32_t);
    uint32_t extra_dwords = packet.header.count + 2 - packet_size_dwords;
    uint32_t num_pairs = 1;  // One pair included in the packet
    num_pairs += extra_dwords / (sizeof(PairInfo) / sizeof(uint32_t));
    DIVE_ASSERT(extra_dwords % (sizeof(PairInfo) / sizeof(uint32_t)) == 0);

    // First pair included at the end of the packet
    uint32_t end_offset_byte = 0;
    uint64_t cur_pair_addr = va_addr + sizeof(PM4_ME_LOAD_SH_REG) - sizeof(PairInfo);
    for (uint16_t pair = 0; pair < num_pairs; ++pair)
    {
        PairInfo pair_info;
        if (!mem_manager.CopyMemory(&pair_info, submit_index, cur_pair_addr, sizeof(PairInfo)))
            return false;

        uint32_t cur_offset_dword = (pair_info.m_reg_offset + pair_info.m_num_dwords);
        uint32_t cur_offset_byte = cur_offset_dword * sizeof(uint32_t);
        if (cur_offset_byte > end_offset_byte)
            end_offset_byte = cur_offset_byte;

        // Pass m_num_dwords worth of registers to HandleSetShRegister()
        // Do not update tracking because this packet is used as an initialization pass
        if (!HandleSetShRegister(mem_manager,
                                 submit_index,
                                 (uint16_t)pair_info.m_reg_offset,
                                 addr + pair_info.m_reg_offset * sizeof(uint32_t),
                                 (uint16_t)pair_info.m_num_dwords,
                                 false))
        {
            return false;
        }
        cur_pair_addr += sizeof(PairInfo);
    }

    // Set m_buffer_size to the last offset byte loaded from
    m_buffer_va = addr;
    m_buffer_size = end_offset_byte;
    return true;
}

//--------------------------------------------------------------------------------------------------
bool EmulateStateTracker::HandleLoadShRegIndexPacket(const IMemoryManager &mem_manager,
                                                     uint32_t              submit_index,
                                                     uint64_t              va_addr)
{
    PM4_ME_LOAD_SH_REG_INDEX packet;
    if (!mem_manager.CopyMemory(&packet, submit_index, va_addr, sizeof(PM4_ME_LOAD_SH_REG_INDEX)))
        return false;

    // Only the following are used in the driver right now, afaik
    DIVE_ASSERT(packet.bitfields2.index == index__pfp_load_sh_reg_index__direct_addr);

    uint64_t lo = packet.bitfields2.mem_addr_lo;
    uint64_t hi = packet.mem_addr_hi;
    uint64_t addr = ((hi << 32) | (lo << 2));
    if (packet.bitfields4.data_format == data_format__pfp_load_sh_reg_index__offset_and_data)
    {
        uint32_t       num_pairs = packet.bitfields5.num_dwords;
        const uint32_t kRegPairSize = 2 * sizeof(uint32_t);
        for (uint32_t pair = 0; pair < num_pairs; ++pair)
        {
            struct RegOffsetDword
            {
                uint32_t m_reg_offset : 16;
                uint32_t m_reserved : 16;
            };
            RegOffsetDword reg_offset_dword;
            uint64_t       cur_pair_addr = addr + pair * kRegPairSize;

            // Get the register offset dword, which is the first uint32_t of the pair
            if (!mem_manager.CopyMemory(&reg_offset_dword,
                                        submit_index,
                                        cur_pair_addr,
                                        sizeof(uint32_t)))
                return false;

            // Pass address of the register data to HandleSetShRegister()
            if (!HandleSetShRegister(mem_manager,
                                     submit_index,
                                     reg_offset_dword.m_reg_offset,
                                     cur_pair_addr + sizeof(uint32_t),
                                     1))
            {
                return false;
            }
        }
        m_buffer_size = num_pairs * kRegPairSize;
    }
    else if (packet.bitfields4.data_format == data_format__pfp_load_sh_reg_index__offset_and_size)
    {
        for (uint16_t reg = 0; reg < packet.bitfields5.num_dwords; ++reg)
        {
            uint16_t packet_reg_offset = packet.bitfields4.reg_offset;
            uint16_t reg_offset = packet_reg_offset + reg;
            uint64_t cur_addr = addr + reg * sizeof(uint32_t);

            // Pass address of the register data to HandleSetShRegister()
            if (!HandleSetShRegister(mem_manager, submit_index, reg_offset, cur_addr, 1))
            {
                return false;
            }
        }
        m_buffer_size = packet.bitfields5.num_dwords * sizeof(uint32_t);
    }
    m_buffer_va = addr;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool EmulateStateTracker::HandleSetShRegister(const IMemoryManager &mem_manager,
                                              uint32_t              submit_index,
                                              uint16_t              reg_offset,
                                              uint64_t              reg_data_va,
                                              uint16_t              reg_count,
                                              bool                  update_tracking)
{
    return false;
}

//--------------------------------------------------------------------------------------------------
bool EmulateStateTracker::HandleLoadContextRegPacket(const IMemoryManager &mem_manager,
                                                     uint32_t              submit_index,
                                                     uint64_t              va_addr)
{
    PM4_ME_LOAD_CONTEXT_REG packet;
    if (!mem_manager.CopyMemory(&packet, submit_index, va_addr, sizeof(PM4_ME_LOAD_CONTEXT_REG)))
        return false;

    // The LOAD_CONTEXT_REG is only processed if the corresponding bit in CONTEXT_CONTROL packet is
    // set (Can also be enabled by the RLC, but Dive does not have visibility there)
    if (!m_load_per_context_state)
        return true;

    // One of the uses of the LoadContextReg packet is to set the CONTEXT_REG_BASE. However, this
    // packet is not used in this way by the ICD. So assert here to make sure it will not be used
    // this way - because otherwise the state tracker would have to keep track of the
    // CONTEXT_REG_BASE as well. Note: This packet is used to set CONTEXT_REG_BASE when both word4 &
    // word5 are 0 (sufficient to check just the num_dwords) and the packet size is only big enough
    // for the PM4_ME_LOAD_CONTEXT_REG
    DIVE_ASSERT(packet.header.count != (sizeof(PM4_ME_LOAD_CONTEXT_REG) / sizeof(uint32_t) - 2) ||
                packet.bitfields5.num_dwords != 0);

    uint64_t lo = packet.bitfields2.base_addr_lo;
    uint64_t hi = packet.base_addr_hi;
    uint64_t addr = ((hi << 32) | (lo << 2));

    // How many pairs is determined by packet size, with the first pair embedded in packet
    struct PairInfo
    {
        uint32_t m_reg_offset : 16;
        uint32_t : 16;
        uint32_t m_num_dwords : 14;
        uint32_t : 18;
    };
    static_assert(sizeof(PairInfo) == 2 * sizeof(uint32_t), "Incorrect size!");
    uint32_t packet_size_dwords = sizeof(PM4_ME_LOAD_CONTEXT_REG) / sizeof(uint32_t);
    uint32_t extra_dwords = packet.header.count + 2 - packet_size_dwords;
    uint32_t num_pairs = 1;  // One pair included in the packet
    num_pairs += extra_dwords / (sizeof(PairInfo) / sizeof(uint32_t));
    DIVE_ASSERT(extra_dwords % (sizeof(PairInfo) / sizeof(uint32_t)) == 0);

    // First pair included at the end of the packet
    uint32_t end_offset_byte = 0;
    uint64_t cur_pair_addr = va_addr + sizeof(PM4_ME_LOAD_CONTEXT_REG) - sizeof(PairInfo);
    for (uint16_t pair = 0; pair < num_pairs; ++pair)
    {
        PairInfo pair_info;
        if (!mem_manager.CopyMemory(&pair_info, submit_index, cur_pair_addr, sizeof(PairInfo)))
            return false;

        uint32_t cur_offset_dword = (pair_info.m_reg_offset + pair_info.m_num_dwords);
        uint32_t cur_offset_byte = cur_offset_dword * sizeof(uint32_t);
        if (cur_offset_byte > end_offset_byte)
            end_offset_byte = cur_offset_byte;

        // Pass m_num_dwords worth of registers to HandleSetContextRegister()
        // Do not update tracking because this packet is used as an initialization pass
        if (!HandleSetContextRegister(mem_manager,
                                      submit_index,
                                      (uint16_t)pair_info.m_reg_offset,
                                      addr + pair_info.m_reg_offset * sizeof(uint32_t),
                                      (uint16_t)pair_info.m_num_dwords,
                                      false))
        {
            return false;
        }
        cur_pair_addr += sizeof(PairInfo);
    }

    // Set m_buffer_size to the last offset byte loaded from
    m_buffer_va = addr;
    m_buffer_size = end_offset_byte;
    return true;
}

//--------------------------------------------------------------------------------------------------
bool EmulateStateTracker::HandleLoadContextRegIndexPacket(const IMemoryManager &mem_manager,
                                                          uint32_t              submit_index,
                                                          uint64_t              va_addr)
{
    PM4_ME_LOAD_CONTEXT_REG_INDEX packet;
    if (!mem_manager.CopyMemory(&packet,
                                submit_index,
                                va_addr,
                                sizeof(PM4_ME_LOAD_CONTEXT_REG_INDEX)))
        return false;

    // Only the following are used in the driver right now, afaik
    DIVE_ASSERT(packet.bitfields2.index == index__pfp_load_context_reg_index__direct_addr);

    uint64_t lo = packet.bitfields2.mem_addr_lo;
    uint64_t hi = packet.mem_addr_hi;
    uint64_t addr = ((hi << 32) | (lo << 2));
    if (packet.bitfields4.data_format == data_format__pfp_load_context_reg_index__offset_and_data)
    {
        uint32_t              num_pairs = packet.bitfields5.num_dwords;
        static const uint32_t kRegPairSize = 2 * sizeof(uint32_t);
        for (uint32_t pair = 0; pair < num_pairs; ++pair)
        {
            struct RegOffsetDword
            {
                uint32_t m_reg_offset : 16;
                uint32_t m_reserved : 16;
            };

            RegOffsetDword reg_offset_dword;
            uint64_t       cur_pair_addr = addr + pair * kRegPairSize;

            // Get the register offset, which is the first uint32_t of the pair
            if (!mem_manager.CopyMemory(&reg_offset_dword,
                                        submit_index,
                                        cur_pair_addr,
                                        sizeof(uint32_t)))
                return false;

            // Pass address of the register data to HandleSetContextRegister()
            if (!HandleSetContextRegister(mem_manager,
                                          submit_index,
                                          reg_offset_dword.m_reg_offset,
                                          cur_pair_addr + sizeof(uint32_t),
                                          1))
            {
                return false;
            }
        }
        m_buffer_size = num_pairs * kRegPairSize;
    }
    else if (packet.bitfields4.data_format ==
             data_format__pfp_load_context_reg_index__offset_and_size)
    {
        for (uint16_t reg = 0; reg < packet.bitfields5.num_dwords; ++reg)
        {
            uint16_t packet_reg_offset = packet.bitfields4.reg_offset;
            uint16_t reg_offset = packet_reg_offset + reg;
            uint64_t cur_addr = addr + reg * sizeof(uint32_t);

            // Pass address of the register data to HandleSetContextRegister()
            if (!HandleSetContextRegister(mem_manager, submit_index, reg_offset, cur_addr, 1))
            {
                return false;
            }
        }
        m_buffer_size = packet.bitfields5.num_dwords * sizeof(uint32_t);
    }

    m_buffer_va = addr;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool EmulateStateTracker::HandleSetContextRegister(const IMemoryManager &mem_manager,
                                                   uint32_t              submit_index,
                                                   uint16_t              reg_offset,
                                                   uint64_t              reg_data_va,
                                                   uint16_t              reg_count,
                                                   bool                  update_tracking)
{
    for (uint16_t reg = 0; reg < reg_count; ++reg)
    {
        uint16_t cur_reg_offset = reg_offset + reg;
        uint64_t cur_reg_addr = reg_data_va + reg * sizeof(uint32_t);

        uint32_t cur_reg_data;
        if (!mem_manager.CopyMemory(&cur_reg_data, submit_index, cur_reg_addr, sizeof(uint32_t)))
            return false;

        m_context_data[cur_reg_offset] = cur_reg_data;

        if (update_tracking)
            m_context_is_set[cur_reg_offset / 8] |= (1 << (cur_reg_offset % 8));
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
void EmulateStateTracker::HandleUserDataRegister(ShaderStage stage,
                                                 uint16_t    reg_offset,
                                                 uint16_t    start_reg,
                                                 uint16_t    num_regs)
{
}
*/
//--------------------------------------------------------------------------------------------------
bool EmulateStateTracker::IsUConfigStateSet(uint16_t reg) const
{
    return false;
}

//--------------------------------------------------------------------------------------------------
uint32_t EmulateStateTracker::GetUConfigRegData(uint16_t reg) const
{
    return UINT32_MAX;
}

//--------------------------------------------------------------------------------------------------
bool EmulateStateTracker::IsShStateSet(uint16_t reg) const
{
    return false;
}

//--------------------------------------------------------------------------------------------------
uint32_t EmulateStateTracker::GetShRegData(uint16_t reg) const
{
    return UINT32_MAX;
}

//--------------------------------------------------------------------------------------------------
uint32_t EmulateStateTracker::GetNumberOfUserDataRegsSetSinceLastEvent(ShaderStage stage) const
{
    // return m_num_user_data_regs_set_last_event[(uint32_t)stage];
    return 0;
}

//--------------------------------------------------------------------------------------------------
bool EmulateStateTracker::IsUserDataRegsSetSinceLastEvent(ShaderStage stage, uint8_t offset) const
{
    // Note: Gfx User_data 0, 1, 29, 30, 31 reserved for system
    //       Compute User_data 0, 1, 13, 14, 15 reserved for system
    //       Do not track these!
    // DIVE_ASSERT(offset >= 2);

    // return ((m_user_data_regs_set_last_event[(uint32_t)stage] & (1 << offset)) != 0);
    return false;
}
//--------------------------------------------------------------------------------------------------
uint64_t EmulateStateTracker::GetCurShaderAddr(ShaderStage stage) const
{
    return UINT64_MAX;
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
/*
//--------------------------------------------------------------------------------------------------
void EmulateStateTracker::UpdateUserDataRegsSetSinceLastEvent(ShaderStage stage,
                                                              uint8_t     user_data_reg)
{
    // Range sanity check
    DIVE_ASSERT(
    (stage == ShaderStage::kShaderStageCs && user_data_reg < m_num_user_data_regs_compute) ||
    (stage != ShaderStage::kShaderStageCs && user_data_reg < m_num_user_data_regs_gfx));
    DIVE_ASSERT(m_num_user_data_regs_compute <= 32 && m_num_user_data_regs_gfx <= 32);
    uint32_t prev = m_user_data_regs_set_last_event[(uint32_t)stage];
    m_user_data_regs_set_last_event[(uint32_t)stage] |= 1 << user_data_reg;
    if (m_user_data_regs_set_last_event[(uint32_t)stage] != prev)
        m_num_user_data_regs_set_last_event[(uint32_t)stage]++;
}
*/
// =================================================================================================
// EmulatePM4
// =================================================================================================
EmulatePM4::EmulatePM4() {}

//--------------------------------------------------------------------------------------------------
bool EmulatePM4::ExecuteSubmit(IEmulateCallbacks        &callbacks,
                               const IMemoryManager     &mem_manager,
                               uint32_t                  submit_index,
                               uint32_t                  num_ibs,
                               const IndirectBufferInfo *ib_ptr)
{
    // TODO: Get rid of these variables!!!
    // ############################################################
    m_submit_index = submit_index;
    m_num_ibs = num_ibs;
    m_ib_ptr = ib_ptr;
    // ############################################################

    // Used to keep track of progress of emulation so far
    EmulateState emu_state;
    emu_state.m_dcb.m_top_of_stack = EmulateState::CbState::kPrimaryRing;
    // if (!GetNextValidIb(mem_manager,
    //                     &emu_state.m_dcb.m_ib_index,
    //                     callbacks,
    //                     ib_ptr,
    //                     m_submit_index,
    //                     0))
    //     return false;
    // emu_state.m_dcb.m_cur_va = UINT64_MAX;
    // emu_state.m_ccb.m_cur_va = UINT64_MAX;
    emu_state.m_de_counter = 0;
    emu_state.m_ce_counter = 0;
    emu_state.m_is_dcb_blocked = false;
    // emu_state.m_dcb.m_is_in_ib1_chain_ib = false;
    // emu_state.m_dcb.m_is_in_ib2 = false;
    // emu_state.m_dcb.m_top_of_stack = EmulateState::CbState::kPrimaryRing;
    // emu_state.m_ccb.m_is_in_ib1_chain_ib = false;
    // emu_state.m_ccb.m_is_in_ib2 = false;
    EmulateState::IbStack *primary_ring = &emu_state.m_dcb
                                           .m_ib_stack[EmulateState::CbState::kPrimaryRing];
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
                     &emu_state.m_dcb))
        {
            return false;
        }
    }
    if (!AdvanceToQueuedIB(mem_manager, &emu_state.m_dcb, callbacks))
        return false;

    // Could be pointing to an IB that needs to be "skipped"
    // Advance to the next valid IB in that case
    if (!CheckAndAdvanceIB(mem_manager, &emu_state.m_dcb, callbacks))
        return false;

    // Should always be emulating something in an IB. If it's parked at the primary ring,
    // then that means emulation has completed
    while (emu_state.m_dcb.m_top_of_stack != EmulateState::CbState::kPrimaryRing)
    {
        // Callbacks + advance
        EmulateState::IbStack *cur_ib_level = &emu_state.m_dcb
                                               .m_ib_stack[emu_state.m_dcb.m_top_of_stack];

        Pm4Header header;
        if (!mem_manager.CopyMemory(&header,
                                    m_submit_index,
                                    cur_ib_level->m_cur_va,
                                    sizeof(Pm4Header)))
        {
            DIVE_ERROR_MSG("Unable to access packet at 0x%llx\n",
                           (unsigned long long)cur_ib_level->m_cur_va);
            return false;
        }

        // We only care about Type 2, 4 and 7 packets
        Pm4Type type = Pm4Type::kUnknown;
        if (header.type == 2)  // NOP
            type = Pm4Type::kType2;
        else if (header.type == 4)  // Register write
            type = Pm4Type::kType4;
        if (header.type == 7)  // PM4 opcode
            type = Pm4Type::kType7;

        // Check validity of packet
        if (type == Pm4Type::kType4)
        {
            Pm4Type4Header *type4_header = (Pm4Type4Header *)&header;
            if (type4_header->offset_parity != CalcParity(type4_header->offset))
                return false;
            if (type4_header->count_parity != CalcParity(type4_header->count))
                return false;
        }
        else if (type == Pm4Type::kType7)
        {
            Pm4Type7Header *type7_header = (Pm4Type7Header *)&header;
            if (type7_header->opcode_parity != CalcParity(type7_header->opcode))
                return false;
            if (type7_header->count_parity != CalcParity(type7_header->count))
                return false;
            if (type7_header->zeroes != 0)
                return false;
        }

        if (!emu_state.m_is_dcb_blocked)
            if (!callbacks.OnPacket(mem_manager,
                                    m_submit_index,
                                    emu_state.m_dcb.m_ib_index,
                                    cur_ib_level->m_cur_va,
                                    type,
                                    header.u32All))
                return false;
        if (!AdvanceCb(mem_manager, &emu_state, callbacks, type, header.u32All))
            return false;
    }  // while there are packets left in submit
    return true;
}

//--------------------------------------------------------------------------------------------------
bool EmulatePM4::AdvanceCb(const IMemoryManager &mem_manager,
                           EmulateState         *emu_state_ptr,
                           IEmulateCallbacks    &callbacks,
                           Pm4Type               type,
                           uint32_t              header) const
{
    Pm4Type7Header type7_header;
    type7_header.u32All = header;

    // Deal with calls and chains
    if (type == Pm4Type::kType7 && (type7_header.opcode == CP_INDIRECT_BUFFER ||
                                    type7_header.opcode == CP_INDIRECT_BUFFER_PFE ||
                                    type7_header.opcode == CP_INDIRECT_BUFFER_PFD ||
                                    type7_header.opcode == CP_INDIRECT_BUFFER_CHAIN ||
                                    type7_header.opcode == CP_COND_INDIRECT_BUFFER_PFE ||
                                    type7_header.opcode == CP_COND_INDIRECT_BUFFER_PFD))
    {
        PM4_CP_INDIRECT_BUFFER ib_packet;
        if (!mem_manager.CopyMemory(&ib_packet,
                                    m_submit_index,
                                    emu_state_ptr->m_dcb.GetCurIb()->m_cur_va,
                                    sizeof(PM4_CP_INDIRECT_BUFFER)))
        {
            DIVE_ERROR_MSG("Unable to access packet at 0x%llx\n",
                           (unsigned long long)emu_state_ptr->m_dcb.GetCurIb()->m_cur_va);
            return false;
        }
        uint64_t ib_addr = ((uint64_t)ib_packet.ADDR_HI << 32) | (uint64_t)ib_packet.ADDR_LO;
        IbType   ib_type = (type7_header.opcode == CP_INDIRECT_BUFFER_CHAIN) ? IbType::kChain :
                                                                               IbType::kCall;
        if (!QueueIB(ib_addr, ib_packet.SIZE, false, ib_type, &emu_state_ptr->m_dcb))
        {
            return false;
        }
        AdvancePacket(&emu_state_ptr->m_dcb, header);
        if (!AdvanceToQueuedIB(mem_manager, &emu_state_ptr->m_dcb, callbacks))
            return false;
    }
    // Parse CP_SET_DRAW_STATE, since it references implicit IBs
    else if (type == Pm4Type::kType7 && type7_header.opcode == CP_SET_DRAW_STATE)
    {
        // For simplicity sake, treat CP_SET_DRAW_STATE essentially as multiple
        // CALLs (i.e. jump to the next IB level), although the hardware probably
        // doesn't do that.
        PM4_CP_SET_DRAW_STATE packet;
        if (!mem_manager.CopyMemory(&packet,
                                    m_submit_index,
                                    emu_state_ptr->m_dcb.GetCurIb()->m_cur_va,
                                    (type7_header.count + 1) * sizeof(uint32_t)))
        {
            DIVE_ERROR_MSG("Unable to access packet at 0x%llx\n",
                           (unsigned long long)emu_state_ptr->m_dcb.GetCurIb()->m_cur_va);
            return false;
        }
        uint32_t packet_size = (packet.HEADER.count * sizeof(uint32_t));
        uint32_t array_size = packet_size / sizeof(PM4_CP_SET_DRAW_STATE::ARRAY_ELEMENT);
        DIVE_ASSERT((packet_size % sizeof(PM4_CP_SET_DRAW_STATE::ARRAY_ELEMENT)) == 0);
        bool ib_queued = false;
        for (uint32_t i = 0; i < array_size; i++)
        {
            if (packet.ARRAY[i].bitfields0.DISABLE_ALL_GROUPS)
                break;
            if (packet.ARRAY[i].bitfields0.DISABLE)
                continue;

            ib_queued = true;
            uint64_t ib_addr = ((uint64_t)packet.ARRAY[i].bitfields2.ADDR_HI << 32) |
                               (uint64_t)packet.ARRAY[i].bitfields1.ADDR_LO;
            if (!QueueIB(ib_addr,
                         packet.ARRAY[i].bitfields0.COUNT,
                         false,
                         IbType::kDrawState,
                         &emu_state_ptr->m_dcb))
            {
                return false;
            }
        }
        AdvancePacket(&emu_state_ptr->m_dcb, header);
        if (ib_queued)
            if (!AdvanceToQueuedIB(mem_manager, &emu_state_ptr->m_dcb, callbacks))
                return false;
    }
    else
    {
        AdvancePacket(&emu_state_ptr->m_dcb, header);
    }

    // Could be pointing to an IB that needs to be "skipped", or reached the end of current IB.
    // Advance to the next valid IB in that case
    if (!CheckAndAdvanceIB(mem_manager, &emu_state_ptr->m_dcb, callbacks))
        return false;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool EmulatePM4::QueueIB(uint64_t               ib_addr,
                         uint32_t               ib_size_in_dwords,
                         bool                   skip,
                         IbType                 ib_type,
                         EmulateState::CbState *cb_state) const
{
    // Grab current stack level info
    EmulateState::IbStack *cur_ib_level = cb_state->GetCurIb();

    if (cur_ib_level->m_ib_queue_size >= EmulateState::kMaxPendingIbs)
    {
        DIVE_ASSERT(false);
        return false;
    }

    // Add to list of IBs to CALL/CHAIN to
    cur_ib_level->m_ib_queue_addrs[cur_ib_level->m_ib_queue_size] = ib_addr;
    cur_ib_level->m_ib_queue_sizes_in_dwords[cur_ib_level->m_ib_queue_size] = ib_size_in_dwords;
    cur_ib_level->m_ib_queue_skip[cur_ib_level->m_ib_queue_size] = skip;
    cur_ib_level->m_ib_queue_type = ib_type;
    cur_ib_level->m_ib_queue_size++;
    return true;
}

//--------------------------------------------------------------------------------------------------
bool EmulatePM4::AdvanceToQueuedIB(const IMemoryManager  &mem_manager,
                                   EmulateState::CbState *cb_state,
                                   IEmulateCallbacks     &callbacks) const
{
    // Grab current stack level info
    EmulateState::IbStack *cur_ib_level = cb_state->GetCurIb();

    // Check if there are any queued IBs to advance to. If not, early out.
    if (cur_ib_level->m_ib_queue_index >= cur_ib_level->m_ib_queue_size)
        return true;

    // Can't advance to a queued up IB if there are no IBs queued up!
    DIVE_ASSERT(cur_ib_level->m_ib_queue_size != 0);

    if (cur_ib_level->m_ib_queue_type != IbType::kChain)
    {
        DIVE_ASSERT(cb_state->m_top_of_stack < EmulateState::CbState::kTotalIbLevels - 1);

        // If it's calling into another IB from the primary ring, then update the ib index. Note
        // that only IBs called from primary ring as labelled as "normal" ibs
        if (cur_ib_level->m_ib_queue_type == IbType::kNormal)
            cb_state->m_ib_index = cur_ib_level->m_ib_queue_index;

        // Enter next IB level (CALL)
        cb_state->m_top_of_stack = (EmulateState::CbState::IbLevel)(cb_state->m_top_of_stack + 1);

        uint32_t               index = cur_ib_level->m_ib_queue_index;
        EmulateState::IbStack *new_ib_level = cb_state->GetCurIb();
        new_ib_level->m_cur_va = cur_ib_level->m_ib_queue_addrs[index];
        new_ib_level->m_cur_ib_size_in_dwords = cur_ib_level->m_ib_queue_sizes_in_dwords[index];
        new_ib_level->m_cur_ib_skip = cur_ib_level->m_ib_queue_skip[index];
        new_ib_level->m_cur_ib_addr = new_ib_level->m_cur_va;
        new_ib_level->m_cur_ib_type = cur_ib_level->m_ib_queue_type;
        new_ib_level->m_ib_queue_index = new_ib_level->m_ib_queue_size = 0;
    }
    else
    {
        DIVE_ASSERT(cur_ib_level->m_ib_queue_size == 1);
        DIVE_ASSERT(cur_ib_level->m_ib_queue_index == 0);

        // The CHAIN packet should be the last packet in the current IB. Double check this.
        uint64_t cur_end_addr = cur_ib_level->m_cur_ib_addr +
                                cur_ib_level->m_cur_ib_size_in_dwords * sizeof(uint32_t);
        if (cur_ib_level->m_cur_va != cur_end_addr)
        {
            DIVE_ASSERT(false);
            return false;
        }
        cur_ib_level->m_cur_va = cur_ib_level->m_ib_queue_addrs[0];
        cur_ib_level->m_cur_ib_size_in_dwords = cur_ib_level->m_ib_queue_sizes_in_dwords[0];
        cur_ib_level->m_cur_ib_skip = cur_ib_level->m_ib_queue_skip[0];
        cur_ib_level->m_cur_ib_addr = cur_ib_level->m_cur_va;
    }

    // Advance the queue index to next element in the queue
    cur_ib_level->m_ib_queue_index++;

    // "Update" current IB level since a CALL could've happened
    cur_ib_level = cb_state->GetCurIb();

    // It's possible that an application has already reset/destroyed the command buffer. Check
    // whether the memory is valid (ie. overwritten), and skip it if it isn't
    bool skip_ib = !mem_manager.IsValid(m_submit_index,
                                        cur_ib_level->m_cur_va,
                                        cur_ib_level->m_cur_ib_size_in_dwords * sizeof(uint32_t));
    cur_ib_level->m_cur_ib_skip |= skip_ib;

    // Start-Ib Callback
    {
        IndirectBufferInfo call_chain_ib_info;
        call_chain_ib_info.m_va_addr = cur_ib_level->m_cur_va;
        call_chain_ib_info.m_size_in_dwords = cur_ib_level->m_cur_ib_size_in_dwords;
        call_chain_ib_info.m_skip = cur_ib_level->m_cur_ib_skip;
        if (!callbacks.OnIbStart(m_submit_index,
                                 cb_state->m_ib_index,
                                 call_chain_ib_info,
                                 cur_ib_level->m_cur_ib_type))
        {
            return false;
        }
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool EmulatePM4::CheckAndAdvanceIB(const IMemoryManager  &mem_manager,
                                   EmulateState::CbState *cb_state,
                                   IEmulateCallbacks     &callbacks) const
{
    // Could be pointing to an IB that needs to be "skipped", or reached the end of current IB.
    // Loop until pointing to a "valid" IB.
    // m_cur_va == UINT64_MAX when there are no more IBs to execute (i.e. set for primary ring)
    while (cb_state->GetCurIb()->m_cur_va != UINT64_MAX && cb_state->GetCurIb()->EndOfCurIb())
    {
        // Done with current IB. Jump back to an earlier level.
        if (!AdvanceOutOfIB(cb_state, callbacks))
            return false;

        // Handle any pending queued IBs
        if (!AdvanceToQueuedIB(mem_manager, cb_state, callbacks))
            return false;
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
void EmulatePM4::AdvancePacket(EmulateState::CbState *cb_state, uint32_t header) const
{
    uint32_t packet_size = GetPacketSize(header);
    cb_state->GetCurIb()->m_cur_va += packet_size * sizeof(uint32_t);
}

//--------------------------------------------------------------------------------------------------
bool EmulatePM4::AdvanceOutOfIB(EmulateState::CbState *cb_state, IEmulateCallbacks &callbacks) const
{
    EmulateState::IbStack *cur_ib_level = cb_state->GetCurIb();

    IndirectBufferInfo ib_info;
    ib_info.m_va_addr = cur_ib_level->m_cur_va;
    ib_info.m_size_in_dwords = cur_ib_level->m_cur_ib_size_in_dwords;
    ib_info.m_skip = cur_ib_level->m_cur_ib_skip;

    // End-Ib Callback
    if (!callbacks.OnIbEnd(m_submit_index, cb_state->m_ib_index, ib_info))
        return false;

    cb_state->m_top_of_stack = (EmulateState::CbState::IbLevel)(cb_state->m_top_of_stack - 1);
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
    case CP_EXEC_CS: return true;
    };
    return false;
}
//--------------------------------------------------------------------------------------------------
bool IsDrawEventOpcode(uint32_t opcode)
{
    switch (opcode)
    {
    case CP_DRAW_INDX:
    case CP_DRAW_INDX_2:
    case CP_DRAW_INDX_OFFSET:
    case CP_DRAW_INDIRECT:
    case CP_DRAW_INDX_INDIRECT:
    case CP_DRAW_INDIRECT_MULTI:
    case CP_DRAW_AUTO: return true;
    };
    return false;
}

//--------------------------------------------------------------------------------------------------
uint32_t GetPacketSize(uint32_t header)
{
    Pm4Header pm4_header;
    pm4_header.u32All = header;

    // Assumes the packet is valid, so doesn't check the parity bits at all
    switch (pm4_header.type)
    {
    case 2: return 1;
    case 4:
    {
        Pm4Type4Header *type4_header = (Pm4Type4Header *)&pm4_header;
        return type4_header->count + 1;
    }
    case 7:
        Pm4Type7Header *type7_header = (Pm4Type7Header *)&pm4_header;
        return type7_header->count + 1;
    }
    DIVE_ASSERT(false);
    return 0;
}

}  // namespace Dive
