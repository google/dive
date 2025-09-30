/*
 Copyright 2025 Google LLC

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#include <filesystem>
#include <string_view>
#include <array>
#include <vector>
#include <string>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace Dive
{

class TemporaryFiles
{
public:
    void Initialize();
    // Remove all temporary file, then initialize.
    void Reset();
    // Remove everything, include temporary directory.
    void Cleanup();

    std::filesystem::path GetDir() const { return m_dir; }

    // Register a file as temporary file.
    void Register(std::filesystem::path filename) { m_files.insert(filename); }

private:
    std::filesystem::path m_dir;
    // Delete the directory if we are the owner of this directory.
    bool m_is_exclusive_dir = false;
    // All files that need to be deleted on exit.
    std::unordered_set<std::filesystem::path> m_files;
};

enum class DiveKnownFileTypes : size_t
{
    kMainFile,  // Main capture file.
    kRdFile,
    kGfxrFile,
    kGfxaFile,
    kGpuTime,
    kPerfCounterMetric,
};

constexpr std::array kDiveKnownFileTypes = {
    DiveKnownFileTypes::kRdFile,
    DiveKnownFileTypes::kGfxrFile,
    DiveKnownFileTypes::kGfxaFile,
    DiveKnownFileTypes::kGpuTime,
    DiveKnownFileTypes::kPerfCounterMetric,
};

class DiveVirtualFilesystem
{
public:
    struct File
    {
        std::string             m_name;
        std::optional<uint64_t> m_size;
        std::filesystem::path   m_disk_path;
        uint64_t                m_offset;
        // Expected extract path if needed.
        std::filesystem::path m_extract_path;
    };

    void Append(File f);
    // Return the expected file name for file type.
    std::string GetKnownFilename(DiveKnownFileTypes file_type);

private:
    std::vector<File> m_files;

    std::array<std::string, kDiveKnownFileTypes.size()> m_known_filenames;

    std::unordered_map<std::string, size_t> m_index;
};

class DiveFileContainer
{
public:
    DiveFileContainer();
    ~DiveFileContainer();

    DiveFileContainer(const DiveFileContainer&) = delete;
    DiveFileContainer(DiveFileContainer&&) = delete;
    DiveFileContainer& operator=(const DiveFileContainer&) = delete;
    DiveFileContainer& operator=(DiveFileContainer&&) = delete;

    std::filesystem::path GetMainFilePath() const;

    bool Load(std::filesystem::path source);
    bool Save(std::filesystem::path target);

    // Remove temporaray file.
    void Cleanup();

    const std::filesystem::path& GetGfxrFilePath() const { return m_gfxr_file_path; }
    const std::filesystem::path& GetGfxaFilePath() const { return m_gfxa_file_path; }

    // We probably don't need to have temporary file for rd file.
    const std::filesystem::path& GetRdFilePath() const { return m_rd_file_path; }

    std::optional<std::filesystem::path> GetFilePath(std::string_view filename) const;

    std::filesystem::path GetTempFilePath(std::string_view filename) const;

    void RegisterFile(std::string_view filename, std::filesystem::path fspath);

    DiveVirtualFilesystem&       GetFiles() { return *m_vfs; }
    const DiveVirtualFilesystem& GetFiles() const { return *m_vfs; }

private:
    TemporaryFiles m_temp_files;

    std::unique_ptr<DiveVirtualFilesystem> m_vfs;

    bool m_require_cleanup = false;

    std::filesystem::path m_capture_dir;
    std::filesystem::path m_temp_dir;

    std::filesystem::path m_source_file_path;
    std::filesystem::path m_main_file_path;
    std::filesystem::path m_rd_file_path;
    std::filesystem::path m_gfxa_file_path;
    std::filesystem::path m_gfxr_file_path;

    std::unordered_map<std::string, std::filesystem::path> m_files;

    void Reset();
    bool SaveImpl(std::filesystem::path target);
};

}  // namespace Dive
