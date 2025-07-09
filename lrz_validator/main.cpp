/*
 Copyright 2024 Google LLC

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
#include <iostream>
#include <optional>
#include <vector>

#include "dive_core/data_core.h"
#include "pm4_info.h"

bool ValidataLRZ(const Dive::CaptureMetadata &meta_data, const std::string &output_file_name)
{
    size_t                       event_count = meta_data.m_event_info.size();
    const Dive::EventStateInfo  &event_state = meta_data.m_event_state;
    bool                         lrz_test_passed = true;
    std::optional<std::ofstream> output_file = std::nullopt;
    if (!output_file_name.empty())
    {
        std::cout << "Output detailed validation result to \"" << output_file_name << "\""
                  << std::endl;
        output_file.emplace(std::ofstream(output_file_name));
    }

    std::function<void(const std::string &)> OutputDetails;

    if (output_file.has_value())
    {
        OutputDetails = [&output_file](const std::string &str) { *output_file << str; };
    }
    else
    {
        OutputDetails = [](const std::string &) {};
    }

    for (size_t i = 0; i < event_count; ++i)
    {
        const Dive::EventInfo &info = meta_data.m_event_info[i];
        // We only output the drawcalls in direct/binning mode
        if ((info.m_type == Dive::EventInfo::EventType::kDraw) &&
            ((info.m_render_mode == Dive::RenderModeType::kDirect) ||
             (info.m_render_mode == Dive::RenderModeType::kBinning)))
        {
            // This is just to align the strings so that they are easier to read
            auto AppendSpace = [](std::string &str, uint32_t desired_len) {
                if (str.length() < desired_len)
                {
                    str.append(desired_len - str.length(), ' ');
                }
            };

            const uint32_t event_id = static_cast<uint32_t>(i);
            auto event_state_it = event_state.find(static_cast<Dive::EventStateId>(event_id));

            const uint32_t desired_draw_string_len = 64;
            std::string    draw_string = info.m_str;
            AppendSpace(draw_string, desired_draw_string_len);
            OutputDetails(draw_string + "\t");

            const bool  is_depth_test_enabled = event_state_it->DepthTestEnabled();
            const bool  is_depth_write_enabled = event_state_it->DepthWriteEnabled();
            VkCompareOp zfunc = event_state_it->DepthCompareOp();

            OutputDetails("DepthTest:" +
                          std::string(is_depth_test_enabled ? "Enabled\t" : "Disabled\t"));
            OutputDetails("DepthWrite:" +
                          std::string(is_depth_write_enabled ? "Enabled\t" : "Disabled\t"));

            std::string zfunc_str = "Invalid";
            switch (zfunc)
            {
            case VK_COMPARE_OP_NEVER:
                zfunc_str = "Never";
                break;
            case VK_COMPARE_OP_LESS:
                zfunc_str = "Less";
                break;
            case VK_COMPARE_OP_EQUAL:
                zfunc_str = "Equal";
                break;
            case VK_COMPARE_OP_LESS_OR_EQUAL:
                zfunc_str = "Less or Equal";
                break;
            case VK_COMPARE_OP_GREATER:
                zfunc_str = "Greater";
                break;
            case VK_COMPARE_OP_NOT_EQUAL:
                zfunc_str = "Not Equal";
                break;
            case VK_COMPARE_OP_GREATER_OR_EQUAL:
                zfunc_str = "Greater or Equal";
                break;
            case VK_COMPARE_OP_ALWAYS:
                zfunc_str = "Always";
                break;
            default:
                DIVE_ASSERT(false);
                break;
            }
            const uint32_t desired_zfunc_str_len = 16;
            AppendSpace(zfunc_str, desired_zfunc_str_len);
            OutputDetails("DepthFunc:" + zfunc_str + "\t");
            if (is_depth_test_enabled)
            {
                const bool lrz_enabled = event_state_it->LRZEnabled();
                if (!lrz_enabled)
                {
                    OutputDetails("LRZ:Disabled\t");
                    // if depth func is Always or Never, we don't really care about LRZ
                    if (is_depth_test_enabled &&
                        ((zfunc != VK_COMPARE_OP_NEVER) && (zfunc != VK_COMPARE_OP_ALWAYS)))
                    {
                        lrz_test_passed = false;
                        OutputDetails("[WARNING!] LRZ is disabled with performance penalties!");
                    }
                }
                else
                {
                    OutputDetails("LRZ:Enabled\t");
                }
            }
            else
            {
                OutputDetails("LRZ:Disabled\t");
            }
            OutputDetails("\n");
        }
    }
    return lrz_test_passed;
}

int main(int argc, char **argv)
{
    Pm4InfoInit();

    // Handle args
    if ((argc != 2) && (argc != 3))
    {
        std::cout << "You need to call: lrz_validator <input_file_name.rd> "
                     "<output_details_file_name.txt>(optional)";
        return 0;
    }
    char *input_file_name = argv[1];

    std::string output_file_name = "";
    if (argc == 3)
    {
        output_file_name = argv[2];
    }

    // Load capture
    std::unique_ptr<Dive::DataCore> data_core = std::make_unique<Dive::DataCore>();
    Dive::CaptureData::LoadResult   load_res = data_core->LoadPm4CaptureData(input_file_name);
    if (load_res != Dive::CaptureData::LoadResult::kSuccess)
    {
        std::cout << "Loading capture \"" << input_file_name << "\" failed!";
        return 0;
    }
    std::cout << "Capture file \"" << input_file_name << "\" is loaded!\n";

    // Create meta data
    if (!data_core->CreateMetaData())
    {
        std::cout << "Failed to create meta data!";
        return 0;
    }
    std::cout << "Validating LRZ...\n";

    // LRZ Validation
    const Dive::CaptureMetadata &meta_data = data_core->GetCaptureMetadata();
    const bool                   lrz_test_passed = ValidataLRZ(meta_data, output_file_name);
    if (lrz_test_passed)
    {
        std::cout << "[LRZ Pass] LRZ is correctly set for all drawcalls!\n";
    }
    else
    {
        std::cout << "[LRZ Fail] Some drawcalls have LRZ disabled but depth test is enabled and "
                     "depth func is not set to NEVER or ALWAYS!\n";
    }

    return 1;
}
