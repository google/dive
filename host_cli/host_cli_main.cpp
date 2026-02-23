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
#include "absl/flags/usage_config.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "data_core_wrapper.h"
#include "utils/version_info.h"

namespace
{
constexpr std::array kAllowedInputFileExtensions = {".gfxr"};
}  // namespace

ABSL_FLAG(std::string, input_file_path, "",
          "If specified, this file is loaded with the type is inferred from the extension. "
          "Supported extensions: .gfxr");
ABSL_FLAG(std::string, output_gfxr_path, "",
          "If specified, a new .gfxr file will be generated from the original file "
          "(--input_file_path) and any specified modifications");
ABSL_FLAG(std::vector<std::string>, delete_gfxr_blocks, {},
          "If specified, the blocks with these ids will be omitted from the modified .gfxr file. "
          "Example: --delete_gfxr_blocks=1,2");

struct ValidatedFlags
{
    bool input_gfxr_file = false;
    bool output_gfxr_file = false;
    std::vector<int> delete_block_ids = {};
};

absl::StatusOr<ValidatedFlags> ValidateFlags()
{
    ValidatedFlags valid_flags = {};

    std::string input_file_path = absl::GetFlag(FLAGS_input_file_path);
    if (!input_file_path.empty())
    {
        std::filesystem::path input_fp = input_file_path;
        std::string ext = input_fp.extension().string();
        if ((ext.empty()) ||
            std::find(kAllowedInputFileExtensions.begin(), kAllowedInputFileExtensions.end(),
                      ext) == kAllowedInputFileExtensions.end())
        {
            return absl::InvalidArgumentError(absl::StrFormat(
                "unrecognizable extension provided for --input_file_path: %s", ext));
        }
        if (ext == ".gfxr")
        {
            valid_flags.input_gfxr_file = true;
        }
    }

    std::string output_gfxr_path = absl::GetFlag(FLAGS_output_gfxr_path);
    if (!output_gfxr_path.empty())
    {
        if (!valid_flags.input_gfxr_file)
        {
            return absl::InvalidArgumentError(
                "if --output_gfxr_path is specified, then --input_file_path must also be specified "
                "for a .gfxr file");
        }
        valid_flags.output_gfxr_file = true;
    }

    std::vector<std::string> delete_gfxr_block = absl::GetFlag(FLAGS_delete_gfxr_blocks);
    if (!delete_gfxr_block.empty())
    {
        if (!valid_flags.input_gfxr_file)
        {
            return absl::InvalidArgumentError(
                "if --delete_gfxr_blocks is specified, then --input_file_path must also be "
                "specified for a .gfxr file");
        }
        for (auto const& ele : delete_gfxr_block)
        {
            int i = 0;
            if (!absl::SimpleAtoi(ele, &i))
            {
                return absl::InvalidArgumentError(absl::StrFormat(
                    "flag --delete_gfxr_blocks accepts comma-separated integers, invalid input: %s",
                    ele));
            }
            valid_flags.delete_block_ids.push_back(i);
        }
    }

    return valid_flags;
}

int main(int argc, char** argv)
{
    absl::FlagsUsageConfig flags_usage_config;
    flags_usage_config.version_string = Dive::GetCompleteVersionString;
    absl::SetFlagsUsageConfig(flags_usage_config);
    absl::SetProgramUsageMessage(absl::StrCat(
        "This CLI tool is intended to provide access to the dive_core library for utility and for "
        "testing. Currently it supports manipulation of .gfxr files. Sample usage:\n\n",
        argv[0], " --help"));
    absl::ParseCommandLine(argc, argv);

    absl::StatusOr<ValidatedFlags> valid_flags = ValidateFlags();
    if (!valid_flags.ok())
    {
        std::cout << valid_flags.status().message() << std::endl;
        return 1;
    }

    Dive::HostCli::DataCoreWrapper data_core;

    if (valid_flags->input_gfxr_file)
    {
        if (absl::Status res = data_core.LoadGfxrFile(absl::GetFlag(FLAGS_input_file_path));
            !res.ok())
        {
            std::cout << res << std::endl;
            return 1;
        }

        if (!valid_flags->delete_block_ids.empty())
        {
            if (absl::Status res = data_core.RemoveGfxrBlocks(valid_flags->delete_block_ids);
                !res.ok())
            {
                std::cout << res << std::endl;
                return 1;
            }
        }

        if (!valid_flags->output_gfxr_file)
        {
            // Nothing further to do with the loaded .gfxr file
            return 0;
        }

        if (absl::Status res = data_core.WriteNewGfxrFile(absl::GetFlag(FLAGS_output_gfxr_path));
            !res.ok())
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
