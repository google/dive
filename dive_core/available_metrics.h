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
#include <filesystem>
#include <string>
#include <unordered_map>
#include <optional>

namespace Dive
{

enum class MetricType : uint8_t
{
    kUnknown = 0,
    kCount = 1,
    kPercent = 2,
};

/*
Available metrics is in the format of
"MetricID,MetricType,Key,Name,Description"
*/

struct MetricInfo
{
    uint32_t    m_metric_id;
    MetricType  m_metric_type;
    std::string m_key;
    std::string m_name;
    std::string m_description;
};

class AvailableMetrics
{
public:
    // Load available metrics from a CSV file
    static std::optional<AvailableMetrics> LoadFromCsv(const std::filesystem::path& file_path);

    // Get the metric info for a given key
    const MetricInfo* GetMetricInfo(const std::string& key) const;

    // Get the metric type for a given key, return kUnknown if not found.
    MetricType GetMetricType(const std::string& key) const;

private:
    AvailableMetrics() = default;
    std::unordered_map<std::string, MetricInfo> m_metrics;
};

}  // namespace Dive
