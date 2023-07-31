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

// Warning: This is a common file that is used by both the Dive GUI tool and possibly by
// the driver code later on

// =================================================================================================
// Provides emulation of the submits present in the capture. In particular, the Emulate class
// handles all the messy details of dealing with calls/chains and other hardware specific details.
// A set of callbacks are provided for clients for certain important events that occur in the command
// buffer. Note that the "order" that the callbacks are in is the order the commands are executed in
// the emulation.
// =================================================================================================

#pragma once
#include <stdint.h>
#include "dive_core/common/pm4_packets/pfp_pm4_packets.h"
#include "gpudefs.h"

namespace Dive
{

// Forward declaration
class IMemoryManager;

// Should Type7Opcodes be auto-generated instead?
enum Type7Opcodes
{
    CP_ME_INIT = 0x48,
    CP_NOP = 0x10,
    CP_YIELD_ENABLE = 0x1c,
    CP_PREEMPT_TOKEN = 0x1e,
    CP_INDIRECT_BUFFER_PFE = 0x3f,
    CP_INDIRECT_BUFFER_CHAIN = 0x57,
    CP_INDIRECT_BUFFER_PFD = 0x37,
    CP_WAIT_FOR_IDLE = 0x26,
    CP_WAIT_REG_MEM = 0x3c,
    CP_WAIT_REG_EQ = 0x52,
    CP_SMMU_TABLE_UPDATE = 0x53,
    CP_CONTEXT_REG_BUNCH = 0x5c,
    CP_CONTEXT_REG_BUNCH2 = 0x5d,
    CP_REG_RMW = 0x21,
    CP_SET_BIN_DATA5 = 0x2f,
    CP_REG_TO_MEM = 0x3e,
    CP_MEM_WRITE = 0x3d,
    CP_MEM_WRITE_CNTR = 0x4f,
    CP_COND_EXEC = 0x44,
    CP_COND_WRITE5 = 0x45,
    CP_EVENT_WRITE = 0x46,
    CP_EVENT_WRITE_SHD = 0x58,
    CP_EVENT_WRITE_CFL = 0x59,
    CP_EVENT_WRITE_ZPD = 0x5b,
    CP_RUN_OPENCL = 0x31,
    CP_DRAW_INDX = 0x22,
    CP_LOAD_STATE6 = 0x36,
    CP_LOAD_STATE6_FRAG = 0x34,
    CP_SET_SUBDRAW_SIZE = 0x35,
    CP_SKIP_IB2_ENABLE_LOCAL = 0x23,
    CP_SET_STATE = 0x25,
    CP_SET_CONSTANT = 0x2d,
    CP_IM_LOAD = 0x27,
    CP_IM_LOAD_IMMEDIATE = 0x2b,
    CP_SET_BIN_DATA5_OFFSET = 0x2e,
    CP_INVALIDATE_STATE = 0x3b,
    CP_REG_TO_SCRATCH = 0x4a,
    CP_START_BIN = 0x50,
    CP_END_BIN = 0x51,
    CP_CONTEXT_UPDATE = 0x5e,
    CP_INTERRUPT = 0x40,
    CP_BLIT = 0x2c,
    CP_SET_DRAW_INIT_FLAGS = 0x4b,
    CP_SET_PROTECTED_MODE = 0x5f,
    CP_BOOTSTRAP_UCODE = 0x6f,
    CP_LOAD_STATE4 = 0x30,
    CP_COND_INDIRECT_BUFFER_PFE = 0x3a,
    CP_LOAD_STATE6_GEOM = 0x32,
    CP_SCRATCH_WRITE = 0x4c,
    CP_TEST_TWO_MEMS = 0x71,
    CP_REG_WR_NO_CTXT = 0x78,
    CP_RECORD_PFP_TIMESTAMP = 0x11,
    CP_SET_SECURE_MODE = 0x66,
    CP_WAIT_FOR_ME = 0x13,
    CP_SET_DRAW_STATE = 0x43,
    CP_DRAW_INDX_OFFSET = 0x38,
    CP_DRAW_INDIRECT = 0x28,
    CP_DRAW_INDX_INDIRECT = 0x29,
    CP_DRAW_INDIRECT_MULTI = 0x2a,
    CP_DRAW_AUTO = 0x24,
    CP_DRAW_PRED_ENABLE_GLOBAL = 0x19,
    CP_DRAW_PRED_ENABLE_LOCAL = 0x1a,
    CP_DRAW_PRED_SET = 0x4e,
    CP_REG_TO_MEM_OFFSET_MEM = 0x74,
    CP_SCRATCH_TO_REG = 0x4d,
    CP_WAIT_MEM_WRITES = 0x12,
    CP_COND_REG_EXEC = 0x47,
    CP_MEM_TO_REG = 0x42,
    CP_EXEC_CS_INDIRECT = 0x41,
    CP_EXEC_CS = 0x33,
    CP_SET_MARKER = 0x65,
    CP_SET_PSEUDO_REG = 0x56,
    CP_SKIP_IB2_ENABLE_GLOBAL = 0x1d,
    CP_WHERE_AM_I = 0x62,
    CP_SET_VISIBILITY_OVERRIDE = 0x64,
    CP_PREEMPT_ENABLE_GLOBAL = 0x69,
    CP_PREEMPT_ENABLE_LOCAL = 0x6a,
    CP_CONTEXT_SWITCH_YIELD = 0x6b,
    CP_PREEMPT_DISABLE = 0x6c,
    CP_COMPUTE_CHECKPOINT = 0x6e,
    CP_MEM_TO_MEM = 0x73,
    CP_REG_TEST = 0x39,
    CP_SET_MODE = 0x63,
    CP_REG_TO_MEM_OFFSET_REG = 0x72,
    CP_WAIT_TIMESTAMP = 0x14,
    CP_WAIT_TWO_REGS = 0x70,
    CP_MEMCPY = 0x75,
    CP_CONTEXT_SWITCH = 0x54,
    CP_SET_CTXSWITCH_IB = 0x55,
    CP_REG_WRITE = 0x6d,
    CP_GLOBAL_TIMESTAMP = 0x15,
    CP_LOCAL_TIMESTAMP = 0x16,
    CP_THREAD_CONTROL = 0x17,
    CP_RESOURCE_LIST = 0x18,
    CP_BV_BR_COUNT_OPS = 0x1b,
    CP_UNK49 = 0x49,
};

//--------------------------------------------------------------------------------------------------
struct IndirectBufferInfo
{
    uint64_t m_va_addr;
    uint32_t m_size_in_dwords;

