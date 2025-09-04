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

#include "dive_core/available_gpu_time.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <optional>

namespace Dive
{

bool AvailableGpuTiming::LoadFromCsv(const std::filesystem::path& file_path)
{
    std::cout << "Loading GPU timing statistics from file..." << std::endl;
    if (m_loaded)
    {
        std::cerr << "Cannot load this object again" << std::endl;
        return false;
    }
    m_loaded = true;

    if (file_path.extension() != ".csv")
    {
        std::cerr << "Unexpected file extension: " << file_path << std::endl;
        return false;
    }

    std::ifstream file(file_path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return false;
    }

    std::string line;
    // Check header line
    if (!std::getline(file, line) || line.empty() ||
        line.find(kExpectedHeader) == std::string::npos)
    {
        std::cerr << "Unexpected header: " << line << std::endl;
        return false;
    }

    uint32_t row = 1;
    while (std::getline(file, line))
    {
        bool res = LoadLine(row, line);
        if (!res)
        {
            std::cerr << "Could not parse row (" << row << ") line: " << line << std::endl;
            return false;
        }
        row++;
    }

    Validate();
    return IsValid();
}

bool AvailableGpuTiming::LoadFromString(const std::string& full_text)
{
    if (m_loaded)
    {
        std::cerr << "Cannot load this object again" << std::endl;
        return false;
    }
    m_loaded = true;

    std::stringstream ss(full_text);
    std::string       line;
    // Check header line
    if (!std::getline(ss, line) || line.empty() || line.find(kExpectedHeader) == std::string::npos)
    {
        std::cerr << "Unexpected header: " << line << std::endl;
        return false;
    }

    uint32_t row = 1;
    while (std::getline(ss, line))
    {
        bool res = LoadLine(row, line);
        if (!res)
        {
            std::cerr << "Could not parse row (" << row << ") line: " << line << std::endl;
            return false;
        }
        row++;
    }

    Validate();
    return IsValid();
}

bool AvailableGpuTiming::LoadLine(uint32_t row, const std::string& line, uint32_t expected_columns)
{
    std::stringstream        ss(line);
    std::string              field;
    std::vector<std::string> fields;
    while (std::getline(ss, field, ','))
    {
        fields.push_back(field);
    }

    if (fields.size() == 0)
    {
        return true;
    }

    if (fields.size() != expected_columns)
    {
        std::cerr << "Unexpected number of columns: " << fields.size() << std::endl;
        return false;
    }

    uint32_t id = static_cast<uint32_t>(std::stoi(fields[1]));

    Stats stats;
    stats.mean_ms = std::stof(fields[2]);
    stats.median_ms = std::stof(fields[3]);

    Entry entry;
    if (fields[0] == "Frame")
    {
        if (row != 1)
        {
            std::cerr << "Unexpected Frame stats on row: " << row << std::endl;
            return false;
        }

        entry.object_type = ObjectType::kFrame;
        entry.per_frame_id = 0;
        m_frame_stats = stats;
        m_total_frames = id;
    }
    else if (fields[0] == "CommandBuffer")
    {
        entry.object_type = ObjectType::kCommandBuffer;
        if (m_command_buffer_stats.size() != id)
        {
            std::cerr << "Unexpected CommandBuffer id: " << id << std::endl;
            std::cerr << "Current m_command_buffer_stats.size(): " << m_command_buffer_stats.size()
                      << std::endl;
            return false;
        }
        entry.per_frame_id = id;
        m_command_buffer_stats.push_back(stats);
    }
    else if (fields[0] == "RenderPass")
    {
        entry.object_type = ObjectType::kRenderPass;
        if (m_render_pass_stats.size() != id)
        {
            std::cerr << "Unexpected RenderPass id: " << id << std::endl;
            std::cerr << "Current m_render_pass_stats.size(): " << m_render_pass_stats.size()
                      << std::endl;
            return false;
        }
        entry.per_frame_id = id;
        m_render_pass_stats.push_back(stats);
    }
    else
    {
        std::cerr << "Unrecognizable field: " << fields[0] << std::endl;
        return false;
    }
    m_ordered_entries.push_back(entry);

    if (row != m_ordered_entries.size())
    {
        std::cerr << "Inconsistent id of row processed (" << row << ") with entries recorded ("
                  << m_ordered_entries.size() << ")" << std::endl;
        return false;
    }

    return true;
}

void AvailableGpuTiming::Validate()
{
    if ((m_render_pass_stats.size() + m_command_buffer_stats.size() + 1) !=
        m_ordered_entries.size())
    {
        std::cerr << "Inconsistent number of entries: " << m_ordered_entries.size() << std::endl;
        std::cerr << "Frame row: 1" << std::endl;
        std::cerr << "CommandBuffer rows: " << m_command_buffer_stats.size() << std::endl;
        std::cerr << "RenderPass rows: " << m_render_pass_stats.size() << std::endl;
        return;
    }
    m_valid = true;
    return;
}

std::optional<AvailableGpuTiming::Stats> AvailableGpuTiming::GetStats(ObjectType object_type,
                                                                      uint32_t   object_id) const
{
    if (!m_valid)
    {
        std::cerr << "Invalid AvailableGpuTiming object" << std::endl;
        return std::nullopt;
    }

    if (object_type == ObjectType::kFrame)
    {
        return m_frame_stats;
    }
    else if (object_type == ObjectType::kCommandBuffer)
    {
        if (object_id >= m_command_buffer_stats.size())
        {
            std::cerr << "Out of bounds (CommandBuffer) object_id: " << object_id << std::endl;
            return std::nullopt;
        }
        return m_command_buffer_stats[object_id];
    }
    else if (object_type == ObjectType::kRenderPass)
    {
        if (object_id >= m_render_pass_stats.size())
        {
            std::cerr << "Out of bounds (RenderPass) object_id: " << object_id << std::endl;
            return std::nullopt;
        }
        return m_render_pass_stats[object_id];
    }

    std::cerr << "Unexpected ObjectType: " << static_cast<int>(object_type) << std::endl;
    return std::nullopt;
}

std::optional<AvailableGpuTiming::Stats> AvailableGpuTiming::GetStats(uint32_t object_id) const
{
    if (!m_valid)
    {
        std::cerr << "Invalid AvailableGpuTiming object" << std::endl;
        return std::nullopt;
    }

    if ((object_id < 1) || object_id > m_ordered_entries.size())
    {
        std::cerr << "Out of bounds (row) object_id: " << object_id << std::endl;
        return std::nullopt;
    }

    Entry entry = m_ordered_entries[object_id - 1];
    return AvailableGpuTiming::GetStats(entry.object_type, entry.per_frame_id);
}

}  // namespace Dive
