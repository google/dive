
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

#include "common.h"
#include "dive_capture_format.h"
#include "emulate_pm4.h"
#include "memory_manager_base.h"

#include "pm4_packets/ce_pm4_packets.h"
#include "pm4_packets/me_pm4_packets.h"
#include "pm4_packets/pfp_pm4_packets.h"

#include <stdarg.h>
#include <cerrno>
#include <cstdio>
#include <cstring>

namespace Dive
{

// =================================================================================================
// EmulateConstantEngine
// =================================================================================================
uint32_t EmulateConstantEngine::CalculateInternalBufferSize(uint32_t ce_ram_size)
{
    DIVE_ASSERT(ce_ram_size % sizeof(uint32_t) == 0);

    uint32_t buffer_size = 0;

    // Shadow memory
    buffer_size += ce_ram_size;

    // Dump address (ie: a 64-bit address for every entry in the CP-RAM)
    buffer_size += (ce_ram_size / sizeof(uint32_t)) * sizeof(uint64_t);

    return buffer_size;
}

//--------------------------------------------------------------------------------------------------
void EmulateConstantEngine::Init(void *buffer_ptr, uint32_t ce_ram_size)
{
    m_ce_ram_size = ce_ram_size;
    m_ce_ram_shadow = (uint8_t *)buffer_ptr;
    m_ce_ram_dump_addr = (uint64_t *)((uint8_t *)buffer_ptr + m_ce_ram_size);
    memset(m_ce_ram_dump_addr, 0xFF, (m_ce_ram_size / sizeof(uint32_t)) * sizeof(uint64_t));
    m_dumped_dword_beg = UINT16_MAX;
    m_dumped_dword_end = 0;
}

//--------------------------------------------------------------------------------------------------
bool EmulateConstantEngine::OnCcbPacket(const IMemoryManager        &mem_manager,
                                        uint32_t                     submit_index,
                                        uint32_t                     ib_index,
                                        uint64_t                     va_addr,
                                        const PM4_PFP_TYPE_3_HEADER &header)
{
    return false;
}

//--------------------------------------------------------------------------------------------------
bool EmulateConstantEngine::GetDumpedOffset(uint32_t *offset_ptr,
                                            uint32_t *num_bytes_ptr,
                                            uint64_t  va_addr) const
{
    // The idea here is that the driver compacts and re-uses CE-RAM between events, so the range of
    // dumped bytes is usually rather small. So checking against a dumped entry range is simpler and
    // tends to be faster than using something like a hashmap
    uint32_t start_dword = UINT32_MAX;
    for (uint32_t dword = m_dumped_dword_beg; dword < m_dumped_dword_end; ++dword)
    {
        if (m_ce_ram_dump_addr[dword] == va_addr)
        {
            start_dword = dword;
            break;
        }
    }
    if (start_dword == UINT32_MAX)
        return false;

    // Find largest contiguous sequence of dumped memory
    uint64_t next_addr = va_addr + sizeof(uint32_t);
    for (uint32_t dword = start_dword + 1; dword < m_dumped_dword_end; ++dword)
    {
        if (m_ce_ram_dump_addr[dword] == next_addr)
        {
            next_addr += sizeof(uint32_t);
        }
    }

    *offset_ptr = start_dword * sizeof(uint32_t);
    *num_bytes_ptr = (uint32_t)(next_addr - va_addr);
    return true;
}

//--------------------------------------------------------------------------------------------------
void *EmulateConstantEngine::GetCERam(uint32_t offset) const
{
    return (void *)((uint8_t *)m_ce_ram_shadow + offset);
}

//--------------------------------------------------------------------------------------------------
void EmulateConstantEngine::CopyCERam(void *memory_ptr, uint32_t offset, uint32_t num_bytes) const
{
    DIVE_ASSERT(offset + num_bytes <= m_ce_ram_size);
    memcpy(memory_ptr, (void *)((uint8_t *)m_ce_ram_shadow + offset), num_bytes);
}

// =================================================================================================
// EmulateStateTracker
// =================================================================================================
EmulateStateTracker::EmulateStateTracker() {}

//--------------------------------------------------------------------------------------------------
void EmulateStateTracker::Init(uint8_t num_user_data_regs_gfx, uint8_t num_user_data_regs_compute)
{
    m_num_user_data_regs_gfx = num_user_data_regs_gfx;
    m_num_user_data_regs_compute = num_user_data_regs_compute;

    memset(&m_sh_is_set[0], 0, sizeof(m_sh_is_set));
    memset(&m_context_is_set[0], 0, sizeof(m_context_is_set));
    memset(&m_uconfig_is_set[0], 0, sizeof(m_uconfig_is_set));
    memset(&m_user_data_regs_set_last_event[0], 0, sizeof(m_user_data_regs_set_last_event));
    memset(&m_num_user_data_regs_set_last_event[0], 0, sizeof(m_num_user_data_regs_set_last_event));
}

//--------------------------------------------------------------------------------------------------
bool EmulateStateTracker::OnDcbPacket(const IMemoryManager        &mem_manager,
                                      uint32_t                     submit_index,
                                      uint32_t                     ib_index,
                                      uint64_t                     va_addr,
                                      const PM4_PFP_TYPE_3_HEADER &header)
{
    return true;
}

//--------------------------------------------------------------------------------------------------
bool EmulateStateTracker::OnCcbPacket(const IMemoryManager        &mem_manager,
                                      uint32_t                     submit_index,
                                      uint32_t                     ib_index,
                                      uint64_t                     va_addr,
                                      const PM4_PFP_TYPE_3_HEADER &header)
{
    return true;
}

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
bool EmulateStateTracker::IsContextStateSet(uint16_t reg) const
{
    return false;
}

//--------------------------------------------------------------------------------------------------
uint32_t EmulateStateTracker::GetContextRegData(uint16_t reg) const
{
    return UINT32_MAX;
}

//--------------------------------------------------------------------------------------------------
uint32_t EmulateStateTracker::GetNumberOfUserDataRegsSetSinceLastEvent(ShaderStage stage) const
{
    return m_num_user_data_regs_set_last_event[(uint32_t)stage];
}

//--------------------------------------------------------------------------------------------------
bool EmulateStateTracker::IsUserDataRegsSetSinceLastEvent(ShaderStage stage, uint8_t offset) const
{
    // Note: Gfx User_data 0, 1, 29, 30, 31 reserved for system
    //       Compute User_data 0, 1, 13, 14, 15 reserved for system
    //       Do not track these!
    DIVE_ASSERT(offset >= 2);

    return ((m_user_data_regs_set_last_event[(uint32_t)stage] & (1 << offset)) != 0);
}
//--------------------------------------------------------------------------------------------------
uint64_t EmulateStateTracker::GetCurShaderAddr(ShaderStage stage) const
{
    return UINT64_MAX;
}

//--------------------------------------------------------------------------------------------------
uint64_t EmulateStateTracker::GetBufferAddr() const
{
    return m_buffer_va;
}

//--------------------------------------------------------------------------------------------------
uint32_t EmulateStateTracker::GetBufferSize() const
{
    return m_buffer_size;
}

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

// =================================================================================================
// EmulatePM4
// =================================================================================================
EmulatePM4::EmulatePM4() {}

//--------------------------------------------------------------------------------------------------
bool EmulatePM4::ExecuteKMDRing(IEmulateCallbacks    &callbacks,
                                const IMemoryManager &mem_manager,
                                bool                  universal_queue,
                                uint64_t              ring_start_va,
                                uint32_t              ring_size,
                                uint64_t              ring_true_start_va,
                                uint64_t              ring_full_size)
{
    return false;
}

//--------------------------------------------------------------------------------------------------
bool EmulatePM4::ExecuteSubmit(IEmulateCallbacks        &callbacks,
                               const IMemoryManager     &mem_manager,
                               uint32_t                  submit_index,
                               uint32_t                  num_ibs,
                               const IndirectBufferInfo *ib_ptr)
{
    m_submit_index = submit_index;
    m_num_ibs = num_ibs;
    m_ib_ptr = ib_ptr;

    // Used to keep track of progress of emulation so far
    EmulateState emu_state;
    if (!GetNextValidIb(mem_manager,
                        &emu_state.m_dcb.m_ib_index,
                        callbacks,
                        ib_ptr,
                        m_submit_index,
                        0,
                        true))
        return false;
    if (!GetNextValidIb(mem_manager,
                        &emu_state.m_ccb.m_ib_index,
                        callbacks,
                        ib_ptr,
                        m_submit_index,
                        0,
                        false))
        return false;
    emu_state.m_dcb.m_va = UINT64_MAX;
    emu_state.m_ccb.m_va = UINT64_MAX;
    emu_state.m_dcb.m_vmid = UINT8_MAX;
    emu_state.m_ccb.m_vmid = UINT8_MAX;
    emu_state.m_de_counter = 0;
    emu_state.m_ce_counter = 0;
    emu_state.m_is_dcb_blocked = false;
    emu_state.m_dcb.m_is_in_ib1_chain_ib = false;
    emu_state.m_dcb.m_is_in_ib2 = false;
    emu_state.m_ccb.m_is_in_ib1_chain_ib = false;
    emu_state.m_ccb.m_is_in_ib2 = false;
    if (emu_state.m_dcb.m_ib_index != UINT32_MAX)
    {
        const IndirectBufferInfo &ib_info = ib_ptr[emu_state.m_dcb.m_ib_index];
        emu_state.m_dcb.m_va = ib_info.m_va_addr;
        emu_state.m_dcb.m_vmid = ib_info.m_vmid;
    }
    if (emu_state.m_ccb.m_ib_index != UINT32_MAX)
    {
        const IndirectBufferInfo &ib_info = ib_ptr[emu_state.m_ccb.m_ib_index];
        emu_state.m_ccb.m_va = ib_info.m_va_addr;
        emu_state.m_ccb.m_vmid = ib_info.m_vmid;
    }

    while (emu_state.m_dcb.m_va != UINT64_MAX || emu_state.m_ccb.m_va != UINT64_MAX)
    {
        // Sanity check: Dcb cannot be both blocked and finished processing
        DIVE_ASSERT((emu_state.m_is_dcb_blocked && emu_state.m_dcb.m_va != UINT64_MAX) ||
                    !emu_state.m_is_dcb_blocked);

        uint64_t va = emu_state.m_ccb.m_va;
        if (!emu_state.m_is_dcb_blocked && emu_state.m_dcb.m_va != UINT64_MAX)
            va = emu_state.m_dcb.m_va;

        if (emu_state.m_is_dcb_blocked && emu_state.m_ccb.m_va == UINT64_MAX)
        {
            DIVE_ERROR_MSG("Dcb hang at 0x%llx waiting on ccb, but no ccb packets left!\n",
                           (unsigned long long)emu_state.m_dcb.m_va);
            return false;
        }

        // Callbacks + advance
        if (emu_state.m_dcb.m_va != UINT64_MAX)
        {
            PM4_PFP_TYPE_3_HEADER header;
            if (!mem_manager.CopyMemory(&header,
                                        m_submit_index,
                                        emu_state.m_dcb.m_va,
                                        sizeof(PM4_PFP_TYPE_3_HEADER)))
            {
                DIVE_ERROR_MSG("Unable to access packet at 0x%llx\n", (unsigned long long)va);
                return false;
            }

            if (!emu_state.m_is_dcb_blocked)
                if (!callbacks.OnDcbPacket(mem_manager,
                                           m_submit_index,
                                           emu_state.m_dcb.m_ib_index,
                                           emu_state.m_dcb.m_va,
                                           header))
                    return false;
            if (!AdvanceDcb(mem_manager, &emu_state, callbacks, header))
                return false;
        }

        // CE should only be running if out of DCB or if DE blocked
        if (emu_state.m_is_dcb_blocked || emu_state.m_dcb.m_va == UINT64_MAX)
        {
            if (emu_state.m_ccb.m_va != UINT64_MAX)
            {
                PM4_PFP_TYPE_3_HEADER header;
                if (!mem_manager.CopyMemory(&header,
                                            m_submit_index,
                                            emu_state.m_ccb.m_va,
                                            sizeof(PM4_PFP_TYPE_3_HEADER)))
                {
                    DIVE_ERROR_MSG("Unable to access packet at 0x%llx\n", (unsigned long long)va);
                    return false;
                }

                if (!callbacks.OnCcbPacket(mem_manager,
                                           m_submit_index,
                                           emu_state.m_ccb.m_ib_index,
                                           emu_state.m_ccb.m_va,
                                           header))
                    return false;
                if (!AdvanceCcb(mem_manager, &emu_state, callbacks, header))
                    return false;
            }
        }
    }  // while there are packets left in submit
    return true;
}

//--------------------------------------------------------------------------------------------------
bool EmulatePM4::GetNextValidIb(const IMemoryManager     &mem_manager,
                                uint32_t                 *ib_index_ptr,
                                IEmulateCallbacks        &callbacks,
                                const IndirectBufferInfo *ib_ptr,
                                uint32_t                  submit_index,
                                uint32_t                  start_ib_index,
                                bool                      dcb_desired) const
{
    bool found_valid_ib = false;
    *ib_index_ptr = GetNextIb(submit_index, start_ib_index, dcb_desired);
    while (!found_valid_ib && (*ib_index_ptr != UINT32_MAX))
    {
        IndirectBufferInfo ib_info = ib_ptr[*ib_index_ptr];

        ib_info.m_skip = !mem_manager.IsValid(m_submit_index,
                                              ib_info.m_va_addr,
                                              ib_info.m_size_in_dwords * sizeof(uint32_t));

        if (!callbacks.OnIbStart(m_submit_index, *ib_index_ptr, ib_info, IbType::kNormal))
            return false;

        // Keep going until found an ib that does not have m_skip set to true
        found_valid_ib = true;
        if (ib_info.m_skip)
        {
            if (!callbacks.OnIbEnd(m_submit_index, *ib_index_ptr, ib_info))
                return false;

            *ib_index_ptr = GetNextIb(submit_index, *ib_index_ptr + 1, dcb_desired);
            found_valid_ib = false;
        }
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
uint32_t EmulatePM4::GetNextIb(uint32_t submit_index,
                               uint32_t start_ib_index,
                               bool     dcb_desired) const
{
    for (uint32_t ib_index = start_ib_index; ib_index < m_num_ibs; ++ib_index)
    {
        const IndirectBufferInfo &ib_info = m_ib_ptr[ib_index];
        if (dcb_desired && !ib_info.m_is_constant_engine)
            return ib_index;
        if (!dcb_desired && ib_info.m_is_constant_engine)
            return ib_index;
    }
    return UINT32_MAX;
}

//--------------------------------------------------------------------------------------------------
bool EmulatePM4::AdvanceDcb(const IMemoryManager        &mem_manager,
                            EmulateState                *emu_state_ptr,
                            IEmulateCallbacks           &callbacks,
                            const PM4_PFP_TYPE_3_HEADER &header) const
{
    return false;
}

//--------------------------------------------------------------------------------------------------
bool EmulatePM4::AdvanceCcb(const IMemoryManager        &mem_manager,
                            EmulateState                *emu_state_ptr,
                            IEmulateCallbacks           &callbacks,
                            const PM4_PFP_TYPE_3_HEADER &header) const
{
    return false;
}

//--------------------------------------------------------------------------------------------------
bool EmulatePM4::AdvanceToIB(const IMemoryManager        &mem_manager,
                             bool                         is_chain,
                             uint64_t                     ib_addr,
                             uint32_t                     ib_size_in_dwords,
                             const IndirectBufferInfo    &ib_info,
                             EmulateState::CbState       *cb_state,
                             bool                         is_constant_engine,
                             IEmulateCallbacks           &callbacks,
                             const PM4_PFP_TYPE_3_HEADER &header) const
{
    if (!is_chain)
    {
        // Call packet: CP is transitioning into an IB2
        cb_state->m_is_in_ib2 = true;
        cb_state->m_ib2_addr = ib_addr;
        cb_state->m_ib2_size_in_dwords = ib_size_in_dwords;
        cb_state->m_ib1_return_va = cb_state->m_va + (header.count + 2) * sizeof(uint32_t);
    }
    else
    {
        // Chain packet
        if (!cb_state->m_is_in_ib2)
        {
            // Currently in IB1. Mark that it is in a CHAIN IB, so that a CALL to IB2 can return
            // to this CHAIN IB
            cb_state->m_is_in_ib1_chain_ib = true;
            cb_state->m_ib1_chain_addr = ib_addr;
            cb_state->m_ib1_chain_size_in_dwords = ib_size_in_dwords;
        }
        else
        {
            // Currently in IB2. No need to keep track that it is in a CHAIN IB, since after the
            // end of the CHAIN sequence, will go straight back to IB1
            cb_state->m_ib2_addr = ib_addr;
            cb_state->m_ib2_size_in_dwords = ib_size_in_dwords;
        }
    }
    cb_state->m_va = ib_addr;

    // Start-Ib Callback
    {
        // It's possible that an application has already reset/destroyed the command buffer. Check
        // whether the memory is valid (ie. overwritten), and skip it if it isn't
        bool skip_ib = !mem_manager.IsValid(m_submit_index,
                                            ib_addr,
                                            ib_size_in_dwords * sizeof(uint32_t));

        IndirectBufferInfo call_chain_ib_info;
        call_chain_ib_info.m_va_addr = ib_addr;
        call_chain_ib_info.m_vmid = cb_state->m_vmid;
        call_chain_ib_info.m_size_in_dwords = ib_size_in_dwords;
        call_chain_ib_info.m_is_constant_engine = is_constant_engine;
        call_chain_ib_info.m_skip = skip_ib;
        IbType type = is_chain ? IbType::kChain : IbType::kCall;
        if (!callbacks.OnIbStart(m_submit_index, cb_state->m_ib_index, call_chain_ib_info, type))
            return false;

        if (skip_ib)
        {
            if (!AdvanceOutOfIB(mem_manager, ib_info, cb_state, is_constant_engine, callbacks))
                return false;
        }
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
bool EmulatePM4::AdvancePacket(const IMemoryManager        &mem_manager,
                               const IndirectBufferInfo    &ib_info,
                               EmulateState::CbState       *cb_state,
                               bool                         is_constant_engine,
                               IEmulateCallbacks           &callbacks,
                               const PM4_PFP_TYPE_3_HEADER &header) const
{
    return false;
}

//--------------------------------------------------------------------------------------------------
bool EmulatePM4::AdvanceOutOfIB(const IMemoryManager     &mem_manager,
                                const IndirectBufferInfo &ib_info,
                                EmulateState::CbState    *cb_state,
                                bool                      is_constant_engine,
                                IEmulateCallbacks        &callbacks) const
{
    // End-Ib Callback
    if (!callbacks.OnIbEnd(m_submit_index, cb_state->m_ib_index, ib_info))
        return false;

    // A call packet return
    if (cb_state->m_is_in_ib2)
        cb_state->m_va = cb_state->m_ib1_return_va;
    else
    {
        uint32_t ib_index;
        if (!GetNextValidIb(mem_manager,
                            &ib_index,
                            callbacks,
                            m_ib_ptr,
                            m_submit_index,
                            cb_state->m_ib_index + 1,
                            !is_constant_engine))
            return false;
        cb_state->m_ib_index = ib_index;
        cb_state->m_va = UINT64_MAX;
        if (ib_index != UINT32_MAX)
        {
            const IndirectBufferInfo &next_ib_info = m_ib_ptr[ib_index];
            cb_state->m_va = next_ib_info.m_va_addr;
            cb_state->m_vmid = next_ib_info.m_vmid;
        }
    }

    // If currently not in an IB2, then must either be in an IB1 or a chain IB1
    // In any case, either reached the end of the IB1 or the chain IB1, so set to false
    if (!cb_state->m_is_in_ib2)
        cb_state->m_is_in_ib1_chain_ib = false;

    // If this was true, then reached the end of the IB2 and no longer in an IB2
    // So can set this unconditionally to false
    cb_state->m_is_in_ib2 = false;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool IsDrawDispatchDmaEvent(uint32_t opcode)
{
    return false;
}

//--------------------------------------------------------------------------------------------------
bool IsDispatchEventOpcode(uint32_t opcode)
{
    return false;
}
//--------------------------------------------------------------------------------------------------
bool IsDrawEventOpcode(uint32_t opcode)
{
    return false;
}

//--------------------------------------------------------------------------------------------------
bool IsDmaEventOpcode(uint32_t opcode)
{
    return false;
}

//--------------------------------------------------------------------------------------------------
bool IsCcbOpcode(uint32_t opcode)
{
    return false;
}
}  // namespace Dive
