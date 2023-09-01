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

// =================================================================================================
// Provides emulation of the submits present in the capture. In particular, the Emulate class
// handles all the messy details of sync-ing up ce/de counters, dealing with calls/chains, and other
// hardware specific details. A set of callbacks are provided for clients for certain important
// events that occur in the command buffer. Note that the "order" that the callbacks are in is the
// order the commands are executed in the emulation.
// =================================================================================================

#pragma once
#include <stdint.h>
#include "dive_core/common/pm4_packets/pfp_pm4_packets.h"
#include "gpudefs.h"

namespace Dive
{

// Forward declaration
class IMemoryManager;

//--------------------------------------------------------------------------------------------------
struct IndirectBufferInfo
{
    uint64_t m_va_addr;
    uint32_t m_size_in_dwords;
    uint8_t  m_vmid;
    bool     m_is_constant_engine;

    // Set to true to avoid emulating this IB, probably because it was not captured
    bool m_skip;
};

//--------------------------------------------------------------------------------------------------
class IEmulateCallbacks
{
public:
    // Callback on an IB start. Also called for all call/chain IBs
    // A return value of false indicates to the emulator to skip parsing this IB
    virtual bool OnIbStart(uint32_t                  submit_index,
                           uint32_t                  ib_index,
                           const IndirectBufferInfo &ib_info,
                           IbType                    type)
    {
        return true;
    }

    // Callback for an IB end
    virtual bool OnIbEnd(uint32_t                  submit_index,
                         uint32_t                  ib_index,
                         const IndirectBufferInfo &ib_info)
    {
        return true;
    }

    // Callback for each DCB packet. Called in order of emulation
    virtual bool OnDcbPacket(const IMemoryManager        &mem_manager,
                             uint32_t                     submit_index,
                             uint32_t                     ib_index,
                             uint64_t                     va_addr,
                             const PM4_PFP_TYPE_3_HEADER &header)
    {
        return true;
    }

    // Callback for each CCB packet. Called in order of emulation
    virtual bool OnCcbPacket(const IMemoryManager        &mem_manager,
                             uint32_t                     submit_index,
                             uint32_t                     ib_index,
                             uint64_t                     va_addr,
                             const PM4_PFP_TYPE_3_HEADER &header)
    {
        return true;
    }
};

//--------------------------------------------------------------------------------------------------
// Emulation of CE and associated CE-RAM writes/dumps
class EmulateConstantEngine
{
public:
    // Calculate the buffer size needed internally by EmulateConstantEngine
    static uint32_t CalculateInternalBufferSize(uint32_t ce_ram_size);

    // Set the buffer required for CE-RAM tracking. This is because the caller is responsible for
    // any and all memory allocations used by this class
    void Init(void *buffer_ptr, uint32_t ce_ram_size);

    // Call this function to update the emulation
    bool OnCcbPacket(const IMemoryManager        &mem_manager,
                     uint32_t                     submit_index,
                     uint32_t                     ib_index,
                     uint64_t                     va_addr,
                     const PM4_PFP_TYPE_3_HEADER &header);

    // Returns false if an address range starting at va_addr was not dumped via CE. Otherwise the
    // dumped CE-RAM byte offset is returned as well as the number of consecutive entries that were
    // dumped
    bool GetDumpedOffset(uint32_t *offset_ptr, uint32_t *num_bytes_ptr, uint64_t va_addr) const;

    // Get pointer to the given offset of shadowed CE-RAM
    void *GetCERam(uint32_t offset) const;

    // Copy the given range from the shadowed CE-RAM
    void CopyCERam(void *memory_ptr, uint32_t offset, uint32_t num_bytes) const;

private:
    // Size of the CE-RAM to track
    uint32_t m_ce_ram_size;

    // Shadow what's written to the ce-ram
    uint8_t *m_ce_ram_shadow;

    // Address (if any) of address the entry was dumped to
    // This is a 64-bit address entry for each 32-bit DWORD entry in the CE-RAM
    uint64_t *m_ce_ram_dump_addr;

    // Beginning & end of range of dwords in CE-RAM that have been dumped
    uint32_t m_dumped_dword_beg;
    uint32_t m_dumped_dword_end;
};

//--------------------------------------------------------------------------------------------------
// Emulation state tracker. Optional to reduce unnecessary memory and processing overhead.
class EmulateStateTracker
{
public:
    EmulateStateTracker();

    void Init(uint8_t num_user_data_regs_gfx, uint8_t num_user_data_regs_compute);

    // Call these functions to update the state tracker
    bool OnDcbPacket(const IMemoryManager        &mem_manager,
                     uint32_t                     submit_index,
                     uint32_t                     ib_index,
                     uint64_t                     va_addr,
                     const PM4_PFP_TYPE_3_HEADER &header);
    bool OnCcbPacket(const IMemoryManager        &mem_manager,
                     uint32_t                     submit_index,
                     uint32_t                     ib_index,
                     uint64_t                     va_addr,
                     const PM4_PFP_TYPE_3_HEADER &header);

