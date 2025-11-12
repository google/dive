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

// =====================================================================================================================
// The VulkanCommandHierarchy class parses and creates a tree of Nodes in the command buffer. The
// primary client for this class is the Model class for the Vulkan command and argument Views in the
// UI.
// =====================================================================================================================

#include "dive_core/command_hierarchy.h"
#include "dive_core/common/emulate_pm4.h"
#include "dive_core/gfxr_capture_data.h"
#include <stack>

namespace Dive
{
class GfxrVulkanCommandHierarchyCreator
{
public:
    GfxrVulkanCommandHierarchyCreator(CommandHierarchy      &command_hierarchy,
                                      const GfxrCaptureData &capture_data);
    ~GfxrVulkanCommandHierarchyCreator();

    bool CreateTrees(bool used_in_mixed_command_hierarchy = false);
    bool ProcessGfxrSubmits(const GfxrCaptureData &capture_date);

    void OnGfxrSubmit(uint32_t                                   submit_index,
                      const DiveAnnotationProcessor::SubmitInfo &submit_info);
    void OnCommand(const DiveAnnotationProcessor::VulkanCommandInfo &vk_cmd_info,
                   uint64_t                                          draw_call_count,
                   std::vector<uint64_t>                            &render_pass_draw_call_counts);

    bool ProcessVkCmds(const std::vector<DiveAnnotationProcessor::VulkanCommandInfo> &vkCmds,
                       uint64_t                     draw_call_counts,
                       const std::vector<uint64_t> &render_pass_draw_call_counts);

    const DiveVector<DiveVector<uint64_t>> &GetNodeChildren(uint64_t type) const
    {
        return m_node_children[type];
    }

    const std::unordered_map<uint64_t, uint64_t> &GetCreatedDiveIndices() const
    {
        return m_dive_indices_to_local_indices_map;
    }

    void ClearCreatedDiveIndices() { m_dive_indices_to_local_indices_map.clear(); }

private:
    void     GetArgs(const nlohmann::ordered_json &j,
                     uint64_t                      curr_index,
                     const std::string            &current_path = "");
    void     CreateTopologies();
    uint64_t AddNode(NodeType type, std::string &&desc);
    void     AddChild(CommandHierarchy::TopologyType type,
                      uint64_t                       node_index,
                      uint64_t                       child_node_index);
    void     ConditionallyAddChild(uint64_t node_index);

    uint64_t               m_cur_submit_node_index = 0;
    uint64_t               m_cur_command_buffer_node_index = 0;
    std::stack<uint64_t>   m_cur_parent_node_index_stack;
    CommandHierarchy      &m_command_hierarchy;
    const GfxrCaptureData &m_capture_data;
    // This is a list of child indices per node, ie. topology info
    // Once parsing is complete, we will create a topology from this
    DiveVector<DiveVector<uint64_t>> m_node_children[CommandHierarchy::kTopologyTypeCount];
    DiveVector<uint64_t>             m_node_root_node_indices[CommandHierarchy::kTopologyTypeCount];
    Topology                         m_topology[CommandHierarchy::kTopologyTypeCount];
    bool                             m_used_in_mixed_command_hierarchy = false;
    std::unordered_map<uint64_t, uint64_t> m_dive_indices_to_local_indices_map;
};
}  // namespace Dive
