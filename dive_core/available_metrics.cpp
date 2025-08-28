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

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace Dive
{

namespace
{
constexpr int kAvailableMetricsFieldCount = 5;
}
// Helper to remove leading/trailing quotes from a string
static void remove_quotes(std::string& str)
{
    if (str.length() >= 2 && str.front() == '"' && str.back() == '"')
    {
        str = str.substr(1, str.length() - 2);
    }
}

bool AvailableMetrics::LoadFromCsv(const char* file_path)
{
    std::ifstream file(file_path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return false;
    }

    m_metrics.clear();

    std::string line;
    // Skip header line
    if (!std::getline(file, line))
    {
        return false;
    }

    while (std::getline(file, line))
    {
        std::stringstream        ss(line);
        std::string              field;
        std::vector<std::string> fields;
        while (std::getline(ss, field, ','))
        {
            fields.push_back(field);
        }

        if (fields.size() < kAvailableMetricsFieldCount)
        {
            continue;  // Skip malformed lines
        }

        MetricInfo info;
        info.m_metric_id = static_cast<uint8_t>(std::stoi(fields[0]));
        info.m_metric_type = static_cast<MetricType>(std::stoul(fields[1]));
        info.m_key = fields[2];
        info.m_name = fields[3];
        info.m_description = fields[4];
        for (size_t i = 5; i < fields.size(); ++i)
        {
            info.m_description += ", " + fields[i];
        }

        remove_quotes(info.m_key);
        remove_quotes(info.m_name);
        remove_quotes(info.m_description);

        m_metrics[info.m_key] = info;
    }

    return true;
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