    // Accessing state tracking info
    bool     IsUConfigStateSet(uint16_t reg) const;
    uint32_t GetUConfigRegData(uint16_t reg) const;
    bool     IsShStateSet(uint16_t reg) const;
    uint32_t GetShRegData(uint16_t reg) const;
    bool     IsContextStateSet(uint16_t reg) const;
    uint32_t GetContextRegData(uint16_t reg) const;

    // Check if any user data registers were set since last draw/dispatch
    // The driver only sets them on a per-event basis, and checking IsShStateSet() tracks since the
    // last time state tracking was reset
    // Note: Does not track any of the system-reserved user data registers!
    uint32_t GetNumberOfUserDataRegsSetSinceLastEvent(ShaderStage stage) const;
    bool     IsUserDataRegsSetSinceLastEvent(ShaderStage stage, uint8_t offset) const;

    // Returns UINT64_MAX if shader not set
    uint64_t GetCurShaderAddr(ShaderStage stage) const;

    // Get address/size of most recent PM4 buffer (eg. extended user data, sh load buffers, etc)
    uint64_t GetBufferAddr() const;
    uint32_t GetBufferSize() const;

private:
    bool HandleLoadUConfigRegPacket(const IMemoryManager &mem_manager,
                                    uint32_t              submit_index,
                                    uint64_t              va_addr);
    bool HandleSetUConfigRegister(const IMemoryManager &mem_manager,
                                  uint32_t              submit_index,
                                  uint16_t              reg_offset,
                                  uint64_t              reg_data_va,
                                  uint16_t              reg_count,
                                  bool                  update_tracking = true);
    bool HandleLoadShRegPacket(const IMemoryManager &mem_manager,
                               uint32_t              submit_index,
                               uint64_t              va_addr);
    bool HandleLoadShRegIndexPacket(const IMemoryManager &mem_manager,
                                    uint32_t              submit_index,
                                    uint64_t              va_addr);
    bool HandleSetShRegister(const IMemoryManager &mem_manager,
                             uint32_t              submit_index,
                             uint16_t              reg_offset,
                             uint64_t              reg_data_va,
                             uint16_t              reg_count,
                             bool                  update_tracking = true);
    bool HandleLoadContextRegPacket(const IMemoryManager &mem_manager,
                                    uint32_t              submit_index,
                                    uint64_t              va_addr);
    bool HandleLoadContextRegIndexPacket(const IMemoryManager &mem_manager,
                                         uint32_t              submit_index,
                                         uint64_t              va_addr);
    bool HandleSetContextRegister(const IMemoryManager &mem_manager,
                                  uint32_t              submit_index,
                                  uint16_t              reg_offset,
                                  uint64_t              reg_data_va,
                                  uint16_t              reg_count,
                                  bool                  update_tracking = true);
    void HandleUserDataRegister(ShaderStage stage,
                                uint16_t    reg_offset,
                                uint16_t    start_reg,
                                uint16_t    num_regs);
    void UpdateUserDataRegsSetSinceLastEvent(ShaderStage stage, uint8_t user_data_reg);

    // Register tracking data
    static const uint32_t kNumPersistentRegs = 1 /*Pal::Gfx9::PERSISTENT_SPACE_END -
                                                Pal::Gfx9::PERSISTENT_SPACE_START*/
    ;
    uint8_t               m_sh_is_set[(kNumPersistentRegs / 8) + 1];
    static const uint32_t kNumContextRegs = 1 /*Pal::Gfx9::Gfx09_10::CONTEXT_SPACE_END -
                                             Pal::Gfx9::CONTEXT_SPACE_START*/
    ;
    uint32_t m_context_data[kNumContextRegs];
    uint8_t  m_context_is_set[(kNumContextRegs / 8) + 1];

    static const uint32_t kNumUConfigRegs = /*Pal::Gfx9::UCONFIG_SPACE_END -
                                            Pal::Gfx9::UCONFIG_SPACE_START +*/
    1;
    uint32_t m_uconfig_data[kNumUConfigRegs];
    uint8_t  m_uconfig_is_set[(kNumUConfigRegs / 8) + 1];

    static const uint32_t kNumShaderStages = (uint32_t)ShaderStage::kShaderStageCount;
    uint8_t               m_num_user_data_regs_set_last_event[kNumShaderStages];
    uint32_t              m_user_data_regs_set_last_event[kNumShaderStages];

    // Flags to determine if shadow-related load functions are executed or not
    bool m_load_global_uconfig = false;
    bool m_load_gfx_sh_regs = false;
    bool m_load_cs_sh_regs = false;
    bool m_load_per_context_state = false;

    // Any recently accessed PM4 buffers (such as extended user data, load buffers, etc)
    uint64_t m_buffer_va;
    uint32_t m_buffer_size;

    // Emulation configuration
    uint8_t m_num_user_data_regs_gfx;
    uint8_t m_num_user_data_regs_compute;
};

//--------------------------------------------------------------------------------------------------
class EmulatePM4
{
public:
    EmulatePM4();

