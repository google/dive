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
// A set of callbacks are provided for clients for certain important events that occur in the
// command buffer. Note that the "order" that the callbacks are in is the order the commands are
// executed in the emulation.
// =================================================================================================

#pragma once
#include <stdint.h>
#include "adreno.h"
#include "dive_core/common/pm4_packets/pfp_pm4_packets.h"
#include "gpudefs.h"

namespace Dive
{

// Forward declaration
class IMemoryManager;
class SubmitInfo;

struct IndirectBufferInfo
{
    uint64_t m_va_addr;
    uint32_t m_size_in_dwords;
    uint8_t  m_ib_level;
    uint32_t m_enable_mask;

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
enum class ShaderEnableBit : uint32_t
{
    kBINNING = 0x1,
    kGMEM = 0x2,
    kSYSMEM = 0x4,
};

//--------------------------------------------------------------------------------------------------
// The number of bits IB enable masks: BINNING, GMEM, SYSMEM
static constexpr uint32_t kShaderEnableBitCount = 3;

//--------------------------------------------------------------------------------------------------
class IEmulateCallbacks
{
public:
    bool ProcessSubmits(const std::vector<SubmitInfo> &submits, const IMemoryManager &mem_manager);

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
    virtual bool OnPacket(const IMemoryManager &mem_manager,
                          uint32_t              submit_index,
                          uint32_t              ib_index,
                          uint64_t              va_addr,
                          Pm4Type               type,
                          uint32_t              header)
    {
        return true;
    }

    virtual void OnSubmitStart(uint32_t submit_index, const SubmitInfo &submit_info) = 0;
    virtual void OnSubmitEnd(uint32_t submit_index, const SubmitInfo &submit_info) = 0;
};

//--------------------------------------------------------------------------------------------------
// Emulation state tracker. Optional to reduce unnecessary memory and processing overhead.
class EmulateStateTracker
{
public:
    EmulateStateTracker();

    // Call these functions to update the state tracker
    bool OnPacket(const IMemoryManager &mem_manager,
                  uint32_t              submit_index,
                  uint32_t              ib_index,
                  uint64_t              va_addr,
                  Pm4Type               type,
                  uint32_t              header);

    // Accessing state tracking info
    bool     IsUConfigStateSet(uint16_t reg) const;
    uint32_t GetUConfigRegData(uint16_t reg) const;
    bool     IsShStateSet(uint16_t reg) const;
    uint32_t GetShRegData(uint16_t reg) const;

    // Check if any user data registers were set since last draw/dispatch
    // The driver only sets them on a per-event basis, and checking IsShStateSet() tracks since the
    // last time state tracking was reset
    // Note: Does not track any of the system-reserved user data registers!
    uint32_t GetNumberOfUserDataRegsSetSinceLastEvent(ShaderStage stage) const;
    bool     IsUserDataRegsSetSinceLastEvent(ShaderStage stage, uint8_t offset) const;

    // Returns UINT64_MAX if shader not set
    uint64_t GetCurShaderAddr(ShaderStage stage, uint32_t enable_mask) const;

    // Get address/size of most recent PM4 buffer (eg. extended user data, sh load buffers, etc)
    uint64_t GetBufferAddr() const;
    uint32_t GetBufferSize() const;

    // Reset the state tracker
    void Reset();

    void PushEnableMask(uint32_t enable_mask);

    void PopEnableMask();

    uint32_t GetRegValue(uint32_t offset, uint32_t enable_mask) const;

    uint32_t GetRegValue(uint32_t offset) const;

    uint64_t GetReg64Value(uint32_t offset, uint32_t enable_mask) const;

    uint64_t GetReg64Value(uint32_t offset) const;

    void SetReg(uint32_t offset, uint32_t value);

    bool IsRegSet(uint32_t offset) const;

    bool IsRegSet(uint32_t offset, uint32_t enable_mask) const;

private:
    static constexpr size_t kNumRegs = 0xffff + 1;
    uint32_t                m_reg[kShaderEnableBitCount][kNumRegs];
    uint8_t                 m_reg_is_set[kShaderEnableBitCount][(kNumRegs / 8) + 1];
    uint32_t                m_enable_mask = (1u << kShaderEnableBitCount) - 1;
    std::vector<uint32_t>   m_enable_mask_stack;
};

//--------------------------------------------------------------------------------------------------
class EmulatePM4
{
public:
    enum IbLevel
    {
        kPrimaryRing,
        kIb1,
        kIb2,
        kIb3,

        // There's no IB4 in the GPU, but it's useful to have one in the emulator in case a
        // CP_SET_DRAW_STATE is called from an IB3, since we're emulating those packets as
        // CALLs (I'm not sure if that's even possible, but better safe than sorry!)
        kIb4,

