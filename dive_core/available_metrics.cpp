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
#include <array>
#include "utils/string_utils.h"

namespace Dive
{

namespace
{
const std::array kExpectedHeaders = { "MetricID", "MetricType", "Key", "Name", "Description" };
}  // namespace

std::unique_ptr<AvailableMetrics> AvailableMetrics::LoadFromCsv(
const std::filesystem::path& file_path)
{
    std::ifstream file(file_path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return nullptr;
    }

    auto        available_metrics = std::unique_ptr<AvailableMetrics>(new AvailableMetrics());
    std::string line;
    // Read and validate header line
    if (!StringUtils::GetTrimmedLine(file, line) || line.empty())
    {
        std::cerr << "File is empty or missing header: " << file_path << std::endl;
        return nullptr;
    }
    size_t line_number = 1;

    std::stringstream header_ss(line);
    std::string       header_field;
    size_t            column_index = 0;
    while (StringUtils::GetTrimmedField(header_ss, header_field, ','))
    {
        if (column_index < kExpectedHeaders.size())
        {
            if (header_field != kExpectedHeaders[column_index])
            {
                std::cerr << "Invalid header in file: " << file_path << std::endl;
                return nullptr;
            }
        }
        column_index++;
    }
    if (column_index != kExpectedHeaders.size())
    {
        std::cerr << "Invalid header format in " << file_path << ". Expected "
                  << kExpectedHeaders.size() << " columns, found " << column_index << "."
                  << std::endl;
        return nullptr;
    }

    while (StringUtils::GetTrimmedLine(file, line))
    {
        line_number++;
        std::stringstream        ss(line);
        std::string              field;
        std::vector<std::string> fields;
        while (StringUtils::GetTrimmedField(ss, field, ','))
        {
            fields.push_back(field);
        }

        if (fields.size() != kExpectedHeaders.size())
        {
            std::cerr << "Skipping malformed row at line " << line_number << " in " << file_path
                      << ": Found " << fields.size() << " columns, expected "
                      << kExpectedHeaders.size() << "." << std::endl;
            continue;
        }

        MetricInfo info{};
        uint32_t   metric_type_val = 0;
        if (!StringUtils::SafeConvertFromString(fields[0], info.m_metric_id) ||
            !StringUtils::SafeConvertFromString(fields[1], metric_type_val))
        {
            continue;
        }
        info.m_metric_type = static_cast<MetricType>(metric_type_val);
        info.m_key = std::move(fields[2]);
        info.m_name = std::move(fields[3]);
        info.m_description = std::move(fields[4]);

        available_metrics->m_metrics.emplace(info.m_key, std::move(info));
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

std::vector<std::string> AvailableMetrics::GetAllMetricKeys() const
{
    std::vector<std::string> keys;
    keys.reserve(m_metrics.size());

    for (const auto& pair : m_metrics)
    {
        keys.push_back(pair.first);
    }

    return keys;
}

}  // namespace Dive
