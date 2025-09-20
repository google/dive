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

#include "dive_core/perf_metrics_data.h"

#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>

#include "dive_core/command_hierarchy.h"
#include "dive_core/available_metrics.h"
#include "dive_core/common/string_utils.h"

namespace Dive
{

namespace
{
constexpr std::array kFixedHeaders = { "ContextID",   "ProcessID", "FrameID",
                                       "CmdBufferID", "DrawID",    "DrawType",
                                       "DrawLabel",   "ProgramID", "LRZState" };

struct ParseHeadersResult
{
    std::vector<std::string>       metric_names;
    std::vector<const MetricInfo*> metric_infos;
};

std::optional<ParseHeadersResult> ParseHeaders(const std::string&      line,
                                               const AvailableMetrics& available_metrics)
{
    std::vector<const MetricInfo*> metric_infos;
    std::vector<std::string>       metric_names;

    std::stringstream header_ss(line);
    std::string       header_field;
    int               column_index = 0;
    while (StringUtils::GetTrimmedField(header_ss, header_field, ','))
    {
        if (column_index < kFixedHeaders.size())
        {
            if (header_field != kFixedHeaders[column_index])
                return std::nullopt;  // Fixed header mismatch
        }
        else
        {
            metric_names.push_back(header_field);
            metric_infos.push_back(available_metrics.GetMetricInfo(header_field));
        }
        column_index++;
    }
    if (column_index < kFixedHeaders.size())
    {
        return std::nullopt;  // Not enough columns
    }

    return ParseHeadersResult{ std::move(metric_names), std::move(metric_infos) };
}

std::optional<PerfMetricsRecord> ParseRecordFixedFields(const std::vector<std::string>& fields)
{
    if (fields.size() < kFixedHeaders.size())
    {
        return std::nullopt;
    }

    PerfMetricsRecord record{};
    if (!StringUtils::SafeConvertFromString(fields[0], record.m_context_id) ||
        !StringUtils::SafeConvertFromString(fields[1], record.m_process_id) ||
        !StringUtils::SafeConvertFromString(fields[2], record.m_frame_id) ||
        !StringUtils::SafeConvertFromString(fields[3], record.m_cmd_buffer_id) ||
        !StringUtils::SafeConvertFromString(fields[4], record.m_draw_id) ||
        !StringUtils::SafeConvertFromString(fields[5], record.m_draw_type) ||
        !StringUtils::SafeConvertFromString(fields[6], record.m_draw_label) ||
        !StringUtils::SafeConvertFromString(fields[7], record.m_program_id) ||
        !StringUtils::SafeConvertFromString(fields[8], record.m_lrz_state))
    {
        return std::nullopt;  // Parsing failed
    }
    return record;
}

template<typename T>
bool ParseAndEmplaceMetric(const std::string& s, std::vector<std::variant<int64_t, float>>& values)
{
    T val{};
    if (StringUtils::SafeConvertFromString(s, val))
    {
        values.emplace_back(val);
        return true;
    }
    return false;
}

bool ParseMetrics(const std::vector<std::string>&       fields,
                  const std::vector<const MetricInfo*>& metric_infos,
                  PerfMetricsRecord&                    record)
{
    for (size_t i = 0; i < metric_infos.size(); ++i)
    {
        const std::string& value_str = fields[kFixedHeaders.size() + i];
        const MetricInfo*  info = metric_infos[i];
        if (info == nullptr)
        {
            // Unknown metric, this is an error. The number of metric values
            // will not match the number of metric names.
            return false;
        }

        bool parsed = false;
        switch (info->m_metric_type)
        {
        case MetricType::kCount:
            parsed = ParseAndEmplaceMetric<int64_t>(value_str, record.m_metric_values);
            break;
        case MetricType::kPercent:
            parsed = ParseAndEmplaceMetric<float>(value_str, record.m_metric_values);
            break;
        default:
            // kUnknown or other types are not supported.
            break;
        }
        if (!parsed)
        {
            return false;
        }
    }
    return true;
}

}  // namespace

std::unique_ptr<PerfMetricsData> PerfMetricsData::LoadFromCsv(
const std::filesystem::path&      file_path,
std::unique_ptr<AvailableMetrics> available_metrics)
{
    std::vector<PerfMetricsRecord> records;
    if (!available_metrics)
    {
        auto available_metrics_path = file_path.parent_path().append("available_settings.csv");
        if (std::filesystem::exists(available_metrics_path))
        {
            available_metrics = AvailableMetrics::LoadFromCsv(available_metrics_path);
        }
    }
    if (!available_metrics)
    {
        return nullptr;
    }

    std::ifstream file(file_path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return nullptr;
    }

    std::string line;
    // Read header line
    if (!StringUtils::GetTrimmedLine(file, line) || line.empty())
    {
        return nullptr;
    }
    auto headers_opt = ParseHeaders(line, *available_metrics);
    if (!headers_opt.has_value())
    {
        return nullptr;
    }

    auto& metric_names = headers_opt->metric_names;
    auto& metric_infos = headers_opt->metric_infos;

    // Read data lines
    while (StringUtils::GetTrimmedLine(file, line))
    {
        std::stringstream        ss(line);
        std::string              field;
        std::vector<std::string> fields;
        while (StringUtils::GetTrimmedField(ss, field, ','))
        {
            fields.push_back(field);
        }

        if (fields.size() != kFixedHeaders.size() + metric_names.size())
        {
            continue;  // Skip malformed lines
        }

        auto record = ParseRecordFixedFields(fields);
        if (!record.has_value())
        {
            continue;  // Skip malformed lines
        }

        if (ParseMetrics(fields, metric_infos, *record))
        {
            records.push_back(*std::move(record));
        }
    }

    return std::unique_ptr<PerfMetricsData>(new PerfMetricsData(std::move(metric_names),
                                                                std::move(metric_infos),
                                                                std::move(records),
                                                                std::move(available_metrics)));
}

PerfMetricsData::PerfMetricsData(std::vector<std::string>          metric_names,
                                 std::vector<const MetricInfo*>    metric_infos,
                                 std::vector<PerfMetricsRecord>    records,
                                 std::unique_ptr<AvailableMetrics> available_metrics) :
    m_metric_names(std::move(metric_names)),
    m_metric_infos(std::move(metric_infos)),
    m_records(std::move(records)),
    m_available_metrics(std::move(available_metrics))
{
}

class PerfMetricsDataProvider::Correlator
{
public:
    using SubmitRef = uint64_t;
    using CommandBufferRef = uint64_t;
    using DrawRef = uint64_t;
    using PatternIndex = size_t;
    static constexpr PatternIndex kInvalidPatternIndex = std::numeric_limits<PatternIndex>::max();

