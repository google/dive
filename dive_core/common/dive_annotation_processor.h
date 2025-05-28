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
#include "dive_core/stl_replacement.h"
#include "third_party/gfxreconstruct/framework/decode/annotation_handler.h"
#include "util/defines.h"
#include "util/platform.h"

struct ApiCallInfo;

/// Manages writing
class DiveAnnotationProcessor : public gfxrecon::decode::AnnotationHandler
{
public:
    struct VulkanCommandInfo
    {
    public:
        VulkanCommandInfo(const std::string& name, uint32_t index)
        {
            m_name = name;
            m_index = index;
        }
        VulkanCommandInfo(const gfxrecon::util::DiveFunctionData& data)
        {
            m_name = data.GetFunctionName();
            m_index = data.GetCmdBufferIndex();
            m_args = data.GetArgs();
        }
        const std::string&            GetVkCmdName() const { return m_name; }
        uint32_t                      GetVkCmdIndex() const { return m_index; }
        void                          SetCmdCount(uint32_t cmd_count) { m_cmd_count = cmd_count; }
        uint32_t                      GetCmdCount() const { return m_cmd_count; }
        const nlohmann::ordered_json& GetArgs() const { return m_args; }

    private:
        std::string            m_name;
        uint32_t               m_index;
        uint32_t               m_cmd_count;  // Only used by vkBeginCommandBuffers
        nlohmann::ordered_json m_args;
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
        void               SetCommandBufferCount(uint32_t cmd_buffer_count)
        {
            m_cmd_buffer_count = cmd_buffer_count;
        }
        uint32_t GetCommandBufferCount() const { return m_cmd_buffer_count; }
        const DiveVector<VulkanCommandInfo>& GetVkCmds() const { return vulkan_cmds; }
        void AppendVkCmd(VulkanCommandInfo vkCmd) { vulkan_cmds.push_back(vkCmd); }

    private:
        std::string                   m_name;
        uint32_t                      m_cmd_buffer_count;
        DiveVector<VulkanCommandInfo> vulkan_cmds;
    };

    DiveAnnotationProcessor();
    ~DiveAnnotationProcessor();

    void EndStream();
    bool IsValid() const;

    /// Finalise the current block and stream it out.
    void WriteBlockEnd(const gfxrecon::util::DiveFunctionData& function_data) override;

    void WriteMarker(const char* name, const std::string_view marker_type, uint64_t frame_number);

    /// @brief Convert annotations, which are simple {type:enum, key:string, value:string} objects.
    virtual void ProcessAnnotation(uint64_t                         block_index,
                                   gfxrecon::format::AnnotationType type,
                                   const std::string&               label,
                                   const std::string&               data) override;

    bool WriteBinaryFile(const std::string& filename, uint64_t data_size, const uint8_t* data);

    inline void SetCurrentBlockIndex(uint64_t block_index) { m_block_index_ = block_index; }

    DiveVector<std::unique_ptr<SubmitInfo>> getSubmits() { return std::move(m_submits); }

private:
    DiveVector<std::unique_ptr<SubmitInfo>> m_submits;
    SubmitInfo*                             m_curr_submit = nullptr;
    uint32_t                                m_command_buffer_count = 0;
    std::vector<VulkanCommandInfo> m_pre_submit_commands;  // Buffer for commands before a submit
    uint64_t                       m_block_index_;
};
