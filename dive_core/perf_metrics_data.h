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
#include <unordered_map>
#include <memory>
#include <optional>
#include <functional>
#include <utility>

namespace Dive
{

class AvailableMetrics;
struct MetricInfo;

// A key for performance metrics, combining command buffer ID and draw ID.
using PerfMetricsKey = std::pair<uint64_t, uint32_t>;

// Hash function for PerfMetricsKey.
struct PerfMetricsKeyHash
{
    std::size_t operator()(const PerfMetricsKey& k) const
    {
        return std::hash<uint64_t>()(k.first) ^ (std::hash<uint32_t>()(k.second) << 1);
    }
};

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
    friend class PerfMetricsDataProviderTest;

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

class PerfMetricsDataProvider
{
public:
    static std::unique_ptr<PerfMetricsDataProvider> Create(std::unique_ptr<PerfMetricsData> data);

    // Get the total number of unique command buffers of this dataset.
    size_t GetCommandBufferCount() const { return m_cmd_buffer_list.size(); }

    // Given an index(0 based) of the command buffer array, return the number fo drawcalls in that
    // command buffer.
    size_t GetDrawCountForCommandBuffer(size_t command_buffer_index) const;

    // Given the index of the command  buffer and the index of draw call in that command buffer,
    // returns the |PerfMetricsRecord| for the combination.
    std::optional<std::reference_wrapper<const PerfMetricsRecord>> GetComputedRecord(
    size_t command_buffer_index,
    size_t draw_call_index) const;

    // Get the all of the metrics for a frame. The metrics are computed average of the input
    // dataset, ordered by command buffer appearance and then draw ID appearance order.
    const std::vector<PerfMetricsRecord>& GetComputedRecords() const { return m_computed_records; }

    // Get the names of the captured metrics
    const std::vector<std::string>& GetMetricsNames() const;

    // Given the index of the metric, returns the description for that metric.
    const std::string& GetMetricsDescription(size_t metric_index) const;

private:
    PerfMetricsDataProvider(std::unique_ptr<PerfMetricsData> data);

    std::unique_ptr<PerfMetricsData> m_raw_data;
    std::vector<PerfMetricsRecord>   m_computed_records;  // calculated based on the |m_raw_data|

    std::vector<uint64_t>                                          m_cmd_buffer_list;
    std::unordered_map<uint64_t, std::vector<uint32_t>>            m_cmd_buffer_to_draw_id;
    std::unordered_map<PerfMetricsKey, size_t, PerfMetricsKeyHash> m_metric_key_to_index;
};

}  // namespace Dive
