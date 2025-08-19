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

#include "dive_command_hierarchy.h"
#include "dive_core/common/emulate_pm4.h"
#include "dive_strings.h"
#include <cstdint>
#include <iostream>

namespace Dive
{

// =================================================================================================
// DiveCommandHierarchyCreator
// =================================================================================================
DiveCommandHierarchyCreator::DiveCommandHierarchyCreator(
CommandHierarchy                  &command_hierarchy,
GfxrVulkanCommandHierarchyCreator &gfxr_command_hierarchy_creator,
CommandHierarchyCreator           &pm4_command_hierarchy_creator) :
    m_command_hierarchy(command_hierarchy),
    m_gfxr_command_hierarchy_creator(gfxr_command_hierarchy_creator),
    m_pm4_command_hierarchy_creator(pm4_command_hierarchy_creator)
{
}

//--------------------------------------------------------------------------------------------------
bool DiveCommandHierarchyCreator::ProcessDiveSubmits(
const DiveVector<SubmitInfo>                                            &submits,
const IMemoryManager                                                    &mem_manager,
const std::vector<std::unique_ptr<DiveAnnotationProcessor::SubmitInfo>> &gfxr_submits)
{
    bool result = m_pm4_command_hierarchy_creator.ProcessSubmits(submits, mem_manager);

    if (result != false)
    {
        result = m_gfxr_command_hierarchy_creator.ProcessGfxrSubmits(gfxr_submits);
    }
    else
    {
        return result;
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
bool DiveCommandHierarchyCreator::CreateTrees(DiveCaptureData        &dive_capture_data,
                                              bool                    flatten_chain_nodes,
                                              std::optional<uint64_t> reserve_size)
{
    m_pm4_command_hierarchy_creator.CreateTrees(flatten_chain_nodes, false, reserve_size);

    m_gfxr_command_hierarchy_creator.CreateTrees(true);

    if (!ProcessDiveSubmits(dive_capture_data.GetPm4CaptureData().GetSubmits(),
                            dive_capture_data.GetPm4CaptureData().GetMemoryManager(),
                            dive_capture_data.GetGfxrCaptureData().GetGfxrSubmits()))
    {
        return false;
    }

    CreateTopologies();

    return true;
}

//--------------------------------------------------------------------------------------------------
void DiveCommandHierarchyCreator::OnCommand(uint32_t                                   parent_index,
                                            DiveAnnotationProcessor::VulkanCommandInfo vk_cmd_info)
{
    m_gfxr_command_hierarchy_creator.OnCommand(parent_index, vk_cmd_info);
}

//--------------------------------------------------------------------------------------------------
void DiveCommandHierarchyCreator::OnGfxrSubmit(
uint32_t                                   submit_index,
const DiveAnnotationProcessor::SubmitInfo &submit_info)
{
    m_gfxr_command_hierarchy_creator.OnGfxrSubmit(submit_index, submit_info);
}

//--------------------------------------------------------------------------------------------------
void DiveCommandHierarchyCreator::CreateTopologies()
{
    uint64_t total_num_children[CommandHierarchy::kTopologyTypeCount] = {};
    uint64_t total_num_shared_children[CommandHierarchy::kTopologyTypeCount] = {};

    // Convert the m_node_children temporary structure into CommandHierarchy's topologies
    for (uint32_t topology = 0; topology < CommandHierarchy::kTopologyTypeCount; ++topology)
    {
        size_t num_pm4_nodes = m_pm4_command_hierarchy_creator.GetNodeChildren(topology, 0).size();
        size_t total_num_nodes = num_pm4_nodes +
                                 m_gfxr_command_hierarchy_creator.GetNodeChildren(topology).size();

        SharedNodeTopology &cur_topology = m_command_hierarchy.m_topology[topology];
        cur_topology.SetNumNodes(total_num_nodes);

        // Optional loop: Pre-reserve to prevent the resize() from allocating memory later
        // Note: The number of children for some of the topologies have been determined
        // earlier in this function already
        if (total_num_children[topology] == 0 && total_num_shared_children[topology] == 0)
        {
            for (uint64_t node_index = 0; node_index < num_pm4_nodes; ++node_index)
            {
                total_num_children[topology] += m_pm4_command_hierarchy_creator
                                                .GetNodeChildren(topology, 0)[node_index]
                                                .size();
                total_num_shared_children[topology] += m_pm4_command_hierarchy_creator
                                                       .GetNodeChildren(topology, 1)[node_index]
                                                       .size();
            }
        }
        cur_topology.m_children_list.reserve(total_num_children[topology]);
        cur_topology.m_shared_children_indices.reserve(total_num_shared_children[topology]);

        DiveVector<uint64_t> combined_root_children = m_pm4_command_hierarchy_creator
                                                      .GetNodeChildren(topology, 0)[0];

        for (uint64_t node_index = 1; node_index < num_pm4_nodes; ++node_index)
        {
            DIVE_ASSERT(m_pm4_command_hierarchy_creator.GetNodeChildren(topology, 0).size() ==
                        m_pm4_command_hierarchy_creator.GetNodeChildren(topology, 1).size());
            cur_topology
            .AddChildren(node_index,
                         m_pm4_command_hierarchy_creator.GetNodeChildren(topology, 0)[node_index]);
            cur_topology.AddSharedChildren(node_index,
                                           m_pm4_command_hierarchy_creator
                                           .GetNodeChildren(topology, 1)[node_index]);
        }

        cur_topology.m_start_shared_child = std::move(
        m_pm4_command_hierarchy_creator.GetNodeStartSharedChildren(topology));
        cur_topology.m_end_shared_child = std::move(
        m_pm4_command_hierarchy_creator.GetNodeEndSharedChildren(topology));
        cur_topology.m_root_node_index = std::move(
        m_pm4_command_hierarchy_creator.GetNodeRootNodeIndices(topology));

        // Add the gfxr nodes to the topology.
        if (topology == CommandHierarchy::kAllEventTopology)
        {
            // Build the internal GFXR hierarchy for all non-submit nodes.
            // This includes the self-loop filter to ensure no submit nodes are included.
            const auto &gfxr_dive_indices = m_gfxr_command_hierarchy_creator
                                            .GetCreatedDiveIndices();
            for (uint64_t node_index = num_pm4_nodes; node_index < total_num_nodes; ++node_index)
            {
                uint64_t    gfxr_dive_index = gfxr_dive_indices.at(node_index);
                const auto &children = m_gfxr_command_hierarchy_creator.GetNodeChildren(
                topology)[gfxr_dive_index];

                DiveVector<uint64_t> filtered_children;
                for (uint64_t child_node : children)
                {
                    if (child_node != node_index)
                    {
                        if (m_command_hierarchy.GetNodeType(child_node) !=
                            NodeType::kGfxrVulkanSubmitNode)
                        {
                            filtered_children.push_back(child_node);
                        }
                    }
                }
                cur_topology.AddChildren(node_index, filtered_children);
            }

            // Identify ALL GFXR submit nodes.
            DiveVector<uint64_t> gfxr_submit_nodes;
            for (uint64_t node_index = num_pm4_nodes; node_index < total_num_nodes; ++node_index)
            {
                std::string desc = m_command_hierarchy.GetNodeDesc(node_index);
                if (m_command_hierarchy.GetNodeType(node_index) == NodeType::kGfxrVulkanSubmitNode)
                {
                    gfxr_submit_nodes.push_back(node_index);
                }
            }

            // Append GFXR submit nodes to the root's combined child list
            if (!gfxr_submit_nodes.empty())
            {
                for (uint64_t node : gfxr_submit_nodes)
                {
                    combined_root_children.push_back(node);
                }
            }
        }

        cur_topology.AddChildren(0, combined_root_children);

        // Ensure the topology is filled. This is necessary while a single vector is used to create
        // the mixed command hierarchy.
        cur_topology.m_start_shared_child.resize(total_num_nodes);
        cur_topology.m_end_shared_child.resize(total_num_nodes);
        cur_topology.m_root_node_index.resize(total_num_nodes);
    }
}

}  // namespace Dive
