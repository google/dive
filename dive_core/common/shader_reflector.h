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
// Handles shader resource descriptor (SRD) reflection. That is, given a state-tracker, this module
// will try to figure out all SRDs being referenced by the current shader(s) used by the current
// draw/dispatch event
// =================================================================================================
#pragma once
#include <stdint.h>
#include "emulate_pm4.h"

namespace Dive
{

// Forward declaration
class IMemoryManager;

//--------------------------------------------------------------------------------------------------
// Copy of relevant constants, enums, and structs from gfx9chip.h
// Cannot include that file outside of PAL

constexpr uint8_t NumUserDataRegistersCompute = 16;
constexpr uint8_t NumUserDataRegisters = 32;

enum BUF_INDEX_STRIDE : uint32_t
{
    BUF_INDEX_STRIDE_8B = 0,
    BUF_INDEX_STRIDE_16B = 1,
    BUF_INDEX_STRIDE_32B = 2,
    BUF_INDEX_STRIDE_64B = 3,
};

//--------------------------------------------------------------------------------------------------
class IShaderReflectorCallbacks
{
public:
    // Callbacks on each SRD table used by the shader. The SRD table is a buffer that contains 1 or
    // more SRDs, each of which might be a different type
    virtual bool OnSRDTable(ShaderStage shader_stage,
                            uint64_t    va_addr,
                            uint64_t    size,
                            void       *user_ptr) = 0;
    virtual bool OnSRDTable(ShaderStage shader_stage,
                            void       *data_ptr,
                            uint64_t    va_addr,
                            uint64_t    size,
                            void       *user_ptr) = 0;
};

//--------------------------------------------------------------------------------------------------
class ShaderReflector
{
public:
    bool Process(IShaderReflectorCallbacks   &callbacks,
                 const IMemoryManager        &memory_manager,
                 const EmulateStateTracker   &state_tracker,
                 const EmulateConstantEngine &constant_engine_emu,
                 uint32_t                     submit_index,
                 uint32_t                     opcode,
                 void                        *user_ptr);

private:
    bool HandlePotentialSRDTable(IShaderReflectorCallbacks   &callbacks,
                                 const IMemoryManager        &memory_manager,
                                 const EmulateConstantEngine &constant_engine_emu,
                                 ShaderStage                  shader_stage,
                                 uint32_t                     submit_index,
                                 uint64_t                     shader_addr,
                                 uint32_t                     user_data,
                                 void                        *user_ptr);

    struct UserParam
    {
        // Input
        const IMemoryManager      &m_memory_manager;
        IShaderReflectorCallbacks &m_callbacks;
        ShaderStage                m_shader_stage;
        uint32_t                   m_submit_index;
        void                      *m_user_ptr;

        // Output
        uint64_t m_table_size;

        UserParam(const IMemoryManager      &memory_manager,
                  IShaderReflectorCallbacks &callbacks,
                  ShaderStage                shader_stage,
                  uint32_t                   submit_index,
                  void                      *user_ptr) :
            m_memory_manager(memory_manager),
            m_callbacks(callbacks),
            m_shader_stage(shader_stage),
            m_submit_index(submit_index),
            m_user_ptr(user_ptr),
            m_table_size(0)
        {
        }
    };
    static bool GetDescriptorDataFromTable(const void *data_ptr,
                                           uint64_t    va_addr,
                                           uint64_t    max_size,
                                           void       *user_ptr);
};

//--------------------------------------------------------------------------------------------------

}  // namespace Dive