    // Set to true to avoid emulating this IB, probably because it was not captured
    bool m_skip;
};

// clang-format off
union Pm4Type4Header
{
    struct
    {
        uint32_t count          : 7;
        uint32_t count_parity   : 1;
        uint32_t offset         : 19;
        uint32_t offset_parity  : 1;
        uint32_t type           : 4;
    };
    uint32_t u32All;
};

union Pm4Type7Header
{
    struct
    {
        uint32_t count          : 15;
        uint32_t count_parity   : 1;
        uint32_t opcode         : 7;
        uint32_t opcode_parity  : 1;
        uint32_t zeroes         : 4;
        uint32_t type           : 4;
    };
    uint32_t u32All;
};

enum class Pm4Type { kType2, kType4, kType7, kUnknown };
// clang-format on

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

    // Callback for each Pm4 packet. Called in order of emulation
    virtual bool OnPacket(const IMemoryManager &       mem_manager,
                          uint32_t                     submit_index,
                          uint32_t                     ib_index,
                          uint64_t                     va_addr,
                          Pm4Type                      type,
                          uint32_t                     header)
    {
        return true;
    }
};

//--------------------------------------------------------------------------------------------------
// Emulation state tracker. Optional to reduce unnecessary memory and processing overhead.
class EmulateStateTracker
{
public:
    EmulateStateTracker();

