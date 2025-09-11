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

AvailableGpuTiming::AvailableGpuTiming()
{
    m_stats.resize(static_cast<uint8_t>(ObjectType::nObjectTypes));
}

std::string AvailableGpuTiming::GetObjectTypeString(ObjectType object_type) const
{
    std::stringstream ss;
    switch (object_type)
    {
    case ObjectType::kFrame:
    {
        ss << "Frame";
        break;
    }
    case ObjectType::kCommandBuffer:
    {
        ss << "CommandBuffer";
        break;
    }
    case ObjectType::kRenderPass:
    {
        ss << "RenderPass";
        break;
    }
    default:
    {
        std::cerr << "GetObjectTypeString() failed, object_type OOB: "
                  << static_cast<int>(object_type) << std::endl;
        return "";
    }
    }
    return ss.str();
}

AvailableGpuTiming::ObjectType AvailableGpuTiming::GetObjectType(
const std::string& object_type_str) const
{
    if (object_type_str == "Frame")
    {
        return ObjectType::kFrame;
    }
    else if (object_type_str == "CommandBuffer")
    {
        return ObjectType::kCommandBuffer;
    }
    else if (object_type_str == "RenderPass")
    {
        return ObjectType::kRenderPass;
    }

    // Unrecognized object_type_str
    return ObjectType::nObjectTypes;
}

std::string AvailableGpuTiming::GetColumnTypeString(ColumnType column_type) const
{
    std::stringstream ss;
    switch (column_type)
    {
    case ColumnType::kObjectType:
    {
        ss << "Type";
        break;
    }
    case ColumnType::kId:
    {
        ss << "Id";
        break;
    }
    case ColumnType::kMeanMs:
    {
        ss << "Mean [ms]";
        break;
    }
    case ColumnType::kMedianMs:
    {
        ss << "Median [ms]";
        break;
    }
    default:
    {
        std::cerr << "GetColumnTypeString() failed, object_type OOB: "
                  << static_cast<int>(column_type) << std::endl;
        return "";
    }
    }
    return ss.str();
}

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

    if (!LoadFromStream(file))
    {
        return false;
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
    if (!LoadFromStream(ss))
    {
        return false;
    }

    Validate();
    return IsValid();
}

bool AvailableGpuTiming::LoadFromStream(std::istream& stream)
{
    std::string line;
    uint32_t    row = 0;
    while (std::getline(stream, line))
    {
        if (line.empty())
        {
            continue;
        }
        if (!LoadLine(row, line))
        {
            std::cerr << "Could not parse row (" << row << ") line: " << line << std::endl;
            return false;
        }
        row++;
    }
    return true;
}

bool AvailableGpuTiming::LoadLine(uint32_t row, const std::string& line)
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

    if (fields.size() != GetColumns())
    {
        std::cerr << "Unexpected number of columns: " << fields.size() << std::endl;
        return false;
    }

    // Check header without loading
    if (row == 0)
    {
        for (uint8_t i = 0; i < fields.size(); i++)
        {
            if (fields[i] != GetColumnTypeString(static_cast<ColumnType>(i)))
            {
                std::cerr << "Unexpected header element: " << fields[i] << std::endl;
                return false;
            }
        }
        return true;
    }

    // TODO(b/443122531): Improve integer and float parsing here, this has edge cases that aren't
    // covered
    uint32_t id;
    Stats    stats;
    try
    {
        id = static_cast<uint32_t>(std::stoi(fields[1]));
        stats.mean_ms = std::stof(fields[2]);
        stats.median_ms = std::stof(fields[3]);
    }
    catch (const std::invalid_argument& e)
    {
        std::cerr << "Caught invalid_argument exception: " << e.what() << std::endl;
        return false;
    }
    catch (const std::out_of_range& e)
    {
        std::cerr << "Caught out_of_range exception: " << e.what() << std::endl;
        return false;
    }

    if (fields[1].find('.') != std::string::npos)
    {
        std::cerr << "Expecting an integer id, not float: " << fields[1] << std::endl;
        return false;
    }
    if ((fields[2].find('.') == std::string::npos))
    {
        std::cerr << "Expecting a float mean, not integer: " << fields[2] << std::endl;
        return false;
    }
    if ((fields[3].find('.') == std::string::npos))
    {
        std::cerr << "Expecting a float median, not integer: " << fields[3] << std::endl;
        return false;
    }

    Entry      entry;
    ObjectType object_type = GetObjectType(fields[0]);
    if (object_type == ObjectType::nObjectTypes)
    {
        std::cerr << "Expecting a float median, not integer: " << fields[3] << std::endl;
        return false;
    }

    entry.object_type = object_type;
    entry.per_frame_id = id;
    if (object_type == ObjectType::kFrame)
    {
        // Within the CSV file there is only one row for Frame, and this row's id field is used to
        // store the total number of frames used to calculate the statistics
        m_total_frames = id;
        entry.per_frame_id = 0;
    }

    uint8_t index = static_cast<uint8_t>(object_type);

    if (m_stats[index].size() != entry.per_frame_id)
    {
        std::cerr << "Unexpected id (" << id << ") on row (" << row
                  << ") for object_type: " << GetObjectTypeString(object_type) << std::endl;
        return false;
    }

    m_stats[index].push_back(stats);
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
    size_t total_stats = 0;
    for (size_t i = 0; i < m_stats.size(); i++)
    {
        total_stats += m_stats[i].size();
    }

    if (total_stats != m_ordered_entries.size())
    {
        std::cerr << "Inconsistent number of entries: " << m_ordered_entries.size() << std::endl;

        for (size_t i = 0; i < m_stats.size(); i++)
        {
            std::cerr << GetObjectTypeString(static_cast<ObjectType>(i))
                      << " number of rows: " << m_stats[i].size() << std::endl;
        }
        return;
    }
    m_valid = true;
    return;
}

