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

#include "dive_file_container.h"
#include "dive_file_container_private.h"

#include <array>
#include <map>
#include <cstddef>
#include <cstdlib>
#include <ios>
#include <istream>
#include <memory>
#include <ostream>
#include <random>
#include <cstring>
#include <filesystem>
#include <optional>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <utility>

namespace Dive
{
namespace
{

constexpr char             kDiveTarMagic[6] = "ustar";
constexpr std::string_view kDiveArchiveMagic = "DIVE_ARCHIVE_V0";
constexpr std::string_view kDiveArchiveMagicFull = "DIVE_ARCHIVE_V0.bin";

constexpr std::array<char, 512> kEmptySector = {};

constexpr bool EndsWith(std::string_view str, std::string_view suffix)
{
    return str.size() >= suffix.size() &&
           str.substr(str.size() - suffix.size(), suffix.size()) == suffix;
}

bool IsDiveContainerFormat(std::filesystem::path filename, bool check_dive_magic = true)
{
    std::fstream capture_file(filename, std::ios::in | std::ios::binary);
    if (!capture_file)
    {
        return false;
    }
    DiveFileContainerHeader header = {};
    capture_file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (std::memcmp(header.magic, kDiveTarMagic, 6) != 0)
    {
        return false;
    }
    if (check_dive_magic &&
        std::string_view(header.name, kDiveArchiveMagic.size()) != kDiveArchiveMagic)
    {
        return false;
    }
    return true;
}

std::filesystem::path GetGfxaFilePathFromGfxrFilePath(std::filesystem::path gfxr_file_path)
{

    constexpr std::string_view kTrimStr = "_trim_trigger";
    constexpr std::string_view kAssetStr = "_asset_file";

    auto   filename = gfxr_file_path.filename().replace_extension(".gfxa").string();
    size_t pos = filename.find(kTrimStr);
    if (pos != std::string::npos)
    {
        filename.replace(pos, kTrimStr.size(), kAssetStr);
    }
    else
    {
        return std::filesystem::path();
    }
    return gfxr_file_path.replace_filename(filename);
}

std::filesystem::path GenTempDirName()
{
    static std::mt19937 rng(static_cast<std::mt19937::result_type>(std::time(nullptr)));

    constexpr std::string_view
    kBase57Chars = "23456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

    char name[7] = {};
    for (size_t i = 0; i < std::size(name) - 1; ++i)
    {
        name[i] = kBase57Chars[rng() % kBase57Chars.size()];
    }
    auto temp_root = std::filesystem::temp_directory_path();
    return temp_root / "dive-tmp" / (std::string("dive-") + name);
}

template<size_t N> std::string_view StringViewFromFixedString(const char (&s)[N])
{
    auto   pos = std::string_view(&s[0], N).find('\0');
    size_t size = (pos != std::string_view::npos ? pos : N);
    return std::string_view(&s[0], size);
}

template<size_t N> std::optional<uint64_t> ReadOctal(const char (&s)[N])
{
    uint64_t value = 0;
    for (size_t i = 0; i < N && s[i] != '\0' && s[i] != ' '; ++i)
    {
        if (!('0' <= s[i] && s[i] <= '7'))
        {
            return std::nullopt;
        }
        value *= 8;
        value += (s[i] - '0');
    }
    return value;
}

template<size_t N> bool FormatTarString(char (&s)[N], std::string_view value)
{
    if (N <= value.size())
    {
        return false;
    }
    for (size_t i = 0; i < N; ++i)
    {
        s[i] = (i < value.size() ? value[i] : '\0');
    }
    return true;
}

template<size_t N> bool FormatOctal(char (&s)[N], uint64_t value)
{
    s[N - 1] = '\0';
    for (size_t i = 1; i < N; ++i)
    {
        s[N - 1 - i] = '0' + (value % 8);
        value = value / 8;
    }
    return value == 0;
}

constexpr std::size_t GetTarFilePaddedSize(size_t size)
{
    constexpr size_t kLowMask = 512 - 1;
    constexpr size_t kHighMask = ~static_cast<size_t>(kLowMask);
    return (size + kLowMask) & kHighMask;
}

std::array<bool, 128> GetAllowedFilenameLettersImpl()
{
    std::array<bool, 128> result = {};
    for (int c = '0'; c <= '9'; ++c)
    {
        result[c] = true;
    }
    for (int c = 'a'; c <= 'z'; ++c)
    {
        result[c] = true;
    }
    for (int c = 'A'; c <= 'Z'; ++c)
    {
        result[c] = true;
    }
    result['.'] = true;
    result[' '] = true;
    result['-'] = true;
    result['('] = true;
    result[')'] = true;
    result['_'] = true;
    return result;
}

const std::array<bool, 128>& GetAllowedFilenameLetters()
{
    static auto entry = GetAllowedFilenameLettersImpl();
    return entry;
}

std::string SanitizedFilename(std::string_view name)
{
    auto& allowedLetters = GetAllowedFilenameLetters();
    int   allowLetterCount = static_cast<int>(allowedLetters.size());
    if (auto pos = name.rfind('/'); pos != std::string_view::npos)
    {
        name = name.substr(pos + 1);
    }
    std::string result;
    result.reserve(name.size());
    for (auto c : name)
    {
        if (0 < c && static_cast<int>(c) < allowLetterCount && allowedLetters[c])
        {
            result.push_back(c);
        }
        else
        {
            result.push_back('_');
        }
    }
    return result;
}

bool IsKnownFilename(std::string_view name)
{
    static constexpr std::string_view kKnownSuffix[] = {
        ".gfxr", ".gfxa", ".rd", "_gpu_time.csv", "_profiling_metrics.csv"
    };
    for (auto suffix : kKnownSuffix)
    {
        if (EndsWith(name, suffix))
        {
            return true;
        }
    }
    return false;
}

bool IsKnownAdditionalFilename(std::string_view name)
{
    static constexpr std::string_view kKnownSuffix[] = { "_gpu_time.csv",
                                                         "_profiling_metrics.csv" };
    for (auto suffix : kKnownSuffix)
    {
        if (EndsWith(name, suffix))
        {
            return true;
        }
    }
    return false;
}

std::string GetExtractedName(std::string_view name)
{
    return IsKnownFilename(name) ? SanitizedFilename(name) : std::string();
}

size_t ComputeByteChecksum(const void* data, size_t size)
{
    auto   u8ptr = static_cast<const uint8_t*>(data);
    size_t result = 0;
    for (size_t i = 0; i < size; ++i)
    {
        result += u8ptr[i];
    }
    return result;
}

size_t ComputeTarChecksum(const DiveFileContainerHeader& h)
{
    size_t checksum = 0;
    checksum += ComputeByteChecksum(&h, sizeof(h));
    checksum -= ComputeByteChecksum(&h.checksum, sizeof(h.checksum));
    checksum += 32 * sizeof(h.checksum);
    return checksum;
}

class ArchiveWriter
{
public:
    ArchiveWriter(std::filesystem::path path) :
        m_stream(path, std::ios::out | std::ios::binary)
    {
    }
    void WritePreamble()
    {
        if (!m_stream)
        {
            return;
        }
        DiveFileContainerHeader h;
        FormatTarString(h.name, kDiveArchiveMagicFull);
        FormatOctal(h.size, kEmptySector.size());
        FormatOctal(h.checksum, ComputeTarChecksum(h));
        m_stream.write(reinterpret_cast<const char*>(&h), sizeof(h));
        m_stream.write(kEmptySector.data(), kEmptySector.size());
    }
    void WriteFile(std::string_view name, std::filesystem::path disk_path)
    {
        if (!m_stream)
        {
            return;
        }
        if (!std::filesystem::exists(disk_path))
        {
            return;
        }
        auto size = std::filesystem::file_size(disk_path);

        DiveFileContainerHeader h;
        FormatTarString(h.name, name);
        FormatOctal(h.size, size);
        FormatOctal(h.checksum, ComputeTarChecksum(h));
        std::fstream infile(disk_path, std::ios::in | std::ios::binary);
        if (!infile)
        {
            return;
        }
        m_stream.write(reinterpret_cast<const char*>(&h), sizeof(h));
        WriteData(infile, size);
    }

