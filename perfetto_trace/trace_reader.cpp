/*
 Copyright 2024 Google LLC

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

#if defined(DIVE_ENABLE_PERFETTO)
#include "trace_reader.h"

#include <inttypes.h>
#include <cstdint>
#include <iostream>
#include "include/perfetto/trace_processor/read_trace.h"
#include "include/perfetto/trace_processor/trace_processor.h"

namespace Dive
{

TraceReader::TraceReader(const std::string& trace_file_path) :
    m_trace_file_path(trace_file_path),
    m_trace_processor(
    perfetto::trace_processor::TraceProcessor::CreateInstance(perfetto::trace_processor::Config{}))
{
}

bool TraceReader::LoadTraceFile()
{
    std::function<void(uint64_t parsed_size)> pf = [](uint64_t parsed_size) {
        std::cout << "Read " << parsed_size << std::endl;
    };
    auto res = perfetto::trace_processor::ReadTrace(m_trace_processor.get(),
                                                    m_trace_file_path.c_str(),
                                                    pf);
    if (!res.ok())
    {
        std::cout << "Failed to load trace file" << m_trace_file_path << std::endl;
        return false;
    }
#ifndef NDEBUG
    auto sql_result = m_trace_processor->ExecuteQuery(
    "SELECT ts, dur, name, depth, parent_stack_id, arg_set_id, render_pass, submission_id FROM "
    "gpu_slice ");
    auto&    it = sql_result;
    uint32_t column_width = 16;
    for (uint32_t rows = 0; sql_result.Next(); rows++)
    {
        if (rows % 32 == 0)
        {
            for (uint32_t i = 0; i < sql_result.ColumnCount(); i++)
            {
                printf("%-*.*s ", column_width, column_width, it.GetColumnName(i).c_str());
            }
            printf("\n");

            std::string divider(column_width, '-');
            for (uint32_t i = 0; i < it.ColumnCount(); i++)
            {
                printf("%-*s ", column_width, divider.c_str());
            }
            printf("\n");
        }
        for (uint32_t c = 0; c < it.ColumnCount(); c++)
        {
            auto value = it.Get(c);
            switch (value.type)
            {
            case perfetto::trace_processor::SqlValue::Type::kNull:
                printf("%-*s", column_width, "[NULL]");
                break;
            case perfetto::trace_processor::SqlValue::Type::kDouble:
                printf("%*f", column_width, value.double_value);
                break;
            case perfetto::trace_processor::SqlValue::Type::kLong:
                printf("%*" PRIi64, column_width, value.long_value);
                break;
            case perfetto::trace_processor::SqlValue::Type::kString:
                printf("%-*.*s", column_width, column_width, value.string_value);
                break;
            case perfetto::trace_processor::SqlValue::Type::kBytes:
                printf("%-*s", column_width, "<raw bytes>");
                break;
            }
            printf(" ");
        }
        printf("\n");
    }
#endif
    return true;
}

}  // namespace Dive
#endif
