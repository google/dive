/*
 Copyright 2026 Google LLC

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

#include "dive/utils/dive_archive.h"

// libarchive:
#include <archive.h>
#include <archive_entry.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <string_view>

namespace
{

void ArchiveReadCloseAndFree(struct archive* archive_reader)
{
    if (archive_reader == nullptr)
    {
        return;
    }
    archive_read_close(archive_reader);
    archive_read_free(archive_reader);
}

using UniqueArchiveReadPtr = std::unique_ptr<struct archive, decltype(&ArchiveReadCloseAndFree)>;

UniqueArchiveReadPtr ArchiveReadNew()
{
    return UniqueArchiveReadPtr{archive_read_new(), &ArchiveReadCloseAndFree};
}

void ArchiveWriteCloseAndFree(struct archive* archive_writer)
{
    if (archive_writer == nullptr)
    {
        return;
    }
    archive_write_close(archive_writer);
    archive_write_free(archive_writer);
}

using UniqueArchiveWritePtr = std::unique_ptr<struct archive, decltype(&ArchiveWriteCloseAndFree)>;

UniqueArchiveWritePtr ArchiveWriteNew()
{
    return UniqueArchiveWritePtr{archive_write_new(), &ArchiveWriteCloseAndFree};
}

using UniqueArchiveEntryPtr = std::unique_ptr<struct archive_entry, decltype(&archive_entry_free)>;

UniqueArchiveEntryPtr ArchiveEntryNew()
{
    return UniqueArchiveEntryPtr{archive_entry_new(), &archive_entry_free};
}

constexpr auto kKnownDiveArchiveExtension =
    std::to_array<std::string_view>({".zip", ".tar", ".tar.gz", ".tgz"});

// We don't produce captalized extension, so check for lower case only.
constexpr auto kKnownDiveExtension =
    std::to_array<std::string_view>({".rd", ".gfxr", ".gfxa", ".png", ".csv"});

std::string_view GetArchiveEntryPath(struct archive_entry* entry)
{
    const char* pathname = archive_entry_pathname(entry);
    if (pathname == nullptr)
    {
        return "";
    }
    return std::string_view(pathname);
}

int WriteArchiveEntry(std::ostream& ost, struct archive* archive_reader)
{
    // Make sure we don't expand a bad sparse file.
    constexpr la_int64_t kMaxSkip = 0x40000000;

    la_int64_t processed = 0;

    while (ost)
    {
        const void* buff = nullptr;
        size_t size = 0;
        la_int64_t offset = 0;
        auto res = archive_read_data_block(archive_reader, &buff, &size, &offset);
        if (res < ARCHIVE_OK)
        {
            return res;
        }
        if (size == 0)
        {
            return ARCHIVE_OK;
        }
        if (offset - processed > kMaxSkip)
        {
            return ARCHIVE_FAILED;
        }
        if (processed != offset)
        {
            ost.seekp(offset);
        }
        ost.write(static_cast<const char*>(buff), static_cast<std::streamsize>(size));
        processed += static_cast<la_int64_t>(size);
    }

    return ARCHIVE_FAILED;
}

int WriteArchiveEntry(struct archive* archive_writer, std::istream& ist)
{
    constexpr size_t kBufferSize = 0x100000;  // 1MiB

    la_int64_t processed = 0;
    std::string buffer(kBufferSize, '\0');
    while (ist)
    {
        ist.read(buffer.data(), buffer.size());
        if (!ist && !ist.eof())
        {
            // Actual error occured.
            break;
        }
        size_t read_count = ist.gcount();
        archive_write_data(archive_writer, buffer.data(), read_count);
    }

    return (ist.eof() ? ARCHIVE_OK : ARCHIVE_FAILED);
}

}  // namespace

namespace Dive
{

bool DiveArchive::IsSupportedInputFormat(std::filesystem::path filename)
{
    auto ext = filename.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](auto c) { return std::tolower(c); });
    return std::any_of(std::begin(kKnownDiveArchiveExtension), std::end(kKnownDiveArchiveExtension),
                       [&ext](std::string_view known) { return ext == known; });
}

std::unique_ptr<DiveArchive> DiveArchive::Open(std::filesystem::path filename)
{
    return std::unique_ptr<DiveArchive>(new DiveArchive(filename));
}

bool DiveArchive::Create(std::filesystem::path dst, std::vector<std::filesystem::path> components)
{
    if (dst.extension() != ".zip")
    {
        // Sanity check
        return false;
    }
    auto writer = ArchiveWriteNew();

    archive_write_set_format_zip(writer.get());
    archive_write_set_options(writer.get(), "zip:compression=deflate");
    archive_write_open_filename(writer.get(), dst.string().c_str());

    for (const auto& component : components)
    {
        std::ifstream infile(component, std::ios::binary | std::ios::in);
        if (!infile)
        {
            continue;
        }

        auto entry = ArchiveEntryNew();
        archive_entry_set_pathname(entry.get(), component.filename().string().c_str());

        struct stat st = {};
        stat(component.string().c_str(), &st);
        archive_entry_copy_stat(entry.get(), &st);

        archive_write_header(writer.get(), entry.get());
        WriteArchiveEntry(writer.get(), infile);
    }
    return true;
}

DiveArchive::DiveArchive(std::filesystem::path archive_path) : m_path(archive_path) {}

std::vector<std::filesystem::path> DiveArchive::ExtractTo(std::filesystem::path dst)
{
    constexpr size_t kBlockSize = 0x10000;  // 64KiB, we don't really care.
    auto reader = ArchiveReadNew();
    archive_read_support_format_all(reader.get());
    archive_read_support_filter_all(reader.get());

    if (auto res = archive_read_open_filename(reader.get(), m_path.string().c_str(), kBlockSize);
        res != ARCHIVE_OK)
    {
        return {};
    }

    std::vector<std::filesystem::path> filenames;
    while (true)
    {
        struct archive_entry* entry = nullptr;
        if (auto res = archive_read_next_header(reader.get(), &entry); res != ARCHIVE_OK)
        {
            break;
        }

        if (archive_entry_filetype(entry) != AE_IFREG)
        {
            continue;
        }

        std::filesystem::path entry_path = std::filesystem::path(GetArchiveEntryPath(entry));
        if (std::none_of(std::begin(kKnownDiveExtension), std::end(kKnownDiveExtension),
                         [ext = entry_path.extension().string()](std::string_view expected) {
                             return ext == expected;
                         }))
        {
            continue;
        }

        auto dst_file_path = dst / entry_path.filename();
        std::ofstream dst_file(dst_file_path, std::ios::binary | std::ios::out);
        if (auto res = WriteArchiveEntry(dst_file, reader.get()); res != ARCHIVE_OK)
        {
            break;
        }
        filenames.push_back(dst_file_path);
    }
    return filenames;
}

}  // namespace Dive
