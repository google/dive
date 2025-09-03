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

#include "dive_core/available_metrics.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "dive_core/common/string_utils.h"
#include <optional>

namespace Dive
{

namespace
{
constexpr int kAvailableMetricsFieldCount = 5;
}  // namespace

std::optional<AvailableMetrics> AvailableMetrics::LoadFromCsv(
const std::filesystem::path& file_path)
{
    std::ifstream file(file_path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return std::nullopt;
    }

    AvailableMetrics available_metrics;
    std::string      line;
    // Read and ignore header line
    if (!StringUtils::GetTrimmedLine(file, line) || line.empty() ||
        line.find("MetricID") == std::string::npos)
    {
        return std::nullopt;
    }

    while (StringUtils::GetTrimmedLine(file, line))
    {
        std::stringstream        ss(line);
        std::string              field;
        std::vector<std::string> fields;
        while (StringUtils::GetTrimmedField(ss, field, ','))
        {
            fields.push_back(field);
        }

        if (fields.size() < kAvailableMetricsFieldCount)
        {
            continue;
        }

        MetricInfo info{};
        uint32_t   metric_type_val;
        if (!StringUtils::SafeConvertFromString(fields[0], info.m_metric_id) ||
            !StringUtils::SafeConvertFromString(fields[1], metric_type_val))
        {
            continue;
        }
        info.m_metric_type = static_cast<MetricType>(metric_type_val);
        info.m_key = fields[2];
        info.m_name = fields[3];
        info.m_description = fields[4];
        for (size_t i = 5; i < fields.size(); ++i)
        {
            info.m_description.append(", ").append(fields[i]);
        }

        available_metrics.m_metrics[info.m_key] = info;
    }

    return available_metrics;
}

const MetricInfo* AvailableMetrics::GetMetricInfo(const std::string& key) const
{
    auto it = m_metrics.find(key);
    if (it != m_metrics.end())
    {
        return &it->second;
    }
    return nullptr;
}

MetricType AvailableMetrics::GetMetricType(const std::string& key) const
{
    const MetricInfo* info = GetMetricInfo(key);
    return info ? info->m_metric_type : MetricType::kUnknown;
}

}  // namespace Dive
