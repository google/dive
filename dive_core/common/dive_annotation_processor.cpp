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

DiveAnnotationProcessor::DiveAnnotationProcessor() {}

DiveAnnotationProcessor::~DiveAnnotationProcessor() {}

void DiveAnnotationProcessor::Destroy() {}

void DiveAnnotationProcessor::WriteBlockEnd(gfxrecon::util::DiveFunctionData function_data)
{
    std::string function_name = function_data.GetFunctionName();

    if (function_name == "vkQueueSubmit" || function_name == "vkQueueSubmit2")
    {
        std::unique_ptr<SubmitInfo> submit_ptr = std::make_unique<SubmitInfo>(function_name);

        for (auto it = m_pre_submit_commands.begin(); it != m_pre_submit_commands.end(); ++it)
        {
            submit_ptr->AppendVkCmd(*it);
        }
        m_pre_submit_commands.clear();
        submit_ptr->SetCommandBufferCount(m_command_buffer_count);
        m_submits.push_back(std::move(submit_ptr));
        m_curr_submit = nullptr;
        m_command_buffer_count = 0;
    }
    else if (function_name.find("vkCmd") != std::string::npos ||
             function_name.find("vkBeginCommandBuffer") != std::string::npos)
    {
        // Don't include the vkEndCommandBuffer call.
        if (function_name.find("vkEndCommandBuffer") != std::string::npos)
        {
            return;
        }

        VulkanCommandInfo vkCmd(function_data);

        if (function_name.find("vkBeginCommandBuffer") != std::string::npos)
        {
            m_command_buffer_count++;
        }

        if (m_curr_submit)  // Check if the pointer is not null.
        {
            m_curr_submit->AppendVkCmd(vkCmd);
        }
        else
        {
            // No active submit, so buffer the command.
            m_pre_submit_commands.push_back(vkCmd);
        }
    }
}

// Leave this function empty. It is necessary to avoid further changes to the gfxr file_processor.
void DiveAnnotationProcessor::ProcessAnnotation(uint64_t                         block_index,
                                                gfxrecon::format::AnnotationType type,
                                                const std::string&               label,
                                                const std::string&               data)
{
}
