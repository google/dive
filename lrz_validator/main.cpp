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
#include <vector>

#include "dive_core/data_core.h"
#include "pm4_info.h"

int main(int argc, char **argv)
{
    Pm4InfoInit();
    if ((argc != 2) && (argc != 3))
    {
        std::cout
        << "You need to call: lrz_validator <input_file_name.rd> <output_file_name.txt>(optional)";
        return 0;
    }
    char *input_file_name = argv[1];

    std::string output_file_name = "lrz_validation_result.txt";
    if (argc == 3)
    {
        output_file_name = argv[2];
    }

    Dive::LogCompound               log_compound;
    std::unique_ptr<Dive::DataCore> data_core = std::make_unique<Dive::DataCore>(&log_compound);
    Dive::CaptureData::LoadResult   load_res = data_core->LoadCaptureData(input_file_name);
    if (load_res != Dive::CaptureData::LoadResult::kSuccess)
    {
        std::cout << "Loading capture " << argv[1] << " failed!";
        return 0;
    }

    if (!data_core->CreateMetaData())
    {
        std::cout << "Failed to create meta data!";
        return 0;
    }

    std::cout << "Output validation result to " << output_file_name << std::endl;
    std::ofstream                output_file(output_file_name);
    const Dive::CaptureMetadata &meta_data = data_core->GetCaptureMetadata();
    size_t                       event_count = meta_data.m_event_info.size();
    const Dive::EventStateInfo  &event_state = meta_data.m_event_state;
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
            output_file << draw_string + "\t";

            const bool  is_depth_test_enabled = event_state_it->DepthTestEnabled();
            const bool  is_depth_write_enabled = event_state_it->DepthWriteEnabled();
            VkCompareOp zfunc = event_state_it->DepthCompareOp();

            if (is_depth_test_enabled)
            {
                output_file << "DepthTest:Enabled\t";
            }
            else
            {
                output_file << "DepthTest:Disabled\t";
            }

            if (is_depth_write_enabled)
            {
                output_file << "DepthWrite:Enabled\t";
            }
            else
            {
                output_file << "DepthWrite:Disabled\t";
            }

            std::string zfunc_str = "Invalid";
            switch (zfunc)
            {
            case VK_COMPARE_OP_NEVER: zfunc_str = "Never"; break;
            case VK_COMPARE_OP_LESS: zfunc_str = "Less"; break;
            case VK_COMPARE_OP_EQUAL: zfunc_str = "Equal"; break;
            case VK_COMPARE_OP_LESS_OR_EQUAL: zfunc_str = "Less or Equal"; break;
            case VK_COMPARE_OP_GREATER: zfunc_str = "Greater"; break;
            case VK_COMPARE_OP_NOT_EQUAL: zfunc_str = "Not Equal"; break;
            case VK_COMPARE_OP_GREATER_OR_EQUAL: zfunc_str = "Greater or Equal"; break;
            case VK_COMPARE_OP_ALWAYS: zfunc_str = "Always"; break;
            default: DIVE_ASSERT(false); break;
            }
            const uint32_t desired_zfunc_str_len = 16;
            AppendSpace(zfunc_str, desired_zfunc_str_len);
            output_file << "DepthFunc:" + zfunc_str + "\t";
            if (is_depth_test_enabled)
            {
                const bool lrz_enabled = event_state_it->LRZEnabled();
                if (!lrz_enabled)
                {
                    output_file << "LRZ:Disabled\t";
                    // if depth func is Always or Never, we don't really care about LRZ
                    if (is_depth_test_enabled &&
                        ((zfunc != VK_COMPARE_OP_NEVER) && (zfunc != VK_COMPARE_OP_ALWAYS)))
                    {
                        output_file
                        << "[WARNING!!!] LRZ is disabled for this drawcall but depth test is "
                           "enabled, and depth func is not Never or Always!";
                    }
                }
                else
                {
                    output_file << "LRZ:Enabled\t";
                }
            }
            else
            {
                output_file << "LRZ:Disabled\t";
            }
            output_file << std::endl;
        }
    }

    return 1;
}