std::optional<AvailableGpuTiming::Stats> AvailableGpuTiming::GetStatsByType(
ObjectType object_type,
uint32_t   object_id) const
{
    if (!m_valid)
    {
        std::cerr << "Invalid AvailableGpuTiming object" << std::endl;
        return std::nullopt;
    }

    uint8_t index = static_cast<uint8_t>(object_type);
    if ((index < 0) || (index >= m_stats.size()))
    {
        std::cerr << "m_stats does not contain stats for object_type: " << index << std::endl;
        return std::nullopt;
    }

    if (object_id >= m_stats[index].size())
    {
        std::cerr << "m_stats does not contain stats for object_type(" << index << ") object_id ("
                  << object_id << ")" << std::endl;
        return std::nullopt;
    }

    return m_stats[index][object_id];
}

std::optional<AvailableGpuTiming::Stats> AvailableGpuTiming::GetStatsByRow(uint32_t row_id) const
{
    if (!m_valid)
    {
        std::cerr << "Invalid AvailableGpuTiming object" << std::endl;
        return std::nullopt;
    }

    if ((row_id < 1) || row_id > m_ordered_entries.size())
    {
        std::cerr << "Out of bounds (row) row_id: " << row_id << std::endl;
        return std::nullopt;
    }

    Entry entry = m_ordered_entries[row_id - 1];
    return AvailableGpuTiming::GetStatsByType(entry.object_type, entry.per_frame_id);
}

std::string AvailableGpuTiming::GetColumnHeader(int col) const
{
    if ((col < 0) || (col >= static_cast<int>(ColumnType::nColumnTypes)))
    {
        std::cerr << "Invalid col for GetColumnHeader: " << col << std::endl;
        return "";
    }
    return GetColumnTypeString(static_cast<ColumnType>(col));
}

std::string AvailableGpuTiming::GetCell(int row, int col) const
{
    if (!m_valid)
    {
        std::cerr << "Invalid AvailableGpuTiming object" << std::endl;
        return "";
    }

    if ((row < 0) || (row >= GetRows()))
    {
        std::cerr << "GetCell() OOB error, row: " << row << " expecting: [0-" << (GetRows() - 1)
                  << "]" << std::endl;
        return "";
    }

    if ((col < 0) || (col >= GetColumns()))
    {
        std::cerr << "GetCell() OOB error, col: " << col << " expecting: [0-" << (GetColumns() - 1)
                  << "]" << std::endl;
        return "";
    }

    std::stringstream ss;

    if (col < 2)
    {
        // First two columns are type and id
        const Entry& entry = m_ordered_entries[row];

        if (col == 0)
        {
            ss << GetObjectTypeString(entry.object_type);
            return ss.str();
        }

        ss << entry.per_frame_id;
        return ss.str();
    }

    // Offset because QTableView first row (frame info) is index 0
    auto ret = GetStatsByRow(row + 1);
    if (!ret.has_value())
    {
        std::cerr << "GetStatsByRow() failed, row: " << row << std::endl;
        return "";
    }
    Stats& stats = *ret;

    switch (col)
    {
    case 2:
    {
        ss << std::setprecision(kDisplayFloatPrecision) << std::fixed << stats.mean_ms;
        return ss.str();
    }
    case 3:
    {
        ss << std::setprecision(kDisplayFloatPrecision) << std::fixed << stats.median_ms;
        return ss.str();
    }
    default:
    {
        std::cerr << "GetCell() OOB error, col: " << col << " expected: [2-" << (GetColumns() - 1)
                  << "]" << std::endl;
        return "";
    }
    }
}

int AvailableGpuTiming::GetRows() const
{
    if (!m_valid)
    {
        std::cerr << "Invalid AvailableGpuTiming object" << std::endl;
        return -1;
    }
    return static_cast<int>(m_ordered_entries.size());
}

}  // namespace Dive
