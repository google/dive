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

// TODO: Eventually the .dive file support and the raw data support in `cli/` will be migrated here
// and the old cli will be deprecated

#include <filesystem>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/status/status.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_cat.h"

#include "common/dive_version.h"
#include "data_core_wrapper.h"

namespace
{
constexpr const char kDiveVersionSHA1String[] = DIVE_VERSION_SHA1;
constexpr std::array kAllowedInputFileExtensions = { ".gfxr" };
}  // namespace

ABSL_FLAG(bool, version_info, false, "Shows the version of Dive host tools and quits");
ABSL_FLAG(std::string,
          input_file_path,
          "",
          "If specified, this file is loaded with the type is inferred from the extension. "
          "Supported extensions: .gfxr");
ABSL_FLAG(std::string,
          output_gfxr_path,
          "",
          "If specified, a new .gfxr file will be generated from the original file "
          "(--input_file_path) and any specified modifications");
ABSL_FLAG(std::vector<std::string>,
          delete_gfxr_blocks,
          {},
          "If specified, the blocks with these ids will be omitted from the modified .gfxr file. "
          "Example: --delete_gfxr_blocks=1,2");

absl::Status ValidateFlags()
{
    std::string input_file_ext = "";

    std::string input_file_path = absl::GetFlag(FLAGS_input_file_path);
    if (!input_file_path.empty())
    {
        std::filesystem::path input_fp = input_file_path;
        std::string           ext = input_fp.extension().string();
        if ((ext.empty()) || std::find(kAllowedInputFileExtensions.begin(),
                                       kAllowedInputFileExtensions.end(),
                                       ext) == kAllowedInputFileExtensions.end())
        {
            return absl::InvalidArgumentError(
            absl::StrFormat("unrecognizable extension provided for --input_file_path: %s", ext));
        }
        input_file_ext = ext;
    }

    std::string output_gfxr_path = absl::GetFlag(FLAGS_output_gfxr_path);
    if (!output_gfxr_path.empty())
    {
        if (input_file_ext != ".gfxr")
        {
            return absl::InvalidArgumentError(
            "if --output_gfxr_path is specified, then --input_file_path must also be specified for "
            "a .gfxr file");
        }
    }

    std::vector<std::string> delete_gfxr_block = absl::GetFlag(FLAGS_delete_gfxr_blocks);
    if (!delete_gfxr_block.empty())
    {
        if (input_file_ext != ".gfxr")
        {
            return absl::InvalidArgumentError("if --delete_gfxr_blocks is specified, then "
                                              "--input_file_path must also be specified for "
                                              "a .gfxr file");
        }
        for (auto const& ele : delete_gfxr_block)
        {
            int i;
            if (!absl::SimpleAtoi(ele, &i))
            {
                return absl::InvalidArgumentError(
                absl::StrFormat("flag --delete_gfxr_blocks accepts comma-separated integers, "
                                "invalid input: %s",
                                ele));
            }
        }
    }

    return absl::OkStatus();
}

std::string GetDiveRepositoryVersion()
{
    if constexpr (std::size(kDiveVersionSHA1String) > 0 && kDiveVersionSHA1String[0] != 0)
    {
        return kDiveVersionSHA1String;
    }
    else
    {
        return "(unknown version)";
    }
}

int main(int argc, char** argv)
{
    absl::SetProgramUsageMessage(
    absl::StrCat("This CLI tool is intended to provide access to the dive_core"
                 "\nlibrary for utility and for testing. Currently it supports"
                 "\nmanipulation of .gfxr files. Sample usage:\n\n",
                 argv[0],
                 " --help"));
    absl::ParseCommandLine(argc, argv);

    // Early termination flags
    bool show_version = absl::GetFlag(FLAGS_version_info);
    if (show_version)
    {
        std::cout << "Dive " << GetDiveRepositoryVersion() << std::endl;
        return 0;
    }

    absl::Status res = ValidateFlags();
    if (!res.ok())
    {
        std::cout << res << std::endl;
        return 1;
    }

    Dive::HostCli::DataCoreWrapper data_core;

    std::filesystem::path input_file_path = absl::GetFlag(FLAGS_input_file_path);
    if (input_file_path.extension().string() == ".gfxr")
    {
        absl::Status res = data_core.LoadGfxrFile(input_file_path.string());
        if (!res.ok())
        {
            std::cout << res << std::endl;
            return 1;
        }

        std::vector<std::string> delete_gfxr_block = absl::GetFlag(FLAGS_delete_gfxr_blocks);
        if (!delete_gfxr_block.empty())
        {
            std::vector<int> delete_block_ids;
            for (auto const& ele : delete_gfxr_block)
            {
                int id;
                absl::SimpleAtoi(ele, &id);
                delete_block_ids.push_back(id);
            }
            res = data_core.RemoveGfxrBlocks(delete_block_ids);
            if (!res.ok())
            {
                std::cout << res << std::endl;
                return 1;
            }
        }

        std::string output_gfxr_path = absl::GetFlag(FLAGS_output_gfxr_path);
        if (output_gfxr_path.empty())
        {
            // Nothing further to do with the loaded .gfxr file
            return 0;
        }

        res = data_core.WriteNewGfxrFile(output_gfxr_path);
        if (!res.ok())
        {
            std::cout << res << std::endl;
            return 1;
        }
        return 0;
    }

    // No action taken, print usage message
    std::cout << absl::ProgramUsageMessage() << std::endl;
    return 0;
}
