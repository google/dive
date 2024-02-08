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

#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "include/perfetto/trace_processor/trace_processor.h"
#include "perfetto/trace_processor/basic_types.h"

namespace Dive
{
namespace ptp = ::perfetto::trace_processor;
// Subset of https://perfetto.dev/docs/analysis/sql-tables#gpu_slice, which was captured in Dive.
struct GpuSliceData
{
    int64_t  m_id;        //	Unique identifier for this slice.
    int64_t  m_ts;        // The timestamp at the start of the slice (in nanoseconds).
    int64_t  m_duration;  // The duration of the slice (in nanoseconds).
    uint32_t m_submission_id;
    std::string
    m_name;  // The name of the slice. The name describes what was happening during the slice.
    int64_t  m_stack_id;
    int64_t  m_parent_stack_id;
    uint32_t m_parent_id;   // The id of the parent (i.e. immediate ancestor) slice for this slice
    uint32_t m_arg_set_id;  // The id of the argument set associated with this slice
    int64_t  m_render_pass;
};

// Args table from https://perfetto.dev/docs/analysis/sql-tables#args
struct ArgsData
{
    int64_t     m_id;
    std::string m_type;
    uint32_t    m_arg_set_id;
    std::string m_flat_key;
    int64_t m_int_value;  // Only one of the int_value/string_value/real_value will be available.
    std::string m_string_value;
    double      m_real_value;
    std::string m_value_type;
};

// SubmissionData holds GpuSliceData per submission_id, and the data is ordered by timestamp.
struct SubmissionData
{
    uint32_t                  m_submission_id;
    std::vector<GpuSliceData> m_data;
};

struct SurfaceData
{
    int64_t     m_surface_id;
    int64_t     m_ts;
    int64_t     m_duration;
    int64_t     m_submission_id;
    std::string m_name;
    uint32_t    m_arg_set_id;  // The id of the argument set associated with this slice
    int64_t     m_render_pass;
    int64_t     m_command_buffer;

    std::vector<ArgsData> m_args;
};

class GpuSliceDataParser
{
public:
    GpuSliceDataParser(std::unique_ptr<ptp::TraceProcessor> trace_processor) :
        m_trace_processor(std::move(trace_processor))
    {
    }

    std::vector<SubmissionData> ParseSubmissionData();
    std::vector<SurfaceData>    ParseSurfaceData();

private:
    std::vector<int64_t>  ParseDiveTraceTimestamp();
    std::vector<int64_t>  ParseSubmissionIds(int64_t start_ts, int64_t end_ts);
    std::vector<int64_t>  ParseSurfaceIds(int64_t start_ts, int64_t end_ts);
    int64_t               ParseStackIdForSurface(int64_t submission_id);
    SubmissionData        ParseSubmission(int64_t submission_id);
    std::vector<ArgsData> ParseArgsData(uint32_t arg_set_id);
    SurfaceData           ParseSurfaceData(int64_t surface_id);

    std::unique_ptr<ptp::TraceProcessor> m_trace_processor;

    template<typename T, typename std::enable_if_t<std::is_integral_v<T>, T>* = nullptr>
    void GetValue(const ptp::SqlValue& value, T& field)
    {
        field = value.AsLong();
    }
    template<typename T, typename std::enable_if_t<std::is_floating_point_v<T>, T>* = nullptr>
    void GetValue(const ptp::SqlValue& value, T& field)
    {
        field = value.AsDouble();
    }

    template<typename T, typename std::enable_if_t<std::is_same_v<std::string, T>, T>* = nullptr>
    void GetValue(const ptp::SqlValue& value, T& field)
    {
        field = value.AsString();
    }

    template<typename T>
    void GetQueryValue(ptp::Iterator&                  it,
                       int                             index,
                       T&                              field,
                       const std::vector<std::string>& field_name)
    {
        if (index > field_name.size())
        {
            std::cerr << "GetQueryValue:: Index out of range" << std::endl;
            return;
        }

        if (field_name[index] != it.GetColumnName(index))
        {
            std::cerr << "GetQueryValue: field name didn't match column name" << std::endl;
            return;
        }

        ptp::SqlValue value = it.Get(index);
        GetValue(value, field);
    }
};

}  // namespace Dive