    void WritePostamble()
    {
        if (!m_stream)
        {
            return;
        }
        m_stream.write(kEmptySector.data(), kEmptySector.size());
    }

    bool Ok() const { return !!m_stream; }

private:
    std::fstream m_stream;

    bool WriteData(std::istream& ist, size_t n)
    {
        char   buf[512] = {};
        auto   begin_pos = m_stream.tellp();
        size_t remain = n;
        while (ist && m_stream && remain > 0)
        {
            size_t size = std::min<size_t>(512, remain);
            ist.read(buf, size);
            m_stream.write(buf, size);
            if (size < 512)
            {
                // Tar does 512 block.
                m_stream.write(kEmptySector.data(), 512 - size);
            }
            remain -= size;
        }
        auto count = m_stream.tellp() - begin_pos;
        return count == static_cast<std::streamoff>(GetTarFilePaddedSize(n));
    }
};

class ArchiveReader
{
public:
    static constexpr size_t kSectorSize = 512;
    ArchiveReader(std::filesystem::path path) :
        m_stream(path, std::ios::in | std::ios::binary)
    {
    }

    bool Next()
    {
        if (!m_stream)
        {
            return false;
        }
        if (m_remain > 0)
        {
            m_stream.seekg(m_remain, std::ios::cur);
            m_remain = 0;
        }
        m_stream.read(reinterpret_cast<char*>(&m_header), sizeof(m_header));
        m_offset += sizeof(m_header);
        if (memcmp(m_header.magic, "ustar", std::size(m_header.magic)) != 0)
        {
            return false;
        }
        auto size = ReadOctal(m_header.size);
        if (!size)
        {
            return false;
        }
        m_remain = GetTarFilePaddedSize(*size);
        return !!m_stream;
    }