    // Emulate a submit
    static const uint32_t kMaxNumIbsPerSubmit = 16;

    // Emulate the passed-in buffer_ptr, and treat it as the KMD ring buffer
    bool ExecuteKMDRing(IEmulateCallbacks    &callbacks,
                        const IMemoryManager &mem_manager,
                        bool                  universal_queue,
                        uint64_t              ring_start_va,
                        uint32_t              ring_size,
                        uint64_t              ring_true_start_va,
                        uint64_t              ring_full_size);

    bool ExecuteSubmit(IEmulateCallbacks        &callbacks,
                       const IMemoryManager     &mem_manager,
                       uint32_t                  submit_index,
                       uint32_t                  num_ibs,
                       const IndirectBufferInfo *ib_ptr);

private:
    // Keep all emulation state together
    struct EmulateState
    {
        struct CbState
        {
            uint32_t m_ib_index;
            uint64_t m_va;
            uint8_t  m_vmid;
            bool     m_is_in_ib1_chain_ib;
            bool     m_is_in_ib2;
            uint64_t m_ib1_chain_addr;
            uint32_t m_ib1_chain_size_in_dwords;
            uint64_t m_ib2_addr;
            uint32_t m_ib2_size_in_dwords;
            uint64_t m_ib1_return_va;
        };
        int32_t m_de_counter;
        int32_t m_ce_counter;
        bool    m_is_dcb_blocked;

        CbState m_dcb;
        CbState m_ccb;
    };

    // Calls GetNextIb() repeatedly until a valid IB is encountered, with OnIbStart() callback
    // called for all candidate Ibs, even "invalid ones". An IB is "invalid" if its m_skip is
    // set to true.
    bool GetNextValidIb(const IMemoryManager     &mem_manager,
                        uint32_t                 *ib_index_ptr,
                        IEmulateCallbacks        &callbacks,
                        const IndirectBufferInfo *ib_ptr,
                        uint32_t                  submit_index,
                        uint32_t                  start_ib_index,
                        bool                      dcb_desired) const;

    // Find next index of dcb (if dcb_desired==1) or ccb (if dcb_desired==0) ib. Returns UINT32_MAX
    // if not found. Dcb - Draw command buffer. Ccb - Constant command buffer. Ib - Indirect Buffer.
    uint32_t GetNextIb(uint32_t submit_index, uint32_t start_ib_index, bool dcb_desired) const;

    // Advance dcb pointer after advancing past the packet header. Returns "true" if dcb is blocked.
    bool AdvanceDcb(const IMemoryManager        &mem_manager,
                    EmulateState                *emu_state_ptr,
                    IEmulateCallbacks           &callbacks,
                    const PM4_PFP_TYPE_3_HEADER &header) const;

    // Advance ccb pointer after advancing past the packet header. Returns "true" if dcb is still
    // blocked.
    bool AdvanceCcb(const IMemoryManager        &mem_manager,
                    EmulateState                *emu_state_ptr,
                    IEmulateCallbacks           &callbacks,
                    const PM4_PFP_TYPE_3_HEADER &header) const;

    // Helper function to help with advancing emulation to IB
    bool AdvanceToIB(const IMemoryManager        &mem_manager,
                     bool                         is_chain,
                     uint64_t                     ib_addr,
                     uint32_t                     ib_size_in_dwords,
                     const IndirectBufferInfo    &ib_info,
                     EmulateState::CbState       *cb_state,
                     bool                         is_constant_engine,
                     IEmulateCallbacks           &callbacks,
                     const PM4_PFP_TYPE_3_HEADER &header) const;

    // Helper function to help with advancing emulation to next packet
    bool AdvancePacket(const IMemoryManager        &mem_manager,
                       const IndirectBufferInfo    &ib_info,
                       EmulateState::CbState       *cb_state,
                       bool                         is_constant_engine,
                       IEmulateCallbacks           &callbacks,
                       const PM4_PFP_TYPE_3_HEADER &header) const;

    // Helper function to help with advancing emulation out of IB
    bool AdvanceOutOfIB(const IMemoryManager     &mem_manager,
                        const IndirectBufferInfo &ib_info,
                        EmulateState::CbState    *cb_state,
                        bool                      is_constant_engine,
                        IEmulateCallbacks        &callbacks) const;

    // Emulation source
    uint32_t                  m_submit_index;
    uint32_t                  m_num_ibs;
    const IndirectBufferInfo *m_ib_ptr;
};

//--------------------------------------------------------------------------------------------------
bool IsDrawDispatchDmaEvent(uint32_t opcode);
bool IsDispatchEventOpcode(uint32_t opcode);
bool IsDrawEventOpcode(uint32_t opcode);
bool IsDmaEventOpcode(uint32_t opcode);
bool IsCcbOpcode(uint32_t opcode);
bool IsDcbOpcode(uint32_t opcode);

}  // namespace Dive
