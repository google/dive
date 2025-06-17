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

#include "gfxr_vulkan_command_hierarchy.h"
#include "dive_core/common/emulate_pm4.h"
#include "dive_strings.h"

namespace Dive
{

// =================================================================================================
// GfxrVulkanCommandHierarchyCreator
// =================================================================================================
GfxrVulkanCommandHierarchyCreator::GfxrVulkanCommandHierarchyCreator(
CommandHierarchy &command_hierarchy,
CaptureData      &capture_data) :
    m_command_hierarchy(command_hierarchy),
    m_capture_data(capture_data)
{
}

//--------------------------------------------------------------------------------------------------
bool GfxrVulkanCommandHierarchyCreator::CreateTrees()
{
    // Clear/Reset internal data structures, just in case
    m_command_hierarchy = CommandHierarchy();

    // Add a dummy root node for easier management
    uint64_t root_node_index = AddNode(NodeType::kRootNode, "");
    DIVE_VERIFY(root_node_index == Topology::kRootNodeIndex);

    if (!ProcessGfxrSubmits(m_capture_data.GetGfxrSubmits(), m_capture_data.GetMemoryManager()))
    {
        return false;
    }
    // Convert the info in m_node_children into GfxrVulkanCommandHierarchy's topologies
    CreateTopologies();

    return true;
}

//--------------------------------------------------------------------------------------------------
uint64_t GfxrVulkanCommandHierarchyCreator::AddNode(NodeType type, std::string &&desc)
{
    uint64_t node_index = m_command_hierarchy.AddGfxrNode(type, std::move(desc));

    for (uint32_t i = 0; i < CommandHierarchy::kTopologyTypeCount; ++i)
    {
        DIVE_ASSERT(m_node_children[i][kDirectChildren].size() == node_index);
        m_node_children[i][kDirectChildren].resize(m_node_children[i][kDirectChildren].size() + 1);

        m_node_root_node_index[i].resize(m_node_root_node_index[i].size() + 1);
    }
    return node_index;
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandHierarchyCreator::AddChild(CommandHierarchy::TopologyType type,
                                                 uint64_t                       node_index,
                                                 uint64_t                       child_node_index)
{
    DIVE_ASSERT(node_index < m_node_children[type][kDirectChildren].size());
    m_node_children[type][kDirectChildren][node_index].push_back(child_node_index);
}

void GfxrVulkanCommandHierarchyCreator::GetArgs(const nlohmann::ordered_json &json_args,
                                                uint64_t                      curr_index,
                                                const std::string            &current_path)
{
    if (json_args.is_object())
    {
        for (auto const &[key, val] : json_args.items())
        {
            if (val.is_object())
            {
                uint64_t object_node_index = AddNode(NodeType::kGfxrVulkanCommandArgNode,
                                                     key.c_str());
                AddChild(CommandHierarchy::TopologyType::kSubmitTopology,
                         curr_index,
                         object_node_index);

                GetArgs(val, object_node_index, "");
            }
            else if (val.is_array())
            {
                uint64_t array_node_index = AddNode(NodeType::kGfxrVulkanCommandArgNode,
                                                    key.c_str());
                AddChild(CommandHierarchy::TopologyType::kSubmitTopology,
                         curr_index,
                         array_node_index);
                for (size_t i = 0; i < val.size(); ++i)
                {
                    const auto &element = val[i];
                    if (element.is_object())
                    {
                        GetArgs(element, array_node_index, "");
                    }
                    else if (element.is_array())
                    {
                        uint64_t
                        nested_array_node_index = AddNode(NodeType::kGfxrVulkanCommandArgNode,
                                                          "element_" + std::to_string(i));
                        AddChild(CommandHierarchy::TopologyType::kSubmitTopology,
                                 array_node_index,
                                 nested_array_node_index);
                        GetArgs(element, nested_array_node_index, "");
                    }
                    else
                    {
                        std::ostringstream vk_cmd_arg_string_stream;
                        vk_cmd_arg_string_stream << element;
                        uint64_t arg_index = AddNode(NodeType::kGfxrVulkanCommandArgNode,
                                                     vk_cmd_arg_string_stream.str());
                        AddChild(CommandHierarchy::TopologyType::kSubmitTopology,
                                 array_node_index,
                                 arg_index);
                    }
                }
            }
            else
            {
                std::ostringstream vk_cmd_arg_string_stream;
                vk_cmd_arg_string_stream << key << ":" << val;  // key:value format for primitive
                uint64_t vk_cmd_arg_index = AddNode(NodeType::kGfxrVulkanCommandArgNode,
                                                    vk_cmd_arg_string_stream.str());
                AddChild(CommandHierarchy::TopologyType::kSubmitTopology,
                         curr_index,
                         vk_cmd_arg_index);
            }
        }
    }
    else if (json_args.is_array())
    {
        for (size_t i = 0; i < json_args.size(); ++i)
        {
            const auto &element = json_args[i];
            if (element.is_object() || element.is_array())
            {
                GetArgs(element, curr_index, "");
            }
            else
            {
                std::ostringstream vk_cmd_arg_string_stream;
                vk_cmd_arg_string_stream << element;
                uint64_t arg_index = AddNode(NodeType::kGfxrVulkanCommandArgNode,
                                             vk_cmd_arg_string_stream.str());
                AddChild(CommandHierarchy::TopologyType::kSubmitTopology, curr_index, arg_index);
            }
        }
    }
    else
    {
        if (!current_path.empty())
        {
            std::ostringstream vk_cmd_arg_string_stream;
            vk_cmd_arg_string_stream << current_path << ":" << json_args;
            uint64_t vk_cmd_arg_index = AddNode(NodeType::kGfxrVulkanCommandArgNode,
                                                vk_cmd_arg_string_stream.str());
            AddChild(CommandHierarchy::TopologyType::kSubmitTopology, curr_index, vk_cmd_arg_index);
        }
    }
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandHierarchyCreator::OnCommand(
uint32_t                                   parent_index,
DiveAnnotationProcessor::VulkanCommandInfo vk_cmd_info)
{
    std::ostringstream vk_cmd_string_stream;
    vk_cmd_string_stream << vk_cmd_info.GetVkCmdName();
    if (vk_cmd_info.GetVkCmdName() == "vkBeginCommandBuffer" ||
        vk_cmd_info.GetVkCmdName() == "vkEndCommandBuffer")
    {
        uint64_t cmd_buffer_index = AddNode(NodeType::kGfxrVulkanCommandBufferNode,
                                            vk_cmd_string_stream.str());
        m_cur_command_buffer_node_index = cmd_buffer_index;
        GetArgs(vk_cmd_info.GetArgs(), m_cur_command_buffer_node_index, "");
        AddChild(CommandHierarchy::TopologyType::kSubmitTopology,
                 m_cur_submit_node_index,
                 cmd_buffer_index);
    }
    else
    {
        uint64_t vk_cmd_index = AddNode(NodeType::kGfxrVulkanCommandNode,
                                        vk_cmd_string_stream.str());
        GetArgs(vk_cmd_info.GetArgs(), vk_cmd_index, "");
        AddChild(CommandHierarchy::TopologyType::kSubmitTopology,
                 m_cur_command_buffer_node_index,
                 vk_cmd_index);
    }
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandHierarchyCreator::OnGfxrSubmit(
uint32_t                                   submit_index,
const DiveAnnotationProcessor::SubmitInfo &submit_info)
{
    std::ostringstream submit_string_stream;
    submit_string_stream << submit_info.GetSubmitText() << ": " << submit_index;
    submit_string_stream << ", Command Buffer Count: "
                         << std::to_string(submit_info.GetCommandBufferCount());
    // Create submit node
    uint64_t submit_node_index = AddNode(NodeType::kGfxrVulkanSubmitNode,
                                         submit_string_stream.str());

    // Add submit node to the other topologies as children to the root node
    AddChild(CommandHierarchy::kSubmitTopology, Topology::kRootNodeIndex, submit_node_index);
    AddChild(CommandHierarchy::kAllEventTopology, Topology::kRootNodeIndex, submit_node_index);
    m_cur_submit_node_index = submit_node_index;
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandHierarchyCreator::CreateTopologies()
{
    uint64_t total_num_children[CommandHierarchy::kTopologyTypeCount] = {};

    // Convert the m_node_children temporary structure into CommandHierarchy's topologies
    for (uint32_t topology = 0; topology < CommandHierarchy::kTopologyTypeCount; ++topology)
    {
        size_t    num_nodes = m_node_children[topology][kDirectChildren].size();
        Topology &cur_topology = m_command_hierarchy.m_topology[topology];
        cur_topology.SetNumNodes(num_nodes);

        if (total_num_children[topology] == 0)
        {
            for (uint64_t node_index = 0; node_index < num_nodes; ++node_index)
            {
                auto &node_children = m_node_children[topology];
                total_num_children[topology] += node_children[kDirectChildren][node_index].size();
            }
        }

        cur_topology.m_children_list.reserve(total_num_children[topology]);

        for (uint64_t node_index = 0; node_index < num_nodes; ++node_index)
        {
            cur_topology.AddChildren(node_index,
                                     m_node_children[topology][kDirectChildren][node_index]);
        }
    }
}
}  // namespace Dive