    bool ExtractFileTo(std::filesystem::path path)
    {
        std::fstream outfile(path, std::ios::out | std::ios::binary);
        if (!outfile)
        {
            return false;
        }
        auto   begin_pos = m_stream.tellg();
        size_t size = *ReadOctal(m_header.size);
        size_t remain = size;
        if (remain > m_remain)
        {
            return false;
        }
        m_remain = 0;
        while (m_stream && outfile && remain > 0)
        {
            size_t size = std::min<size_t>(512, remain);
            m_stream.read(m_buf, 512);  // Tar does 512 block.
            outfile.write(m_buf, size);
            remain -= size;
        }
        auto count = m_stream.tellg() - begin_pos;
        return count == static_cast<std::streamoff>(GetTarFilePaddedSize(size));
    }

    uint64_t GetOffset() const { return m_offset; }

    std::string_view Name() const { return StringViewFromFixedString(m_header.name); }
    uint64_t         Size() const { return *ReadOctal(m_header.size); }
    bool IsRegular() const { return m_header.filetype == '0' || m_header.filetype == 0; }

private:
    std::fstream m_stream;
    uint64_t     m_offset = 0;
    uint64_t     m_remain = 0;

    DiveFileContainerHeader m_header;
    char                    m_buf[kSectorSize];
};

// Extract to out_path, return the main capture file.
std::optional<std::filesystem::path> ExtractDiveArchiveTo(TemporaryFiles&        temp_files,
                                                          std::filesystem::path  archive_path,
                                                          DiveVirtualFilesystem& vfs)
{
    ArchiveReader reader(archive_path);
    std::string   main_file;
    while (reader.Next())
    {
        auto name = reader.Name();
        auto size = reader.Size();

        if (!reader.IsRegular())
        {
            continue;
        }
        auto file_info = DiveVirtualFilesystem::File{
            .m_name = GetExtractedName(name),
            .m_size = size,
            .m_disk_path = archive_path,
            .m_offset = reader.GetOffset(),
            .m_extract_path = temp_files.GetDir() / GetExtractedName(name),
        };
        vfs.Append(file_info);

        if (main_file.empty())
        {
            if (EndsWith(file_info.m_name, ".gfxr"))
            {
                main_file = file_info.m_extract_path;
            }
            else if (EndsWith(file_info.m_name, ".rd"))
            {
                main_file = file_info.m_extract_path;
            }
        }

        if (!file_info.m_extract_path.empty() && IsKnownFilename(file_info.m_name))
        {
            temp_files.Register(file_info.m_extract_path);
            reader.ExtractFileTo(file_info.m_extract_path);
        }
    }
    if (!main_file.empty() && std::filesystem::exists(main_file))
    {
        return main_file;
    }
    return std::nullopt;
}

}  // namespace

void TemporaryFiles::Initialize()
{
    if (m_dir.empty())
    {
        constexpr auto kMaxRetry = 3;
        for (int retry = 0; retry < kMaxRetry; ++retry)
        {
            auto name = GenTempDirName();
            if (std::filesystem::create_directories(name))
            {
                m_is_exclusive_dir = true;
                m_dir = name;
                break;
            }
        }
    }

    if (m_dir.empty())
    {
        // We can't create a random directory for some reason.
        // Don't cleanup since we may remove data from other programs.
        m_is_exclusive_dir = false;
        m_dir = std::filesystem::temp_directory_path();
    }
}

void TemporaryFiles::Reset()
{
    std::unordered_set<std::filesystem::path> files;
    std::swap(files, m_files);
    for (auto filename : files)
    {
        std::filesystem::remove(filename);
    }
    Initialize();
}

void TemporaryFiles::Cleanup()
{
    std::unordered_set<std::filesystem::path> files;
    std::swap(files, m_files);
    for (auto filename : files)
    {
        std::filesystem::remove(filename);
    }
    if (m_is_exclusive_dir)
    {
        std::filesystem::remove_all(m_dir);
        m_is_exclusive_dir = false;
        m_dir.clear();
    }
}

void DiveVirtualFilesystem::Append(File file)
{
    constexpr auto kMainFileIndex = static_cast<size_t>(DiveKnownFileTypes::kMainFile);
    if (m_known_filenames[kMainFileIndex].empty())
    {
        const std::string& name = file.m_name;
        if (EndsWith(name, ".gfxr"))
        {
            m_known_filenames[kMainFileIndex] = name;
        }
        else if (EndsWith(name, ".rd"))
        {
            m_known_filenames[kMainFileIndex] = name;
        }
    }
    m_index[file.m_name] = m_files.size();
    m_files.push_back(file);
}

std::string DiveVirtualFilesystem::GetKnownFilename(DiveKnownFileTypes file_type)
{
    size_t file_type_index = static_cast<size_t>(file_type);
    if (file_type_index < m_known_filenames.size())
    {
        return m_known_filenames[file_type_index];
    }
    return "";
}

DiveFileContainer::DiveFileContainer()
{
    Reset();
}

DiveFileContainer::~DiveFileContainer() {}

void DiveFileContainer::Reset()
{
    if (m_require_cleanup && !m_temp_dir.empty())
    {
        std::filesystem::remove_all(m_temp_dir);
    }
    m_vfs = std::make_unique<DiveVirtualFilesystem>();
    m_capture_dir = std::filesystem::path();
    m_temp_dir = std::filesystem::path();
    m_source_file_path = "";
    m_main_file_path = "";
    m_rd_file_path = "";
    m_gfxa_file_path = "";
    m_gfxr_file_path = "";
    m_require_cleanup = false;
    m_files.clear();
}

std::filesystem::path DiveFileContainer::GetMainFilePath() const
{
    if (!m_gfxr_file_path.empty())
    {
        return m_gfxr_file_path;
    }
    if (!m_rd_file_path.empty())
    {
        return m_rd_file_path;
    }
    return m_main_file_path;
}

bool DiveFileContainer::Load(std::filesystem::path source)
{
    m_temp_files.Reset();
    Reset();
    m_source_file_path = source;
    if (!std::filesystem::exists(source))
    {
        return false;
    }
    auto filename = source.filename().string();
    if (source.extension() == ".tar" && IsDiveContainerFormat(source, false))
    {
        auto main_file_path = ExtractDiveArchiveTo(m_temp_files, source, *m_vfs);
        if (main_file_path)
        {
            m_main_file_path = *main_file_path;
        }
        else
        {
            return false;
        }
    }
    else if (source.extension() == ".dive" && IsDiveContainerFormat(source))
    {
        auto main_file_path = ExtractDiveArchiveTo(m_temp_files, source, *m_vfs);
        if (main_file_path)
        {
            m_main_file_path = *main_file_path;
        }
        else
        {
            return false;
        }
    }
    else
    {
        m_main_file_path = source;
    }

    if (m_main_file_path.extension() == ".gfxr")
    {
        m_gfxr_file_path = m_main_file_path;
        if (auto related = m_main_file_path.replace_extension(".rd");
            std::filesystem::exists(related))
        {
            m_rd_file_path = related;
        }
    }
    else if (m_main_file_path.extension() == ".rd")
    {
        m_rd_file_path = m_main_file_path;
        if (auto related = m_main_file_path.replace_extension(".gfxr");
            std::filesystem::exists(related))
        {
            m_gfxr_file_path = related;
        }
    }
    else
    {
        return false;
    }

    m_capture_dir = m_main_file_path.parent_path();

    if (!m_gfxr_file_path.empty())
    {
        auto file_path = GetGfxaFilePathFromGfxrFilePath(m_gfxr_file_path);
        if (!file_path.empty() && std::filesystem::exists(file_path))
        {
            m_gfxa_file_path = file_path;
        }
    }
    if (!m_rd_file_path.empty())
    {
        m_files[m_rd_file_path.filename().string()] = m_rd_file_path;
    }
    if (!m_gfxr_file_path.empty())
    {
        m_files[m_gfxr_file_path.filename().string()] = m_gfxr_file_path;
    }
    if (!m_gfxa_file_path.empty())
    {
        m_files[m_gfxa_file_path.filename().string()] = m_gfxa_file_path;
    }
    return true;
}

bool DiveFileContainer::Save(std::filesystem::path target)
{
    if (target != m_source_file_path)
    {
        return SaveImpl(target);
    }
    m_temp_files.Initialize();

    auto temp_save_target = m_temp_files.GetDir() / "dive-save.tar";
    if (!SaveImpl(temp_save_target))
    {
        if (std::filesystem::exists(temp_save_target))
        {
            std::filesystem::remove(temp_save_target);
        }
        return false;
    }
    std::filesystem::rename(temp_save_target, target);
    return false;
}

bool DiveFileContainer::SaveImpl(std::filesystem::path target)
{
    // Sort the entries.
    std::map<std::string, std::filesystem::path> files;
    for (auto entry : m_files)
    {
        files[entry.first] = entry.second;
    }

    for (auto entry : std::filesystem::directory_iterator(m_capture_dir))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }
        auto filename = entry.path().filename().string();
        if (!IsKnownAdditionalFilename(filename))
        {
            continue;
        }
        std::cerr << entry.path() << std::endl;
        files[SanitizedFilename(filename)] = entry.path();
    }
    ArchiveWriter writer(target);
    if (!writer.Ok())
    {
        return false;
    }
    writer.WritePreamble();
    for (auto& entry : files)
    {
        writer.WriteFile(entry.first, entry.second);
    }
    writer.WritePostamble();
    return true;
}

void DiveFileContainer::Cleanup()
{
    Reset();
    m_temp_files.Cleanup();
}

std::optional<std::filesystem::path> DiveFileContainer::GetFilePath(std::string_view filename) const
{

    if (auto filepath = m_temp_dir / filename; std::filesystem::exists(filepath))
    {
        return filepath;
    }
    if (auto filepath = m_capture_dir / filename; std::filesystem::exists(filepath))
    {
        return filepath;
    }
    return std::nullopt;
}

std::filesystem::path DiveFileContainer::GetTempFilePath(std::string_view filename) const
{
    return m_temp_dir / filename;
}

void DiveFileContainer::RegisterFile(std::string_view filename, std::filesystem::path fspath)
{
    m_files[std::string(filename)] = fspath;
}

}  // namespace Dive
