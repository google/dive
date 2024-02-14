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

#include "trace_reader.h"

#include <cstdint>
#include <iostream>
#include "include/perfetto/trace_processor/read_trace.h"
#include "include/perfetto/trace_processor/trace_processor.h"
#include "perfetto/trace_processor/status.h"

namespace Dive
{

namespace ptp = perfetto::trace_processor;

TraceReader::TraceReader(const std::string& trace_file_path) :
    m_trace_file_path(trace_file_path),
    m_trace_processor(ptp::TraceProcessor::CreateInstance(ptp::Config{}))
{
}

bool TraceReader::LoadTraceFile()
{
    std::function<void(uint64_t parsed_size)> pf = [](uint64_t parsed_size) {
        std::cout << "Read " << parsed_size << std::endl;
    };
    auto res = ptp::ReadTrace(m_trace_processor.get(), m_trace_file_path.c_str(), pf);
    if (!res.ok())
    {
        std::cout << "Failed to load trace file" << m_trace_file_path << std::endl;
        return false;
    }
#ifndef NDEBUG
    std::vector<SubmissionData> submission_data;
    std::vector<SurfaceData>    surface_data;
    PopulatePerfettoTraceData(submission_data, surface_data);
#endif
    return true;
}

bool TraceReader::LoadTraceFileFromBuffer(const std::vector<uint8_t>& data)
{
    size_t             data_size = data.size();
    ptp::TraceBlob     blob = ptp::TraceBlob::CopyFrom(data.data(), data_size);
    ptp::TraceBlobView blob_view(std::move(blob), 0, static_cast<size_t>(data_size));
    return m_trace_processor->Parse(std::move(blob_view)).ok();
}

bool TraceReader::PopulatePerfettoTraceData(std::vector<SubmissionData>& submission_data,
                                            std::vector<SurfaceData>&    surface_data)
{
    GpuSliceDataParser sp(std::move(m_trace_processor));
    submission_data = sp.ParseSubmissionData();

#ifndef NDEBUG
    for (const auto& d : submission_data)
    {
        std::cout << "m_submission_id" << d.m_submission_id << std::endl;
        for (const auto& slice : d.m_data)
        {
            std::cout << "id: " << slice.m_id << ", name: " << slice.m_name
                      << ", ts: " << slice.m_ts << std::endl;
        }
    }
#endif

    surface_data = sp.ParseSurfaceData();
#ifndef NDEBUG
    for (const auto& data : surface_data)
    {
        std::cout << "id " << data.m_surface_id << std::endl;
        std::cout << "ts " << data.m_ts << std::endl;
        std::cout << "name:  " << data.m_name << std::endl;
        for (const auto& arg : data.m_args)
        {
            std::cout << "id: " << arg.m_id << std::endl;

            std::cout << "m_flag_key: " << arg.m_flat_key << std::endl;
            std::cout << "m_string_value: " << arg.m_string_value << std::endl;
        }
    }
#endif
    return true;
}

}  // namespace Dive
