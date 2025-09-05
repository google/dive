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
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>

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

        auto record_opt = ParseRecordFixedFields(fields);
        if (!record_opt.has_value())
        {
            continue;  // Skip malformed lines
        }

        if (ParseMetrics(fields, metric_infos, *record_opt))
        {
            records.push_back(std::move(*record_opt));
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

std::unique_ptr<PerfMetricsDataProvider> PerfMetricsDataProvider::Create(
std::unique_ptr<PerfMetricsData> data)
{
    if (!data)
        return nullptr;

    return std::unique_ptr<PerfMetricsDataProvider>(new PerfMetricsDataProvider(std::move(data)));
}

PerfMetricsDataProvider::PerfMetricsDataProvider(std::unique_ptr<PerfMetricsData> data) :
    m_raw_data(std::move(data))
{
    // This map will store intermediate sums and counts for averaging.
    // Key: {cmd_buffer_id, draw_id}
    // Value: {vector of metric sums, count of records}
    std::unordered_map<PerfMetricsKey, std::pair<std::vector<double>, uint32_t>, PerfMetricsKeyHash>
    sums_and_counts;

    // This map will store the first record for each draw call to preserve non-metric data.
    // Key: {cmd_buffer_id, draw_id}
    // Value: const PerfMetricsRecord*
    std::unordered_map<PerfMetricsKey, const PerfMetricsRecord*, PerfMetricsKeyHash> first_records;

    std::unordered_set<PerfMetricsKey, PerfMetricsKeyHash> seen_draw_calls;
    std::unordered_set<uint64_t>                           seen_cmd_buffers;

    const size_t num_metrics = m_raw_data->GetMetricNames().size();

    for (const auto& record : m_raw_data->GetRecords())
    {
        PerfMetricsKey key = { record.m_cmd_buffer_id, record.m_draw_id };
        // m_cmd_buffer_list maintains the order of command buffers as they appear in the raw data.
        if (seen_cmd_buffers.find(record.m_cmd_buffer_id) == seen_cmd_buffers.end())
        {
            m_cmd_buffer_list.push_back(record.m_cmd_buffer_id);
            seen_cmd_buffers.insert(record.m_cmd_buffer_id);
        }

        if (seen_draw_calls.find(key) == seen_draw_calls.end())
        {
            m_cmd_buffer_to_draw_id[record.m_cmd_buffer_id].push_back(record.m_draw_id);
            first_records[key] = &record;
            seen_draw_calls.insert(key);
        }

        auto& draw_call_data = sums_and_counts[key];
        if (draw_call_data.second == 0)
        {
            draw_call_data.first.resize(num_metrics, 0.0);
        }

        if (record.m_metric_values.size() == num_metrics)
        {
            for (size_t i = 0; i < num_metrics; ++i)
            {
                std::visit([&](auto&& arg) { draw_call_data.first[i] += static_cast<double>(arg); },
                           record.m_metric_values[i]);
            }
            draw_call_data.second++;
        }
    }

    // Now, build the final m_computed_records vector with averaged records in order.
    for (const auto& cmd_buffer_id : m_cmd_buffer_list)
    {
        const auto& draw_ids = m_cmd_buffer_to_draw_id.at(cmd_buffer_id);
        for (uint32_t draw_id : draw_ids)
        {
            PerfMetricsKey key = { cmd_buffer_id, draw_id };
            const auto&    sums_pair = sums_and_counts.at(key);
            const auto&    sums = sums_pair.first;
            const auto     count = sums_pair.second;

            if (count > 0)
            {
                PerfMetricsRecord averaged_record = *first_records.at(key);
                averaged_record.m_metric_values.clear();
                averaged_record.m_metric_values.reserve(num_metrics);
                for (size_t i = 0; i < num_metrics; ++i)
                {
                    const auto*  metric_info = m_raw_data->GetMetricInfos()[i];
                    const double average = sums[i] / count;
                    if (metric_info && metric_info->m_metric_type == MetricType::kCount)
                    {
                        averaged_record.m_metric_values.emplace_back(static_cast<int64_t>(average));
                    }
                    else  // kPercent or others, store as float
                    {
                        averaged_record.m_metric_values.emplace_back(static_cast<float>(average));
                    }
                }
                m_metric_key_to_index[key] = m_computed_records.size();
                m_computed_records.push_back(std::move(averaged_record));
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

std::optional<std::reference_wrapper<const PerfMetricsRecord>>
PerfMetricsDataProvider::GetComputedRecord(size_t command_buffer_index,
                                           size_t draw_call_index) const
{
    if (command_buffer_index >= m_cmd_buffer_list.size())
    {
        return std::nullopt;
    }

    const uint64_t command_buffer_id = m_cmd_buffer_list[command_buffer_index];
    const auto     it_draw_ids = m_cmd_buffer_to_draw_id.find(command_buffer_id);
    if (it_draw_ids == m_cmd_buffer_to_draw_id.end())
    {
        return std::nullopt;
    }

    const auto& draw_ids = it_draw_ids->second;
    if (draw_call_index >= draw_ids.size())
    {
        return std::nullopt;
    }

    const uint32_t       draw_call_id = draw_ids[draw_call_index];
    const PerfMetricsKey metric_key{ command_buffer_id, draw_call_id };

    const auto it_record_index = m_metric_key_to_index.find(metric_key);
    if (it_record_index == m_metric_key_to_index.end())
    {
        return std::nullopt;
    }

    const size_t record_index = it_record_index->second;
    if (record_index >= m_computed_records.size())
    {
        return std::nullopt;
    }
    return std::cref(m_computed_records[record_index]);
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

size_t PerfMetricsDataProvider::GetDrawCountForCommandBuffer(size_t command_buffer_index) const
{
    if (command_buffer_index >= m_cmd_buffer_list.size())
        return 0;

    uint64_t cmd_id = m_cmd_buffer_list[command_buffer_index];
    return m_cmd_buffer_to_draw_id.at(cmd_id).size();
}
}  // namespace Dive
