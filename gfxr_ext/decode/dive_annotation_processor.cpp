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

#include "dive_annotation_processor.h"
#include <cstdint>
#include <iostream>
#include <ostream>
#include "decode/api_decoder.h"
#include "util/logging.h"
#include "util/output_stream.h"

void DiveAnnotationProcessor::WriteBlockEnd(const gfxrecon::util::DiveFunctionData& function_data)
{
    std::string function_name = function_data.GetFunctionName();
    const auto& args = function_data.GetArgs();

    if (function_name == "vkQueueSubmit" || function_name == "vkQueueSubmit2")
    {
        std::unique_ptr<SubmitInfo> submit_ptr = std::make_unique<SubmitInfo>(function_name);

        if (args.count("submitCount"))
        {
            const auto& submits = args["pSubmits"];
            for (const auto& submit : submits)
            {
                if (submit.count("pCommandBuffers"))
                {
                    const auto& command_buffers = submit["pCommandBuffers"];
                    for (const auto& cmd_buffer : command_buffers)
                    {
                        submit_ptr->vk_command_buffer_handles.push_back(cmd_buffer);
                    }
                }
            }
        }
        submit_ptr->none_cmd_vk_commands = std::move(m_none_cmd_vk_commands_per_submit_cache);
        m_submits.push_back(std::move(submit_ptr));
    }
    else
    {
        VulkanCommandInfo vkCmd(function_data);
        if (args.count("commandBuffer") != 0)
        {
            uint64_t cmd_handle = args["commandBuffer"];

            // There could be cases where vkBeginCommandBuffer is called after vkEndCommandBuffer
            // without having the command buffer submitted
            if (vkCmd.name == "vkBeginCommandBuffer")
            {
                m_cmd_vk_commands_cache[cmd_handle].clear();
            }

            m_cmd_vk_commands_cache[cmd_handle].push_back(vkCmd);
        }
        else
        {
            m_none_cmd_vk_commands_per_submit_cache.push_back(vkCmd);
        }
    }
}
