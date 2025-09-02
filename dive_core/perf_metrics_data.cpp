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

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

#include "dive_core/available_metrics.h"

namespace Dive
{
namespace
{
constexpr int     kPerfMetricsFixFieldCount = 9;
const char* const kFixedHeaders[] = { "ContextID",   "ProcessID", "FrameID",
                                      "CmdBufferID", "DrawID",    "DrawType",
                                      "DrawLabel",   "ProgramID", "LRZState" };

void trim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); })
            .base(),
            s.end());
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
}

// Helper functions for safe string to number conversion
bool safe_stoull(const std::string& s, uint64_t& out)
{
    char*       end = nullptr;
    const char* start = s.c_str();
    errno = 0;
    unsigned long long val = std::strtoull(start, &end, 10);
    if (errno == ERANGE || *end != '\0' || start == end)
    {
        return false;
    }
    out = val;
    return true;
}

bool safe_stoul(const std::string& s, uint32_t& out)
{
    char*       end = nullptr;
    const char* start = s.c_str();
    errno = 0;
    unsigned long val = std::strtoul(start, &end, 10);
    if (errno == ERANGE || *end != '\0' || start == end)
    {
        return false;
    }
    out = val;
    return true;
}

bool safe_stoll(const std::string& s, int64_t& out)
{
    char*       end = nullptr;
    const char* start = s.c_str();
    errno = 0;
    long long val = std::strtoll(start, &end, 10);
    if (errno == ERANGE || *end != '\0' || start == end)
    {
        return false;
    }
    out = val;
    return true;
}

bool safe_stof(const std::string& s, float& out)
{
    char*       end = nullptr;
    const char* start = s.c_str();
    errno = 0;
    float val = std::strtof(start, &end);
    if (errno == ERANGE || *end != '\0' || start == end)
    {
        return false;
    }
    out = val;
    return true;
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
    if (!std::getline(file, line) || line.empty())
    {
        return nullptr;
    }
    trim(line);

    std::stringstream header_ss(line);
    std::string       header_field;
    int               column_index = 0;
    while (std::getline(header_ss, header_field, ','))
    {
        trim(header_field);
        if (column_index < kPerfMetricsFixFieldCount)
        {
            if (header_field != kFixedHeaders[column_index])
                return nullptr;
        }
        else
        {
            metric_names.push_back(header_field);
            metric_infos.push_back(available_metrics.GetMetricInfo(header_field));
        }
        column_index++;
    }
    if (column_index < kPerfMetricsFixFieldCount)
        return nullptr;

    while (std::getline(file, line))
    {
        trim(line);
        std::stringstream        ss(line);
        std::string              field;
        std::vector<std::string> fields;
        while (std::getline(ss, field, ','))
        {
            trim(field);
            fields.push_back(field);
        }

        if (fields.size() != kPerfMetricsFixFieldCount + metric_names.size())
        {
            continue;  // Skip malformed lines
        }

        PerfMetricsRecord record{};
        uint32_t          draw_type = 0, lrz_state = 0;
        if (!safe_stoull(fields[0], record.m_context_id) ||
            !safe_stoull(fields[1], record.m_process_id) ||
            !safe_stoull(fields[2], record.m_frame_id) ||
            !safe_stoull(fields[3], record.m_cmd_buffer_id) ||
            !safe_stoul(fields[4], record.m_draw_id) || !safe_stoul(fields[5], draw_type) ||
            !safe_stoul(fields[6], record.m_draw_label) ||
            !safe_stoull(fields[7], record.m_program_id) || !safe_stoul(fields[8], lrz_state))
        {
            continue;  // Skip lines with parsing errors
        }
        record.m_draw_type = static_cast<uint8_t>(draw_type);
        record.m_lrz_state = static_cast<uint8_t>(lrz_state);

        bool all_metrics_parsed = true;
        for (size_t i = 0; i < metric_infos.size(); ++i)
        {
            const std::string& value_str = fields[kPerfMetricsFixFieldCount + i];
            const MetricInfo*  info = metric_infos[i];
            if (info)
            {
                switch (info->m_metric_type)
                {
                case MetricType::kCount:
                {
                    int64_t val = 0;
                    if (safe_stoll(value_str, val))
                    {
                        record.m_metric_values.emplace_back(val);
                    }
                    else
                    {
                        all_metrics_parsed = false;
                    }
                    break;
                }
                case MetricType::kPercent:
                {
                    float val = 0.0f;
                    if (safe_stof(value_str, val))
                    {
                        record.m_metric_values.emplace_back(val);
                    }
                    else
                    {
                        all_metrics_parsed = false;
                    }
                    break;
                }
                default:
                    break;
                }
            }
            if (!all_metrics_parsed)
            {
                break;
            }
        }
        if (all_metrics_parsed)
        {
            records.push_back(record);
        }
    }

    return std::unique_ptr<PerfMetricsData>(
    new PerfMetricsData(std::move(metric_names), std::move(metric_infos), std::move(records)));
}

}  // namespace Dive
