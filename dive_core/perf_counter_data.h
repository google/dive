/*
 Copyright 2025 Google LLC
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
#include <string>
#include <variant>
#include <vector>

namespace Dive
{

class AvailableMetrics;
struct MetricInfo;

// The metrics result csv file is informat of
// "ContextID,ProcessID,FrameID,CmdBufferID,DrawID,DrawType,DrawLabel,ProgramID,LRZState,COUNTER_A,COUNTER_B,
// ... "
struct PerfCounterRecord
{
    uint64_t                                  m_context_id;
    uint64_t                                  m_process_id;
    uint64_t                                  m_frame_id;
    uint64_t                                  m_cmd_buffer_id;
    uint32_t                                  m_draw_id;
    uint32_t                                  m_draw_label;
    uint64_t                                  m_program_id;
    uint8_t                                   m_draw_type;
    uint8_t                                   m_lrz_state;
    std::vector<std::variant<int64_t, float>> m_counter_values;
};

class PerfCounterData
{
public:
    // Load performance counter data from a CSV file
    bool LoadFromCsv(const char* file_path, const AvailableMetrics& available_metrics);

    // Get all performance counter records
    const std::vector<PerfCounterRecord>& GetRecords() const { return m_records; }

    // Get the names of the performance counters
    const std::vector<std::string>& GetCounterNames() const { return m_counter_names; }

    // Get the metric info of the performance counters
    const std::vector<const MetricInfo*>& GetCounterInfo() const { return m_counter_infos; }

private:
    std::vector<std::string>       m_counter_names;
    std::vector<const MetricInfo*> m_counter_infos;
    std::vector<PerfCounterRecord> m_records;
};

}  // namespace Dive
