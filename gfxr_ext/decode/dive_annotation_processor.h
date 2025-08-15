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
        void               SetCommandBufferCount(uint32_t command_buffer_count)
        {
            m_command_buffer_count = command_buffer_count;
        }
        uint32_t GetCommandBufferCount() const { return m_command_buffer_count; }
        const std::vector<VulkanCommandInfo>& GetVulkanCommands() const
        {
            return m_vulkan_commands;
        }
        void SetVulkanCommands(const std::vector<VulkanCommandInfo>& vulkan_commands)
        {
            m_vulkan_commands = std::move(vulkan_commands);
        }

    private:
        std::vector<VulkanCommandInfo> m_vulkan_commands{};
        std::string                    m_name{ "" };
        uint32_t                       m_command_buffer_count{ 0 };
    };

    DiveAnnotationProcessor() {}
    ~DiveAnnotationProcessor() {}

    void EndStream();
    bool IsValid() const;

    // Finalize the current block and stream it out.
    void WriteBlockEnd(const gfxrecon::util::DiveFunctionData& function_data) override;

    void WriteMarker(const char* name, const std::string_view marker_type, uint64_t frame_number);

    // @brief Convert annotations, which are simple {type:enum, key:string, value:string} objects.
    virtual void ProcessAnnotation(uint64_t                         block_index,
                                   gfxrecon::format::AnnotationType type,
                                   const std::string&               label,
                                   const std::string&               data) override
    {
    }

    bool WriteBinaryFile(const std::string& filename, uint64_t data_size, const uint8_t* data);

    std::vector<std::unique_ptr<SubmitInfo>> GetSubmits() { return std::move(m_submits); }

private:
    std::vector<VulkanCommandInfo>
    m_current_submit_commands;  // Buffer for commands before a submit
    std::vector<std::unique_ptr<SubmitInfo>> m_submits;
    uint32_t                                 m_current_submit_command_buffer_count = 0;
};
