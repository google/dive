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

// =====================================================================================================================
// The VulkanCommandHierarchy class parses and creates a tree of Nodes in the command buffer. The
// primary client for this class is the Model class for the Vulkan command and argument Views in the
// UI.
// =====================================================================================================================

#include "dive_core/command_hierarchy.h"

namespace Dive
{
class GfxrVulkanCommandHierarchyCreator : public IEmulateCallbacks
{
public:
    GfxrVulkanCommandHierarchyCreator(CommandHierarchy &command_hierarchy,
                                      CaptureData      &capture_data);

    bool CreateTrees();

private:
    void     GetArgs(const nlohmann::ordered_json &j,
                     uint64_t                      curr_index,
                     const std::string            &current_path = "");
    void     CreateTopologies();
    void     OnGfxrSubmit(uint32_t                                   submit_index,
                          const DiveAnnotationProcessor::SubmitInfo &submit_info);
    uint64_t AddNode(NodeType type, std::string &&desc);
    void     AddChild(CommandHierarchy::TopologyType type,
                      uint64_t                       node_index,
                      uint64_t                       child_node_index);
    void OnCommand(uint32_t submit_index, DiveAnnotationProcessor::VulkanCommandInfo vk_cmd_info);
    uint64_t          m_cur_submit_node_index = 0;
    uint64_t          m_cur_command_buffer_node_index = 0;
    CommandHierarchy &m_command_hierarchy;
    CaptureData      &m_capture_data;
    // This is a list of child indices per node, ie. topology info
    // Once parsing is complete, we will create a topology from this
    DiveVector<DiveVector<uint64_t>> m_node_children[CommandHierarchy::kTopologyTypeCount];
    DiveVector<uint64_t>             m_node_root_node_index[CommandHierarchy::kTopologyTypeCount];
};
}  // namespace Dive