    void Reset();

    void AnalyzeCommands(const CommandHierarchy&);

    void AnalyzeRecords(const std::vector<PerfMetricsRecord>&);

    std::optional<PatternIndex> MatchOf(size_t index) const;

    size_t GetPatternSize() const { return m_pattern_size; }

    std::optional<DrawRef> GetDrawRef(PatternIndex offset) const
    {
        if (offset >= m_draws.size())
        {
            return std::nullopt;
        }
        return m_draws[offset];
    }
    std::optional<PatternIndex> GetPatternIndex(DrawRef ref) const
    {
        auto iter = m_draw_to_index.find(ref);
        if (iter == m_draw_to_index.end())
        {
            return std::nullopt;
        }
        return iter->second;
    }

private:
    static void ExtractDraws(const CommandHierarchy&                 command_hierarchy,
                             std::vector<uint64_t>&                  out_draws,
                             std::unordered_map<uint64_t, uint64_t>& out_mapping);

    static bool IsSimilar(size_t n, const PerfMetricsRecord* a, const PerfMetricsRecord* b);

    std::vector<DrawRef>                      m_draws;
    std::unordered_map<DrawRef, PatternIndex> m_draw_to_index;

    size_t                    m_pattern_size;
    std::vector<PatternIndex> m_record_to_index;
};

std::optional<PerfMetricsDataProvider::Correlator::PatternIndex>
PerfMetricsDataProvider::Correlator::MatchOf(size_t index) const
{

    if (index >= m_record_to_index.size())
    {
        return std::nullopt;
    }
    PatternIndex pattern_index = m_record_to_index[index];
    if (pattern_index >= m_pattern_size)
    {
        return std::nullopt;
    }
    return pattern_index;
}

void PerfMetricsDataProvider::Correlator::ExtractDraws(
const CommandHierarchy&                 command_hierarchy,
std::vector<uint64_t>&                  out_draws,
std::unordered_map<uint64_t, uint64_t>& out_mapping)
{
    std::unordered_map<uint64_t, uint64_t> draw_to_index;

    std::vector<uint64_t> actual_draws;
    std::vector<uint64_t> alias_draws;

    std::vector<uint64_t>* draws = &actual_draws;

    auto dedupe = [&]() {
        draws = &actual_draws;
        if (alias_draws.size() <= actual_draws.size())
        {
            size_t base = actual_draws.size() - alias_draws.size();
            for (size_t i = 0; i < alias_draws.size(); ++i)
            {
                draw_to_index[alias_draws[i]] = base + i;
            }
        }
        alias_draws.clear();
    };

    auto& alias_marker = command_hierarchy.GetFilterExcludeIndices(
    Dive::CommandHierarchy::kBinningPassOnly);

    for (size_t i = 0; i < command_hierarchy.size(); ++i)
    {
        auto node_type = command_hierarchy.GetNodeType(i);
        if (node_type == Dive::NodeType::kSubmitNode)
        {
            dedupe();
        }
        if (node_type == Dive::NodeType::kRenderMarkerNode)
        {
            dedupe();
            if (alias_marker.find(i) != alias_marker.end())
            {
                draws = &alias_draws;
            }
        }
        if (!Dive::IsDrawDispatchNode(node_type))
        {
            continue;
        }
        draws->push_back(i);
    }
    dedupe();
    for (size_t i = 0; i < actual_draws.size(); ++i)
    {
        draw_to_index[actual_draws[i]] = i;
    }
    out_draws = std::move(actual_draws);
    out_mapping = std::move(draw_to_index);
}

void PerfMetricsDataProvider::Correlator::Reset()
{
    m_draws.clear();
    m_draw_to_index.clear();

    m_pattern_size = 0;
    m_record_to_index.clear();
}

void PerfMetricsDataProvider::Correlator::AnalyzeCommands(const CommandHierarchy& command_hierarchy)
{
    ExtractDraws(command_hierarchy, m_draws, m_draw_to_index);
}

bool PerfMetricsDataProvider::Correlator::IsSimilar(size_t                   n,
                                                    const PerfMetricsRecord* a,
                                                    const PerfMetricsRecord* b)
{
    for (size_t i = 0; i < n; ++i)
    {
        if (a[i].m_cmd_buffer_id != b[i].m_cmd_buffer_id)
        {
            return false;
        }
        if (a[i].m_draw_id != b[i].m_draw_id)
        {
            return false;
        }
        // The rest of fields are assumed to be same, except for frame id.
    }
    return true;
}

void PerfMetricsDataProvider::Correlator::AnalyzeRecords(
const std::vector<PerfMetricsRecord>& records)
{
    m_pattern_size = 0;
    m_record_to_index.clear();
    
    if (records.empty())
    {
        return;
    }
    size_t template_frame_start = 0;
    size_t template_frame_size = 0;

    // Find the frame with the max number of draw calls, and use that as template.
    {
        size_t frame_start = 0;
        auto   emit_frame = [&](size_t start, size_t end) {
            if (end - start > template_frame_size)
            {
                template_frame_start = start;
                template_frame_size = end - start;
            }
        };
        for (size_t i = 0; i < records.size(); ++i)
        {
            if (records[frame_start].m_frame_id != records[i].m_frame_id)
            {
                emit_frame(frame_start, i);
                frame_start = i;
            }
        }
        emit_frame(frame_start, records.size());
    }

    if (!m_draws.empty() && m_draws.size() != template_frame_size)
    {
        // Draw pattern is different between capture file and metric file.
        return;
    }

    std::vector<PatternIndex> record_to_index(records.size(), kInvalidPatternIndex);
    {
        size_t frame_start = 0;
        auto   emit_frame = [&](size_t start, size_t end) {
            size_t frame_size = end - start;
            if (frame_size != template_frame_size)
            {
                // Mismatch number of draw calls.
                return;
            }
            if (!IsSimilar(frame_size, &records[template_frame_start], &records[start]))
            {
                // Bad data?
                return;
            }
            for (int i = 0; i < frame_size; ++i)
            {
                record_to_index[start + i] = i;
            }
        };
        for (size_t i = 0; i < records.size(); ++i)
        {
            if (records[frame_start].m_frame_id != records[i].m_frame_id)
            {
                emit_frame(frame_start, i);
                frame_start = i;
            }
        }
        emit_frame(frame_start, records.size());
    }

    m_pattern_size = template_frame_size;
    m_record_to_index = std::move(record_to_index);
}

std::unique_ptr<PerfMetricsDataProvider> PerfMetricsDataProvider::Create()
{
    return std::unique_ptr<PerfMetricsDataProvider>(new PerfMetricsDataProvider);
}
PerfMetricsDataProvider::~PerfMetricsDataProvider() = default;
PerfMetricsDataProvider::PerfMetricsDataProvider() :
    m_correlator(new Correlator)
{
}

void PerfMetricsDataProvider::Update(const CommandHierarchy&          command_hierarchy,
                                     std::unique_ptr<PerfMetricsData> data)
{
    if (!data)
    {
        return;
    }

    m_raw_data = std::move(data);
    m_computed_records.clear();

    const size_t num_metrics = m_raw_data->GetMetricNames().size();
    const auto&  records = m_raw_data->GetRecords();
    m_correlator->Reset();
    m_correlator->AnalyzeCommands(command_hierarchy);
    m_correlator->AnalyzeRecords(records);

    const size_t pattern_size = m_correlator->GetPatternSize();
    m_computed_records.resize(pattern_size);

    // This map will store intermediate sums and counts for averaging.
    // Key: {cmd_buffer_id, draw_id}
    // Value: {vector of metric sums, count of records}
    struct MetricSumWithRecordCount
    {
        std::vector<double> m_metric_sums{};
        uint32_t            m_record_count = 0;
    };

    std::vector<MetricSumWithRecordCount> sums_and_counts;
    sums_and_counts.resize(pattern_size);

    for (size_t record_index = 0; record_index < records.size(); ++record_index)
    {
        auto pattern_index = m_correlator->MatchOf(record_index);
        if (!pattern_index)
        {
            break;
        }
        auto& record = records[record_index];
        auto& draw_call_data = sums_and_counts[*pattern_index];
        if (draw_call_data.m_record_count == 0)
        {
            draw_call_data.m_metric_sums.resize(num_metrics);

            m_computed_records[*pattern_index] = record;
        }

        if (record.m_metric_values.size() == num_metrics)
        {
            for (size_t i = 0; i < num_metrics; ++i)
            {
                std::visit([&](
                           auto&&
                           arg) { draw_call_data.m_metric_sums[i] += static_cast<double>(arg); },
                           record.m_metric_values[i]);
            }
            draw_call_data.m_record_count++;
        }
    }

    // Now, build the final m_computed_records vector with averaged records in order.
    for (size_t draw_index = 0; draw_index < pattern_size; ++draw_index)
    {
        PerfMetricsRecord& averaged_record = m_computed_records[draw_index];
        averaged_record.m_metric_values.clear();
        // frame_id for aggregated data is meaningless.
        averaged_record.m_frame_id = 0;

        const auto count = sums_and_counts[draw_index].m_record_count;
        auto&      sums = sums_and_counts[draw_index].m_metric_sums;
        if (count == 0 || sums.empty())
        {
            continue;
        }

        averaged_record.m_metric_values.reserve(num_metrics);
        for (size_t i = 0; i < num_metrics; ++i)
        {
            const auto* metric_info = m_raw_data->GetMetricInfos()[i];
            if (metric_info && metric_info->m_metric_type == MetricType::kCount)
            {
                const int64_t average = static_cast<int64_t>(sums[i] / count + 0.5);
                averaged_record.m_metric_values.emplace_back(average);
            }
            else  // kPercent or others, store as float
            {
                const float average = static_cast<float>(sums[i] / count);
                averaged_record.m_metric_values.emplace_back(average);
            }
        }
    }
}

const std::vector<std::string> PerfMetricsDataProvider::GetRecordHeader() const
{
    std::vector<std::string> full_header;
    full_header.reserve(kFixedHeaders.size() + GetMetricsNames().size());
    for (const auto& header : kFixedHeaders)
    {
        full_header.push_back(header);
    }
    for (const auto& metric_name : GetMetricsNames())
    {
        full_header.push_back(metric_name);
    }
    return full_header;
}

std::optional<uint64_t> PerfMetricsDataProvider::GetCorrelatedComputedRecordIndex(
uint64_t draw_ref) const
{
    return m_correlator->GetPatternIndex(draw_ref);
}

const std::vector<std::string>& PerfMetricsDataProvider::GetMetricsNames() const
{
    return m_raw_data->GetMetricNames();
}

std::string_view PerfMetricsDataProvider::GetMetricsDescription(size_t metric_index) const
{
    const auto& metrics_info = m_raw_data->GetMetricInfos();
    if (metric_index >= metrics_info.size() || metrics_info[metric_index] == nullptr)
    {
        return {};
    }
    return metrics_info[metric_index]->m_description;
}

}  // namespace Dive
