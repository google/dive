
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
#include "memory_manager_base.h"
#include "shader_reflector.h"

namespace Dive
{

// =================================================================================================
// ShaderReflector
// =================================================================================================
bool ShaderReflector::Process(IShaderReflectorCallbacks   &callbacks,
                              const IMemoryManager        &memory_manager,
                              const EmulateStateTracker   &state_tracker,
                              const EmulateConstantEngine &constant_engine_emu,
                              uint32_t                     submit_index,
                              uint32_t                     opcode,
                              void                        *user_ptr)

{
    return false;
}

//--------------------------------------------------------------------------------------------------
bool ShaderReflector::HandlePotentialSRDTable(IShaderReflectorCallbacks   &callbacks,
                                              const IMemoryManager        &memory_manager,
                                              const EmulateConstantEngine &constant_engine_emu,
                                              ShaderStage                  shader_stage,
                                              uint32_t                     submit_index,
                                              uint64_t                     shader_addr,
                                              uint32_t                     user_data,
                                              void                        *user_ptr)
{
    // Could be pointer to a descriptor set, spill table, vertex buffer table, or just random data
    // from a push constant. Pointers are 32-bits, where the top 32-bits comes from the shader addr
    uint64_t potential_table_addr = (shader_addr & (UINT64_MAX - UINT32_MAX)) | user_data;

    UserParam info(memory_manager, callbacks, shader_stage, submit_index, user_ptr);

    // Some SRD Tables are written to and dumped from CE RAM. This is true for vertex buffers, spill
    // tables, etc. Descriptor set tables are not.
    uint32_t offset, num_bytes;
    if (constant_engine_emu.GetDumpedOffset(&offset, &num_bytes, potential_table_addr))
    {
        void *data_ptr = constant_engine_emu.GetCERam(offset);

        // Return value is ignored, because no further callbacks will be called
        GetDescriptorDataFromTable(data_ptr, potential_table_addr, num_bytes, &info);

        if (info.m_table_size > 0)
        {
            // There are valid SRDs in the table, so do callback on the table
            if (!callbacks.OnSRDTable(shader_stage,
                                      data_ptr,
                                      potential_table_addr,
                                      info.m_table_size,
                                      user_ptr))
                return false;
        }
    }
    else
    {
        // The size of the SRD Table is unknown, so it is the job of the callback function to
        // iterate through the table and stop when invalid SRD data is found
        if (!memory_manager.GetMemoryOfUnknownSizeViaCallback(submit_index,
                                                              potential_table_addr,
                                                              GetDescriptorDataFromTable,
                                                              &info))
            return false;

        if (info.m_table_size > 0)
        {
            // There are valid SRDs in the table, so do callback on the table
            if (!callbacks.OnSRDTable(shader_stage,
                                      potential_table_addr,
                                      info.m_table_size,
                                      user_ptr))
                return false;
        }
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
bool ShaderReflector::GetDescriptorDataFromTable(const void *data_ptr,
                                                 uint64_t    va_addr,
                                                 uint64_t    max_size,
                                                 void       *user_ptr)
{
    return false;
}

}  // namespace Dive
