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

#include "dive_core/perf_counter_data.h"

#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "dive_core/available_metrics.h"

namespace Dive
{
namespace
{
constexpr int kPerfCounterFixFieldCount = 9;
}

// Helper functions for safe string to number conversion
bool safe_stoull(const std::string& s, uint64_t& out)
{
    char*       end;
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
    char*       end;
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
    char*       end;
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
    char*       end;
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

bool PerfCounterData::LoadFromCsv(const char* file_path, const AvailableMetrics& available_metrics)
{
    std::ifstream file(file_path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return false;
    }

    m_records.clear();
    m_counter_names.clear();
    m_counter_infos.clear();

    std::string line;
    // Read header line
    if (!std::getline(file, line))
    {
        return false;
    }

    std::stringstream header_ss(line);
    std::string       header_field;
    int               column_index = 0;
    while (std::getline(header_ss, header_field, ','))
    {
        if (column_index >= kPerfCounterFixFieldCount)
        {
            m_counter_names.push_back(header_field);
            m_counter_infos.push_back(available_metrics.GetMetricInfo(header_field));
        }
        column_index++;
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

        if (fields.size() != kPerfCounterFixFieldCount + m_counter_names.size())
        {
            continue;  // Skip malformed lines
        }

        PerfCounterRecord record;
        uint32_t          draw_type, lrz_state;
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

        bool all_counters_parsed = true;
        for (size_t i = 0; i < m_counter_infos.size(); ++i)
        {
            const std::string& value_str = fields[kPerfCounterFixFieldCount + i];
            const MetricInfo*  info = m_counter_infos[i];
            if (info)
            {
                switch (info->m_metric_type)
                {
                case MetricType::kCount:
                {
                    int64_t val;
                    if (safe_stoll(value_str, val))
                    {
                        record.m_counter_values.emplace_back(val);
                    }
                    else
                    {
                        all_counters_parsed = false;
                    }
                    break;
                }
                case MetricType::kPercent:
                {
                    float val;
                    if (safe_stof(value_str, val))
                    {
                        record.m_counter_values.emplace_back(val);
                    }
                    else
                    {
                        all_counters_parsed = false;
                    }
                    break;
                }
                default:
                    break;
                }
            }
            if (!all_counters_parsed)
            {
                break;
            }
        }
        if (all_counters_parsed)
        {
            m_records.push_back(record);
        }
    }

    return true;
}

}  // namespace Dive
