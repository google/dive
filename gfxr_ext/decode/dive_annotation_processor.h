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
    public:
        VulkanCommandInfo(const std::string& name, uint32_t index) :
            m_args(),
            m_name(name),
            m_index(index),
            m_cmd_count(0)
        {
        }

        VulkanCommandInfo(const gfxrecon::util::DiveFunctionData& data) :
            m_args(data.GetArgs()),
            m_name(data.GetFunctionName()),
            m_index(data.GetCmdBufferIndex()),
            m_cmd_count(0)
        {
        }

        VulkanCommandInfo() :
            m_args({}),
            m_name(""),
            m_index(0),
            m_cmd_count(0)
        {
        }
        const std::string&            GetVkCmdName() const { return m_name; }
        uint32_t                      GetVkCmdIndex() const { return m_index; }
        void                          SetCmdCount(uint32_t cmd_count) { m_cmd_count = cmd_count; }
        uint32_t                      GetCmdCount() const { return m_cmd_count; }
        const nlohmann::ordered_json& GetArgs() const { return m_args; }

    private:
        nlohmann::ordered_json m_args;
        std::string            m_name;
        uint32_t               m_index;
        uint32_t               m_cmd_count;  // Only used by vkBeginCommandBuffers
    };

    struct SubmitInfo
    {
    public:
        SubmitInfo() = default;

        SubmitInfo(const std::string& name) :
            m_name(name)
        {
        }

        const std::string& GetSubmitText() const { return m_name; }

        size_t GetCommandBufferCount() const { return m_vk_command_buffer_handles.size(); }
        const std::vector<VulkanCommandInfo>& GetNoneCmdVkCommands() const
        {
            return m_none_cmd_vk_commands;
        }
        void TakeNoneCmdVkCommands(std::vector<VulkanCommandInfo>& commands)
        {
            m_none_cmd_vk_commands = std::move(commands);
        }

        void TakeVkCommandBufferHandles(std::vector<uint64_t>& handles)
        {
            m_vk_command_buffer_handles = std::move(handles);
        }

        const std::vector<uint64_t>& GetCommandBufferHandles() const
        {
            return m_vk_command_buffer_handles;
        }

    private:
        std::vector<VulkanCommandInfo> m_none_cmd_vk_commands{};
        std::vector<uint64_t>          m_vk_command_buffer_handles{};
        std::string                    m_name{ "" };
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
    std::vector<VulkanCommandInfo> m_none_cmd_vk_commands_per_submit_cache;
    std::unordered_map<uint64_t, std::vector<VulkanCommandInfo>> m_cmd_vk_commands_cache;
    std::vector<std::unique_ptr<SubmitInfo>>                     m_submits;
};
