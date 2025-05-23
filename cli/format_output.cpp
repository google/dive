/*
 Copyright 2021 Google LLC

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

#include <algorithm>
#include <array>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>

#include "dive_core/capture_data.h"
#include "dive_core/command_hierarchy.h"
#include "dive_core/data_core.h"
#include "dive_core/dive_strings.h"

#include "../dive_core/shader_disassembly.h"

#include "cli.h"
#include "format_output.h"

namespace Dive
{
namespace cli
{

//--------------------------------------------------------------------------------------------------
const char *GetOpCodeStringSafe(uint32_t op_code)
{
    return "UNKNOWN";
}

//--------------------------------------------------------------------------------------------------
const char *CaptureTypeToString(Dive::CaptureDataHeader::CaptureType type)
{
    switch (type)
    {
    case Dive::CaptureDataHeader::CaptureType::kSingleFrame:
        return "Single Frame";
    case Dive::CaptureDataHeader::CaptureType::kBeginEndRange:
        return "Begin End Range";
    }

    DIVE_ASSERT(false);
    return "Unknown";
}

//--------------------------------------------------------------------------------------------------
void PrintSharedNodes(std::ostream                 &out,
                      const Dive::CommandHierarchy *command_hierarchy_ptr,
                      const Dive::Topology         &topology,
                      uint64_t                      node_index,
                      uint32_t                      num_tabs)
{
    for (uint64_t child = 0; child < topology.GetNumSharedChildren(node_index); ++child)
    {
        uint64_t child_node_index = topology.GetSharedChildNodeIndex(node_index, child);
        auto     child_type = command_hierarchy_ptr->GetNodeType(child_node_index);
        if (child_type == Dive::NodeType::kPacketNode)
        {
            for (uint32_t tab = 0; tab < num_tabs; ++tab)
                out << "  ";

            auto addr = command_hierarchy_ptr->GetPacketNodeAddr(child_node_index);

            auto f = out.flags();
            out << "[";
            out << std::setfill('0') << std::setw(16) << std::hex;
            out << addr;
            out.flags(f);
            out << "] ";
        }

        out << command_hierarchy_ptr->GetNodeDesc(child_node_index) << std::endl;

        for (uint32_t f = 0; f < topology.GetNumChildren(child_node_index); ++f)
        {
            uint64_t fc_idx = topology.GetChildNodeIndex(child_node_index, f);
            out << "      " << command_hierarchy_ptr->GetNodeDesc(fc_idx) << std::endl;
        }
    }
}

//--------------------------------------------------------------------------------------------------
void VisitNodes(const Dive::CommandHierarchy           *command_hierarchy_ptr,
                const Dive::Topology                   &topology,
                uint64_t                                node_index,
                uint32_t                                depth,
                std::function<bool(uint64_t, uint32_t)> visitor)
{
    if (visitor(node_index, depth))
    {
        for (uint64_t child = 0; child < topology.GetNumChildren(node_index); ++child)
        {
            uint64_t child_node_index = topology.GetChildNodeIndex(node_index, child);
            VisitNodes(command_hierarchy_ptr, topology, child_node_index, depth + 1, visitor);
        }
    }
}

//--------------------------------------------------------------------------------------------------
void PrintNodes(std::ostream                 &out,
                const Dive::CommandHierarchy *command_hierarchy_ptr,
                const Dive::Topology         &topology,
                uint64_t                      node_index,
                bool                          verbose)
{
    VisitNodes(command_hierarchy_ptr,
               topology,
               node_index,
               0,
               [&out, command_hierarchy_ptr, &topology, verbose](uint64_t node_index,
                                                                 uint32_t depth) -> bool {
                   for (uint32_t tab = 0; tab < depth; ++tab)
                       out << "  ";
                   if (depth > 0)
                       out << "| ";

                   out << command_hierarchy_ptr->GetNodeDesc(node_index) << std::endl;

                   if (verbose && 0 == topology.GetNumChildren(node_index))
                   {
                       PrintSharedNodes(out,
                                        command_hierarchy_ptr,
                                        topology,
                                        node_index,
                                        depth + 1);
                   }
                   return true;
               });
}

//--------------------------------------------------------------------------------------------------
LoadResult PrintBlock(std::ostream      &out,
                      std::istream      &capture_file,
                      const std::string &prefix,
                      const BlockInfo   &block_info);

//--------------------------------------------------------------------------------------------------
std::string BlockTypeToString(BlockType bt)
{
    char c0 = (char)((uint32_t)bt) & 0xff;
    char c1 = (char)((uint32_t)bt >> 8) & 0xff;
    char c2 = (char)((uint32_t)bt >> 16) & 0xff;
    char c3 = (char)((uint32_t)bt >> 24) & 0xff;
    char buf[] = { c0, c1, c2, c3, 0 };
    return std::string(buf);
}

//--------------------------------------------------------------------------------------------------
bool IsPossibleBlock(const BlockInfo &info)
{
    switch (info.m_block_type)
    {
    case BlockType::kCapture:
        return true;
    case BlockType::kMemoryAlloc:
        return true;
    case BlockType::kSubmit:
        return true;
    case BlockType::kMemoryRaw:
        return true;
    case BlockType::kRgp:
        return true;
    case BlockType::kPresent:
        return true;
    case BlockType::kRing:
        return true;
    case BlockType::kText:
        return true;
    case BlockType::kRegisters:
        return true;
    case BlockType::kWaveState:
        return true;
    case BlockType::kVulkanMetadata:
        return true;
    default:
        break;
    }
    return false;
}

LoadResult DiscoverBlocks(std::ostream &out, std::istream &capture_file)
{
    std::vector<char> data{ std::istreambuf_iterator<char>(capture_file),
                            std::istreambuf_iterator<char>() };
    auto              stream_flags = out.flags();
    out << "File size: " << std::dec << data.size() << " (0x" << std::hex << data.size() << ")"
        << std::endl;
    out << std::hex;
    out << "Blocks found:" << std::endl;
    for (size_t pos = 0; pos + sizeof(BlockInfo) < data.size(); pos++)
    {
        BlockInfo info;
        memcpy(&info, &data[pos], sizeof(BlockInfo));
        if (!IsPossibleBlock(info))
        {
            continue;
        }
        size_t next_block = pos + info.m_data_size + sizeof(BlockInfo);
        bool   likely_block = false;
        if (next_block == data.size())
        {
            likely_block = true;
        }
        if (next_block + sizeof(BlockInfo) < data.size())
        {
            BlockInfo next_block_info;
            memcpy(&next_block_info, &data[next_block], sizeof(BlockInfo));
            if (IsPossibleBlock(next_block_info))
            {
                likely_block = true;
            }
        }
        out << "  " << BlockTypeToString(info.m_block_type) << (likely_block ? " " : "?") << " "
            << std::setfill('0') << std::setw(8) << pos << "-" << std::setfill('0') << std::setw(8)
            << pos + info.m_data_size + sizeof(BlockInfo) << std::endl;
    }
    out.flags(stream_flags);
    return LoadResult::kSuccess;
}

//--------------------------------------------------------------------------------------------------
LoadResult PrintBlocks(std::ostream      &out,
                       std::istream      &capture_file,
                       const std::string &prefix,
                       std::streampos     end_pos = 0)
{
    BlockInfo block_info;
    while (capture_file.read((char *)&block_info, sizeof(block_info)))
    {
        auto block_start = capture_file.tellg();
        auto print_res = PrintBlock(out, capture_file, prefix, block_info);
        if (print_res != LoadResult::kSuccess)
        {
            return print_res;
        }

        // Seek to end of block.
        capture_file.seekg(block_start);
        capture_file.seekg(block_info.m_data_size, std::ios::cur);

        auto expected_pos = block_start + static_cast<std::streampos>(block_info.m_data_size);
        if (capture_file.tellg() != expected_pos)
        {
            return LoadResult::kCorruptData;
        }
        if (end_pos != 0 && capture_file.tellg() >= end_pos)
        {
            if (capture_file.tellg() > end_pos)
            {
                return LoadResult::kCorruptData;
            }
            break;
        }
    }

    return LoadResult::kSuccess;
}

//--------------------------------------------------------------------------------------------------
LoadResult PrintBlock(std::ostream      &out,
                      std::istream      &capture_file,
                      const std::string &prefix,
                      const BlockInfo   &block_info)
{
    char c0 = (char)((uint32_t)block_info.m_block_type) & 0xff;
    char c1 = (char)((uint32_t)block_info.m_block_type >> 8) & 0xff;
    char c2 = (char)((uint32_t)block_info.m_block_type >> 16) & 0xff;
    char c3 = (char)((uint32_t)block_info.m_block_type >> 24) & 0xff;

    auto block_offset = (size_t)capture_file.tellg();
    out << prefix << c0 << c1 << c2 << c3 << ": offset " << block_offset << ", size "
        << block_info.m_data_size;
    out << ", end " << (block_offset + block_info.m_data_size);

    switch (block_info.m_block_type)
    {
    case BlockType::kCapture:
    {
        // The capture data always begins with some metadata info
        CaptureDataHeader data_header;
        capture_file.read((char *)&data_header, sizeof(data_header));
        bool incompatible = ((data_header.m_major_version != kCaptureMajorVersion) ||
                             (data_header.m_minor_version > kCaptureMinorVersion));
        // Cannot open version 0.1 due to CaptureDataHeader change
        incompatible |= ((data_header.m_major_version == 0) && (data_header.m_minor_version == 1));
        if (incompatible)
        {
            std::cerr << "Incompatible capture version " << data_header.m_major_version << "."
                      << data_header.m_minor_version << std::endl;
            std::cerr << "Supported version: " << kCaptureMajorVersion << "."
                      << kCaptureMinorVersion << std::endl;
        }

        out << std::endl;
        out << prefix << " |   ";
        out << "Capture Type: " << CaptureTypeToString(data_header.m_capture_type) << std::endl;
        out << prefix << " --> ";
        out << "GPU device ID 0x" << std::hex << data_header.m_device_id << ", revision 0x"
            << data_header.m_device_revision << std::dec << std::endl;

        std::streampos end_pos = (std::streampos)block_offset +
                                 (std::streampos)block_info.m_data_size;
        // Capture block itself contains multiple blocks, so recurse.
        auto res = PrintBlocks(out, capture_file, prefix + "   ", end_pos);
        if (LoadResult::kSuccess != res)
        {
            return res;
        }
    }
    break;
    case BlockType::kMemoryRaw:
    {
        MemoryRawDataHeader memory_raw_data_header;
        if (!capture_file.read((char *)&memory_raw_data_header, sizeof(memory_raw_data_header)))
            return LoadResult::kFileIoError;

        out << std::endl;
        out << prefix << " --> ";
        auto f = out.flags();
        out << "[";
        out << std::right << std::setfill('0') << std::setw(16) << std::hex;
        out << memory_raw_data_header.m_va_addr << " - ";
        out << std::setfill('0') << std::setw(16) << std::hex;
        out << (memory_raw_data_header.m_va_addr + memory_raw_data_header.m_size_in_bytes);
        out.flags(f);
        out << "] ";
        out << std::right << std::dec << memory_raw_data_header.m_size_in_bytes << " bytes";
        out.flags(f);
    }
    break;

    case BlockType::kText:
    {
        TextBlockHeader text_header;
        if (!capture_file.read((char *)&text_header, sizeof(text_header)))
            return LoadResult::kFileIoError;

        std::string name;
        name.reserve(text_header.m_name_len);
        if (!std::getline(capture_file, name, '\0'))
            return LoadResult::kFileIoError;

        out << std::endl;
        out << prefix << " --> ";
        out << "text: " << name << ", " << text_header.m_size_in_bytes << " bytes";
    }
    break;

    default:
        break;
    }

    out << std::endl;
    return LoadResult::kSuccess;
}

//--------------------------------------------------------------------------------------------------
LoadResult PrintCaptureFileBlocks(std::ostream &out, const char *file_name)
{
    // Open the file stream
    std::fstream capture_file(file_name, std::ios::in | std::ios::binary);
    if (!capture_file.is_open())
    {
        std::cerr << "Not able to open: " << file_name << std::endl;
        return LoadResult::kFileIoError;
    }

    // Read file header
    FileHeader file_header;
    if (!capture_file.read((char *)&file_header, sizeof(file_header)))
    {
        std::cerr << "Not able to read: " << file_name << std::endl;
        return LoadResult::kFileIoError;
    }

    if (file_header.m_file_id != kDiveFileId)
        return LoadResult::kCorruptData;
    if (file_header.m_file_version != kDiveFileVersion)
        return LoadResult::kVersionError;

    LoadResult res = PrintBlocks(out, capture_file, "");
    if (res == LoadResult::kCorruptData)
    {
        capture_file.clear();
        capture_file.seekg(0, std::ios::beg);
        out << std::endl;
        out << "File is corrupted." << std::endl;
        DiscoverBlocks(out, capture_file);
    }
    return res;
}

//--------------------------------------------------------------------------------------------------
LoadResult ReadCaptureDataHeader(const char *file_name, Dive::CaptureDataHeader *data_header)
{
    // Open the file stream
    std::fstream capture_file(file_name, std::ios::in | std::ios::binary);
    if (!capture_file.is_open())
        return LoadResult::kFileIoError;

    // Read file header
    FileHeader file_header;
    if (!capture_file.read((char *)&file_header, sizeof(file_header)))
        return LoadResult::kFileIoError;
    if (file_header.m_file_id != kDiveFileId || file_header.m_file_version != kDiveFileVersion)
        return LoadResult::kVersionError;

    BlockInfo block_info;
    while (capture_file.read((char *)&block_info, sizeof(block_info)))
    {
        auto block_start = capture_file.tellg();
        if (block_info.m_block_type == BlockType::kCapture)
        {
            capture_file.read((char *)data_header, sizeof(*data_header));
            // Cannot open version 0.1 due to CaptureDataHeader change
            if ((data_header->m_major_version == 0) && (data_header->m_minor_version == 1))
                return LoadResult::kVersionError;
            if ((data_header->m_major_version != kCaptureMajorVersion) ||
                (data_header->m_minor_version > kCaptureMinorVersion))
                return LoadResult::kVersionError;
            return LoadResult::kSuccess;
        }

        // Seek to end of block.
        capture_file.seekg(block_start);
        capture_file.seekg(block_info.m_data_size, std::ios::cur);
    }

    return LoadResult::kCorruptData;
}

//--------------------------------------------------------------------------------------------------
void ExtractTopology(std::filesystem::path         path,
                     const Dive::CommandHierarchy *command_hierarchy_ptr,
                     const Dive::Topology         *topology_ptr)
{
    std::ofstream out(path);
    if (!out)
    {
        std::cerr << "Can't open " << path << " for writing" << std::endl;
        return;
    }

    uint64_t root_num_children = topology_ptr->GetNumChildren(Dive::Topology::kRootNodeIndex);
    for (uint64_t child = 0; child < root_num_children; ++child)
    {
        uint64_t child_node_index = topology_ptr->GetChildNodeIndex(Dive::Topology::kRootNodeIndex,
                                                                    child);
        PrintNodes(out, command_hierarchy_ptr, *topology_ptr, child_node_index, true);
    }
}
//--------------------------------------------------------------------------------------------------
std::string CleanFilename(const std::string &in)
{
    std::string out = in;
    for (char &c : out)
    {
        if (!(isalnum(c) || c == '.' || c == '_'))
        {
            c = '_';
        }
    }
    return out;
}

//--------------------------------------------------------------------------------------------------
// FIXME pointers?
void ExtractAssets(const char                   *dir,
                   const char                   *capture_filename,
                   const Dive::CaptureData      &capture_data,
                   const Dive::CommandHierarchy *command_hierarchy)
{
    auto dir_path = std::filesystem::path(dir);
    std::filesystem::create_directories(dir_path);

    {
        auto          out_path = dir_path / "blocks.txt";
        std::ofstream out(out_path);
        if (!out)
        {
            std::cerr << "Can't open " << out_path << " for writing" << std::endl;
            return;
        }
        PrintCaptureFileBlocks(out, capture_filename);
    }

    if (capture_data.GetNumText() > 0)
    {
        std::filesystem::create_directories(dir_path / "text");
        for (uint32_t i = 0; i < capture_data.GetNumText(); i++)
        {
            auto         &text = capture_data.GetText(i);
            auto          out_path = dir_path / "text" / CleanFilename(text.GetName());
            std::ofstream out(out_path, std::ios::out | std::ios::binary);
            size_t        text_size = text.GetSize();
            if (text_size > 0 && text.GetText()[text_size - 1] == 0)
                text_size--;  // Don't write the trailing '\0'
            out.write(text.GetText(), text_size);
        }
    }

    if (command_hierarchy)
    {
        ExtractTopology(dir_path / "submits.txt",
                        command_hierarchy,
                        &command_hierarchy->GetSubmitHierarchyTopology());
        ExtractTopology(dir_path / "events.txt",
                        command_hierarchy,
                        &command_hierarchy->GetAllEventHierarchyTopology());
    }
}

//--------------------------------------------------------------------------------------------------
bool ParseCapture(const char                              *filename,
                  std::unique_ptr<Dive::CaptureData>      *out_capture_data,
                  std::unique_ptr<Dive::CommandHierarchy> *out_command_hierarchy)
{
    Dive::LogConsole                    log;
    std::unique_ptr<Dive::CaptureData> &capture_data = *out_capture_data;
    capture_data = std::make_unique<Dive::CaptureData>(&log);
    if (capture_data->LoadFile(filename) != Dive::CaptureData::LoadResult::kSuccess)
    {
        capture_data.reset();
        std::cerr << "Not able to open: " << filename << std::endl;
        return false;
    }

    std::unique_ptr<Dive::CommandHierarchy> &command_hierarchy = *out_command_hierarchy;
    command_hierarchy = std::make_unique<Dive::CommandHierarchy>();
    std::unique_ptr<EmulateStateTracker> state_tracker(new EmulateStateTracker);
    Dive::CommandHierarchyCreator        creator(*state_tracker);
    if (!creator.CreateTrees(command_hierarchy.get(), *capture_data, true, std::nullopt, &log))
    {
        command_hierarchy.reset();
        std::cerr << "Error parsing capture!" << std::endl;
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
int PrintTopology(const char *filename, TopologyName topology, bool verbose)
{
    Dive::LogConsole   log;
    Dive::CaptureData *capture_data_ptr = new Dive::CaptureData(&log);
    if (capture_data_ptr->LoadFile(filename) != Dive::CaptureData::LoadResult::kSuccess)
    {
        std::cerr << "Not able to open: " << filename << std::endl;
        return EXIT_FAILURE;
    }

    std::unique_ptr<Dive::CommandHierarchy>    command_hierarchy_ptr(new Dive::CommandHierarchy());
    std::unique_ptr<Dive::EmulateStateTracker> state_tracker(new EmulateStateTracker);
    Dive::CommandHierarchyCreator              creator(*state_tracker);
    if (!creator
         .CreateTrees(command_hierarchy_ptr.get(), *capture_data_ptr, true, std::nullopt, &log))
    {
        std::cerr << "Error parsing capture!" << std::endl;
        return EXIT_FAILURE;
    }

    const Dive::Topology *topology_ptr = nullptr;
    switch (topology)
    {
    case TopologyName::kTopologySubmit:
        topology_ptr = &command_hierarchy_ptr->GetSubmitHierarchyTopology();
        break;
    case TopologyName::kTopologyEvent:
        topology_ptr = &command_hierarchy_ptr->GetAllEventHierarchyTopology();
        break;
    default:
        abort();  // This should be checked during args parsing.
    }

    uint64_t root_num_children = topology_ptr->GetNumChildren(Dive::Topology::kRootNodeIndex);
    for (uint64_t child = 0; child < root_num_children; ++child)
    {
        uint64_t child_node_index = topology_ptr->GetChildNodeIndex(Dive::Topology::kRootNodeIndex,
                                                                    child);
        PrintNodes(std::cout,
                   command_hierarchy_ptr.get(),
                   *topology_ptr,
                   child_node_index,
                   verbose);
    }

    return EXIT_SUCCESS;
}

//--------------------------------------------------------------------------------------------------
int ExtractCapture(const char *filename, const char *extract_assets)
{
    Dive::LogConsole                log;
    std::unique_ptr<Dive::DataCore> data = std::make_unique<Dive::DataCore>(&log);
    if (data->LoadCaptureData(filename) != Dive::CaptureData::LoadResult::kSuccess)
    {
        std::cerr << "Load capture failed." << std::endl;
        return EXIT_FAILURE;
    }

    const Dive::CommandHierarchy *command_hierarchy = nullptr;
    if (data->ParseCaptureData())
    {
        command_hierarchy = &data->GetCommandHierarchy();
    }
    else
    {
        std::cerr << "Parse capture data failed." << std::endl;
        return EXIT_FAILURE;
    }

    ExtractAssets(extract_assets, filename, data->GetCaptureData(), command_hierarchy);

    return EXIT_SUCCESS;
}

//--------------------------------------------------------------------------------------------------
int ModifyGFXRCapture(const char *original_filename, const char *new_filename)
{
    Dive::LogConsole                log;
    std::unique_ptr<Dive::DataCore> data = std::make_unique<Dive::DataCore>(&log);
    if (data->LoadCaptureData(original_filename) != Dive::CaptureData::LoadResult::kSuccess)
    {
        std::cerr << "Load GFXR capture failed." << std::endl;
        return EXIT_FAILURE;
    }

    if (!data->WriteNewGFXRCaptureData(new_filename))
    {
        std::cerr << "Write modified GFXR capture failed." << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

}  // namespace cli
}  // namespace Dive
