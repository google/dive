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

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace Dive
{

/*
AvailableGpuTiming parses CSV format file (gpu_time.csv) produced by looping GFXR replay into
available timing info statistics.

TODO: Retrieve GPU timing info from device to host through network/ rather than using a CSV file.
*/

namespace
{
constexpr char kExpectedHeader[] = "Type,Id,Mean [ms],Median [ms]";
constexpr int  kExpectedColumns = 4;
}  // namespace

class AvailableGpuTiming
{
public:
    enum class ObjectType : uint8_t
    {
        kFrame = 0,
        kCommandBuffer = 1,
        kRenderPass = 2,
        nObjectTypes = 3,
    };

    struct Entry
    {
        ObjectType object_type;
        uint32_t   per_frame_id;
    };

    struct Stats
    {
        float mean_ms;
        float median_ms;
    };

    AvailableGpuTiming() = default;
    ~AvailableGpuTiming() = default;

    // Load statistics from a CSV file and flag as loaded afterwards
    bool LoadFromCsv(const std::filesystem::path& file_path);

    // Load statistics from a string and flag as loaded afterwards
    // For unit testing
    bool LoadFromString(const std::string& full_text);

    // Get the statistic info with the ObjectType and the object_id (nth object of type ObjectType)
    // If the object_type is kFrame, the object_id value will be disregarded
    std::optional<Stats> GetStatsByType(ObjectType object_type, uint32_t object_id) const;

    // Get the statistic info with the row_id (representing the row in file order, header is row 0)
    std::optional<Stats> GetStatsByRow(uint32_t row_id) const;

    // Validate entries to stats counts
    bool IsValid() const { return m_valid; };

private:
    // Load statistics from stream
    bool LoadFromStream(std::istream& stream);

    // Load statistics from non-header CSV row
    bool LoadLine(uint32_t           row,
                  const std::string& line,
                  uint32_t           expected_columns = kExpectedColumns);

    // Check m_ordered_entries against info stored in *_stats members
    void Validate();

    std::vector<Entry> m_ordered_entries = {};  // Preserved row order from file
    Stats              m_frame_stats = {};
    std::vector<Stats> m_command_buffer_stats = {};
    std::vector<Stats> m_render_pass_stats = {};
    uint32_t m_total_frames = 0;  // The number of frames the statistics were collected from
    bool     m_loaded = false;    // If true, prevent further loading
    bool     m_valid = false;     // Validated at loading time
};

}  // namespace Dive
