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

#include "dive_core/available_metrics.h"
#include "dive_core/common/string_utils.h"

namespace Dive
{
namespace
{
constexpr std::array kFixedHeaders = { "ContextID",   "ProcessID", "FrameID",
                                       "CmdBufferID", "DrawID",    "DrawType",
                                       "DrawLabel",   "ProgramID", "LRZState" };

bool ParseHeaders(const std::string&              line,
                  const AvailableMetrics&         available_metrics,
                  std::vector<std::string>&       metric_names,
                  std::vector<const MetricInfo*>& metric_infos)
{
    std::stringstream header_ss(line);
    std::string       header_field;
    int               column_index = 0;
    while (StringUtils::GetTrimmedField(header_ss, header_field, ','))
    {
        if (column_index < kFixedHeaders.size())
        {
            if (header_field != kFixedHeaders[column_index])
                return false;
        }
        else
        {
            metric_names.push_back(header_field);
            metric_infos.push_back(available_metrics.GetMetricInfo(header_field));
        }
        column_index++;
    }

    return column_index >= kFixedHeaders.size();
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
                  std::optional<PerfMetricsRecord>&     record_opt)
{
    bool all_metrics_parsed = true;
    for (size_t i = 0; i < metric_infos.size(); ++i)
    {
        const std::string& value_str = fields[kFixedHeaders.size() + i];
        const MetricInfo*  info = metric_infos[i];
        if (info)
        {
            switch (info->m_metric_type)
            {
            case MetricType::kCount:
                all_metrics_parsed = ParseAndEmplaceMetric<int64_t>(value_str,
                                                                    record_opt->m_metric_values);
                break;
            case MetricType::kPercent:
                all_metrics_parsed = ParseAndEmplaceMetric<float>(value_str,
                                                                  record_opt->m_metric_values);
                break;
            default:
                all_metrics_parsed = false;
                break;
            }
        }
        if (!all_metrics_parsed)
        {
            break;
        }
    }
    return all_metrics_parsed;
}

}  // namespace

std::unique_ptr<PerfMetricsData> PerfMetricsData::LoadFromCsv(
const std::filesystem::path& file_path,
const AvailableMetrics&      available_metrics)
{
    std::vector<std::string>       metric_names;
    std::vector<const MetricInfo*> metric_infos;
    std::vector<PerfMetricsRecord> records;

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

    if (!ParseHeaders(line, available_metrics, metric_names, metric_infos))
    {
        return nullptr;
    }

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

        if (ParseMetrics(fields, metric_infos, record_opt))
        {
            records.push_back(*record_opt);
        }
    }

    return std::unique_ptr<PerfMetricsData>(
    new PerfMetricsData(std::move(metric_names), std::move(metric_infos), std::move(records)));
}

}  // namespace Dive
