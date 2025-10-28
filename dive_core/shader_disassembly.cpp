/*
 Copyright 2021 Google LLC

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

#include "shader_disassembly.h"
#include <iostream>
#include "dive_core/common/memory_manager_base.h"
#include "pm4_info.h"

#ifdef _MSC_VER
#    include <stdio.h>
#    include <stdlib.h>

// To avoid warning from disasm.h
#    pragma warning(disable : 4996)
#endif
extern "C"
{
#include "third_party/mesa/src/freedreno/common/disasm.h"
}

namespace Dive
{

//--------------------------------------------------------------------------------------------------
bool Disassemble(const uint8_t*                             shader_memory,
                 uint64_t                                   shader_address,
                 size_t                                     shader_max_size,
                 std::vector<ShaderInstruction>*            instructions,
                 std::function<std::string(uint64_t index)> on_emit,
                 std::string&                               output,
                 ILog*                                      log_ptr)
{
    return false;
}

//--------------------------------------------------------------------------------------------------
std::string DisassembleA3XX(const uint8_t*       data,
                            size_t               max_size,
                            struct shader_stats* stats,
                            enum debug_t         debug)
{
    disasm_a3xx_set_debug(debug);

#ifdef _MSC_VER
    FILE*   disasm_file = NULL;
    errno_t err = tmpfile_s(&disasm_file);
    if (err != 0)
    {
        DIVE_ASSERT(err == 0);
    }
    DIVE_ASSERT(disasm_file != NULL);
#else
    char*  disasm_buf = nullptr;
    size_t disasm_buf_size = 0;
    FILE*  disasm_file = open_memstream(&disasm_buf, &disasm_buf_size);
#endif

    size_t code_size = max_size / sizeof(uint32_t);
    int    res = disasm_a3xx_stat(reinterpret_cast<uint32_t*>(const_cast<uint8_t*>(data)),
                               static_cast<int>(code_size),
                               0,
                               disasm_file,
                               GetGPUID(),
                               stats);
    ((void)(res));  // avoid unused variable
    DIVE_ASSERT(res != -1);
#ifdef _MSC_VER
    rewind(disasm_file);
    std::string disasm_output;
    char        buffer[1024];
    size_t      bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), disasm_file)) > 0)
    {
        disasm_output.append(buffer, bytes_read);
    }
    fclose(disasm_file);
    return disasm_output;
#else
    fflush(disasm_file);
    disasm_buf[disasm_buf_size] = '\0';
    std::string disasm(disasm_buf);
    fclose(disasm_file);
    free(disasm_buf);
    return disasm;
#endif
}

//--------------------------------------------------------------------------------------------------
Disassembly::Disassembly(const IMemoryManager& mem_manager,
                         uint32_t              submit_index,
                         uint64_t              address,
                         ILog*                 log) :
    m_mem_manager(mem_manager),
    m_submit_index(submit_index),
    m_address(address),
    m_log(log)
{
    ((void)(m_log));  // avoid unused variable
}

//--------------------------------------------------------------------------------------------------
void Disassembly::Disassemble()
{
    if (!m_disassembled_data)
    {
        DisassembledData disassembled_data;
        uint64_t         max_size = m_mem_manager.GetMaxContiguousSize(m_submit_index, m_address);

        // The disassembler does not early-out when it encounters an "end" instruction (at least not
        // in its "prepass"), so passing it a too-big max_size can make the disassembly very slow!
        // Let's set an arbitrary limit for now. The "correct" fix would be for the disassembler to
        // early-out.
        uint64_t kMaxSizeLimit = 64 * 1024;
        if (max_size > kMaxSizeLimit)
            max_size = kMaxSizeLimit;

        uint8_t* data_ptr = new uint8_t[max_size];
        DIVE_VERIFY(
        m_mem_manager.RetrieveMemoryData(data_ptr, m_submit_index, m_address, max_size));

        struct shader_stats stats;
        std::string         disasm = DisassembleA3XX(data_ptr, max_size, &stats, PRINT_RAW);
        std::istringstream  disasm_istr(disasm);
        unsigned            opc_cat = 0;
        unsigned            n = 0;
        unsigned            cycles = 0;
        uint32_t            dword0 = 0, dword1 = 0;
        int                 prefix_len = 0;
        for (std::string line; std::getline(disasm_istr, line);)
        {
#ifdef _MSC_VER
            sscanf_s(line.c_str(),
                     " :%d:%04d:%04d[%08xx_%08xx] %n",
                     &opc_cat,
                     &n,
                     &cycles,
                     &dword1,
                     &dword0,
                     &prefix_len);
#else
            sscanf(line.c_str(),
                   " :%d:%04d:%04d[%08xx_%08xx] %n",
                   &opc_cat,
                   &n,
                   &cycles,
                   &dword1,
                   &dword0,
                   &prefix_len);
#endif
            std::string instr(&line[prefix_len]);
            if (n >= disassembled_data.m_instructions_text.size())
            {
                disassembled_data.m_instructions_text.resize(n + 1);
                disassembled_data.m_instructions_raw.resize(n + 1);
            }
            if (disassembled_data.m_instructions_text[n].size() > 0)
            {
                disassembled_data.m_instructions_text[n] += "\n";
            }
            disassembled_data.m_instructions_text[n] += instr;

            disassembled_data.m_instructions_raw[n] = (static_cast<uint64_t>(dword1) << 32) |
                                                      dword0;
        }
        disassembled_data.m_gpr_count = (stats.fullreg + 3) / 4;
        disassembled_data.m_listing = DisassembleA3XX(data_ptr, max_size, &stats, PRINT_STATS);
        delete[] data_ptr;
        m_disassembled_data = disassembled_data;
    }
}

}  // namespace Dive
