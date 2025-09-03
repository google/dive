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
#include <array>

#include "dive_core/available_metrics.h"

namespace Dive
{
namespace
{
constexpr std::array kFixedHeaders = { "ContextID",   "ProcessID", "FrameID",
                                       "CmdBufferID", "DrawID",    "DrawType",
                                       "DrawLabel",   "ProgramID", "LRZState" };

void Trim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); })
            .base(),
            s.end());
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
}

// Helper functions for safe string to number conversion
bool SafeStoull(const std::string& s, uint64_t& out)
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

bool SafeStoul(const std::string& s, uint32_t& out)
{
    char*       end = nullptr;
    const char* start = s.c_str();
    errno = 0;
    unsigned long val = std::strtoul(start, &end, 10);
    if (errno == ERANGE || *end != '\0' || start == end)
    {
        return false;
    }
    out = static_cast<uint32_t>(val);
    return true;
}

bool SafeStoll(const std::string& s, int64_t& out)
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

bool SafeStof(const std::string& s, float& out)
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

bool GetTrimmedLine(std::ifstream& file, std::string& line)
{
    if (!std::getline(file, line))
        return false;
    Trim(line);
    return true;
}

bool GetTrimmedField(std::stringstream& ss, std::string& field, char delimiter = ',')
{
    if (!std::getline(ss, field, delimiter))
        return false;
    Trim(field);
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
    if (!GetTrimmedLine(file, line) || line.empty())
    {
        return nullptr;
    }

    std::stringstream header_ss(line);
    std::string       header_field;
    int               column_index = 0;
    while (GetTrimmedField(header_ss, header_field, ','))
    {
        if (column_index < kFixedHeaders.size())
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
    if (column_index < kFixedHeaders.size())
        return nullptr;

    while (GetTrimmedLine(file, line))
    {
        std::stringstream        ss(line);
        std::string              field;
        std::vector<std::string> fields;
        while (GetTrimmedField(ss, field, ','))
        {
            fields.push_back(field);
        }

        if (fields.size() != kFixedHeaders.size() + metric_names.size())
        {
            continue;  // Skip malformed lines
        }

        PerfMetricsRecord record{};
        uint32_t          draw_type = 0, lrz_state = 0;
        if (!SafeStoull(fields[0], record.m_context_id) ||
            !SafeStoull(fields[1], record.m_process_id) ||
            !SafeStoull(fields[2], record.m_frame_id) ||
            !SafeStoull(fields[3], record.m_cmd_buffer_id) ||
            !SafeStoul(fields[4], record.m_draw_id) || !SafeStoul(fields[5], draw_type) ||
            !SafeStoul(fields[6], record.m_draw_label) ||
            !SafeStoull(fields[7], record.m_program_id) || !SafeStoul(fields[8], lrz_state))
        {
            continue;  // Skip lines with parsing errors
        }
        record.m_draw_type = static_cast<uint8_t>(draw_type);
        record.m_lrz_state = static_cast<uint8_t>(lrz_state);

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
                {
                    int64_t val = 0;
                    if (SafeStoll(value_str, val))
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
                    if (SafeStof(value_str, val))
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
