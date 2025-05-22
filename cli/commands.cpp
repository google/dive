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
#include "commands.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>

#include "format_output.h"

namespace Dive
{
namespace cli
{

//--------------------------------------------------------------------------------------------------
Command::Command(const char* name, Visibility vis) : m_name(name), m_visibility(vis) {}

Command::~Command() {}

std::string Command::ProgramName(const char* fullpath)
{
    std::filesystem::path name = fullpath;
    return name.filename().string();
}

//--------------------------------------------------------------------------------------------------
struct HelpCommand : Command
{
    HelpCommand(const std::map<std::string, const Command*>*);
    const std::map<std::string, const Command*>& m_commands;

    int         operator()(int argc, int at, char** argv) const override;
    int         Help(int argc, int at, char** argv) const override;
    std::string Description() const override;

    static bool Visible(bool allow_internal, Visibility visiblility);
};

HelpCommand::HelpCommand(const std::map<std::string, const Command*>* commands) :
    Command("help", kNormal), m_commands(*commands)
{
}

bool HelpCommand::Visible(bool allow_internal, Visibility visiblility)
{
    return visiblility == kNormal || (allow_internal && visiblility == kInternal);
}

int HelpCommand::operator()(int argc, int at, char** argv) const
{
    bool allow_internal = false;
    if (at + 1 < argc)
    {
        auto iter = m_commands.find(argv[at + 1]);
        if (iter != m_commands.end())
        {
            return iter->second->Help(argc, at + 1, argv);
        }
        allow_internal = (std::string("--internal") == argv[at + 1]);
    }
    size_t max_name_len = 0;
    for (auto iter : m_commands)
    {
        if (Visible(allow_internal, iter.second->GetVisibility()))
        {
            max_name_len = std::max(max_name_len, iter.first.size());
        }
    }
    std::cout << "Dive command line utilities" << std::endl;
    std::cout << std::endl;

    std::cout << "Usage: " << std::endl;
    std::cout << "  " << ProgramName(argv[0]) << " <command> [<args>]" << std::endl;
    std::cout << std::endl;

    std::cout << "Available Commands:" << std::endl;
    for (auto iter : m_commands)
    {
        if (Visible(allow_internal, iter.second->GetVisibility()))
        {
            std::cout << "  " << iter.first << std::setw(max_name_len - iter.first.size() + 2) << ""
                      << iter.second->Description() << std::endl;
        }
    }
    return (at + 1 == argc ? EXIT_SUCCESS : EXIT_FAILURE);
}

int HelpCommand::Help(int argc, int at, char** argv) const
{
    std::cout << "usage: " << ProgramName(argv[0]) << " " << GetName() << " [command]" << std::endl;
    return EXIT_SUCCESS;
}

std::string HelpCommand::Description() const { return "print help menu"; }

//--------------------------------------------------------------------------------------------------
struct VersionCommand : Command
{
    VersionCommand();
    int         operator()(int argc, int at, char** argv) const override;
    int         Help(int argc, int at, char** argv) const override;
    std::string Description() const override;
};

VersionCommand::VersionCommand() : Command("version", kNormal) {}

int VersionCommand::operator()(int argc, int at, char** argv) const
{
    const char* rev = Dive::cli::RepositoryVersion();
    if (rev == nullptr)
    {
        rev = "(unknown revision)";
    }
    std::cout << "DiveCLI " << rev << std::endl;
    std::cout << "Capture Format: v" << Dive::cli::FileFormatVersion() << std::endl;
    return EXIT_SUCCESS;
}

int VersionCommand::Help(int argc, int at, char** argv) const
{
    std::cout << "usage: " << ProgramName(argv[0]) << " " << GetName() << std::endl;
    return EXIT_SUCCESS;
}

std::string VersionCommand::Description() const { return "display version of DiveCLI"; }

//--------------------------------------------------------------------------------------------------
struct ExtractCommand : Command
{
    ExtractCommand();
    static int  Run(const char* dive_file, const char* output_dir);
    int         operator()(int argc, int at, char** argv) const override;
    int         Help(int argc, int at, char** argv) const override;
    std::string Description() const override;
};

ExtractCommand::ExtractCommand() : Command("extract", kNormal) {}

int ExtractCommand::Run(const char* dive_file, const char* output_dir)
{
    std::string out;
    if (output_dir != nullptr)
    {
        out = output_dir;
    }
    else
    {
        out = dive_file;
        std::string::size_type pos = out.rfind('.');
        if (pos != std::string::npos)
        {
            out = out.substr(0, pos);
        }
        else
        {
            out = out + "_out";
        }
    }
    return Dive::cli::ExtractCapture(dive_file, out.c_str());
}

int ExtractCommand::operator()(int argc, int at, char** argv) const
{
    if (argc - at == 2)
    {
        return Run(argv[at + 1], nullptr);
    }
    else if (argc - at == 4)
    {
        if (strcmp(argv[at + 1], "-o") == 0 || strcmp(argv[at + 1], "--output") == 0)
        {
            return Run(argv[at + 3], argv[at + 2]);
        }
    }
    Help(argc, at, argv);
    return EXIT_FAILURE;
}

int ExtractCommand::Help(int argc, int at, char** argv) const
{
    std::cout << "usage: " << ProgramName(argv[0]) << " " << GetName() << " [-o <dir>] <.dive>"
              << std::endl;
    std::cout << "  -o,--output <dir>: output directory name" << std::endl;
    return EXIT_SUCCESS;
}

std::string ExtractCommand::Description() const { return "extract the content of a dive file"; }

//--------------------------------------------------------------------------------------------------
struct ModifyGFXRCommand : Command
{
    ModifyGFXRCommand();
    static int  Run(const char* dive_file, const char* output_dir);
    int         operator()(int argc, int at, char** argv) const override;
    int         Help(int argc, int at, char** argv) const override;
    std::string Description() const override;
};

ModifyGFXRCommand::ModifyGFXRCommand() : Command("modify-gfxr", kNormal) {}

int ModifyGFXRCommand::Run(const char* original_gfxr_file, const char* new_gfxr_file)
{
    if (strcmp(original_gfxr_file, new_gfxr_file) == 0)
    {
        std::cout << "cannot overwrite original GFXR file" << std::endl;
        return EXIT_FAILURE;
    }
    return Dive::cli::ModifyGFXRCapture(original_gfxr_file, new_gfxr_file);
}

int ModifyGFXRCommand::operator()(int argc, int at, char** argv) const
{
    if (argc - at == 4)
    {
        if (strcmp(argv[at + 1], "-o") == 0 || strcmp(argv[at + 1], "--output") == 0)
        {
            return Run(argv[at + 3], argv[at + 2]);
        }
    }
    Help(argc, at, argv);
    return EXIT_FAILURE;
}

int ModifyGFXRCommand::Help(int argc, int at, char** argv) const
{
    std::cout << "usage: " << ProgramName(argv[0]) << " " << GetName()
              << " -o <new_gfxr> <original_gfxr>" << std::endl;
    std::cout << "  -o,--output <file>: output GFXR file name" << std::endl;
    return EXIT_SUCCESS;
}

std::string ModifyGFXRCommand::Description() const
{
    return "create a new GFXR file from an existing one with modifications";
}

//--------------------------------------------------------------------------------------------------
struct PacketCommand : Command
{
    PacketCommand();
    static int  PrintPacketHeader(const char* header);
    int         operator()(int argc, int at, char** argv) const override;
    int         Help(int argc, int at, char** argv) const override;
    std::string Description() const override;
};

PacketCommand::PacketCommand() : Command("packet", kInternal) {}

int PacketCommand::operator()(int argc, int at, char** argv) const
{
    if (at + 2 != argc)
    {
        return Help(argc, at, argv);
    }
    return PrintPacketHeader(argv[at + 1]);
}

int PacketCommand::Help(int argc, int at, char** argv) const
{
    std::cout << "usage: " << ProgramName(argv[0]) << " " << GetName() << " <packet_header_in_hex>"
              << std::endl;
    return EXIT_SUCCESS;
}

std::string PacketCommand::Description() const { return "decode packet header"; }

int PacketCommand::PrintPacketHeader(const char* header)
{
    uint32_t                    header_value = strtoul(header, nullptr, 16);
    Dive::PM4_PFP_TYPE_3_HEADER pm4_header;
    pm4_header.u32All = header_value;

    std::cout << "PACKET 0x" << std::hex << std::setfill('0') << std::setw(8) << header_value
              << "OP : 0x" << std::hex << pm4_header.opcode << ": "
              << GetOpCodeStringSafe(pm4_header.opcode) << std::endl;

    return EXIT_SUCCESS;
}

//--------------------------------------------------------------------------------------------------
struct InfoCommand : Command
{
    InfoCommand();
    static int  PrintFileMetadata(const char* filename);
    static int  PrintCaptureFileBlocks(const char* filename);
    int         operator()(int argc, int at, char** argv) const override;
    int         Help(int argc, int at, char** argv) const override;
    std::string Description() const override;
};

InfoCommand::InfoCommand() : Command("info", kInternal) {}

int InfoCommand::operator()(int argc, int at, char** argv) const
{
    if (at + 2 != argc)
    {
        return Help(argc, at, argv);
    }
    int res_type = PrintFileMetadata(argv[at + 1]);
    int res_block = PrintCaptureFileBlocks(argv[at + 1]);
    return (res_type != EXIT_SUCCESS ? res_type : res_block);
}

int InfoCommand::Help(int argc, int at, char** argv) const
{
    std::cout << "usage: " << ProgramName(argv[0]) << " " << GetName() << " <filename.dive>"
              << std::endl;
    return EXIT_SUCCESS;
}

std::string InfoCommand::Description() const { return "print basic information about a dive file"; }

int InfoCommand::PrintFileMetadata(const char* filename)
{
    Dive::CaptureDataHeader       header;
    Dive::CaptureData::LoadResult res = ReadCaptureDataHeader(filename, &header);
    if (res != Dive::CaptureData::LoadResult::kSuccess &&
        res != Dive::CaptureData::LoadResult::kVersionError)
    {
        std::cerr << "Can't read " << filename << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Dive capture file: " << filename << std::endl;
    std::cout << "Version: " << header.m_major_version << "." << header.m_minor_version << "."
              << header.m_revision << std::endl;

    if (res == Dive::CaptureData::LoadResult::kVersionError)
    {
        std::cerr << "Version unsupported by this version of DiveCLI" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Capture type: " << CaptureTypeToString(header.m_capture_type) << "" << std::endl;

    if (header.m_pal_version != 0)
    {
        std::cout << "PAL version: " << header.m_pal_version << std::endl;
    }
    std::cout << "Features: " << (header.m_capture_pm4 ? "PM4," : "")
              << (header.m_capture_sqtt ? "SQTT," : "") << (header.m_capture_spm ? "SPM," : "")
              << (header.m_defer_capture ? "DEFER_CAPTURE," : "")
              << (header.m_enable_inst_trace ? "INST_TRACE," : "")
              << (header.m_reset_memory_tracker ? "MEM_TRACKER," : "") << std::endl;
    if (header.m_device_id != 0 || header.m_device_revision != 0)
    {
        std::cout << "GPU Device ID: " << std::hex << std::setfill('0') << std::setw(4)
                  << header.m_device_id << ":" << std::setw(4) << header.m_device_revision
                  << std::setfill(' ') << std::dec << std::endl;
    }

    return EXIT_SUCCESS;
}

int InfoCommand::PrintCaptureFileBlocks(const char* filename)
{
    Dive::CaptureData::LoadResult res = Dive::cli::PrintCaptureFileBlocks(std::cout, filename);
    if (res == Dive::CaptureData::LoadResult::kSuccess)
    {
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

//--------------------------------------------------------------------------------------------------

struct RawPM4Command : Command
{
    RawPM4Command();
    static bool PrintRawPm4(const char* file_name, int raw_cmd_buffer_type);
    int         operator()(int argc, int at, char** argv) const override;
    int         Help(int argc, int at, char** argv) const override;
    std::string Description() const override;
};

RawPM4Command::RawPM4Command() : Command("rawpm4", kInternal) {}

int RawPM4Command::operator()(int argc, int at, char** argv) const
{
    if (at + 3 != argc)
    {
        return Help(argc, at, argv);
    }
    int buffer_type = 0;

    if (!strcmp("gfx", argv[at + 1]))
    {
        buffer_type = 0;
    }
    else if (!strcmp("dma", argv[at + 1]))
    {
        buffer_type = 1;
    }
    else
    {
        std::cerr << "Unknown command buffer type " << argv[at + 1] << std::endl;
        Help(argc, at, argv);
    }

    if (!PrintRawPm4(argv[at + 2], buffer_type))
    {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int RawPM4Command::Help(int argc, int at, char** argv) const
{
    std::cout << "usage: " << ProgramName(argv[0]) << " " << GetName() << " <gfx|dma> <data.bin>"
              << std::endl;
    return EXIT_SUCCESS;
}

std::string RawPM4Command::Description() const { return "opens and parses raw command stream"; }

bool RawPM4Command::PrintRawPm4(const char* file_name, int raw_cmd_buffer_type)
{
    std::fstream raw_file(file_name, std::ios::in | std::ios::binary);
    if (!raw_file.is_open())
        return false;

    raw_file.seekg(0, std::ios::end);
    std::streamsize size = raw_file.tellg();
    raw_file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!raw_file.read(buffer.data(), size))
        return false;

    Dive::EngineType engine_type = Dive::EngineType::kUniversal;
    Dive::QueueType  queue_type = Dive::QueueType::kUniversal;
    switch (raw_cmd_buffer_type)
    {
    case 0:
        break;  // Gfx
    case 1:     // Dma
        engine_type = Dive::EngineType::kDma;
        queue_type = Dive::QueueType::kDma;
        break;
    }

    std::unique_ptr<EmulateStateTracker> state_tracker(new EmulateStateTracker);
    Dive::CommandHierarchyCreator        cmd_hier_creator(*state_tracker);
    Dive::CommandHierarchy               command_hierarchy;
    Dive::LogConsole                     log;
    if (!cmd_hier_creator.CreateTrees(&command_hierarchy,
                                      engine_type,
                                      queue_type,
                                      (uint32_t*)buffer.data(),
                                      (uint32_t)(size / sizeof(uint32_t)),
                                      &log))
        return false;

    const Dive::Topology* topology_ptr = &command_hierarchy.GetSubmitHierarchyTopology();
    uint64_t root_num_children = topology_ptr->GetNumChildren(Dive::Topology::kRootNodeIndex);
    for (uint64_t child = 0; child < root_num_children; ++child)
    {
        uint64_t child_node_index = topology_ptr->GetChildNodeIndex(Dive::Topology::kRootNodeIndex,
                                                                    child);
        PrintNodes(std::cout, &command_hierarchy, *topology_ptr, child_node_index, true);
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
const Command& CommandOf<HelpCommand>::Get(const std::map<std::string, const Command*>* commands)
{
    static HelpCommand impl(commands);
    return impl;
}

template<typename T> const Command& CommandOf<T>::Get()
{
    static T impl;
    return impl;
}

template const Command& CommandOf<VersionCommand>::Get();
template const Command& CommandOf<ExtractCommand>::Get();
template const Command& CommandOf<ModifyGFXRCommand>::Get();
template const Command& CommandOf<PacketCommand>::Get();
template const Command& CommandOf<InfoCommand>::Get();
template const Command& CommandOf<RawPM4Command>::Get();

}  // namespace cli
}  // namespace Dive