    // Call these functions to update the state tracker
    bool OnPacket(const IMemoryManager &       mem_manager,
                  uint32_t                     submit_index,
                  uint32_t                     ib_index,
                  uint64_t                     va_addr,
                  Pm4Type                      type,
                  uint32_t                     header);

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
    /*
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
    static const uint32_t kNumPersistentRegs = 1; // Pal::Gfx9::PERSISTENT_SPACE_END - Pal::Gfx9::PERSISTENT_SPACE_START
    uint8_t               m_sh_is_set[(kNumPersistentRegs / 8) + 1];
    static const uint32_t kNumContextRegs = 1; // Pal::Gfx9::Gfx09_10::CONTEXT_SPACE_END - Pal::Gfx9::CONTEXT_SPACE_START
    uint32_t m_context_data[kNumContextRegs];
    uint8_t  m_context_is_set[(kNumContextRegs / 8) + 1];

    static const uint32_t kNumUConfigRegs = 1; //Pal::Gfx9::UCONFIG_SPACE_END - Pal::Gfx9::UCONFIG_SPACE_START + 1;
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
    */
};

//--------------------------------------------------------------------------------------------------
class EmulatePM4
{
public:
    EmulatePM4();

    // Emulate a submit
    static const uint32_t kMaxNumIbsPerSubmit = 16;

    bool ExecuteSubmit(IEmulateCallbacks &       callbacks,
                       const IMemoryManager &    mem_manager,
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
                        uint32_t                  start_ib_index) const;

    // Find next index of dcb (if dcb_desired==1) or ccb (if dcb_desired==0) ib. Returns UINT32_MAX
    // if not found. Dcb - Draw command buffer. Ccb - Constant command buffer. Ib - Indirect Buffer.
    uint32_t GetNextIb(uint32_t submit_index, uint32_t start_ib_index) const;

    // Advance dcb pointer after advancing past the packet header. Returns "true" if dcb is blocked.
    bool AdvanceCb(const IMemoryManager &       mem_manager,
                   EmulateState *               emu_state_ptr,
                   IEmulateCallbacks &          callbacks,
                   Pm4Type                      type,
                   uint32_t                     header) const;

    // Helper function to help with advancing emulation to IB
    bool AdvanceToIB(const IMemoryManager        &mem_manager,
                     bool                         is_chain,
                     uint64_t                     ib_addr,
                     uint32_t                     ib_size_in_dwords,
                     const IndirectBufferInfo &   ib_info,
                     EmulateState::CbState *      cb_state,
                     IEmulateCallbacks &          callbacks,
                     Pm4Type                      type,
                     uint32_t                     header) const;

    // Helper function to help with advancing emulation to next packet
    bool AdvancePacket(const IMemoryManager &       mem_manager,
                       const IndirectBufferInfo &   ib_info,
                       EmulateState::CbState *      cb_state,
                       IEmulateCallbacks &          callbacks,
                       Pm4Type                      type,
                       uint32_t                     header) const;

    // Helper function to help with advancing emulation out of IB
    bool AdvanceOutOfIB(const IMemoryManager     &mem_manager,
                        const IndirectBufferInfo &ib_info,
                        EmulateState::CbState *   cb_state,
                        IEmulateCallbacks &       callbacks) const;

    uint32_t CalcParity(uint32_t val);

    // Emulation source
    uint32_t                  m_submit_index;
    uint32_t                  m_num_ibs;
    const IndirectBufferInfo *m_ib_ptr;
};

//--------------------------------------------------------------------------------------------------
bool IsDrawDispatchDmaEvent(uint32_t opcode);
bool IsDispatchEventOpcode(uint32_t opcode);
bool IsDrawEventOpcode(uint32_t opcode);

uint32_t GetPacketSize(uint32_t header);

}  // namespace Dive
