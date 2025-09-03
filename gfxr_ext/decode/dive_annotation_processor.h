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

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include "decode/annotation_handler.h"
#include "util/defines.h"
#include "util/platform.h"

struct ApiCallInfo;

// The DiveAnnotationProcessor is used by the VulkanExportDiveConsumer on each WriteBlockEnd call
// made when processing the vulkan commands. WriteBlockEnd is called passing the function data
// (name, command buffer index, args) and then DiveAnnotationProcessor converts the data to
// SubmitInfo for vkQueueSubmits or VulkanCommandInfo for vulkan commands. These structs are then
// used to construct the command hierarchy displayed in the Dive UI.
class DiveAnnotationProcessor : public gfxrecon::decode::AnnotationHandler
{
public:
    struct VulkanCommandInfo
    {
        explicit VulkanCommandInfo(const gfxrecon::util::DiveFunctionData& data) :
            args(data.GetArgs()),
            name(data.GetFunctionName()),
            index(data.GetCmdBufferIndex())
        {
        }

        nlohmann::ordered_json args = {};
        std::string            name = "";
        uint32_t               index = 0;
    };

    struct SubmitInfo
    {
        explicit SubmitInfo(const std::string& function_name) :
            name(function_name)
        {
        }

        // Keeps all the vk commands that come before this submit and that are not associated with
        // any command buffer
        std::vector<VulkanCommandInfo> none_cmd_vk_commands = {};
        // Keep handles of all command buffers that is submitted by this submission
        std::vector<uint64_t> vk_command_buffer_handles = {};
        std::string           name = "";
    };

    DiveAnnotationProcessor() {}
    ~DiveAnnotationProcessor() {}

    // Finalize the current block and stream it out.
    void WriteBlockEnd(const gfxrecon::util::DiveFunctionData& function_data) override;

    // @brief Convert annotations, which are simple {type:enum, key:string, value:string} objects.
    virtual void ProcessAnnotation(uint64_t                         block_index,
                                   gfxrecon::format::AnnotationType type,
                                   const std::string&               label,
                                   const std::string&               data) override
    {
    }

    std::vector<std::unique_ptr<SubmitInfo>> TakeSubmits() { return std::move(m_submits); }
    std::unordered_map<uint64_t, std::vector<VulkanCommandInfo>> TakeVkCommandsCache()
    {
        return std::move(m_cmd_vk_commands_cache);
    }

private:
    // This is a per submit cache that keeps all vk commands that are not in any command buffer
    std::vector<VulkanCommandInfo> m_none_cmd_vk_commands_per_submit_cache = {};
    // Use command buffer handle as the key to accociate with vk commands
    std::unordered_map<uint64_t, std::vector<VulkanCommandInfo>> m_cmd_vk_commands_cache = {};
    std::vector<std::unique_ptr<SubmitInfo>>                     m_submits = {};
};