        kTotalIbLevels
    };

    static const uint32_t kMaxPendingIbs = 100;
    EmulatePM4();

    // Not sure what the upper bound is. 64 seems reasonably big.
    static const uint32_t kMaxNumIbsPerSubmit = 64;

    // Emulate a submit
    bool ExecuteSubmit(IEmulateCallbacks        &callbacks,
                       const IMemoryManager     &mem_manager,
                       uint32_t                  submit_index,
                       uint32_t                  num_ibs,
                       const IndirectBufferInfo *ib_ptr);

private:
    // Keep all emulation state together
    struct EmulateState
    {
        struct IbStack
        {
            // VA at the current IB level
            // If m_ib_queue_size is non-zero, then it is currently iterating through the queue
            // and executing those IBs. m_cur_va is only applicable once execution returns
            // to the current IB level
            uint64_t m_cur_va;
            uint64_t m_cur_ib_addr;
            uint32_t m_cur_ib_size_in_dwords;
            bool     m_cur_ib_skip;
            uint32_t m_cur_ib_enable_mask;
            IbType   m_cur_ib_type;

            // IB queue (for storing pending CALLs or CHAINs)
            IbType   m_ib_queue_type;
            uint64_t m_ib_queue_addrs[kMaxPendingIbs];
            uint32_t m_ib_queue_sizes_in_dwords[kMaxPendingIbs];
            bool     m_ib_queue_skip[kMaxPendingIbs];
            uint32_t m_ib_queue_enable_mask[kMaxPendingIbs];
            uint32_t m_ib_queue_index;
            uint32_t m_ib_queue_size;

            bool EndOfCurIb() const
            {
                bool endAddr = (m_cur_va >=
                                (m_cur_ib_addr + m_cur_ib_size_in_dwords * sizeof(uint32_t)));
                return (m_cur_ib_skip || endAddr);
            }
        };

        // Index of top-most IB (i.e. IB1)
        uint32_t m_ib_index;

        uint32_t m_submit_index;

        // Stack to managing the different IB levels
        // Top-most element contains the current Program Counter
        // If top of stack is at IB0, then that means there's nothing to execute
        IbStack  m_ib_stack[kTotalIbLevels];
        IbLevel  m_top_of_stack;
        IbStack *GetCurIb() { return &m_ib_stack[m_top_of_stack]; }
    };

    // Advance dcb pointer after advancing past the packet header. Returns "true" if dcb is blocked.
    bool AdvanceCb(const IMemoryManager &mem_manager,
                   EmulateState         *emu_state_ptr,
                   IEmulateCallbacks    &callbacks,
                   Pm4Type               type,
                   uint32_t              header) const;

    // Helper function to queue up an IB for later CALL or CHAIN
    // Use AdvanceToIB to actually jump to the 1st queued up IB
    bool QueueIB(uint64_t      ib_addr,
                 uint32_t      ib_size_in_dwords,
                 bool          skip,
                 IbType        ib_type,
                 EmulateState *emu_state,
                 uint32_t      enable_mask = 7) const;

    // Helper function to help with advancing emulation to IB
    bool AdvanceToQueuedIB(const IMemoryManager &mem_manager,
                           EmulateState         *emu_state,
                           IEmulateCallbacks    &callbacks) const;

    // Helper function to advance to next valid IB IF current one is ended/skipped
    bool CheckAndAdvanceIB(const IMemoryManager &mem_manager,
                           EmulateState         *emu_state,
                           IEmulateCallbacks    &callbacks) const;

    // Helper function to help with advancing emulation to next packet
    void AdvancePacket(EmulateState *emu_state, uint32_t header) const;

    // Helper function to help with advancing emulation out of IB
    bool AdvanceOutOfIB(EmulateState *emu_state, IEmulateCallbacks &callbacks) const;

    uint32_t CalcParity(uint32_t val);

    // Only used by CP_START_BIN/CP_END_BIN
    mutable uint64_t m_cp_start_bin_address = UINT64_MAX;
    mutable uint64_t m_prefix_start_address = UINT64_MAX;
    mutable uint32_t m_prefix_block_dword_size = 0;
    mutable uint32_t m_prefix_block_count = 0;
};

//--------------------------------------------------------------------------------------------------
bool IsDrawDispatchEventOpcode(uint32_t opcode);
bool IsDispatchEventOpcode(uint32_t opcode);
bool IsDrawEventOpcode(uint32_t opcode);

uint32_t GetPacketSize(uint32_t header);

}  // namespace Dive
