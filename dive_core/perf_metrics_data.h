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
#include <filesystem>
#include <memory>

namespace Dive
{

class AvailableMetrics;
struct MetricInfo;

// The performance metrics result csv file is in the format of
// "ContextID,ProcessID,FrameID,CmdBufferID,DrawID,DrawType,DrawLabel,ProgramID,LRZState,COUNTER_A,COUNTER_B,
// ... "
struct PerfMetricsRecord
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
    std::vector<std::variant<int64_t, float>> m_metric_values;
};

class PerfMetricsData
{
public:
    // Load performance metrics data from a CSV file
    static std::unique_ptr<PerfMetricsData> LoadFromCsv(const std::filesystem::path& file_path,
                                                        const AvailableMetrics& available_metrics);

    // Get all performance metrics records
    const std::vector<PerfMetricsRecord>& GetRecords() const { return m_records; }

    // Get the names of the performance metrics
    const std::vector<std::string>& GetMetricNames() const { return m_metric_names; }

    // Get the information of the performance metrics
    const std::vector<const MetricInfo*>& GetMetricInfos() const { return m_metric_infos; }

private:
    PerfMetricsData(std::vector<std::string>       metric_names,
                    std::vector<const MetricInfo*> metric_infos,
                    std::vector<PerfMetricsRecord> records) :
        m_metric_names(std::move(metric_names)),
        m_metric_infos(std::move(metric_infos)),
        m_records(std::move(records))
    {
    }

    std::vector<std::string>       m_metric_names;
    std::vector<const MetricInfo*> m_metric_infos;
    std::vector<PerfMetricsRecord> m_records;
};

}  // namespace Dive
