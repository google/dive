
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
#include "dive_core/capture_data.h"
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
    // type 4 is setting register
    if (header.type != 4)
        return true;

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
uint64_t EmulateStateTracker::GetCurShaderAddr(ShaderStage stage, uint32_t enable_mask) const
{
    uint32_t offset = UINT32_MAX;
    switch (stage)
    {
    case ShaderStage::kShaderStageCs: offset = 0xa9b4 /* SP_CS_OBJ_START */; break;
    case ShaderStage::kShaderStageGs: offset = 0xa88d /* SP_GS_OBJ_START */; break;
    case ShaderStage::kShaderStageHs: offset = 0xa834 /* SP_HS_OBJ_START */; break;
    case ShaderStage::kShaderStagePs: offset = 0xa983 /* SP_FS_OBJ_START */; break;
    case ShaderStage::kShaderStageVs: offset = 0xa81c /* SP_VS_OBJ_START */; break;
    default: return UINT64_MAX;
    }
    if (!IsRegSet(offset, enable_mask))
    {
        return UINT64_MAX;
    }
    return GetReg64Value(offset, enable_mask);
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
uint32_t EmulateStateTracker::GetRegValue(uint32_t offset, uint32_t enable_mask) const
{
    for (unsigned int i = 0; i < kShaderEnableBitCount; ++i)
    {
        if (enable_mask & (1u << i))
        {
            return m_reg[i][offset];
        }
    }
    return 0;
}

//--------------------------------------------------------------------------------------------------
uint32_t EmulateStateTracker::GetRegValue(uint32_t offset) const
{
    return GetRegValue(offset, m_enable_mask);
}

//--------------------------------------------------------------------------------------------------
uint64_t EmulateStateTracker::GetReg64Value(uint32_t offset, uint32_t enable_mask) const
{
    for (unsigned int i = 0; i < kShaderEnableBitCount; ++i)
    {
        if (enable_mask & (1u << i))
        {
            return ((uint64_t)m_reg[i][offset]) | (((uint64_t)m_reg[i][offset + 1]) << 32);
        }
    }
    return 0;
}

//--------------------------------------------------------------------------------------------------
uint64_t EmulateStateTracker::GetReg64Value(uint32_t offset) const
{
    return GetRegValue(offset, m_enable_mask);
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
    return IsRegSet(offset, m_enable_mask);
}

//--------------------------------------------------------------------------------------------------
bool EmulateStateTracker::IsRegSet(uint32_t offset, uint32_t enable_mask) const
{
    for (unsigned int i = 0; i < kShaderEnableBitCount; ++i)
    {
        if (enable_mask & (1u << i))
        {
            return !!(m_reg_is_set[i][offset / 8] & (1 << (offset % 8)));
        }
    }
    return false;
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
        DIVE_VERIFY(mem_manager.CopyMemory(&header,
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
                           IEmulateCallbacks    &callbacks,
                           Pm4Header             header) const
{
    // Note: CP_COND_INDIRECT_BUFFER_PFE is not parsed here because it isn't used by Turnip.
    // When it shows up in production driver captures, the IB addr and size values are all
    // garbage. Not sure what it's used for at this point.
    if (header.type7.opcode == CP_COND_INDIRECT_BUFFER_PFE)
        DIVE_DEBUG_LOG("Packet ignored: CP_COND_INDIRECT_BUFFER_PFE\n");

    // Deal with calls and chains
    if (header.type == 7 && (header.type7.opcode == CP_INDIRECT_BUFFER_PFE ||
                             header.type7.opcode == CP_INDIRECT_BUFFER_PFD ||
                             header.type7.opcode == CP_INDIRECT_BUFFER_CHAIN))
    {

        PM4_CP_INDIRECT_BUFFER ib_packet;
        DIVE_VERIFY(mem_manager.CopyMemory(&ib_packet,
                                           emu_state_ptr->m_submit_index,
                                           emu_state_ptr->GetCurIb()->m_cur_va,
                                           sizeof(PM4_CP_INDIRECT_BUFFER)));
        uint64_t ib_addr = ((uint64_t)ib_packet.ADDR_HI << 32) | (uint64_t)ib_packet.ADDR_LO;
        IbType   ib_type = (header.type7.opcode == CP_INDIRECT_BUFFER_CHAIN) ? IbType::kChain :
                                                                               IbType::kCall;
        emu_state_ptr->GetCurIb()->m_ib_queue_index = 0;
        emu_state_ptr->GetCurIb()->m_ib_queue_size = 0;
        if (!QueueIB(ib_addr, ib_packet.SIZE, false, ib_type, emu_state_ptr))
        {
            return false;
        }
        AdvancePacket(emu_state_ptr, header);
        if (!AdvanceToQueuedIB(mem_manager, emu_state_ptr, callbacks))
            return false;
    }
    // Parse CP_SET_CTXSWITCH_IB, since it references implicit IBs
    else if (header.type == 7 && header.type7.opcode == CP_SET_CTXSWITCH_IB)
    {
        // For simplicity sake, treat CP_SET_CTXSWITCH_IB essentially as a
        // CALL (i.e. jump to the next IB level), although the hardware probably
        // doesn't do that.
        PM4_CP_SET_CTXSWITCH_IB packet;
        DIVE_VERIFY(mem_manager.CopyMemory(&packet,
                                           emu_state_ptr->m_submit_index,
                                           emu_state_ptr->GetCurIb()->m_cur_va,
                                           (header.type7.count + 1) * sizeof(uint32_t)));

        // Sometimes this packet is used for purposes other than to jump to an IB. Check size.
        // Example: When TYPE is SAVE_IB
        AdvancePacket(emu_state_ptr, header);
        if (packet.bitfields2.DWORDS != 0)
        {
            uint64_t ib_addr = ((uint64_t)packet.bitfields1.ADDR_HI << 32) |
                               (uint64_t)packet.bitfields0.ADDR_LO;
            emu_state_ptr->GetCurIb()->m_ib_queue_index = 0;
            emu_state_ptr->GetCurIb()->m_ib_queue_size = 0;
            if (!QueueIB(ib_addr,
                         packet.bitfields2.DWORDS,
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
        DIVE_VERIFY(mem_manager.CopyMemory(&packet,
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
            uint64_t ib_addr = ((uint64_t)packet.ARRAY[i].bitfields2.ADDR_HI << 32) |
                               (uint64_t)packet.ARRAY[i].bitfields1.ADDR_LO;
            if (!QueueIB(ib_addr,
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
        DIVE_VERIFY(mem_manager.CopyMemory(&packet,
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
            DIVE_VERIFY(mem_manager.CopyMemory(&temp_header,
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
        DIVE_VERIFY(mem_manager.CopyMemory(&packet,
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
                                   IEmulateCallbacks    &callbacks) const
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
                                   IEmulateCallbacks    &callbacks) const
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
bool EmulatePM4::AdvanceOutOfIB(EmulateState *emu_state, IEmulateCallbacks &callbacks) const
{
    EmulateState::IbStack *cur_ib_level = emu_state->GetCurIb();

    emu_state->m_top_of_stack = (IbLevel)(emu_state->m_top_of_stack - 1);

    IndirectBufferInfo ib_info;
    ib_info.m_va_addr = cur_ib_level->m_cur_va;
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
    case CP_DRAW_INDX_OFFSET:
    case CP_DRAW_INDIRECT:
    case CP_DRAW_INDX_INDIRECT:
    case CP_DRAW_INDIRECT_MULTI:
    case CP_DRAW_AUTO: return true;
    };
    return false;
}

//--------------------------------------------------------------------------------------------------
uint32_t GetPacketSize(Pm4Header header)
{
    // Assumes the packet is valid, so doesn't check the parity bits at all
    switch (header.type)
    {
    case 2: return 1;
    case 4:
    {
        return header.type4.count + 1;
    }
    case 7: return header.type7.count + 1;
    }
    DIVE_ASSERT(false);
    return 0;
}

// =================================================================================================
// IEmulateCallbacks
// =================================================================================================

bool IEmulateCallbacks::ProcessSubmits(const std::vector<SubmitInfo> &submits,
                                       const IMemoryManager          &mem_manager)
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
