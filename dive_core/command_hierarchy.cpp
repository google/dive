
/*
 Copyright 2019 Google LLC

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

#include "command_hierarchy.h"
#include <assert.h>
#include <algorithm>  // std::transform
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include "dive_core/common/common.h"
#include "dive_core/common/pm4_packets/me_pm4_packets.h"
#include "pm4_capture_data.h"

#include "dive_strings.h"
#include "pm4_info.h"

namespace Dive
{

// =================================================================================================
// Topology
// =================================================================================================
uint64_t Topology::GetNumNodes() const
{
    DIVE_ASSERT(m_node_children.size() == m_node_parent.size());
    DIVE_ASSERT(m_node_children.size() == m_node_child_index.size());
    return m_node_children.size();
}

//--------------------------------------------------------------------------------------------------
uint64_t Topology::GetParentNodeIndex(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_node_parent.size());
    return m_node_parent[node_index];
}
//--------------------------------------------------------------------------------------------------
uint64_t Topology::GetChildIndex(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_node_child_index.size());
    return m_node_child_index[node_index];
}
//--------------------------------------------------------------------------------------------------
uint64_t Topology::GetNumChildren(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_node_children.size());
    return m_node_children[node_index].m_num_children;
}
//--------------------------------------------------------------------------------------------------
uint64_t Topology::GetChildNodeIndex(uint64_t node_index, uint64_t child_index) const
{
    DIVE_ASSERT(node_index < m_node_children.size());
    DIVE_ASSERT(child_index < m_node_children[node_index].m_num_children);
    uint64_t child_list_index = m_node_children[node_index].m_start_index + child_index;
    DIVE_ASSERT(child_list_index < m_children_list.size());
    return m_children_list[child_list_index];
}

//--------------------------------------------------------------------------------------------------
uint64_t Topology::GetNextNodeIndex(uint64_t node_index) const
{
    uint64_t num_children = GetNumChildren(node_index);
    if (num_children > 0)
        return GetChildNodeIndex(node_index, 0);
    while (true)
    {
        if (node_index == kRootNodeIndex)
            return UINT64_MAX;
        uint64_t parent_node_index = GetParentNodeIndex(node_index);
        uint64_t sibling_index = GetChildIndex(node_index) + 1;
        if (sibling_index < GetNumChildren(parent_node_index))
            return GetChildNodeIndex(parent_node_index, sibling_index);
        node_index = parent_node_index;
    }
}

//--------------------------------------------------------------------------------------------------
void Topology::SetNumNodes(uint64_t num_nodes)
{
    m_node_children.resize(num_nodes);
    m_node_parent.resize(num_nodes, UINT64_MAX);
    m_node_child_index.resize(num_nodes, UINT64_MAX);
}

//--------------------------------------------------------------------------------------------------
void Topology::AddChildren(uint64_t node_index, const DiveVector<uint64_t> &children)
{
    DIVE_ASSERT(m_node_children.size() == m_node_parent.size());
    DIVE_ASSERT(m_node_children.size() == m_node_child_index.size());

    // Append to m_children_list
    uint64_t prev_size = m_children_list.size();
    m_children_list.resize(m_children_list.size() + children.size());
    std::copy(children.begin(), children.end(), m_children_list.begin() + prev_size);

    // Set "pointer" to children_list
    DIVE_ASSERT(m_node_children[node_index].m_num_children == 0);
    m_node_children[node_index].m_start_index = prev_size;
    m_node_children[node_index].m_num_children = children.size();

    // Set parent pointer and child_index for each child
    for (uint64_t i = 0; i < children.size(); ++i)
    {
        uint64_t child_node_index = children[i];
        DIVE_ASSERT(child_node_index < m_node_children.size());  // Sanity check

        // Each child can have only 1 parent
        DIVE_ASSERT(m_node_parent[child_node_index] == UINT64_MAX);
        DIVE_ASSERT(m_node_child_index[child_node_index] == UINT64_MAX);
        m_node_parent[child_node_index] = node_index;
        m_node_child_index[child_node_index] = i;
    }
}

// =================================================================================================
// SharedNodeTopology
// =================================================================================================
uint64_t SharedNodeTopology::GetNumNodes() const
{
    DIVE_ASSERT(m_node_children.size() == m_node_shared_children.size());
    DIVE_ASSERT(m_node_children.size() == m_node_parent.size());
    DIVE_ASSERT(m_node_children.size() == m_node_child_index.size());
    DIVE_ASSERT(m_node_children.size() == m_start_shared_child.size());
    DIVE_ASSERT(m_node_children.size() == m_end_shared_child.size());
    DIVE_ASSERT(m_node_children.size() == m_root_node_index.size());
    return m_node_children.size();
}

uint64_t SharedNodeTopology::GetNumSharedChildren(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_node_shared_children.size());
    return m_node_shared_children[node_index].m_num_children;
}
//--------------------------------------------------------------------------------------------------
uint64_t SharedNodeTopology::GetSharedChildNodeIndex(uint64_t node_index,
                                                     uint64_t child_index) const
{
    DIVE_ASSERT(node_index < m_node_shared_children.size());
    DIVE_ASSERT(child_index < m_node_shared_children[node_index].m_num_children);
    uint64_t child_list_index = m_node_shared_children[node_index].m_start_index + child_index;
    DIVE_ASSERT(child_list_index < m_shared_children_indices.size());
    return m_shared_children_indices[child_list_index];
}

//--------------------------------------------------------------------------------------------------
uint64_t SharedNodeTopology::GetStartSharedChildNodeIndex(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_start_shared_child.size());
    return m_start_shared_child[node_index];
}

//--------------------------------------------------------------------------------------------------
uint64_t SharedNodeTopology::GetEndSharedChildNodeIndex(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_end_shared_child.size());
    return m_end_shared_child[node_index];
}

//--------------------------------------------------------------------------------------------------
uint64_t SharedNodeTopology::GetSharedChildRootNodeIndex(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_root_node_index.size());
    return m_root_node_index[node_index];
}

//--------------------------------------------------------------------------------------------------
void SharedNodeTopology::SetNumNodes(uint64_t num_nodes)
{
    m_node_children.resize(num_nodes);
    m_node_shared_children.resize(num_nodes);
    m_node_parent.resize(num_nodes, UINT64_MAX);
    m_node_child_index.resize(num_nodes, UINT64_MAX);
}

//--------------------------------------------------------------------------------------------------
void SharedNodeTopology::AddSharedChildren(uint64_t                    node_index,
                                           const DiveVector<uint64_t> &children)
{
    DIVE_ASSERT(m_node_shared_children.size() == m_node_parent.size());
    DIVE_ASSERT(m_node_shared_children.size() == m_node_child_index.size());

    // Append to m_shared_children_indices
    uint64_t prev_size = m_shared_children_indices.size();
    m_shared_children_indices.resize(m_shared_children_indices.size() + children.size());
    std::copy(children.begin(), children.end(), m_shared_children_indices.begin() + prev_size);

    // Set "pointer" to children_list
    DIVE_ASSERT(m_node_shared_children[node_index].m_num_children == 0);
    m_node_shared_children[node_index].m_start_index = prev_size;
    m_node_shared_children[node_index].m_num_children = children.size();
}

// =================================================================================================
// CommandHierarchy
// =================================================================================================
CommandHierarchy::CommandHierarchy() {}

//--------------------------------------------------------------------------------------------------
CommandHierarchy::~CommandHierarchy() {}

//--------------------------------------------------------------------------------------------------
const SharedNodeTopology &CommandHierarchy::GetSubmitHierarchyTopology() const
{
    return m_topology[kSubmitTopology];
}

//--------------------------------------------------------------------------------------------------
const SharedNodeTopology &CommandHierarchy::GetAllEventHierarchyTopology() const
{
    return m_topology[kAllEventTopology];
}

//--------------------------------------------------------------------------------------------------
NodeType CommandHierarchy::GetNodeType(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_nodes.m_node_type.size());
    return m_nodes.m_node_type[node_index];
}

//--------------------------------------------------------------------------------------------------
const char *CommandHierarchy::GetNodeDesc(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_nodes.m_description.size());
    return m_nodes.m_description[node_index].c_str();
}

//--------------------------------------------------------------------------------------------------
Dive::EngineType CommandHierarchy::GetSubmitNodeEngineType(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_nodes.m_aux_info.size());
    DIVE_ASSERT(m_nodes.m_node_type[node_index] == Dive::NodeType::kSubmitNode);
    const AuxInfo &info = m_nodes.m_aux_info[node_index];
    return info.submit_node.m_engine_type;
}

//--------------------------------------------------------------------------------------------------
uint32_t CommandHierarchy::GetSubmitNodeIndex(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_nodes.m_aux_info.size());
    DIVE_ASSERT(m_nodes.m_node_type[node_index] == Dive::NodeType::kSubmitNode);
    const AuxInfo &info = m_nodes.m_aux_info[node_index];
    return info.submit_node.m_submit_index;
}

//--------------------------------------------------------------------------------------------------
uint8_t CommandHierarchy::GetIbNodeIndex(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_nodes.m_aux_info.size());
    DIVE_ASSERT(m_nodes.m_node_type[node_index] == Dive::NodeType::kIbNode);
    const AuxInfo &info = m_nodes.m_aux_info[node_index];
    return info.ib_node.m_ib_index;
}

//--------------------------------------------------------------------------------------------------
IbType CommandHierarchy::GetIbNodeType(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_nodes.m_aux_info.size());
    DIVE_ASSERT(m_nodes.m_node_type[node_index] == Dive::NodeType::kIbNode);
    const AuxInfo &info = m_nodes.m_aux_info[node_index];
    return (IbType)info.ib_node.m_ib_type;
}

//--------------------------------------------------------------------------------------------------
uint32_t CommandHierarchy::GetIbNodeSizeInDwords(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_nodes.m_aux_info.size());
    DIVE_ASSERT(m_nodes.m_node_type[node_index] == Dive::NodeType::kIbNode);
    const AuxInfo &info = m_nodes.m_aux_info[node_index];
    return info.ib_node.m_size_in_dwords;
}

//--------------------------------------------------------------------------------------------------
bool CommandHierarchy::GetIbNodeIsFullyCaptured(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_nodes.m_aux_info.size());
    DIVE_ASSERT(m_nodes.m_node_type[node_index] == Dive::NodeType::kIbNode);
    const AuxInfo &info = m_nodes.m_aux_info[node_index];
    return info.ib_node.m_fully_captured;
}

//--------------------------------------------------------------------------------------------------
CommandHierarchy::MarkerType CommandHierarchy::GetMarkerNodeType(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_nodes.m_aux_info.size());
    DIVE_ASSERT(m_nodes.m_node_type[node_index] == Dive::NodeType::kMarkerNode);
    const AuxInfo &info = m_nodes.m_aux_info[node_index];
    return info.marker_node.m_type;
}

//--------------------------------------------------------------------------------------------------
uint32_t CommandHierarchy::GetMarkerNodeId(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_nodes.m_aux_info.size());
    DIVE_ASSERT(m_nodes.m_node_type[node_index] == Dive::NodeType::kMarkerNode);
    const AuxInfo &info = m_nodes.m_aux_info[node_index];
    return info.marker_node.m_id;
}

//--------------------------------------------------------------------------------------------------
uint32_t CommandHierarchy::GetEventNodeId(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_nodes.m_aux_info.size());
    DIVE_ASSERT(IsDrawDispatchBlitNode(m_nodes.m_node_type[node_index]));
    const AuxInfo &info = m_nodes.m_aux_info[node_index];
    return info.event_node.m_event_id;
}

//--------------------------------------------------------------------------------------------------
uint64_t CommandHierarchy::GetPacketNodeAddr(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_nodes.m_aux_info.size());
    DIVE_ASSERT(m_nodes.m_node_type[node_index] == Dive::NodeType::kPacketNode);
    const AuxInfo &info = m_nodes.m_aux_info[node_index];
    return info.packet_node.m_addr;
}

//--------------------------------------------------------------------------------------------------
uint8_t CommandHierarchy::GetPacketNodeOpcode(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_nodes.m_aux_info.size());
    DIVE_ASSERT(m_nodes.m_node_type[node_index] == Dive::NodeType::kPacketNode);
    const AuxInfo &info = m_nodes.m_aux_info[node_index];
    return info.packet_node.m_opcode;
}

//--------------------------------------------------------------------------------------------------
uint8_t CommandHierarchy::GetPacketNodeIbLevel(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_nodes.m_aux_info.size());
    DIVE_ASSERT(m_nodes.m_node_type[node_index] == Dive::NodeType::kPacketNode);
    const AuxInfo &info = m_nodes.m_aux_info[node_index];
    return info.packet_node.m_ib_level;
}
//--------------------------------------------------------------------------------------------------
bool CommandHierarchy::GetRegFieldNodeIsCe(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_nodes.m_aux_info.size());
    DIVE_ASSERT(m_nodes.m_node_type[node_index] == Dive::NodeType::kRegNode ||
                m_nodes.m_node_type[node_index] == Dive::NodeType::kFieldNode);
    const AuxInfo &info = m_nodes.m_aux_info[node_index];
    return info.reg_field_node.m_is_ce_packet;
}

//--------------------------------------------------------------------------------------------------
SyncType CommandHierarchy::GetSyncNodeSyncType(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_nodes.m_aux_info.size());
    DIVE_ASSERT(m_nodes.m_node_type[node_index] == Dive::NodeType::kSyncNode);
    const AuxInfo &info = m_nodes.m_aux_info[node_index];
    return (SyncType)info.sync_node.m_sync_type;
}

//--------------------------------------------------------------------------------------------------
SyncInfo CommandHierarchy::GetSyncNodeSyncInfo(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_nodes.m_aux_info.size());
    DIVE_ASSERT(m_nodes.m_node_type[node_index] == Dive::NodeType::kSyncNode);
    const AuxInfo &info = m_nodes.m_aux_info[node_index];
    return info.sync_node.m_sync_info;
}

//--------------------------------------------------------------------------------------------------
uint64_t CommandHierarchy::AddNode(NodeType type, std::string &&desc, AuxInfo aux_info)
{
    return m_nodes.AddNode(type, std::move(desc), aux_info);
}

//--------------------------------------------------------------------------------------------------
uint64_t CommandHierarchy::AddGfxrNode(NodeType type, std::string &&desc)
{
    return m_nodes.AddGfxrNode(type, std::move(desc));
}

//--------------------------------------------------------------------------------------------------
size_t CommandHierarchy::GetEventIndex(uint64_t node_index) const
{
    const DiveVector<uint64_t> &indices = m_nodes.m_event_node_indices;
    auto                        it = std::lower_bound(indices.begin(), indices.end(), node_index);
    if (it == indices.end() || *it != node_index)
    {
        return 0;
    }
    return it - indices.begin() + 1;
}

// =================================================================================================
// CommandHierarchy::Nodes
// =================================================================================================
uint64_t CommandHierarchy::Nodes::AddNode(NodeType type, std::string &&desc, AuxInfo aux_info)
{
    DIVE_ASSERT(m_node_type.size() == m_description.size());
    DIVE_ASSERT(m_node_type.size() == m_aux_info.size());

    m_node_type.push_back(type);
    m_description.push_back(std::move(desc));
    m_aux_info.push_back(aux_info);
    return m_node_type.size() - 1;
}

//--------------------------------------------------------------------------------------------------
uint64_t CommandHierarchy::Nodes::AddGfxrNode(NodeType type, std::string &&desc)
{
    DIVE_ASSERT(m_node_type.size() == m_description.size());

    m_node_type.push_back(type);
    m_description.push_back(std::move(desc));
    // Adds a dummy AuxInfo object to ensure the m_node_type, m_description, and m_aux_info sizes
    // stay the same.
    m_aux_info.push_back(AuxInfo(0));
    return m_node_type.size() - 1;
}

// =================================================================================================
// CommandHierarchy::AuxInfo
// =================================================================================================
CommandHierarchy::AuxInfo::AuxInfo(uint64_t val)
{
    m_u64All = val;
}

//--------------------------------------------------------------------------------------------------
CommandHierarchy::AuxInfo CommandHierarchy::AuxInfo::SubmitNode(Dive::EngineType engine_type,
                                                                uint32_t         submit_index)
{
    AuxInfo info(0);
    info.submit_node.m_engine_type = engine_type;
    info.submit_node.m_submit_index = submit_index;
    return info;
}

//--------------------------------------------------------------------------------------------------
CommandHierarchy::AuxInfo CommandHierarchy::AuxInfo::IbNode(uint32_t ib_index,
                                                            IbType   ib_type,
                                                            uint32_t size_in_dwords,
                                                            bool     fully_captured)
{
    DIVE_ASSERT((ib_index & ((1 << kMaxNumIbsBits) - 1)) == ib_index);
    AuxInfo info(0);
    info.ib_node.m_ib_type = (uint8_t)ib_type;
    info.ib_node.m_ib_index = ib_index & ((1 << kMaxNumIbsBits) - 1);
    info.ib_node.m_size_in_dwords = size_in_dwords;
    info.ib_node.m_fully_captured = (fully_captured == true) ? 1 : 0;
    DIVE_ASSERT((IbType)info.ib_node.m_ib_type == ib_type);
    return info;
}

//--------------------------------------------------------------------------------------------------
CommandHierarchy::AuxInfo CommandHierarchy::AuxInfo::PacketNode(uint64_t addr,
                                                                uint8_t  opcode,
                                                                uint8_t  ib_level)
{
    // Addresses should only be 48-bits
    DIVE_ASSERT(addr == (addr & 0x0000FFFFFFFFFFFF));
    AuxInfo info(0);
    info.packet_node.m_addr = (addr & 0x0000FFFFFFFFFFFF);
    info.packet_node.m_opcode = opcode;
    info.packet_node.m_ib_level = ib_level;
    return info;
}

//--------------------------------------------------------------------------------------------------
CommandHierarchy::AuxInfo CommandHierarchy::AuxInfo::RegFieldNode(bool is_ce_packet)
{
    AuxInfo info(0);
    info.reg_field_node.m_is_ce_packet = is_ce_packet;
    return info;
}

//--------------------------------------------------------------------------------------------------
CommandHierarchy::AuxInfo CommandHierarchy::AuxInfo::EventNode(uint32_t event_id)
{
    AuxInfo info(0);
    info.event_node.m_event_id = event_id;
    return info;
}

//--------------------------------------------------------------------------------------------------
CommandHierarchy::AuxInfo CommandHierarchy::AuxInfo::MarkerNode(MarkerType type, uint32_t id)
{
    AuxInfo info(0);
    info.marker_node.m_type = type;
    info.marker_node.m_id = id;
    return info;
}

//--------------------------------------------------------------------------------------------------
CommandHierarchy::AuxInfo CommandHierarchy::AuxInfo::SyncNode(SyncType type, SyncInfo sync_info)
{
    AuxInfo info(0);
    info.sync_node.m_sync_type = (uint32_t)type;
    info.sync_node.m_sync_info = sync_info;
    return info;
}

// =================================================================================================
// CommandHierarchyCreator
// =================================================================================================
CommandHierarchyCreator::CommandHierarchyCreator(CommandHierarchy     &command_hierarchy,
                                                 const Pm4CaptureData &capture_data) :
    m_command_hierarchy(command_hierarchy),
    m_capture_data(capture_data)
{
}

//--------------------------------------------------------------------------------------------------
bool CommandHierarchyCreator::CreateTrees(bool                    flatten_chain_nodes,
                                          std::optional<uint64_t> reserve_size)
{
    // Clear/Reset internal data structures, just in case
    m_command_hierarchy = CommandHierarchy();

    // Optional: Reserve the internal vectors based on passed-in value. Overguessing means more
    // memory used during creation, and potentially more memory used while the capture is loaded.
    // Underguessing means more allocations. For big captures, this is easily in the multi-millions,
    // so pre-reserving the space is a signficiant performance win
    if (reserve_size.has_value())
    {
        for (uint32_t topology = 0; topology < CommandHierarchy::kTopologyTypeCount; ++topology)
        {
            m_node_start_shared_children[topology].reserve(*reserve_size);
            m_node_end_shared_children[topology].reserve(*reserve_size);
            m_node_root_node_indices[topology].reserve(*reserve_size);

            m_node_children[topology][kSingleParentNodeChildren].reserve(*reserve_size);
            m_node_children[topology][kSharedNodeChildren].reserve(*reserve_size);

            m_command_hierarchy.m_nodes.m_node_type.reserve(*reserve_size);
            m_command_hierarchy.m_nodes.m_description.reserve(*reserve_size);
            m_command_hierarchy.m_nodes.m_aux_info.reserve(*reserve_size);
            m_command_hierarchy.m_nodes.m_event_node_indices.reserve(*reserve_size);
        }
    }

    // Add a dummy root node for easier management
    uint64_t root_node_index = AddNode(NodeType::kRootNode, "", 0);
    DIVE_VERIFY(root_node_index == Topology::kRootNodeIndex);

    m_num_events = 0;
    m_flatten_chain_nodes = flatten_chain_nodes;

    if (!ProcessSubmits(m_capture_data.GetSubmits(), m_capture_data.GetMemoryManager()))
    {
        return false;
    }

    // Convert the info in m_node_children into CommandHierarchy's topologies
    CreateTopologies();

    return true;
}

//--------------------------------------------------------------------------------------------------
bool CommandHierarchyCreator::CreateTrees(const Pm4CaptureData   &capture_data,
                                          bool                    flatten_chain_nodes,
                                          std::optional<uint64_t> reserve_size)
{
    // Clear/Reset internal data structures, just in case
    m_command_hierarchy = CommandHierarchy();

    // Optional: Reserve the internal vectors based on passed-in value. Overguessing means more
    // memory used during creation, and potentially more memory used while the capture is loaded.
    // Underguessing means more allocations. For big captures, this is easily in the multi-millions,
    // so pre-reserving the space is a signficiant performance win
    if (reserve_size.has_value())
    {
        for (uint32_t topology = 0; topology < CommandHierarchy::kTopologyTypeCount; ++topology)
        {
            m_node_start_shared_children[topology].reserve(*reserve_size);
            m_node_end_shared_children[topology].reserve(*reserve_size);
            m_node_root_node_indices[topology].reserve(*reserve_size);

            m_node_children[topology][kSingleParentNodeChildren].reserve(*reserve_size);
            m_node_children[topology][kSharedNodeChildren].reserve(*reserve_size);

            m_command_hierarchy.m_nodes.m_node_type.reserve(*reserve_size);
            m_command_hierarchy.m_nodes.m_description.reserve(*reserve_size);
            m_command_hierarchy.m_nodes.m_aux_info.reserve(*reserve_size);
            m_command_hierarchy.m_nodes.m_event_node_indices.reserve(*reserve_size);
        }
    }

    // Add a dummy root node for easier management
    uint64_t root_node_index = AddNode(NodeType::kRootNode, "", 0);
    DIVE_VERIFY(root_node_index == Topology::kRootNodeIndex);

    m_num_events = 0;
    m_flatten_chain_nodes = flatten_chain_nodes;

    if (!ProcessSubmits(capture_data.GetSubmits(), capture_data.GetMemoryManager()))
    {
        return false;
    }

    // Convert the info in m_node_children into CommandHierarchy's topologies
    CreateTopologies();

    return true;
}

//--------------------------------------------------------------------------------------------------
bool CommandHierarchyCreator::CreateTrees(bool                    flatten_chain_nodes,
                                          bool                    createTopologies,
                                          std::optional<uint64_t> reserve_size)
{
    // Clear/Reset internal data structures, just in case
    m_command_hierarchy = CommandHierarchy();

    // Optional: Reserve the internal vectors based on passed-in value. Overguessing means more
    // memory used during creation, and potentially more memory used while the capture is loaded.
    // Underguessing means more allocations. For big captures, this is easily in the multi-millions,
    // so pre-reserving the space is a signficiant performance win
    if (reserve_size.has_value() && reserve_size > 0)
    {
        for (uint32_t topology = 0; topology < CommandHierarchy::kTopologyTypeCount; ++topology)
        {
            m_node_start_shared_children[topology].reserve(*reserve_size);
            m_node_end_shared_children[topology].reserve(*reserve_size);
            m_node_root_node_indices[topology].reserve(*reserve_size);

            m_node_children[topology][kSingleParentNodeChildren].reserve(*reserve_size);
            m_node_children[topology][kSharedNodeChildren].reserve(*reserve_size);

            m_command_hierarchy.m_nodes.m_node_type.reserve(*reserve_size);
            m_command_hierarchy.m_nodes.m_description.reserve(*reserve_size);
            m_command_hierarchy.m_nodes.m_aux_info.reserve(*reserve_size);
            m_command_hierarchy.m_nodes.m_event_node_indices.reserve(*reserve_size);
        }
    }

    // Add a dummy root node for easier management
    uint64_t root_node_index = AddNode(NodeType::kRootNode, "", 0);
    DIVE_VERIFY(root_node_index == Topology::kRootNodeIndex);

    m_num_events = 0;
    m_flatten_chain_nodes = flatten_chain_nodes;

    return true;
}

//--------------------------------------------------------------------------------------------------
bool CommandHierarchyCreator::CreateTrees(EngineType             engine_type,
                                          QueueType              queue_type,
                                          std::vector<uint32_t> &command_dwords,
                                          uint32_t               size_in_dwords)
{
    // Note: This function is mostly a copy/paste from the main CreateTrees() function, but with
    // workarounds to handle a case where there is no marker_data or capture_data
    class TempMemoryManager : public IMemoryManager
    {
    public:
        TempMemoryManager(std::vector<uint32_t> &command_dwords, uint32_t size_in_dwords) :
            m_command_dwords(command_dwords),
            m_size_in_dwords(size_in_dwords)
        {
        }

        // Copy the given va/size from the memory blocks
        virtual bool RetrieveMemoryData(void    *buffer_ptr,
                                        uint32_t submit_index,
                                        uint64_t va_addr,
                                        uint64_t size) const
        {
            if ((va_addr + size) > (m_size_in_dwords * sizeof(uint32_t)))
                return false;

            // Treat the va_addr as an offset
            uint8_t *command_bytes = (uint8_t *)m_command_dwords.data();
            memcpy(buffer_ptr, &command_bytes[va_addr], size);
            return true;
        }
        virtual bool GetMemoryOfUnknownSizeViaCallback(uint32_t     submit_index,
                                                       uint64_t     va_addr,
                                                       PfnGetMemory data_callback,
                                                       void        *user_ptr) const
        {
            DIVE_ASSERT(false);
            return true;
        }
        virtual uint64_t GetMaxContiguousSize(uint32_t submit_index, uint64_t va_addr) const
        {
            DIVE_ASSERT(false);
            return 0;
        }
        virtual bool IsValid(uint32_t submit_index, uint64_t addr, uint64_t size) const
        {
            DIVE_ASSERT(false);
            return true;
        }

    private:
        std::vector<uint32_t> &m_command_dwords;
        uint32_t               m_size_in_dwords;
        ;
    };

    // Clear/Reset internal data structures, just in case
    m_command_hierarchy = CommandHierarchy();

    // Add a dummy root node for easier management
    uint64_t root_node_index = AddNode(NodeType::kRootNode, "", 0);
    DIVE_VERIFY(root_node_index == Topology::kRootNodeIndex);

    m_num_events = 0;
    m_flatten_chain_nodes = false;

    Dive::IndirectBufferInfo ib_info;
    ib_info.m_va_addr = 0x0;
    ib_info.m_size_in_dwords = size_in_dwords;
    ib_info.m_skip = false;
    DiveVector<IndirectBufferInfo> ib_array;
    ib_array.push_back(ib_info);
    const Dive::SubmitInfo submit_info(engine_type, queue_type, 0, false, std::move(ib_array));

    DiveVector<SubmitInfo> submits{ submit_info };
    TempMemoryManager      mem_manager(command_dwords, size_in_dwords);
    if (!ProcessSubmits(submits, mem_manager))
    {
        return false;
    }

    // Convert the info in m_node_children into CommandHierarchy's topologies
    CreateTopologies();
    return true;
}

//--------------------------------------------------------------------------------------------------
bool CommandHierarchyCreator::OnIbStart(uint32_t                  submit_index,
                                        uint32_t                  ib_index,
                                        const IndirectBufferInfo &ib_info,
                                        IbType                    type)
{
    EmulateCallbacksBase::OnIbStart(submit_index, ib_index, ib_info, type);
    m_cur_ib_level = ib_info.m_ib_level;

    // Make all subsequent shared node parent the actual IB-packet
    if (m_cur_ib_packet_node_index != UINT64_MAX)
        m_shared_node_ib_parent_stack[m_cur_ib_level] = m_cur_ib_packet_node_index;

    // Create IB description string
    std::ostringstream ib_string_stream;
    if (type == IbType::kNormal)
    {
        ib_string_stream << "IB: " << ib_index << ", Address: 0x" << std::hex << ib_info.m_va_addr
                         << ", Size (DWORDS): " << std::dec << ib_info.m_size_in_dwords;
    }
    else if (type == IbType::kCall)
    {
        ib_string_stream << "Call IB"
                         << ", Address: 0x" << std::hex << ib_info.m_va_addr
                         << ", Size (DWORDS): " << std::dec << ib_info.m_size_in_dwords;
    }
    else if (type == IbType::kChain)
    {
        ib_string_stream << "Chain IB"
                         << ", Address: 0x" << std::hex << ib_info.m_va_addr
                         << ", Size (DWORDS): " << std::dec << ib_info.m_size_in_dwords;
    }
    else if (type == IbType::kContextSwitchIb)
    {
        ib_string_stream << "ContextSwitch IB"
                         << ", Address: 0x" << std::hex << ib_info.m_va_addr
                         << ", Size (DWORDS): " << std::dec << ib_info.m_size_in_dwords;
    }
    else if (type == IbType::kBinPrefix)
    {
        ib_string_stream << "Bin Prefix IB"
                         << ", Address: 0x" << std::hex << ib_info.m_va_addr
                         << ", Size (DWORDS): " << std::dec << ib_info.m_size_in_dwords;
    }
    else if (type == IbType::kBinCommon)
    {
        ib_string_stream << "Bin Common IB"
                         << ", Address: 0x" << std::hex << ib_info.m_va_addr
                         << ", Size (DWORDS): " << std::dec << ib_info.m_size_in_dwords;
    }
    else if (type == IbType::kFixedStrideDrawTable)
    {
        ib_string_stream << "Fixed Stride Draw Table IB"
                         << ", Address: 0x" << std::hex << ib_info.m_va_addr
                         << ", Size (DWORDS): " << std::dec << ib_info.m_size_in_dwords;
    }
    else if (type == IbType::kDrawState)
    {
        ib_string_stream << "DrawState IB"
                         << ", Address: 0x" << std::hex << ib_info.m_va_addr
                         << ", Size (DWORDS): " << std::dec << ib_info.m_size_in_dwords;

        // Make all subsequent shared node parent be the relevant group in the set_draw_state packet
        uint64_t group_index = UINT64_MAX;
        for (uint32_t i = 0; i < m_group_info_size; ++i)
        {
            if (m_group_info[i].m_group_addr == ib_info.m_va_addr)
            {
                group_index = m_group_info[i].m_group_node_index;
                break;
            }
        }
        DIVE_ASSERT(group_index != UINT64_MAX);
        m_shared_node_ib_parent_stack[m_cur_ib_level] = group_index;
    }

    if (ib_info.m_skip)
        ib_string_stream << ", NOT CAPTURED";

    if (type == IbType::kBinPrefix || type == IbType::kBinCommon)
    {
        // Create a "binprefix" or "bincommon" field node and set it as the parent
        CommandHierarchy::AuxInfo aux_info = CommandHierarchy::AuxInfo::RegFieldNode(false);
        uint64_t node_index = AddNode(NodeType::kFieldNode, ib_string_stream.str(), aux_info);

        // Add it as child to packet_node
        AddChild(CommandHierarchy::kSubmitTopology, m_start_bin_node_index, node_index);
        AddChild(CommandHierarchy::kAllEventTopology, m_start_bin_node_index, node_index);

        m_shared_node_ib_parent_stack[m_cur_ib_level] = node_index;
    }
    else if (type == IbType::kFixedStrideDrawTable)
    {
        // Create a "fixed stride draw table" field node and set it as the parent
        CommandHierarchy::AuxInfo aux_info = CommandHierarchy::AuxInfo::RegFieldNode(false);
        uint64_t node_index = AddNode(NodeType::kFieldNode, ib_string_stream.str(), aux_info);

        // Add it as child to packet_node
        AddChild(CommandHierarchy::kSubmitTopology, m_draw_table_node_index, node_index);
        AddChild(CommandHierarchy::kAllEventTopology, m_draw_table_node_index, node_index);

        m_shared_node_ib_parent_stack[m_cur_ib_level] = node_index;
    }

    // Create the ib node
    CommandHierarchy::AuxInfo aux_info = CommandHierarchy::AuxInfo::IbNode(ib_index,
                                                                           type,
                                                                           ib_info.m_size_in_dwords,
                                                                           !ib_info.m_skip);
    uint64_t ib_node_index = AddNode(NodeType::kIbNode, ib_string_stream.str(), aux_info);

    // Determine parent node
    uint64_t parent_node_index = m_cur_submit_node_index;
    if (!m_ib_stack.empty())
    {
        parent_node_index = m_ib_stack.back();
    }

    if (m_flatten_chain_nodes && type == IbType::kChain)
    {
        // If flatten enabled, then add to the nearest non-chain node parent
        // Find first previous non-CHAIN parent
        for (size_t i = m_ib_stack.size() - 1; i != SIZE_MAX; i--)
        {
            uint64_t index = m_ib_stack[i];
            IbType   cur_type = m_command_hierarchy.GetIbNodeType(index);
            if (cur_type != IbType::kChain)
            {
                parent_node_index = index;
                break;
            }
        }
    }

    AddChild(CommandHierarchy::kSubmitTopology, parent_node_index, ib_node_index);

    m_ib_stack.push_back(ib_node_index);
    m_cmd_begin_packet_node_indices.clear();
    m_cmd_begin_event_node_indices.clear();
    m_new_ib_start = true;
    return true;
}

//--------------------------------------------------------------------------------------------------
bool CommandHierarchyCreator::OnIbEnd(uint32_t                  submit_index,
                                      uint32_t                  ib_index,
                                      const IndirectBufferInfo &ib_info)
{
    EmulateCallbacksBase::OnIbEnd(submit_index, ib_index, ib_info);
    DIVE_ASSERT(!m_ib_stack.empty());

    // Setup root & range of shared children that this IB encompasses
    auto &start_node_stack = m_start_node_stack[CommandHierarchy::kSubmitTopology];

    // m_new_ib_start is true here if there were no packets for this IB
    // (e.g. ib_info.m_skip == true)
    if (m_new_ib_start)
    {
        m_new_ib_start = false;
    }
    else
    {
        DIVE_ASSERT(m_start_node_stack[CommandHierarchy::kSubmitTopology].size() ==
                    m_ib_stack.size());
        SetStartSharedChildrenNodeIndex(CommandHierarchy::kSubmitTopology,
                                        m_ib_stack.back(),
                                        start_node_stack.back());
        SetEndSharedChildrenNodeIndex(CommandHierarchy::kSubmitTopology,
                                      m_ib_stack.back(),
                                      m_last_added_node_index);
        SetSharedChildRootNodeIndex(CommandHierarchy::kSubmitTopology,
                                    m_ib_stack.back(),
                                    m_cur_submit_node_index);
        start_node_stack.pop_back();
    }

    // Note: This callback is only called for the last CHAIN of a series of daisy-CHAIN IBs,
    // because the emulator does not keep track of IBs in an internal stack. So start by
    // popping all consecutive CHAIN IBs
    IbType type;
    type = m_command_hierarchy.GetIbNodeType(m_ib_stack.back());
    while (!m_ib_stack.empty() && type == IbType::kChain)
    {
        m_ib_stack.pop_back();
        start_node_stack.pop_back();
        type = m_command_hierarchy.GetIbNodeType(m_ib_stack.back());
    }

    m_ib_stack.pop_back();
    m_cmd_begin_packet_node_indices.clear();
    m_cmd_begin_event_node_indices.clear();
    m_cur_ib_level = ib_info.m_ib_level;
    return true;
}

//--------------------------------------------------------------------------------------------------
bool CommandHierarchyCreator::OnPacket(const IMemoryManager &mem_manager,
                                       uint32_t              submit_index,
                                       uint32_t              ib_index,
                                       uint64_t              va_addr,
                                       Pm4Header             header)
{
    if (!EmulateCallbacksBase::OnPacket(mem_manager, submit_index, ib_index, va_addr, header))
        return false;
    // THIS IS TEMPORARY! Only deal with typ4 & type7 packets for now
    if ((header.type != 4) && (header.type != 7))
        return true;

    // Create the packet node and add it as child to the current submit_node and ib_node
    uint64_t packet_node_index = AddPacketNode(mem_manager, submit_index, va_addr, false, header);

    if (m_new_event_start)
    {
        m_new_event_start = false;
        m_start_node_stack[CommandHierarchy::kAllEventTopology].push_back(packet_node_index);
    }
    if (m_new_ib_start)
    {
        m_new_ib_start = false;
        m_start_node_stack[CommandHierarchy::kSubmitTopology].push_back(packet_node_index);
    }
    DIVE_ASSERT(m_ib_stack.size() == m_start_node_stack[CommandHierarchy::kSubmitTopology].size());

    uint64_t parent_index = m_shared_node_ib_parent_stack[m_cur_ib_level];
    AddSharedChild(CommandHierarchy::kSubmitTopology, parent_index, packet_node_index);
    AddSharedChild(CommandHierarchy::kAllEventTopology, parent_index, packet_node_index);

    uint32_t opcode = UINT32_MAX;
    if (header.type == 7)
        opcode = header.type7.opcode;

    // Cache packets that may be part of the vkBeginCommandBuffer.
    m_cmd_begin_packet_node_indices.push_back(packet_node_index);

    // Cache set_draw_state packet
    if (opcode == CP_SET_DRAW_STATE)
        CacheSetDrawStateGroupInfo(mem_manager, submit_index, va_addr, packet_node_index, header);

    if (Util::IsEvent(mem_manager, submit_index, va_addr, opcode, m_state_tracker))
    {
        uint64_t event_node_index = UINT64_MAX;
        uint64_t parent_node_index = m_cur_submit_node_index;
        if (m_render_marker_index != kInvalidRenderMarkerIndex)
        {
            parent_node_index = m_render_marker_index;
        }

        // Create the event node
        {
            Pm4Type7Header *type7_header = (Pm4Type7Header *)&header;
            std::string     event_string = Util::GetEventString(mem_manager,
                                                            submit_index,
                                                            va_addr,
                                                            *type7_header,
                                                            m_state_tracker);
            uint32_t        event_id = m_num_events++;

            uint64_t node_index;
            SyncType sync_type = Util::GetSyncType(mem_manager,
                                                   submit_index,
                                                   va_addr,
                                                   opcode,
                                                   m_state_tracker);
            if (sync_type != SyncType::kNone)
            {
                SyncInfo                  sync_info = {};
                CommandHierarchy::AuxInfo aux_info = CommandHierarchy::AuxInfo::SyncNode(sync_type,
                                                                                         sync_info);
                node_index = AddNode(NodeType::kSyncNode, std::move(event_string), aux_info);
            }
            else
            {
                CommandHierarchy::AuxInfo aux_info = CommandHierarchy::AuxInfo::EventNode(event_id);
                if (IsDrawDispatchEventOpcode(opcode))
                {
                    node_index = AddNode(NodeType::kDrawDispatchNode,
                                         std::move(event_string),
                                         aux_info);
                }
                else
                {
                    node_index = AddNode(NodeType::kBlitNode, std::move(event_string), aux_info);
                }
            }
            AppendEventNodeIndex(node_index);
            event_node_index = node_index;
        }

        // Cache nodes that may be part of the vkBeginCommandBuffer.
        m_cmd_begin_event_node_indices.push_back(event_node_index);

        // Setup root & range of shared children that this event encompasses
        SetStartSharedChildrenNodeIndex(CommandHierarchy::kAllEventTopology,
                                        event_node_index,
                                        m_start_node_stack[CommandHierarchy::kAllEventTopology]
                                        .back());
        SetEndSharedChildrenNodeIndex(CommandHierarchy::kAllEventTopology,
                                      event_node_index,
                                      packet_node_index);
        SetSharedChildRootNodeIndex(CommandHierarchy::kAllEventTopology,
                                    event_node_index,
                                    m_cur_submit_node_index);
        m_new_event_start = true;
        m_start_node_stack[CommandHierarchy::kAllEventTopology].pop_back();

        // Add the draw_dispatch_node to the submit_node if currently not inside a marker range.
        // Otherwise append it to the marker at the top of the marker stack.
        AddChild(CommandHierarchy::kAllEventTopology, parent_node_index, event_node_index);
        m_node_parent_info[CommandHierarchy::kAllEventTopology]
                          [event_node_index] = parent_node_index;
    }
    else if ((opcode == CP_INDIRECT_BUFFER_PFE || opcode == CP_INDIRECT_BUFFER_PFD ||
              opcode == CP_INDIRECT_BUFFER_CHAIN || opcode == CP_COND_INDIRECT_BUFFER_PFE ||
              opcode == CP_SET_CTXSWITCH_IB))
    {
        m_cur_ib_packet_node_index = packet_node_index;
    }
    else if (opcode == CP_LOAD_STATE6 || opcode == CP_LOAD_STATE6_GEOM ||
             opcode == CP_LOAD_STATE6_FRAG)
    {
        AppendLoadStateExtBufferNode(mem_manager, submit_index, va_addr, packet_node_index);
    }
    else if (opcode == CP_MEM_TO_REG)
    {
        AppendMemRegNodes(mem_manager, submit_index, va_addr, packet_node_index);
    }
    else if (opcode == CP_START_BIN)
    {
        m_start_bin_node_index = packet_node_index;
    }
    else if (opcode == CP_END_BIN)
    {
        m_start_bin_node_index = UINT64_MAX;
    }
    else if (opcode == CP_FIXED_STRIDE_DRAW_TABLE)
    {
        m_draw_table_node_index = packet_node_index;
    }
    else if (opcode == CP_SET_MARKER)
    {
        PM4_CP_SET_MARKER packet;
        DIVE_VERIFY(mem_manager.RetrieveMemoryData(&packet, submit_index, va_addr, sizeof(packet)));
        // as mentioned in adreno_pm4.xml, only b0-b3 are considered when b8 is not set
        DIVE_ASSERT((packet.u32All0 & 0x100) == 0);
        a6xx_marker marker = static_cast<a6xx_marker>(packet.u32All0 & 0xf);

        std::string desc;
        bool        add_child = true;
        switch (marker)
        {
            // This is emitted at the beginning of the render pass if tiled rendering mode is
            // disabled
        case RM6_BYPASS:
            desc = "Direct Rendering Pass";
            break;
            // This is emitted at the beginning of the binning pass, although the binning pass
            // could be missing even in tiled rendering mode
        case RM6_BINNING:
            desc = "Binning Pass";
            break;
            // This is emitted at the beginning of the tiled rendering pass
        case RM6_GMEM:
            desc = "Tile Rendering Pass";
            break;
            // This is emitted at the end of the tiled rendering pass
        case RM6_ENDVIS:
            // Should be paired with RM6_GMEM only if RM6_BINNING exist
            add_child = false;
            break;
            // This is emitted at the beginning of the resolve pass
        case RM6_RESOLVE:
            desc = "Resolve Pass";
            break;
            // This is emitted for each dispatch
        case RM6_COMPUTE:
            desc = "Compute Dispatch";
            break;
        // This seems to be the end of Resolve Pass
        case RM6_YIELD:
            // should be paired with RM6_RESOLVE
            add_child = false;
            break;
            // TODO(wangra): Might need to handle following markers
        case RM6_BLIT2DSCALE:
        case RM6_IB1LIST_START:
        case RM6_IB1LIST_END:
        default:
            add_child = false;
            break;
        }

        if (add_child)
        {
            // End of previous pass
            if (m_render_marker_index != kInvalidRenderMarkerIndex)
            {
                // Events are fully contained within passes. This means the topmost element of the
                // stack is always tracking the beginning of the next event. Need to pop that now
                // that a new pass is being started
                m_start_node_stack[CommandHierarchy::kAllEventTopology].pop_back();

                // Setup root & range of shared children that this event encompasses
                SetStartSharedChildrenNodeIndex(CommandHierarchy::kAllEventTopology,
                                                m_render_marker_index,
                                                m_start_node_stack
                                                [CommandHierarchy::kAllEventTopology]
                                                .back());
                SetEndSharedChildrenNodeIndex(CommandHierarchy::kAllEventTopology,
                                              m_render_marker_index,
                                              m_last_added_node_index);
                SetSharedChildRootNodeIndex(CommandHierarchy::kAllEventTopology,
                                            m_render_marker_index,
                                            m_cur_submit_node_index);
                m_start_node_stack[CommandHierarchy::kAllEventTopology].pop_back();
            }

            m_render_marker_index = AddNode(NodeType::kRenderMarkerNode, std::move(desc), 0);

            if (marker == RM6_BINNING)
            {
                m_tracking_first_tile_pass_start = true;
                m_command_hierarchy
                .AddToFilterExcludeIndexList(m_render_marker_index,
                                             CommandHierarchy::kFirstTilePassOnly);
            }

            if ((marker == RM6_GMEM) || (marker == RM6_RESOLVE))
            {
                m_command_hierarchy.AddToFilterExcludeIndexList(m_render_marker_index,
                                                                CommandHierarchy::kBinningPassOnly);

                if (!m_tracking_first_tile_pass_start)
                {
                    m_command_hierarchy
                    .AddToFilterExcludeIndexList(m_render_marker_index,
                                                 CommandHierarchy::kFirstTilePassOnly);
                    m_command_hierarchy
                    .AddToFilterExcludeIndexList(m_render_marker_index,
                                                 CommandHierarchy::kBinningAndFirstTilePass);
                }
                if (m_tracking_first_tile_pass_start && (marker == RM6_RESOLVE))
                {
                    m_tracking_first_tile_pass_start = false;
                }
            }

            AddChild(CommandHierarchy::kAllEventTopology,
                     m_cur_submit_node_index,
                     m_render_marker_index);

            // Include the current packet in the scroll to range
            m_start_node_stack[CommandHierarchy::kAllEventTopology].push_back(packet_node_index);

            // Events are fully contained within passes, so we're starting a new event
            // upon starting a new pass
            m_new_event_start = true;
        }
    }

    if (m_render_marker_index != kInvalidRenderMarkerIndex)
    {
        AddSharedChild(CommandHierarchy::kAllEventTopology,
                       m_render_marker_index,
                       packet_node_index);
    }

    m_last_added_node_index = packet_node_index;
    return true;
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::OnSubmitStart(uint32_t submit_index, const SubmitInfo &submit_info)
{
    uint32_t engine_index = static_cast<uint32_t>(submit_info.GetEngineType());
    uint32_t queue_index = static_cast<uint32_t>(submit_info.GetQueueType());

    std::ostringstream submit_string_stream;
    submit_string_stream << "Submit: " << submit_index;
    submit_string_stream << ", Num IBs: " << submit_info.GetNumIndirectBuffers()
                         << ", Engine: " << kEngineTypeStrings[engine_index]
                         << ", Queue: " << kQueueTypeStrings[queue_index]
                         << ", Engine Index: " << (uint32_t)submit_info.GetEngineIndex()
                         << ", Dummy Submit: " << (uint32_t)submit_info.IsDummySubmit();

    // Create submit node
    Dive::EngineType          engine_type = submit_info.GetEngineType();
    CommandHierarchy::AuxInfo aux_info = CommandHierarchy::AuxInfo::SubmitNode(engine_type,
                                                                               submit_index);
    uint64_t                  submit_node_index = AddNode(NodeType::kSubmitNode,
                                         submit_string_stream.str(),
                                         aux_info);

    // Add submit node to the topologies as children to the root node
    AddChild(CommandHierarchy::kSubmitTopology, Topology::kRootNodeIndex, submit_node_index);
    AddChild(CommandHierarchy::kAllEventTopology, Topology::kRootNodeIndex, submit_node_index);

    // Set the submit node to be its own shared child root node
    SetSharedChildRootNodeIndex(CommandHierarchy::kSubmitTopology,
                                submit_node_index,
                                submit_node_index);
    SetSharedChildRootNodeIndex(CommandHierarchy::kAllEventTopology,
                                submit_node_index,
                                submit_node_index);
    m_cur_submit_node_index = submit_node_index;
    m_cur_ib_level = 1;
    m_shared_node_ib_parent_stack[m_cur_ib_level] = m_cur_submit_node_index;
    m_cur_ib_packet_node_index = UINT64_MAX;
    m_ib_stack.clear();
    for (uint32_t i = 0; i < CommandHierarchy::kTopologyTypeCount; i++)
        m_start_node_stack[i].clear();
    m_render_marker_index = kInvalidRenderMarkerIndex;
    m_state_tracker.Reset();
    m_new_event_start = true;
    m_new_ib_start = true;
    m_tracking_first_tile_pass_start = false;
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::OnSubmitEnd(uint32_t submit_index, const SubmitInfo &submit_info)
{
    // For the submit topology, the IBs are inserted in emulation order, and are not necessarily in
    // ib-index order. Sort them here so they appear in order of ib-index.
    DiveVector<uint64_t>
    &submit_children = m_node_children[CommandHierarchy::kSubmitTopology][kSingleParentNodeChildren]
                                      [m_cur_submit_node_index];
    std::sort(submit_children.begin(),
              submit_children.end(),
              [&](uint64_t lhs, uint64_t rhs) -> bool {
                  uint8_t lhs_index = m_command_hierarchy.GetIbNodeIndex(lhs);
                  uint8_t rhs_index = m_command_hierarchy.GetIbNodeIndex(rhs);
                  return lhs_index < rhs_index;
              });

    // Insert present node to event topology, when appropriate
    for (uint32_t i = 0; i < m_capture_data.GetNumPresents(); ++i)
    {
        const PresentInfo &present_info = m_capture_data.GetPresentInfo(i);

        // Check if present exists right after this submit
        if (submit_index != (present_info.GetSubmitIndex()))
            continue;

        std::ostringstream present_string_stream;
        if (present_info.HasValidData())
        {
            const char *format_string = GetVkFormatString(present_info.GetSurfaceVkFormat());
            DIVE_ASSERT(format_string != nullptr);
            uint32_t    vk_color_space = present_info.GetSurfaceVkColorSpaceKHR();
            const char *color_space_string = GetVkColorSpaceKhrString(vk_color_space);
            DIVE_ASSERT(color_space_string != nullptr);
            present_string_stream << "Present: " << i
                                  << ", FullScreen: " << present_info.IsFullScreen() << ", Engine: "
                                  << kEngineTypeStrings[(uint32_t)present_info.GetEngineType()]
                                  << ", Queue: "
                                  << kQueueTypeStrings[(uint32_t)present_info.GetQueueType()]
                                  << ", SurfaceAddr: 0x" << std::hex
                                  << present_info.GetSurfaceAddr() << std::dec
                                  << ", SurfaceSize: " << present_info.GetSurfaceSize()
                                  << ", VkFormat: " << format_string
                                  << ", VkColorSpaceKHR: " << color_space_string;
        }
        else
        {
            present_string_stream << "Present: " << i;
        }
        uint64_t present_node_index = AddNode(NodeType::kPresentNode, present_string_stream.str());
        AddChild(CommandHierarchy::kAllEventTopology, Topology::kRootNodeIndex, present_node_index);
    }
}

//--------------------------------------------------------------------------------------------------
uint64_t CommandHierarchyCreator::AddPacketNode(const IMemoryManager &mem_manager,
                                                uint32_t              submit_index,
                                                uint64_t              va_addr,
                                                bool                  is_ce_packet,
                                                Pm4Header             header)
{
    if (header.type == 7)
    {
        std::ostringstream packet_string_stream;
        packet_string_stream << GetOpCodeString(header.type7.opcode);
        packet_string_stream << " 0x" << std::hex << header.u32All << std::dec;

        CommandHierarchy::AuxInfo aux_info = CommandHierarchy::AuxInfo::PacketNode(va_addr,
                                                                                   header.type7
                                                                                   .opcode,
                                                                                   m_cur_ib_level);

        uint64_t packet_node_index = AddNode(NodeType::kPacketNode,
                                             packet_string_stream.str(),
                                             aux_info);

        if (header.type7.opcode == CP_CONTEXT_REG_BUNCH)
        {
            AppendRegNodes(mem_manager,
                           submit_index,
                           va_addr + sizeof(Pm4Type7Header),  // skip the type7 PM4
                           header.type7.count,
                           packet_node_index);
        }
        else
        {
            // If there are missing packet fields, then output the raw DWORDS directly
            // Some packets, such as CP_LOAD_STATE6_* handle this explicitly elsewhere
            bool append_extra_dwords = (header.type7.opcode != CP_LOAD_STATE6 &&
                                        header.type7.opcode != CP_LOAD_STATE6_GEOM &&
                                        header.type7.opcode != CP_LOAD_STATE6_FRAG);

            const PacketInfo *packet_info_ptr = GetPacketInfo(header.type7.opcode);
            DIVE_ASSERT(packet_info_ptr != nullptr);
            AppendPacketFieldNodes(mem_manager,
                                   submit_index,
                                   va_addr + sizeof(Pm4Type7Header),  // skip the type7 PM4
                                   header.type7.count,
                                   append_extra_dwords,
                                   packet_info_ptr,
                                   packet_node_index);
        }
        return packet_node_index;
    }
    else if (header.type == 4)
    {

        std::ostringstream packet_string_stream;
        packet_string_stream << "TYPE4 REGWRITE";
        packet_string_stream << " 0x" << std::hex << header.u32All << std::dec;

        CommandHierarchy::AuxInfo aux_info = CommandHierarchy::AuxInfo::PacketNode(va_addr,
                                                                                   UINT8_MAX,
                                                                                   m_cur_ib_level);

        uint64_t packet_node_index = AddNode(NodeType::kPacketNode,
                                             packet_string_stream.str(),
                                             aux_info);

        AppendRegNodes(mem_manager, submit_index, va_addr, header, packet_node_index);
        return packet_node_index;
    }
    return UINT32_MAX;  // This is temporary. Shouldn't happen once we properly add the packet node!
}

//--------------------------------------------------------------------------------------------------
void OutputValue(std::ostringstream &string_stream,
                 ValueType           type,
                 uint64_t            value,
                 uint32_t            bit_width = 0,
                 uint32_t            radix = 0)
{
    if (type == ValueType::kBoolean)
    {
        if (value != 0)
            string_stream << "True";
        else
            string_stream << "False";
    }
    else if (type == ValueType::kUint)
    {
        string_stream << value;
    }
    else if (type == ValueType::kInt)
    {
        union
        {
            int32_t  s;
            uint32_t u;
        } union_val;
        // Non-address types are always 32-bit
        DIVE_ASSERT(value <= UINT32_MAX);
        union_val.u = (uint32_t)value;
        string_stream << union_val.s;
    }
    else if (type == ValueType::kFloat)
    {
        // TODO(wangra): need to handle f16, f64 differently
        union
        {
            float    f;
            uint32_t i;
        } union_val;
        // If it's a float, it's not 64-bit wide. So typecast should be ok
        DIVE_ASSERT(value <= UINT32_MAX);
        union_val.i = (uint32_t)value;
        string_stream << union_val.f;
    }
    else if (type == ValueType::kFixed)
    {
        double v = 0.0;
        if (value & (UINT64_C(1) << bit_width))
        {
            v = (((double)((UINT64_C(1) << (bit_width + 1)) - value)) /
                 ((double)(UINT64_C(1) << radix)));
        }
        else
        {
            v = (((double)value) / ((double)(UINT64_C(1) << radix)));
        }
        string_stream << v;
    }
    else if (type == ValueType::kUFixed)
    {
        const double v = (((double)value) / ((double)(UINT64_C(1) << radix)));
        string_stream << v;
    }
    else if (type == ValueType::kRegID)
    {
        string_stream << "r" << (value >> 2) << "."
                      << "xyzw"[value & 0x3];
    }
    else
    {
        string_stream << "0x" << std::hex << value << std::dec;
    }
}

//--------------------------------------------------------------------------------------------------
uint64_t CommandHierarchyCreator::AddRegisterNode(uint32_t       reg,
                                                  uint64_t       reg_value,
                                                  const RegInfo *reg_info_ptr)
{
    // Should never have an "unknown register" unless something is seriously wrong!
    DIVE_ASSERT(reg_info_ptr != nullptr);
    reg_value = reg_value << reg_info_ptr->m_shr;
    // Reg item
    std::ostringstream reg_string_stream;
    if (reg_info_ptr->m_enum_handle != UINT8_MAX)
    {
        const char *enum_str = GetEnumString(reg_info_ptr->m_enum_handle, (uint32_t)reg_value);
        DIVE_ASSERT(enum_str != nullptr);
        reg_string_stream << reg_info_ptr->m_name << ": " << enum_str;
    }
    else
    {
        reg_string_stream << reg_info_ptr->m_name << ": ";
        OutputValue(reg_string_stream,
                    (ValueType)reg_info_ptr->m_type,
                    reg_value,
                    reg_info_ptr->m_bit_width,
                    reg_info_ptr->m_radix);
    }

    CommandHierarchy::AuxInfo aux_info = CommandHierarchy::AuxInfo::RegFieldNode(false);
    uint64_t reg_node_index = AddNode(NodeType::kRegNode, reg_string_stream.str(), aux_info);

    // Go through each field of this register, create a FieldNode out of it and append as child
    // to reg_node_ptr
    for (uint32_t field = 0; field < reg_info_ptr->m_fields.size(); ++field)
    {
        const RegField &reg_field = reg_info_ptr->m_fields[field];
        uint64_t        field_value = ((reg_value & reg_field.m_mask) >> reg_field.m_shift)
                               << reg_field.m_shr;

        // Field item
        std::ostringstream field_string_stream;
        field_string_stream << reg_field.m_name << ": ";
        if (reg_field.m_enum_handle != UINT8_MAX)
        {
            const char *enum_str = GetEnumString(reg_field.m_enum_handle, (uint32_t)field_value);
            if (enum_str != nullptr)
                field_string_stream << enum_str;
            else
                OutputValue(field_string_stream, (ValueType)reg_field.m_type, field_value);
        }
        else
            OutputValue(field_string_stream,
                        (ValueType)reg_field.m_type,
                        field_value,
                        reg_field.m_bit_width,
                        reg_field.m_radix);

        uint64_t field_node_index = AddNode(NodeType::kFieldNode,
                                            field_string_stream.str(),
                                            aux_info);

        // Add it as child to reg_node
        AddChild(CommandHierarchy::kSubmitTopology, reg_node_index, field_node_index);
        AddChild(CommandHierarchy::kAllEventTopology, reg_node_index, field_node_index);
    }
    return reg_node_index;
}

//--------------------------------------------------------------------------------------------------
uint32_t CommandHierarchyCreator::GetMarkerSize(const uint8_t *marker_ptr, size_t num_dwords)
{
    return UINT32_MAX;
}

//--------------------------------------------------------------------------------------------------
bool CommandHierarchyCreator::IsBeginDebugMarkerNode(uint64_t node_index)
{
    NodeType node_type = m_command_hierarchy.GetNodeType(node_index);

    if (node_type == NodeType::kMarkerNode && m_last_user_push_parent_node != UINT64_MAX)
    {
        CommandHierarchy::MarkerType marker_type = m_command_hierarchy.GetMarkerNodeType(
        node_index);
        if (marker_type == CommandHierarchy::MarkerType::kBeginEnd)
        {
            return true;
        }
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::AppendRegNodes(const IMemoryManager &mem_manager,
                                             uint32_t              submit_index,
                                             uint64_t              va_addr,
                                             uint32_t              dword_count,
                                             uint64_t              packet_node_index)
{
    // This version of AppendRegNodes takes in a raw buffer consisting of register offset + value
    // pairs
    uint32_t dword = 0;
    while (dword < dword_count)
    {
        struct RegPair
        {
            uint32_t m_reg_offset;
            uint32_t m_reg_value;
        };
        RegPair  reg_pair;
        uint64_t pair_addr = va_addr + dword * sizeof(uint32_t);
        DIVE_VERIFY(
        mem_manager.RetrieveMemoryData(&reg_pair, submit_index, pair_addr, sizeof(reg_pair)));
        dword += 2;

        const RegInfo *reg_info_ptr = GetRegInfo(reg_pair.m_reg_offset);

        RegInfo temp = {};
        temp.m_name = "Unknown";
        temp.m_enum_handle = UINT8_MAX;
        if (reg_info_ptr == nullptr)
            reg_info_ptr = &temp;

        uint64_t reg_value = reg_pair.m_reg_value;
        if (reg_info_ptr->m_is_64_bit)
        {
            RegPair  new_reg_pair;
            uint64_t new_pair_addr = va_addr + dword * sizeof(uint32_t);
            DIVE_VERIFY(mem_manager.RetrieveMemoryData(&new_reg_pair,
                                                       submit_index,
                                                       new_pair_addr,
                                                       sizeof(new_reg_pair)));

            // Sometimes the upper 32-bits are not set
            // Probably because they're 0s and there's no need to set it
            if (new_reg_pair.m_reg_offset == reg_pair.m_reg_offset + 1)
            {
                dword += 2;
                reg_value |= ((uint64_t)new_reg_pair.m_reg_value) << 32;
            }
        }

        // Create the register node, as well as all its children nodes that describe the various
        // fields set in the single 32-bit register
        uint64_t reg_node_index = AddRegisterNode(reg_pair.m_reg_offset, reg_value, reg_info_ptr);

        // Add it as child to packet node
        AddChild(CommandHierarchy::kSubmitTopology, packet_node_index, reg_node_index);
        AddChild(CommandHierarchy::kAllEventTopology, packet_node_index, reg_node_index);
    }
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::AppendRegNodes(const IMemoryManager &mem_manager,
                                             uint32_t              submit_index,
                                             uint64_t              va_addr,
                                             Pm4Header             header,
                                             uint64_t              packet_node_index)
{
    // This version of AppendRegNodes takes in an offset from the header, and expects a contiguous
    // sequence of register values

    // Go through each register set by this packet
    uint32_t offset_in_bytes = 0;
    uint32_t dword = 0;
    while (dword < header.type4.count)
    {
        uint64_t       reg_va_addr = va_addr + sizeof(header) + offset_in_bytes;
        uint32_t       reg_offset = header.type4.offset + dword;
        const RegInfo *reg_info_ptr = GetRegInfo(reg_offset);

        RegInfo temp = {};
        temp.m_name = "Unknown";
        temp.m_enum_handle = UINT8_MAX;
        if (reg_info_ptr == nullptr)
            reg_info_ptr = &temp;

        uint32_t size_to_read = sizeof(uint32_t);
        if (reg_info_ptr->m_is_64_bit)
            size_to_read = sizeof(uint64_t);
        offset_in_bytes += size_to_read;

        uint64_t reg_value = 0;
        DIVE_VERIFY(
        mem_manager.RetrieveMemoryData(&reg_value, submit_index, reg_va_addr, size_to_read));
        // Create the register node, as well as all its children nodes that describe the various
        // fields set in the single 32-bit register
        uint64_t reg_node_index = AddRegisterNode(reg_offset, reg_value, reg_info_ptr);

        // Add it as child to packet node
        AddChild(CommandHierarchy::kSubmitTopology, packet_node_index, reg_node_index);
        AddChild(CommandHierarchy::kAllEventTopology, packet_node_index, reg_node_index);

        dword++;
        if (reg_info_ptr->m_is_64_bit)
            dword++;
    }
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::AppendContextRegRmwNodes(const IMemoryManager        &mem_manager,
                                                       uint32_t                     submit_index,
                                                       uint64_t                     va_addr,
                                                       const PM4_PFP_TYPE_3_HEADER &header,
                                                       uint64_t packet_node_index)
{
    return;
}

//------------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::AppendIBFieldNodes(const char                  *suffix,
                                                 const IMemoryManager        &mem_manager,
                                                 uint32_t                     submit_index,
                                                 uint64_t                     va_addr,
                                                 bool                         is_ce_packet,
                                                 const PM4_PFP_TYPE_3_HEADER &header,
                                                 uint64_t                     packet_node_index)
{
    return;
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::AppendLoadRegNodes(const IMemoryManager        &mem_manager,
                                                 uint32_t                     submit_index,
                                                 uint64_t                     va_addr,
                                                 uint32_t                     reg_space_start,
                                                 const PM4_PFP_TYPE_3_HEADER &header,
                                                 uint64_t                     packet_node_index)
{
    return;
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::AppendLoadRegIndexNodes(const IMemoryManager        &mem_manager,
                                                      uint32_t                     submit_index,
                                                      uint64_t                     va_addr,
                                                      uint32_t                     reg_space_start,
                                                      const PM4_PFP_TYPE_3_HEADER &header,
                                                      uint64_t packet_node_index)
{
    return;
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::AppendEventWriteFieldNodes(const IMemoryManager        &mem_manager,
                                                         uint32_t                     submit_index,
                                                         uint64_t                     va_addr,
                                                         const PM4_PFP_TYPE_3_HEADER &header,
                                                         const PacketInfo *packet_info_ptr,
                                                         uint64_t          packet_node_index)
{
    return;
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::AppendPacketFieldNodes(const IMemoryManager &mem_manager,
                                                     uint32_t              submit_index,
                                                     uint64_t              va_addr,
                                                     uint32_t              dword_count,
                                                     bool                  append_extra_dwords,
                                                     const PacketInfo     *packet_info_ptr,
                                                     uint64_t              packet_node_index,
                                                     const char           *prefix)
{
    // Loop through each field and append it to packet
    uint32_t base_dword = 0;  // For tracking non-0 array fields
    uint32_t end_dword = UINT32_MAX;

    // Assumption here is that the array (i.e. the part that repeats) covers the whole packet
    bool packet_end_early = false;
    for (uint32_t array = 0; array < packet_info_ptr->m_max_array_size; array++)
    {
        base_dword = (array != 0) ? end_dword : 0;

        // If this is a packet with arrays, and there are more dwords left,
        // then add a parent node for each index
        uint64_t parent_node_index = packet_node_index;
        if ((packet_info_ptr->m_max_array_size > 1) && (base_dword < dword_count))
        {
            std::ostringstream field_string_stream;
            field_string_stream << array;
            CommandHierarchy::AuxInfo aux_info = CommandHierarchy::AuxInfo::RegFieldNode(false);
            uint64_t                  array_node_index = AddNode(NodeType::kFieldNode,
                                                field_string_stream.str(),
                                                aux_info);

            // Add it as child to packet_node
            AddChild(CommandHierarchy::kSubmitTopology, packet_node_index, array_node_index);
            AddChild(CommandHierarchy::kAllEventTopology, packet_node_index, array_node_index);
            parent_node_index = array_node_index;
        }

        for (size_t field = 0; field < packet_info_ptr->m_fields.size(); ++field)
        {
            const PacketField &packet_field = packet_info_ptr->m_fields[field];

            // packet_field.m_dword keeps the total dword count so far, including current field
            uint32_t field_dword = base_dword + packet_field.m_dword;
            end_dword = field_dword;

            // Some packets end early sometimes and do not use all fields (e.g. CP_EVENT_WRITE with
            // CACHE_CLEAN)
            if (field_dword > dword_count)
            {
                packet_end_early = true;
                break;
            }

            uint32_t dword_value = 0;
            // (field_dword - 1) since each field is always 1 32bit register, we don't have any
            // 64bit field
            uint64_t dword_va_addr = va_addr + (field_dword - 1) * sizeof(uint32_t);
            DIVE_VERIFY(mem_manager.RetrieveMemoryData(&dword_value,
                                                       submit_index,
                                                       dword_va_addr,
                                                       sizeof(uint32_t)));

            uint32_t field_value = ((dword_value & packet_field.m_mask) >> packet_field.m_shift)
                                   << packet_field.m_shr;

            // Field item
            std::ostringstream field_string_stream;
            field_string_stream << prefix << packet_field.m_name << ": ";
            if (packet_field.m_enum_handle != UINT8_MAX)
            {
                const char *enum_str = GetEnumString(packet_field.m_enum_handle, field_value);
                if (enum_str != nullptr)
                    field_string_stream << enum_str;
                else
                    OutputValue(field_string_stream, (ValueType)packet_field.m_type, field_value);
            }
            else
                OutputValue(field_string_stream, (ValueType)packet_field.m_type, field_value);

            CommandHierarchy::AuxInfo aux_info = CommandHierarchy::AuxInfo::RegFieldNode(false);
            uint64_t                  field_node_index = AddNode(NodeType::kFieldNode,
                                                field_string_stream.str(),
                                                aux_info);

            // Add it as child to packet_node
            AddChild(CommandHierarchy::kSubmitTopology, parent_node_index, field_node_index);
            AddChild(CommandHierarchy::kAllEventTopology, parent_node_index, field_node_index);
        }

        if (packet_end_early)
            break;
    }

    // If there are missing packet fields, then output the raw DWORDS directly
    // Some packets, such as CP_LOAD_STATE6_* handle this explicitly elsewhere
    if (append_extra_dwords)
    {
        if (end_dword < dword_count)
        {
            for (size_t i = end_dword + 1; i <= dword_count; i++)
            {
                uint32_t dword_value = 0;
                uint64_t dword_va_addr = va_addr + i * sizeof(uint32_t);
                DIVE_VERIFY(mem_manager.RetrieveMemoryData(&dword_value,
                                                           submit_index,
                                                           dword_va_addr,
                                                           sizeof(uint32_t)));

                std::ostringstream field_string_stream;
                field_string_stream << prefix << "(DWORD " << i << "): 0x" << std::hex
                                    << dword_value;

                CommandHierarchy::AuxInfo aux_info = CommandHierarchy::AuxInfo::RegFieldNode(false);
                uint64_t                  field_node_index = AddNode(NodeType::kFieldNode,
                                                    field_string_stream.str(),
                                                    aux_info);

                // Add it as child to packet_node
                AddChild(CommandHierarchy::kSubmitTopology, packet_node_index, field_node_index);
                AddChild(CommandHierarchy::kAllEventTopology, packet_node_index, field_node_index);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::AppendLoadStateExtBufferNode(const IMemoryManager &mem_manager,
                                                           uint32_t              submit_index,
                                                           uint64_t              va_addr,
                                                           uint64_t              packet_node_index)
{
    PM4_CP_LOAD_STATE6 packet;
    DIVE_VERIFY(mem_manager.RetrieveMemoryData(&packet, submit_index, va_addr, sizeof(packet)));

    enum class StateBlockCat
    {
        kTex,
        kShader,
        kIbo
    };
    StateBlockCat cat;
    const bool    is_compute = (packet.bitfields0.STATE_BLOCK == SB6_CS_TEX) ||
                            (packet.bitfields0.STATE_BLOCK == SB6_CS_SHADER) ||
                            (packet.bitfields0.STATE_BLOCK == SB6_CS_IBO);
    switch (packet.bitfields0.STATE_BLOCK)
    {
    case SB6_VS_TEX:
    case SB6_HS_TEX:
    case SB6_DS_TEX:
    case SB6_GS_TEX:
    case SB6_FS_TEX:
    case SB6_CS_TEX:
        cat = StateBlockCat::kTex;
        break;
    case SB6_VS_SHADER:
    case SB6_HS_SHADER:
    case SB6_DS_SHADER:
    case SB6_GS_SHADER:
    case SB6_FS_SHADER:
    case SB6_CS_SHADER:
        cat = StateBlockCat::kShader;
        break;
    case SB6_IBO:
    case SB6_CS_IBO:
        cat = StateBlockCat::kIbo;
        break;
    default:
        DIVE_ASSERT(false);
        cat = StateBlockCat::kTex;
    }

    uint64_t ext_src_addr = 0;
    bool     bindless = false;
    switch (packet.bitfields0.STATE_SRC)
    {
    case SS6_DIRECT:
        ext_src_addr = va_addr + sizeof(PM4_CP_LOAD_STATE6);
        break;
    case SS6_BINDLESS:
    {
        bindless = true;
        const uint32_t base_reg = is_compute ?
                                  GetRegOffsetByName("HLSQ_CS_BINDLESS_BASE0_DESCRIPTOR") :
                                  GetRegOffsetByName("HLSQ_BINDLESS_BASE0_DESCRIPTOR");
        const uint32_t reg = base_reg + (packet.u32All1 >> 28) * 2;

        DIVE_ASSERT(m_state_tracker.IsRegSet(reg));
        DIVE_ASSERT(m_state_tracker.IsRegSet(reg + 1));

        ext_src_addr = m_state_tracker.GetRegValue(reg) & 0xfffffffc;
        ext_src_addr |= ((uint64_t)m_state_tracker.GetRegValue(reg + 1)) << 32;

        ext_src_addr += 4 * (packet.u32All1 & 0xffffff);
    }
    break;
    case SS6_INDIRECT:
        ext_src_addr = packet.u32All1 & 0xfffffffc;
        ext_src_addr |= ((uint64_t)packet.u32All2) << 32;
        break;
    case SS6_UBO:
        // Not sure what this is used for, and even cffdump just sets ext_src_addr=0 in this case
        break;
    }

    auto AppendSharps = [&](const char *sharp_struct_name, uint32_t sharp_struct_size) {
        for (uint32_t i = 0; i < packet.bitfields0.NUM_UNIT; ++i)
        {
            uint64_t          addr = ext_src_addr + i * sharp_struct_size;
            const PacketInfo *packet_info_ptr = GetPacketInfo(0, sharp_struct_name);
            DIVE_ASSERT(packet_info_ptr != nullptr);
            std::ostringstream prefix_stream;
            prefix_stream << "  [" << i << "] ";
            AppendPacketFieldNodes(mem_manager,
                                   submit_index,
                                   addr,
                                   sharp_struct_size / sizeof(uint32_t),
                                   false,
                                   packet_info_ptr,
                                   packet_node_index,
                                   prefix_stream.str().c_str());
        }
    };

    switch (cat)
    {
    case StateBlockCat::kTex:
        if (packet.bitfields0.STATE_TYPE == ST6_SHADER)
            AppendSharps("A6XX_TEX_SAMP", bindless ? 16 : sizeof(A6XX_TEX_SAMP));
        else if (packet.bitfields0.STATE_TYPE == ST6_CONSTANTS)
            AppendSharps("A6XX_TEX_CONST", sizeof(A6XX_TEX_CONST));
        else if (packet.bitfields0.STATE_TYPE == ST6_UBO)
            AppendSharps("A6XX_UBO", bindless ? 16 : sizeof(A6XX_UBO));
        break;
    case StateBlockCat::kShader:
        // Shader program
        if (packet.bitfields0.STATE_TYPE == ST6_SHADER)
        {
        }
        // Constant data
        else if (packet.bitfields0.STATE_TYPE == ST6_CONSTANTS)
        {
            if (ext_src_addr != 0)
            {
                // NUM_UNIT is in unit of float4s
                // Add 4 dwords per line
                AddConstantsToPacketNode<float>(mem_manager,
                                                ext_src_addr,
                                                packet_node_index,
                                                packet.bitfields0.NUM_UNIT * 4,
                                                submit_index,
                                                4);
            }
        }
        else if (packet.bitfields0.STATE_TYPE == ST6_UBO)
            AppendSharps("A6XX_UBO", bindless ? 16 : sizeof(A6XX_UBO));
        else if (packet.bitfields0.STATE_TYPE == ST6_IBO)
        {
            DIVE_ASSERT(packet.bitfields0.STATE_BLOCK == SB6_CS_SHADER);
            AppendSharps("A6XX_TEX_CONST", sizeof(A6XX_TEX_CONST));
        }
        break;
    case StateBlockCat::kIbo:
        if (packet.bitfields0.STATE_TYPE == ST6_SHADER)
            AppendSharps("A6XX_TEX_CONST", bindless ? 16 : sizeof(A6XX_TEX_CONST));
        else if (packet.bitfields0.STATE_TYPE == ST6_CONSTANTS)
        {
            // NUM_UNIT is in unit of 2 dwords
            // Add 2 dwords per line
            AddConstantsToPacketNode<uint32_t>(mem_manager,
                                               ext_src_addr,
                                               packet_node_index,
                                               packet.bitfields0.NUM_UNIT * 2,
                                               submit_index,
                                               2);
        }
        else if (packet.bitfields0.STATE_TYPE == ST6_UBO)
        {
            // TODO(wangra): could dump textures here
        }
        break;
    default:
        DIVE_ASSERT(false);
        break;
    }
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::AppendMemRegNodes(const IMemoryManager &mem_manager,
                                                uint32_t              submit_index,
                                                uint64_t              va_addr,
                                                uint64_t              packet_node_index)
{
    PM4_CP_MEM_TO_REG packet;
    DIVE_VERIFY(mem_manager.RetrieveMemoryData(&packet, submit_index, va_addr, sizeof(packet)));

    // Add base register name
    const RegInfo *reg_info_ptr = GetRegInfo(packet.bitfields0.REG);
    RegInfo        temp = {};
    temp.m_name = "Unknown";
    temp.m_enum_handle = UINT8_MAX;
    if (reg_info_ptr == nullptr)
        reg_info_ptr = &temp;
    std::ostringstream reg_string_stream;
    reg_string_stream << "Base Register: " << reg_info_ptr->m_name;
    CommandHierarchy::AuxInfo aux_info = CommandHierarchy::AuxInfo::RegFieldNode(false);
    uint64_t reg_node_index = AddNode(NodeType::kRegNode, reg_string_stream.str(), aux_info);
    AddChild(CommandHierarchy::kSubmitTopology, packet_node_index, reg_node_index);
    AddChild(CommandHierarchy::kAllEventTopology, packet_node_index, reg_node_index);

    // Add memory data values
    uint64_t ext_src_addr = ((uint64_t)packet.bitfields2.SRC_HI << 32) |
                            (uint64_t)packet.bitfields1.SRC;
    AddConstantsToPacketNode<uint32_t>(mem_manager,
                                       ext_src_addr,
                                       packet_node_index,
                                       packet.bitfields0.CNT,
                                       submit_index,
                                       8);
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::CacheSetDrawStateGroupInfo(const IMemoryManager &mem_manager,
                                                         uint32_t              submit_index,
                                                         uint64_t              va_addr,
                                                         uint64_t  set_draw_state_node_index,
                                                         Pm4Header header)
{
    // Find all the children of the set_draw_state packet, which should contain array indices
    // Using any of the topologies where field nodes are added will work
    uint64_t              index = set_draw_state_node_index;
    DiveVector<uint64_t> &children = m_node_children[CommandHierarchy::kSubmitTopology]
                                                    [kSingleParentNodeChildren][index];

    // Obtain the address of each of the children group IBs
    PM4_CP_SET_DRAW_STATE packet;
    DIVE_VERIFY(mem_manager.RetrieveMemoryData(&packet,
                                               submit_index,
                                               va_addr,
                                               (header.type7.count + 1) * sizeof(uint32_t)));

    // Sanity check: The # of children should match the array size
    uint32_t total_size_bytes = (header.type7.count * sizeof(uint32_t));
    uint32_t per_element_size = sizeof(PM4_CP_SET_DRAW_STATE::ARRAY_ELEMENT);
    uint32_t array_size = total_size_bytes / per_element_size;
    DIVE_ASSERT(total_size_bytes % per_element_size == 0);
    DIVE_ASSERT(children.size() == array_size);

    // Cache group node index and address
    for (uint32_t i = 0; i < array_size; ++i)
    {
        uint64_t ib_addr = ((uint64_t)packet.ARRAY[i].bitfields2.ADDR_HI << 32) |
                           (uint64_t)packet.ARRAY[i].bitfields1.ADDR_LO;
        m_group_info[i].m_group_node_index = children[i];
        m_group_info[i].m_group_addr = ib_addr;
    }

    m_group_info_size = array_size;
}

//--------------------------------------------------------------------------------------------------
uint64_t CommandHierarchyCreator::AddNode(NodeType                  type,
                                          std::string             &&desc,
                                          CommandHierarchy::AuxInfo aux_info)
{
    uint64_t node_index = m_command_hierarchy.AddNode(type, std::move(desc), aux_info);
    for (uint32_t i = 0; i < CommandHierarchy::kTopologyTypeCount; ++i)
    {
        DIVE_ASSERT(m_node_children[i][kSingleParentNodeChildren].size() == node_index);
        DIVE_ASSERT(m_node_children[i][kSharedNodeChildren].size() == node_index);
        m_node_children[i][kSingleParentNodeChildren].resize(
        m_node_children[i][kSingleParentNodeChildren].size() + 1);
        m_node_children[i][kSharedNodeChildren].resize(
        m_node_children[i][kSharedNodeChildren].size() + 1);

        m_node_start_shared_children[i].resize(m_node_start_shared_children[i].size() + 1);
        m_node_end_shared_children[i].resize(m_node_end_shared_children[i].size() + 1);
        m_node_root_node_indices[i].resize(m_node_root_node_indices[i].size() + 1);
        DIVE_ASSERT(m_node_start_shared_children[i].size() == m_node_end_shared_children[i].size());
        DIVE_ASSERT(m_node_start_shared_children[i].size() == m_node_root_node_indices[i].size());
    }

    return node_index;
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::AppendEventNodeIndex(uint64_t node_index)
{
    m_command_hierarchy.m_nodes.m_event_node_indices.push_back(node_index);
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::AddChild(CommandHierarchy::TopologyType type,
                                       uint64_t                       node_index,
                                       uint64_t                       child_node_index)
{
    // Store children info into the temporary m_node_children
    // Use this to create the appropriate topology later
    DIVE_ASSERT(node_index < m_node_children[type][kSingleParentNodeChildren].size());
    m_node_children[type][kSingleParentNodeChildren][node_index].push_back(child_node_index);
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::AddSharedChild(CommandHierarchy::TopologyType type,
                                             uint64_t                       node_index,
                                             uint64_t                       child_node_index)
{
    // Store children info into the temporary m_node_children
    // Use this to create the appropriate topology later
    DIVE_ASSERT(node_index < m_node_children[type][kSharedNodeChildren].size());
    m_node_children[type][kSharedNodeChildren][node_index].push_back(child_node_index);
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::SetStartSharedChildrenNodeIndex(CommandHierarchy::TopologyType type,
                                                              uint64_t node_index,
                                                              uint64_t shared_child_node_index)
{
    DIVE_ASSERT(node_index < m_node_start_shared_children[type].size());
    m_node_start_shared_children[type][node_index] = shared_child_node_index;
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::SetEndSharedChildrenNodeIndex(CommandHierarchy::TopologyType type,
                                                            uint64_t node_index,
                                                            uint64_t shared_child_node_index)
{
    DIVE_ASSERT(node_index < m_node_end_shared_children[type].size());
    m_node_end_shared_children[type][node_index] = shared_child_node_index;
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::SetSharedChildRootNodeIndex(CommandHierarchy::TopologyType type,
                                                          uint64_t                       node_index,
                                                          uint64_t root_node_index)
{
    DIVE_ASSERT(node_index < m_node_root_node_indices[type].size());
    m_node_root_node_indices[type][node_index] = root_node_index;
}

//--------------------------------------------------------------------------------------------------
uint64_t CommandHierarchyCreator::GetChildNodeIndex(CommandHierarchy::TopologyType type,
                                                    uint64_t                       node_index,
                                                    uint64_t child_index) const
{
    DIVE_ASSERT(node_index < m_node_children[type][kSingleParentNodeChildren].size());
    DIVE_ASSERT(child_index < m_node_children[type][kSingleParentNodeChildren][node_index].size());
    return m_node_children[type][kSingleParentNodeChildren][node_index][child_index];
}

//--------------------------------------------------------------------------------------------------
uint64_t CommandHierarchyCreator::GetChildCount(CommandHierarchy::TopologyType type,
                                                uint64_t                       node_index) const
{
    DIVE_ASSERT(node_index < m_node_children[type][kSingleParentNodeChildren].size());
    return m_node_children[type][kSingleParentNodeChildren][node_index].size();
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::CreateTopologies()
{
    uint64_t total_num_children[CommandHierarchy::kTopologyTypeCount] = {};
    uint64_t total_num_shared_children[CommandHierarchy::kTopologyTypeCount] = {};

    // Convert the m_node_children temporary structure into CommandHierarchy's topologies
    for (uint32_t topology = 0; topology < CommandHierarchy::kTopologyTypeCount; ++topology)
    {
        size_t              num_nodes = m_node_children[topology][kSingleParentNodeChildren].size();
        SharedNodeTopology &cur_topology = m_command_hierarchy.m_topology[topology];
        cur_topology.SetNumNodes(num_nodes);

        // Optional loop: Pre-reserve to prevent the resize() from allocating memory later
        // Note: The number of children for some of the topologies have been determined
        // earlier in this function already
        if (total_num_children[topology] == 0 && total_num_shared_children[topology] == 0)
        {
            for (uint64_t node_index = 0; node_index < num_nodes; ++node_index)
            {
                auto &node_children = m_node_children[topology];
                total_num_children[topology] += node_children[kSingleParentNodeChildren][node_index]
                                                .size();
                total_num_shared_children[topology] += node_children[1][node_index].size();
            }
        }
        cur_topology.m_children_list.reserve(total_num_children[topology]);
        cur_topology.m_shared_children_indices.reserve(total_num_shared_children[topology]);

        for (uint64_t node_index = 0; node_index < num_nodes; ++node_index)
        {
            DIVE_ASSERT(m_node_children[topology][kSingleParentNodeChildren].size() ==
                        m_node_children[topology][kSingleParentNodeChildren].size());
            cur_topology
            .AddChildren(node_index,
                         m_node_children[topology][kSingleParentNodeChildren][node_index]);
            cur_topology
            .AddSharedChildren(node_index,
                               m_node_children[topology][kSharedNodeChildren][node_index]);
        }
        cur_topology.m_start_shared_child = std::move(m_node_start_shared_children[topology]);
        cur_topology.m_end_shared_child = std::move(m_node_end_shared_children[topology]);
        cur_topology.m_root_node_index = std::move(m_node_root_node_indices[topology]);
    }
}

//--------------------------------------------------------------------------------------------------
bool CommandHierarchyCreator::EventNodeHelper(uint64_t                      node_index,
                                              std::function<bool(uint32_t)> callback) const
{
    NodeType node_type = m_command_hierarchy.GetNodeType(node_index);
    if (node_type == NodeType::kMarkerNode)
    {
        CommandHierarchy::MarkerType type = m_command_hierarchy.GetMarkerNodeType(node_index);
        if (type == CommandHierarchy::MarkerType::kDiveMetadata)
            return callback(m_command_hierarchy.GetMarkerNodeId(node_index));
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
template<typename T> struct OutputStream
{
    static void SetupFormat(std::ostringstream &stream) {}
};

//--------------------------------------------------------------------------------------------------
template<> struct OutputStream<float>
{
    static void SetupFormat(std::ostringstream &stream)
    {
        stream << std::fixed << std::setw(11) << std::setprecision(6);
    }
};

//--------------------------------------------------------------------------------------------------
template<> struct OutputStream<uint32_t>
{
    static void SetupFormat(std::ostringstream &stream)
    {
        stream << "0x" << std::hex << std::setfill('0') << std::setw(8);
    }
};

//--------------------------------------------------------------------------------------------------
template<typename T>
void CommandHierarchyCreator::AddConstantsToPacketNode(const IMemoryManager &mem_manager,
                                                       uint64_t              ext_src_addr,
                                                       uint64_t              packet_node_index,
                                                       uint32_t              num_dwords,
                                                       uint32_t              submit_index,
                                                       uint32_t              value_count_per_row)
{
    for (uint32_t i = 0; i < num_dwords; i += value_count_per_row)
    {
        std::ostringstream string_stream;
        for (uint32_t j = 0; j < value_count_per_row; ++j)
        {
            if ((i + j) < num_dwords)
            {
                T        value;
                uint64_t addr = ext_src_addr + ((i + j) * sizeof(T));

                // For some reason, some captures refer to memory not backed by memory blocks
                // Let's not treat it as an error, since cffdump handles this gracefully as well
                if (!mem_manager.RetrieveMemoryData(&value, submit_index, addr, sizeof(T)))
                {
                    DIVE_LOG("Indirect constant buffer at 0x%p with no backing memory!",
                             ext_src_addr);
                    return;
                }
                OutputStream<T>::SetupFormat(string_stream);
                string_stream << value << " ";
            }
        }

        // Add it as child to packet_node
        CommandHierarchy::AuxInfo aux_info = CommandHierarchy::AuxInfo::RegFieldNode(false);
        uint64_t const_node_index = AddNode(NodeType::kFieldNode, string_stream.str(), aux_info);
        AddChild(CommandHierarchy::kSubmitTopology, packet_node_index, const_node_index);
        AddChild(CommandHierarchy::kAllEventTopology, packet_node_index, const_node_index);
    }
}

}  // namespace Dive
