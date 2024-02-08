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

#include "gpu_slice_data.h"
#include <cstdint>
#include <iostream>
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"

namespace Dive
{

// Parse the start and end of the Dive tracing timestamps.
std::vector<int64_t> GpuSliceDataParser::ParseDiveTraceTimestamp()
{
    std::vector<int64_t> res;
    int64_t              start_ts;
    int64_t              duration;

    auto it = m_trace_processor->ExecuteQuery(
    "SELECT ts, dur FROM slice WHERE name=\"Dive trace\"");
    if (!it.Status().ok())
    {
        std::cerr << "No \"Dive trace\" found in the perfetto trace." << std::endl;
        return res;
    }
    // Get the ts;
    auto ts_value = it.Get(0);
    assert(ts_value.type == ptp::SqlValue::Type::kLong);
    start_ts = ts_value.AsLong();
    res.push_back(start_ts);

    // Get the duration
    auto dur_value = it.Get(1);
    assert(dur_value.type == ptp::SqlValue::Type::kLong);
    duration = dur_value.AsLong();
    res.push_back(start_ts + duration);

    return res;
}

std::vector<int64_t> GpuSliceDataParser::ParseSubmissionIds(int64_t start_ts, int64_t end_ts)
{
    std::vector<int64_t> submission_ids;
    {
        std::string
             query_str = absl::StrFormat("SELECT submission_id FROM gpu_slice WHERE "
                                         "name=\"vkQueueSubmit\" AND ts >= %ld AND ts <= %ld",
                                    start_ts,
                                    end_ts);
        auto it = m_trace_processor->ExecuteQuery(query_str);
        if (!it.Status().ok())
        {
            std::cerr << "No submission found in the perfetto trace during the dive trace event."
                      << std::endl;
            return submission_ids;
        }

        for (uint32_t rows = 0; it.Next(); rows++)
        {
            auto value = it.Get(0);
            assert(value.type == ptp::SqlValue::Type::kLong);
            submission_ids.push_back(value.AsLong());
        }
        assert(!submission_ids.empty());
    }

    return submission_ids;
}

int64_t GpuSliceDataParser::ParseStackIdForSurface(int64_t submission_id)
{
    int64_t stack_id = -1;

    std::string query_str = absl::StrFormat("SELECT stack_id FROM gpu_slice WHERE "
                                            "submission_id = %ld AND name=\"Surface\"",
                                            submission_id);
    auto        it = m_trace_processor->ExecuteQuery(query_str);
    if (!it.Status().ok())
    {
        std::cerr << "Failed to query Surface data" << std::endl;
        return false;
    }
    auto value = it.Get(0);
    assert(value.type == ptp::SqlValue::Type::kLong);
    stack_id = value.AsLong();

    return stack_id;
}

SubmissionData GpuSliceDataParser::ParseSubmission(int64_t submission_id)
{
    int64_t parent_stack_id = ParseStackIdForSurface(submission_id);

    SubmissionData submit;
    submit.m_submission_id = submission_id;

    std::vector<std::string> output_fields = { "id",
                                               "ts",
                                               "dur",
                                               "submission_id",
                                               "name",
                                               "stack_id",
                                               "parent_stack_id",
                                               "parent_id",
                                               "arg_set_id",
                                               "render_pass" };
    std::string              fields = absl::StrJoin(output_fields, ",");
    std::string
    query_str = absl::StrFormat("SELECT %s FROM gpu_slice WHERE "
                                "submission_id = %ld AND parent_stack_id=%ld order by ts",
                                fields,
                                submission_id,
                                parent_stack_id);

    ptp::Iterator it = m_trace_processor->ExecuteQuery(query_str);
    if (!it.Status().ok())
    {
        std::cerr << "Failed to query submission data for submission_id:" << submission_id
                  << std::endl;
        return submit;
    }
    for (uint32_t rows = 0; it.Next(); rows++)
    {
        GpuSliceData slice;
        int          i = 0;
        GetQueryValue(it, i++, slice.m_id, output_fields);
        GetQueryValue(it, i++, slice.m_ts, output_fields);
        GetQueryValue(it, i++, slice.m_duration, output_fields);
        GetQueryValue(it, i++, slice.m_submission_id, output_fields);
        GetQueryValue(it, i++, slice.m_name, output_fields);
        GetQueryValue(it, i++, slice.m_stack_id, output_fields);
        GetQueryValue(it, i++, slice.m_parent_stack_id, output_fields);
        GetQueryValue(it, i++, slice.m_parent_id, output_fields);
        GetQueryValue(it, i++, slice.m_arg_set_id, output_fields);
        GetQueryValue(it, i++, slice.m_render_pass, output_fields);
        submit.m_data.emplace_back(slice);
    }

    return submit;
}

std::vector<SubmissionData> GpuSliceDataParser::ParseSubmissionData()
{

    std::vector<SubmissionData> submission_data;
    std::vector<int64_t>        start_end_ts = ParseDiveTraceTimestamp();
    assert(start_end_ts.size() == 2);

    std::vector<int64_t> submission_ids = ParseSubmissionIds(start_end_ts[0], start_end_ts[1]);

    for (auto submission_id : submission_ids)
    {
        int64_t surface_id = ParseStackIdForSurface(submission_id);
        submission_data.emplace_back(ParseSubmission(submission_id));
    }

    return submission_data;
}

std::vector<int64_t> GpuSliceDataParser::ParseSurfaceIds(int64_t start_ts, int64_t end_ts)
{
    std::vector<int64_t> surface_ids;

    std::string query_str = absl::StrFormat("SELECT id FROM gpu_slice WHERE "
                                            "name=\"Surface\" AND ts >= %ld AND ts <= %ld",
                                            start_ts,
                                            end_ts);
    auto        sql_result = m_trace_processor->ExecuteQuery(query_str);
    auto&       it = sql_result;
    if (!it.Status().ok())
    {
        std::cerr << "No submission found in the perfetto trace during the dive trace event."
                  << std::endl;
        return surface_ids;
    }

    for (uint32_t rows = 0; sql_result.Next(); rows++)
    {
        auto value = it.Get(0);
        assert(value.type == ptp::SqlValue::Type::kLong);
        surface_ids.push_back(value.AsLong());
    }
    assert(!surface_ids.empty());

    return surface_ids;
}

std::vector<ArgsData> GpuSliceDataParser::ParseArgsData(uint32_t arg_set_id)
{
    std::vector<ArgsData> args_set_data;

    std::string query_str = absl::StrFormat("SELECT * FROM args WHERE "
                                            "args_set_id = %u",
                                            arg_set_id);
    auto        it = m_trace_processor->ExecuteQuery(query_str);
    if (!it.Status().ok())
    {
        std::cerr << "No submission found in the perfetto trace during the dive trace event."
                  << std::endl;
        return args_set_data;
    }

    for (uint32_t rows = 0; it.Next(); rows++)
    {
        // TODO: store args table.
    }

    return args_set_data;
}

SurfaceData GpuSliceDataParser::ParseSurfaceData(int64_t surface_id)
{
    SurfaceData render_stage_data;

    std::vector<std::string> fields = { "id",   "ts",         "dur",         "submission_id",
                                        "name", "arg_set_id", "render_pass", "command_buffer" };
    std::string              field_str = absl::StrJoin(fields, ",");

    std::string   query_str = absl::StrFormat("SELECT %s FROM args WHERE id = %ld",
                                            field_str,
                                            surface_id);
    ptp::Iterator it = m_trace_processor->ExecuteQuery(query_str);
    if (!it.Status().ok())
    {
        std::cerr << "Failed to query surface data for surface_id:" << surface_id << std::endl;
        return render_stage_data;
    }
    for (uint32_t rows = 0; it.Next(); rows++)
    {
        int i = 0;
        GetQueryValue(it, i++, render_stage_data.m_surface_id, fields);
        GetQueryValue(it, i++, render_stage_data.m_ts, fields);
        GetQueryValue(it, i++, render_stage_data.m_duration, fields);
        GetQueryValue(it, i++, render_stage_data.m_submission_id, fields);
        GetQueryValue(it, i++, render_stage_data.m_name, fields);
        GetQueryValue(it, i++, render_stage_data.m_arg_set_id, fields);
        GetQueryValue(it, i++, render_stage_data.m_render_pass, fields);
        GetQueryValue(it, i++, render_stage_data.m_command_buffer, fields);
        break;  // should at most have 1 row.
    }
    // Parse args from args table
    render_stage_data.m_args = ParseArgsData(render_stage_data.m_arg_set_id);

    return render_stage_data;
}

std::vector<SurfaceData> GpuSliceDataParser::ParseSurfaceData()
{
    std::vector<SurfaceData> surface_data;
    std::vector<int64_t>     start_end_ts = ParseDiveTraceTimestamp();
    assert(start_end_ts.size() == 2);

    std::vector<int64_t> surface_ids = ParseSurfaceIds(start_end_ts[0], start_end_ts[1]);

    for (auto surface_id : surface_ids)
    {
        surface_data.emplace_back(ParseSurfaceData(surface_id));
    }

    return surface_data;
}

}  // namespace Dive