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
#include <string_view>
#include <string>
#include <variant>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <memory>
#include <optional>
#include <functional>
#include <utility>
#include <tuple>

namespace Dive
{

class CommandHierarchy;
class AvailableMetrics;
struct MetricInfo;

// A key for performance metrics, combining command buffer ID and draw ID.
struct PerfMetricsKey
{
    uint64_t m_command_buffer_id;
    uint32_t m_draw_id;

    bool operator==(const PerfMetricsKey& other) const
    {
        return m_command_buffer_id == other.m_command_buffer_id && m_draw_id == other.m_draw_id;
    }
    bool operator!=(const PerfMetricsKey& other) const { return !this->operator==(other); }
    bool operator<(const PerfMetricsKey& other) const
    {
        return std::tie(m_command_buffer_id, m_draw_id) <
               std::tie(other.m_command_buffer_id, other.m_draw_id);
    }
};

// Hash function for PerfMetricsKey.
struct PerfMetricsKeyHash
{
    std::size_t operator()(const PerfMetricsKey& k) const
    {
        return std::hash<uint64_t>()(k.m_command_buffer_id) ^
               (std::hash<uint32_t>()(k.m_draw_id) << 1);
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
public:
    // Load performance metrics data from a CSV file
    [[nodiscard]] static std::unique_ptr<PerfMetricsData> LoadFromCsv(
    const std::filesystem::path&      file_path,
    std::unique_ptr<AvailableMetrics> available_metrics);

    // Get all performance metrics records
    const std::vector<PerfMetricsRecord>& GetRecords() const { return m_records; }

    // Get the names of the performance metrics
    const std::vector<std::string>& GetMetricNames() const { return m_metric_names; }

    // Get the information of the performance metrics
    const std::vector<const MetricInfo*>& GetMetricInfos() const { return m_metric_infos; }

    PerfMetricsData(std::vector<std::string>          metric_names,
                    std::vector<const MetricInfo*>    metric_infos,
                    std::vector<PerfMetricsRecord>    records,
                    std::unique_ptr<AvailableMetrics> available_metrics);

private:
    std::vector<std::string>       m_metric_names;
    std::vector<const MetricInfo*> m_metric_infos;
    std::vector<PerfMetricsRecord> m_records;
    // Keep available_metrics alive, since m_metric_infos has raw pointers into it.
    std::unique_ptr<AvailableMetrics> m_available_metrics;
};

class PerfMetricsDataProvider
{
public:
    [[nodiscard]] static std::unique_ptr<PerfMetricsDataProvider> Create(
    std::unique_ptr<PerfMetricsData> = nullptr);

    ~PerfMetricsDataProvider();
    PerfMetricsDataProvider(const PerfMetricsDataProvider&) = delete;
    PerfMetricsDataProvider(PerfMetricsDataProvider&&) = delete;
    PerfMetricsDataProvider& operator=(const PerfMetricsDataProvider&) = delete;
    PerfMetricsDataProvider& operator=(PerfMetricsDataProvider&&) = delete;

    // Update perf metrics data.
    void Update(std::unique_ptr<PerfMetricsData>);

    // Process aggregated statistics
    void Analyze(const CommandHierarchy* = nullptr);

    // Return a row that is corrisponding to the node_index.
    std::optional<uint64_t> GetCorrelatedComputedRecordIndex(uint64_t node_index) const;

    // Get the all of the metrics for a frame. The metrics are computed average of the input
    // dataset, ordered by command buffer appearance and then draw ID appearance order.
    const std::vector<PerfMetricsRecord>& GetComputedRecords() const { return m_computed_records; }

    // Returns the header for the record.
    const std::vector<std::string> GetRecordHeader() const;

    // Get the names of the captured metrics
    const std::vector<std::string>& GetMetricsNames() const;

    // Given the index of the metric, returns the description for that metric.
    std::string_view GetMetricsDescription(size_t metric_index) const;

private:
    class Correlator;

    PerfMetricsDataProvider();

    std::unique_ptr<Correlator> m_correlator;

    std::unique_ptr<PerfMetricsData> m_raw_data;
    std::vector<PerfMetricsRecord>   m_computed_records;  // calculated based on the |m_raw_data|
};

}  // namespace Dive
