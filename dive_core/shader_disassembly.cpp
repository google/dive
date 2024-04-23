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
#include "dive_core/common/common.h"
#include "pm4_info.h"

#ifndef _MSC_VER
extern "C"
{
#    include "third_party/mesa/src/freedreno/common/disasm.h"
}
#endif

namespace Dive
{

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

// TODO(b/309801805): Shader disassembly is disabled on windows due to compiler errors.
#ifndef _MSC_VER
std::string DisassembleA3XX(const uint8_t*       data,
                            size_t               max_size,
                            struct shader_stats* stats,
                            enum debug_t         debug)
{
    disasm_a3xx_set_debug(debug);

    char*  disasm_buf = nullptr;
    size_t disasm_buf_size = 0;
    FILE*  disasm_file = open_memstream(&disasm_buf, &disasm_buf_size);
    int    res = disasm_a3xx_stat(reinterpret_cast<uint32_t*>(const_cast<uint8_t*>(data)),
                               max_size / sizeof(uint32_t),
                               0,
                               disasm_file,
                               GetGPUID(),
                               stats);
    ((void)(res));  // avoid unused variable
    DIVE_ASSERT(res != -1);
    fflush(disasm_file);
    disasm_buf[disasm_buf_size] = '\0';
    std::string disasm(disasm_buf);
    fclose(disasm_file);
    free(disasm_buf);
    return disasm;
}
#endif

void Disassembly::Init(const uint8_t* data, uint64_t address, size_t max_size, ILog* log_ptr)
{
#ifndef _MSC_VER
    struct shader_stats stats;
    std::string         disasm = DisassembleA3XX(data, max_size, &stats, PRINT_RAW);
    std::istringstream  disasm_istr(disasm);
    unsigned            opc_cat = 0;
    unsigned            n = 0;
    unsigned            cycles = 0;
    uint32_t            dword0 = 0, dword1 = 0;
    int                 prefix_len = 0;
    for (std::string line; std::getline(disasm_istr, line);)
    {
        sscanf(line.c_str(),
               " :%d:%04d:%04d[%08xx_%08xx] %n",
               &opc_cat,
               &n,
               &cycles,
               &dword1,
               &dword0,
               &prefix_len);
        std::string instr(&line[prefix_len]);
        if (n >= m_instructions_text.size())
        {
            m_instructions_text.resize(n + 1);
            m_instructions_raw.resize(n + 1);
        }
        if (m_instructions_text[n].size() > 0)
        {
            m_instructions_text[n] += "\n";
        }
        m_instructions_text[n] += instr;

        m_instructions_raw[n] = (static_cast<uint64_t>(dword1) << 32) | dword0;
    }
    m_gpr_count = (stats.fullreg + 3) / 4;
    m_listing = DisassembleA3XX(data, max_size, &stats, PRINT_STATS);
#endif
}

}  // namespace Dive
