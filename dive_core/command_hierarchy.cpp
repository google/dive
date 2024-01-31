
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

#include "dive_strings.h"
#include "log.h"
#include "pm4_info.h"

namespace Dive
{
// =================================================================================================
// Helper Functions
// =================================================================================================
enum class TcCacheOp
{
    kNop = 0,    // Do nothing.
    kWbInvL1L2,  // Flush TCC data and invalidate all TCP and TCC data
    kWbInvL2Nc,  // Flush and invalidate all TCC data that used the non-coherent MTYPE.
    kWbL2Nc,     // Flush all TCC data that used the non-coherent MTYPE.
    kWbL2Wc,     // Flush all TCC data that used the write-combined MTYPE.
    kInvL2Nc,    // Invalidate all TCC data that used the non-coherent MTYPE.
    kInvL2Md,    // Invalidate the TCC's read-only metadata cache.
    kInvL1,      // Invalidate all TCP data.
    kInvL1Vol,   // Invalidate all volatile TCP data.
    kCount
};
const char *TcCacheOpStrings[] = {
    nullptr,      // kNop
    "wbInvL1L2",  // kWbInvL1L2
    "wbInvL2",    // kWbInvL2Nc
    "wbL2",       // kWbL2Nc
    "wbL2Wc",     // kWbL2Wc (Not used)
    "invL2",      // kInvL2Nc
    "invL2Md",    // kInvL2Md
    "invL1",      // kInvL1
    "invL1Vol",   // kInvL1Vol (Not used)
};
TcCacheOp GetCacheOp(uint32_t cp_coher_cntl)
{
    return TcCacheOp::kNop;
}

static const std::string kRenderPassName = "RenderPass";

// =================================================================================================
// Topology
// =================================================================================================
uint64_t Topology::GetNumNodes() const
{
    DIVE_ASSERT(m_node_children.size() == m_node_shared_children.size());
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
uint64_t Topology::GetNumSharedChildren(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_node_shared_children.size());
    return m_node_shared_children[node_index].m_num_children;
}
//--------------------------------------------------------------------------------------------------
uint64_t Topology::GetSharedChildNodeIndex(uint64_t node_index, uint64_t child_index) const
{
    DIVE_ASSERT(node_index < m_node_shared_children.size());
    DIVE_ASSERT(child_index < m_node_shared_children[node_index].m_num_children);
    uint64_t child_list_index = m_node_shared_children[node_index].m_start_index + child_index;
    DIVE_ASSERT(child_list_index < m_shared_children_list.size());
    return m_shared_children_list[child_list_index];
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
    m_node_shared_children.resize(num_nodes);
    m_node_parent.resize(num_nodes, UINT64_MAX);
    m_node_child_index.resize(num_nodes, UINT64_MAX);
}

//--------------------------------------------------------------------------------------------------
void Topology::AddChildren(uint64_t node_index, const std::vector<uint64_t> &children)
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

//--------------------------------------------------------------------------------------------------
void Topology::AddSharedChildren(uint64_t node_index, const std::vector<uint64_t> &children)
{
    DIVE_ASSERT(m_node_shared_children.size() == m_node_parent.size());
    DIVE_ASSERT(m_node_shared_children.size() == m_node_child_index.size());

    // Append to m_shared_children_list
    uint64_t prev_size = m_shared_children_list.size();
    m_shared_children_list.resize(m_shared_children_list.size() + children.size());
    std::copy(children.begin(), children.end(), m_shared_children_list.begin() + prev_size);

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
const Topology &CommandHierarchy::GetEngineHierarchyTopology() const
{
    return m_topology[kEngineTopology];
}
//--------------------------------------------------------------------------------------------------
const Topology &CommandHierarchy::GetSubmitHierarchyTopology() const
{
    return m_topology[kSubmitTopology];
}

//--------------------------------------------------------------------------------------------------
const Topology &CommandHierarchy::GetVulkanDrawEventHierarchyTopology() const
{
    return m_topology[kVulkanEventTopology];
}

//--------------------------------------------------------------------------------------------------
const Topology &CommandHierarchy::GetVulkanEventHierarchyTopology() const
{
    return m_topology[kVulkanCallTopology];
}

//--------------------------------------------------------------------------------------------------
const Topology &CommandHierarchy::GetAllEventHierarchyTopology() const
{
    return m_topology[kAllEventTopology];
}

//--------------------------------------------------------------------------------------------------
const Topology &CommandHierarchy::GetRgpHierarchyTopology() const
{
    return m_topology[kRgpTopology];
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
const std::vector<uint8_t> &CommandHierarchy::GetMetadata(uint64_t node_index) const
{
    DIVE_ASSERT(node_index < m_nodes.m_metadata.size());
    return m_nodes.m_metadata[node_index];
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
    DIVE_ASSERT(m_nodes.m_node_type[node_index] == Dive::NodeType::kDrawDispatchBlitNode);
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
uint64_t CommandHierarchy::AddNode(NodeType           type,
                                   const std::string &desc,
                                   AuxInfo            aux_info,
                                   char              *metadata_ptr,
                                   uint32_t           metadata_size)
{
    return m_nodes.AddNode(type, desc, aux_info, metadata_ptr, metadata_size);
}

//--------------------------------------------------------------------------------------------------
size_t CommandHierarchy::GetEventIndex(uint64_t node_index) const
{
    const std::vector<uint64_t> &indices = m_nodes.m_event_node_indices;
    auto                         it = std::lower_bound(indices.begin(), indices.end(), node_index);
    if (it == indices.end() || *it != node_index)
    {
        return 0;
    }
    return it - indices.begin() + 1;
}

// =================================================================================================
// CommandHierarchy::Nodes
// =================================================================================================
uint64_t CommandHierarchy::Nodes::AddNode(NodeType           type,
                                          const std::string &desc,
                                          AuxInfo            aux_info,
                                          char              *metadata_ptr,
                                          uint32_t           metadata_size)
{
    DIVE_ASSERT(m_node_type.size() == m_description.size());
    DIVE_ASSERT(m_node_type.size() == m_aux_info.size());
    DIVE_ASSERT(m_node_type.size() == m_metadata.size());

    m_node_type.push_back(type);
    m_description.push_back(desc);
    m_aux_info.push_back(aux_info);

    std::vector<uint8_t> temp(metadata_size);
    if (metadata_ptr != nullptr)
        memcpy(&temp[0], metadata_ptr, metadata_size);
    m_metadata.push_back(std::move(temp));

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
CommandHierarchyCreator::CommandHierarchyCreator(EmulateStateTracker &state_tracker) :
    m_state_tracker(state_tracker)
{
    m_state_tracker.Reset();
}

//--------------------------------------------------------------------------------------------------
bool CommandHierarchyCreator::CreateTrees(CommandHierarchy  *command_hierarchy_ptr,
                                          const CaptureData &capture_data,
                                          bool               flatten_chain_nodes,
                                          ILog              *log_ptr)
{
    m_log_ptr = log_ptr;

    m_command_hierarchy_ptr = command_hierarchy_ptr;
    m_capture_data_ptr = &capture_data;

    // Clear/Reset internal data structures, just in case
    *m_command_hierarchy_ptr = CommandHierarchy();

    // Add a dummy root node for easier management
    uint64_t root_node_index = AddNode(NodeType::kRootNode, "", 0);
    DIVE_VERIFY(root_node_index == Topology::kRootNodeIndex);

    // Add each engine type to the frame_node
    std::vector<uint64_t> engine_nodes;
    for (uint32_t engine_type = 0; engine_type < (uint32_t)EngineType::kCount; ++engine_type)
    {
        uint64_t node_index = AddNode(NodeType::kEngineNode, kEngineTypeStrings[engine_type], 0);
        AddChild(CommandHierarchy::kEngineTopology, Topology::kRootNodeIndex, node_index);
    }

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
bool CommandHierarchyCreator::CreateTrees(CommandHierarchy *command_hierarchy_ptr,
                                          EngineType        engine_type,
                                          QueueType         queue_type,
                                          uint32_t         *command_dwords,
                                          uint32_t          size_in_dwords,
                                          ILog             *log_ptr)
{
    // Note: This function is mostly a copy/paste from the main CreateTrees() function, but with
    // workarounds to handle a case where there is no marker_data or capture_data
    class TempMemoryManager : public IMemoryManager
    {
    public:
        TempMemoryManager(uint32_t *command_dwords, uint32_t size_in_dwords) :
            m_command_dwords(command_dwords),
            m_size_in_dwords(size_in_dwords)
        {
        }

        // Copy the given va/size from the memory blocks
        virtual bool CopyMemory(void    *buffer_ptr,
                                uint32_t submit_index,
                                uint64_t va_addr,
                                uint64_t size) const
        {
            if ((va_addr + size) > (m_size_in_dwords * sizeof(uint32_t)))
                return false;

            // Treat the va_addr as an offset
            uint8_t *command_bytes = (uint8_t *)m_command_dwords;
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
        uint32_t *m_command_dwords;
        uint32_t  m_size_in_dwords;
    };

    m_log_ptr = log_ptr;

    m_command_hierarchy_ptr = command_hierarchy_ptr;
    m_capture_data_ptr = nullptr;

    // Clear/Reset internal data structures, just in case
    *m_command_hierarchy_ptr = CommandHierarchy();

    // Add a dummy root node for easier management
    uint64_t root_node_index = AddNode(NodeType::kRootNode, "", 0);
    DIVE_VERIFY(root_node_index == Topology::kRootNodeIndex);

    // Add each engine type to the frame_node
    std::vector<uint64_t> engine_nodes;
    {
        uint64_t node_index = AddNode(NodeType::kEngineNode,
                                      kEngineTypeStrings[(uint32_t)engine_type],
                                      0);
        AddChild(CommandHierarchy::kEngineTopology, Topology::kRootNodeIndex, node_index);
    }

    m_num_events = 0;
    m_flatten_chain_nodes = false;

    Dive::IndirectBufferInfo ib_info;
    ib_info.m_va_addr = 0x0;
    ib_info.m_size_in_dwords = size_in_dwords;
    ib_info.m_skip = false;
    std::vector<IndirectBufferInfo> ib_array;
    ib_array.push_back(ib_info);
    const Dive::SubmitInfo submit_info(engine_type, queue_type, 0, false, std::move(ib_array));

    std::vector<SubmitInfo> submits{ submit_info };
    TempMemoryManager       mem_manager(command_dwords, size_in_dwords);
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
    m_state_tracker.PushEnableMask(ib_info.m_enable_mask);
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
            IbType   cur_type = m_command_hierarchy_ptr->GetIbNodeType(index);
            if (cur_type != IbType::kChain)
            {
                parent_node_index = index;
                break;
            }
        }
    }

    AddChild(CommandHierarchy::kEngineTopology, parent_node_index, ib_node_index);
    AddChild(CommandHierarchy::kSubmitTopology, parent_node_index, ib_node_index);

    m_ib_stack.push_back(ib_node_index);
    m_cmd_begin_packet_node_indices.clear();
    m_cmd_begin_event_node_indices.clear();
    return true;
}

//--------------------------------------------------------------------------------------------------
bool CommandHierarchyCreator::OnIbEnd(uint32_t                  submit_index,
                                      uint32_t                  ib_index,
                                      const IndirectBufferInfo &ib_info)
{
    m_state_tracker.PopEnableMask();
    DIVE_ASSERT(!m_ib_stack.empty());

    // Note: This callback is only called for the last CHAIN of a series of daisy-CHAIN IBs,
    // because the emulator does not keep track of IBs in an internal stack. So start by
    // popping all consecutive CHAIN IBs
    IbType type;
    type = m_command_hierarchy_ptr->GetIbNodeType(m_ib_stack.back());
    while (!m_ib_stack.empty() && type == IbType::kChain)
    {
        m_ib_stack.pop_back();
        type = m_command_hierarchy_ptr->GetIbNodeType(m_ib_stack.back());
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
                                       Pm4Type               type,
                                       uint32_t              header)
{
    if (!m_state_tracker.OnPacket(mem_manager, submit_index, ib_index, va_addr, type, header))
        return false;
    // THIS IS TEMPORARY! Only deal with typ4 & type7 packets for now
    if ((type != Pm4Type::kType4) && (type != Pm4Type::kType7))
        return true;

    // Create the packet node and add it as child to the current submit_node and ib_node
    uint64_t packet_node_index = AddPacketNode(mem_manager,
                                               submit_index,
                                               va_addr,
                                               false,
                                               type,
                                               header);

    uint64_t parent_index = m_shared_node_ib_parent_stack[m_cur_ib_level];
    AddSharedChild(CommandHierarchy::kEngineTopology, parent_index, packet_node_index);
    AddSharedChild(CommandHierarchy::kSubmitTopology, parent_index, packet_node_index);
    AddSharedChild(CommandHierarchy::kAllEventTopology, parent_index, packet_node_index);
    AddSharedChild(CommandHierarchy::kRgpTopology, parent_index, packet_node_index);

    AddSharedChild(CommandHierarchy::kEngineTopology, m_ib_stack.back(), packet_node_index);
    AddSharedChild(CommandHierarchy::kSubmitTopology, m_ib_stack.back(), packet_node_index);

    uint32_t opcode = UINT32_MAX;
    if (type == Pm4Type::kType7)
    {
        Pm4Type7Header *type7_header = (Pm4Type7Header *)&header;
        opcode = type7_header->opcode;
    }

    // Cache packets to be shown on event nodes
    {
        // Cache all packets added (will cache until encounter next event/IB)
        m_packets.Add(opcode, va_addr, packet_node_index);

        // Cache packets that may be part of the vkBeginCommandBuffer.
        m_cmd_begin_packet_node_indices.push_back(packet_node_index);
    }

    // Cache set_draw_state packet
    if (opcode == CP_SET_DRAW_STATE)
        CacheSetDrawStateGroupInfo(mem_manager, submit_index, va_addr, packet_node_index, header);

    bool is_marker = false;
    if (IsDrawDispatchBlitSyncEvent(mem_manager, submit_index, va_addr, opcode))
    {
        uint64_t event_node_index = UINT64_MAX;
        uint64_t parent_node_index = m_cur_submit_node_index;
        if (!m_marker_stack.empty())
        {
            parent_node_index = m_marker_stack.back();
        }

        if (m_render_marker_index != kInvalidRenderMarkerIndex)
        {
            parent_node_index = m_render_marker_index;
        }

        SyncType sync_type = GetSyncType(mem_manager, submit_index, va_addr, opcode);
        if (sync_type != SyncType::kNone)
        {
            // m_num_events++;
            // auto     barrier_it = m_marker_creator.CurrentBarrier();
            // uint64_t sync_event_node_index = AddSyncEventNode(mem_manager,
            //                                                   submit_index,
            //                                                   va_addr,
            //                                                   sync_type,
            //                                                   barrier_it->id());
            // event_node_index = sync_event_node_index;
            // if (barrier_it->IsValid() && barrier_it->BarrierNode() != UINT64_MAX)
            //     parent_node_index = barrier_it->BarrierNode();
            // this->AppendEventNodeIndex(sync_event_node_index);
        }
        else  // Draw/Dispatch/Blit
        {
            std::string draw_dispatch_node_string = GetEventString(mem_manager,
                                                                   submit_index,
                                                                   va_addr,
                                                                   opcode);
            uint32_t    event_id = m_num_events++;
            // auto        marker_it = m_marker_creator.CurrentEvent();
            // if (!(marker_it->IsValid() && marker_it->EventNode() == UINT64_MAX))
            // {
            //     marker_it = nullptr;
            // }
            CommandHierarchy::AuxInfo aux_info = CommandHierarchy::AuxInfo::EventNode(event_id);
            uint64_t draw_dispatch_node_index = AddNode(NodeType::kDrawDispatchBlitNode,
                                                        draw_dispatch_node_string,
                                                        aux_info);
            AppendEventNodeIndex(draw_dispatch_node_index);
            // if (marker_it->IsValid())
            //     m_markers_ptr->SetEventNode(marker_it->id(), draw_dispatch_node_index);

            // auto barrier_it = m_marker_creator.CurrentBarrier();
            // if (barrier_it->IsValid() && barrier_it->BarrierNode() != UINT64_MAX)
            //     parent_node_index = barrier_it->BarrierNode();

            event_node_index = draw_dispatch_node_index;
        }

        // Cache nodes that may be part of the vkBeginCommandBuffer.
        m_cmd_begin_event_node_indices.push_back(event_node_index);

        // Add as children all packets that have been processed since the last event
        // Note: Events only show up in the event topology and internal RGP topology.
        for (uint32_t packet = 0; packet < m_packets.m_packet_node_indices.size(); ++packet)
        {
            uint64_t cur_node_index = m_packets.m_packet_node_indices[packet];
            AddSharedChild(CommandHierarchy::kAllEventTopology, event_node_index, cur_node_index);
            AddSharedChild(CommandHierarchy::kRgpTopology, event_node_index, cur_node_index);
        }
        m_packets.Clear();

        // Add the draw_dispatch_node to the submit_node if currently not inside a marker range.
        // Otherwise append it to the marker at the top of the marker stack.
        // Note: Events only show up in the event topology and internal RGP topology.
        AddChild(CommandHierarchy::kAllEventTopology, parent_node_index, event_node_index);

        m_node_parent_info[CommandHierarchy::kAllEventTopology]
                          [event_node_index] = parent_node_index;

        if (!m_internal_marker_stack.empty())
        {
            parent_node_index = m_internal_marker_stack.back();
        }
        AddChild(CommandHierarchy::kRgpTopology, parent_node_index, event_node_index);
        m_node_parent_info[CommandHierarchy::kRgpTopology][event_node_index] = parent_node_index;
    }
    else if ((opcode == CP_INDIRECT_BUFFER_PFE || opcode == CP_INDIRECT_BUFFER_PFD ||
              opcode == CP_INDIRECT_BUFFER_CHAIN || opcode == CP_COND_INDIRECT_BUFFER_PFE ||
              opcode == CP_SET_CTXSWITCH_IB))
    {
        m_cur_ib_packet_node_index = packet_node_index;

        // m_packets contain packets added to an event node
        // Do not show INDIRECT_BUFFER packets in an event node. A flattening of the packets is
        // required for event nodes, otherwise it can result in a weird situation where all the
        // draw calls are added to an INDIRECT_BUFFER, but the INDIRECT_BUFFER is added only to
        // the first event node
        m_packets.Pop();
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
    // vulkan call NOP packages. Currently contains call parameters(except parameters in array),
    // each call is in one NOP packet.
    else if (opcode == CP_NOP)
    {
        /*
        NopVulkanCallHeader nop_header;
        bool                ret = mem_manager.CopyMemory(&nop_header.u32All,
                                          submit_index,
                                          va_addr + sizeof(PM4_PFP_TYPE_3_HEADER),
                                          sizeof(nop_header));
        DIVE_VERIFY(ret);

        if (nop_header.signature == kNopPayloadSignature)
        {
            is_marker = true;
            uint32_t          vulkan_call_data_len = (header.count + 1) * sizeof(uint32_t);
            std::vector<char> vulkan_call_data(vulkan_call_data_len);
            ret = mem_manager.CopyMemory(vulkan_call_data.data(),
                                         submit_index,
                                         va_addr + sizeof(PM4_PFP_TYPE_3_HEADER),
                                         vulkan_call_data_len);
            DIVE_ASSERT(ret);

            ParseVulkanCallMarker(vulkan_call_data.data(),
                                  vulkan_call_data_len,
                                  m_cur_submit_node_index,
                                  packet_node_index);
            is_marker_parsed = true;
            m_command_hierarchy_ptr->m_has_vulkan_marker = true;
        }
                */
    }
    else if (opcode == CP_SET_MARKER)
    {
        PM4_CP_SET_MARKER packet;
        DIVE_VERIFY(mem_manager.CopyMemory(&packet, submit_index, va_addr, sizeof(packet)));
        // as mentioned in adreno_pm4.xml, only b0-b3 are considered when b8 is not set
        DIVE_ASSERT((packet.u32All0 & 0x100) == 0);
        a6xx_marker    marker = static_cast<a6xx_marker>(packet.u32All0 & 0xf);
        const uint64_t parent_node_index = m_cur_submit_node_index;

        std::string desc;
        bool        add_child = true;
        switch (marker)
        {
            // This is emitted at the begining of the render pass if tiled rendering mode is
            // disabled
        case RM6_BYPASS:
            desc = "Direct Rendering Pass";
            break;
            // This is emitted at the begining of the binning pass, although the binning pass could
            // be missing even in tiled rendering mode
        case RM6_BINNING:
            desc = "Binning Pass";
            break;
            // This is emitted at the begining of the tiled rendering pass
        case RM6_GMEM:
            desc = "Tile Rendering Pass";
            break;
            // This is emitted at the end of the tiled rendering pass
        case RM6_ENDVIS:
            m_render_marker_index = kInvalidRenderMarkerIndex;
            add_child = false;
            break;
            // This is emitted at the begining of the resolve pass
        case RM6_RESOLVE:
            desc = "Resolve Pass";
            break;
            // This is emitted for each dispatch
        case RM6_COMPUTE:
            desc = "Compute Dispatch";
            break;
            // TODO(wangra): Might need to handle following markers
        case RM6_YIELD:
        case RM6_BLIT2DSCALE:
        case RM6_IB1LIST_START:
        case RM6_IB1LIST_END:
        default: add_child = false; break;   
        }

        if (add_child)
        {
            m_render_marker_index = AddNode(NodeType::kRenderMarkerNode, desc, 0);
            AddChild(CommandHierarchy::kAllEventTopology, parent_node_index, m_render_marker_index);
        }
    }

    if (m_render_marker_index != kInvalidRenderMarkerIndex)
    {
        AddSharedChild(CommandHierarchy::kAllEventTopology,
                       m_render_marker_index,
                       packet_node_index);
    }

    // This packet is potentially implicit NOP packet for vkBeginCommandBuffer
    // if (is_marker_parsed && !m_is_parsing_cb_start_marker)
    // {
    //     m_cmd_begin_packet_node_indices.clear();
    //     m_cmd_begin_event_node_indices.clear();
    // }

    if (!is_marker)
    {
        // Add it to all markers on stack, if applicable.
        for (auto it = m_marker_stack.begin(); it != m_marker_stack.end(); ++it)
            AddSharedChild(CommandHierarchy::kAllEventTopology, *it, packet_node_index);

        for (auto it = m_internal_marker_stack.begin(); it != m_internal_marker_stack.end(); ++it)
            AddSharedChild(CommandHierarchy::kRgpTopology, *it, packet_node_index);
    }

    // auto cb_id = m_marker_creator.CurrentCommandBuffer()->id();
    // if (cb_id != m_cur_cb)
    // {
    //     const auto &cbs = m_markers_ptr->CommandBuffers();
    //     if (cbs.IsValidId(cb_id))
    //         m_markers_ptr->SetCommandBufferSubmit(cb_id, m_cur_engine_index);
    //     m_cur_cb = cb_id;
    // }
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

    // Add submit node as child to the appropriate engine node
    uint64_t engine_node_index = GetChildNodeIndex(CommandHierarchy::kEngineTopology,
                                                   Topology::kRootNodeIndex,
                                                   engine_index);
    AddChild(CommandHierarchy::kEngineTopology, engine_node_index, submit_node_index);

    // Add submit node to the other topologies as children to the root node
    AddChild(CommandHierarchy::kSubmitTopology, Topology::kRootNodeIndex, submit_node_index);
    AddChild(CommandHierarchy::kAllEventTopology, Topology::kRootNodeIndex, submit_node_index);
    AddChild(CommandHierarchy::kRgpTopology, Topology::kRootNodeIndex, submit_node_index);
    m_cur_submit_node_index = submit_node_index;
    m_cur_ib_level = 1;
    m_shared_node_ib_parent_stack[m_cur_ib_level] = m_cur_submit_node_index;
    m_cur_ib_packet_node_index = UINT64_MAX;
    m_ib_stack.clear();
    m_render_marker_index = kInvalidRenderMarkerIndex;
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::OnSubmitEnd(uint32_t submit_index, const SubmitInfo &submit_info)
{
    // For the submit topology, the IBs are inserted in emulation order, and are not necessarily in
    // ib-index order. Sort them here so they appear in order of ib-index.
    std::vector<uint64_t> &submit_children = m_node_children[CommandHierarchy::kSubmitTopology][0]
                                                            [m_cur_submit_node_index];
    std::sort(submit_children.begin(),
              submit_children.end(),
              [&](uint64_t lhs, uint64_t rhs) -> bool {
                  uint8_t lhs_index = m_command_hierarchy_ptr->GetIbNodeIndex(lhs);
                  uint8_t rhs_index = m_command_hierarchy_ptr->GetIbNodeIndex(rhs);
                  return lhs_index < rhs_index;
              });

    // If marker stack is not empty, that means those are vkCmdDebugMarkerBeginEXT() calls without
    // the corresponding vkCmdDebugMarkerEndEXT. Clear the market stack for the next submit.
    m_marker_stack.clear();
    m_internal_marker_stack.clear();

    if (!m_packets.m_packet_node_indices.empty())
    {
        uint64_t postamble_state_node_index;
        if (GetChildCount(CommandHierarchy::kAllEventTopology, m_cur_submit_node_index) != 0)
            postamble_state_node_index = AddNode(NodeType::kPostambleStateNode, "State");
        else
            postamble_state_node_index = AddNode(NodeType::kPostambleStateNode, "Postamble State");

        // Add to postamble_state_note all packets that have been processed since the last
        // draw/dispatch
        for (uint32_t packet = 0; packet < m_packets.m_packet_node_indices.size(); ++packet)
        {
            AddSharedChild(CommandHierarchy::kAllEventTopology,
                           postamble_state_node_index,
                           m_packets.m_packet_node_indices[packet]);
            AddSharedChild(CommandHierarchy::kRgpTopology,
                           postamble_state_node_index,
                           m_packets.m_packet_node_indices[packet]);
        }
        m_packets.Clear();

        uint64_t parent_node_index = m_cur_submit_node_index;
        if (m_render_marker_index != kInvalidRenderMarkerIndex)
        {
            parent_node_index = m_render_marker_index;
        }

        // Add the postamble_state_node to the submit_node in the event topology
        AddChild(CommandHierarchy::kAllEventTopology,
                 parent_node_index,
                 postamble_state_node_index);
        AddChild(CommandHierarchy::kRgpTopology, parent_node_index, postamble_state_node_index);
    }

    // Insert present node to event topology, when appropriate
    if (m_capture_data_ptr != nullptr)
    {
        for (uint32_t i = 0; i < m_capture_data_ptr->GetNumPresents(); ++i)
        {
            const PresentInfo &present_info = m_capture_data_ptr->GetPresentInfo(i);

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
                present_string_stream
                << "Present: " << i << ", FullScreen: " << present_info.IsFullScreen()
                << ", Engine: " << kEngineTypeStrings[(uint32_t)present_info.GetEngineType()]
                << ", Queue: " << kQueueTypeStrings[(uint32_t)present_info.GetQueueType()]
                << ", SurfaceAddr: 0x" << std::hex << present_info.GetSurfaceAddr() << std::dec
                << ", SurfaceSize: " << present_info.GetSurfaceSize()
                << ", VkFormat: " << format_string << ", VkColorSpaceKHR: " << color_space_string;
            }
            else
            {
                present_string_stream << "Present: " << i;
            }
            uint64_t present_node_index = AddNode(NodeType::kPresentNode,
                                                  present_string_stream.str());
            AddChild(CommandHierarchy::kAllEventTopology,
                     Topology::kRootNodeIndex,
                     present_node_index);
            AddChild(CommandHierarchy::kRgpTopology, Topology::kRootNodeIndex, present_node_index);
        }
    }
}

//--------------------------------------------------------------------------------------------------
uint64_t CommandHierarchyCreator::AddPacketNode(const IMemoryManager &mem_manager,
                                                uint32_t              submit_index,
                                                uint64_t              va_addr,
                                                bool                  is_ce_packet,
                                                Pm4Type               type,
                                                uint32_t              header)
{
    if (type == Pm4Type::kType7)
    {
        Pm4Type7Header type7_header;
        type7_header.u32All = header;

        std::ostringstream packet_string_stream;
        packet_string_stream << GetOpCodeString(type7_header.opcode);
        packet_string_stream << " 0x" << std::hex << type7_header.u32All << std::dec;

        CommandHierarchy::AuxInfo aux_info = CommandHierarchy::AuxInfo::PacketNode(va_addr,
                                                                                   type7_header
                                                                                   .opcode,
                                                                                   m_cur_ib_level);

        uint64_t packet_node_index = AddNode(NodeType::kPacketNode,
                                             packet_string_stream.str(),
                                             aux_info);
        /*
        if (type7_header.opcode == Pal::Gfx9::IT_SET_CONTEXT_REG)
        {
            // Note: IT_SET_CONTEXT_REG_INDEX does not appear to be used in the driver
            uint32_t start = Pal::Gfx9::CONTEXT_SPACE_START;
            uint32_t end = Pal::Gfx9::Gfx09_10::CONTEXT_SPACE_END;
            AppendRegNodes(mem_manager, submit_index, va_addr, start, end, header,
        packet_node_index);
        }
        else if (type7_header.opcode == Pal::Gfx9::IT_CONTEXT_REG_RMW)
        {
            AppendContextRegRmwNodes(mem_manager, submit_index, va_addr, header, packet_node_index);
        }
        else if ((type7_header.opcode == Pal::Gfx9::IT_SET_UCONFIG_REG) ||
                (type7_header.opcode == Pal::Gfx9::IT_SET_UCONFIG_REG_INDEX))
        {
            uint32_t start = Pal::Gfx9::UCONFIG_SPACE_START;
            uint32_t end = Pal::Gfx9::UCONFIG_SPACE_END;
            AppendRegNodes(mem_manager, submit_index, va_addr, start, end, header,
        packet_node_index);
        }
        else if (type7_header.opcode == Pal::Gfx9::IT_SET_CONFIG_REG)
        {
            uint32_t start = Pal::Gfx9::CONFIG_SPACE_START;
            uint32_t end = Pal::Gfx9::CONFIG_SPACE_END;
            AppendRegNodes(mem_manager, submit_index, va_addr, start, end, header,
        packet_node_index);
        }
        else if ((type7_header.opcode == Pal::Gfx9::IT_SET_SH_REG) ||
                (type7_header.opcode == Pal::Gfx9::IT_SET_SH_REG_INDEX))
        {
            uint32_t start = Pal::Gfx9::PERSISTENT_SPACE_START;
            uint32_t end = Pal::Gfx9::PERSISTENT_SPACE_END;
            AppendRegNodes(mem_manager, submit_index, va_addr, start, end, header,
        packet_node_index);
        }
        else if (type7_header.opcode == Pal::Gfx9::IT_INDIRECT_BUFFER_CNST)
        {
            // IT_INDIRECT_BUFFER_CNST aliases IT_COND_INDIRECT_BUFFER_CNST, but have different
            // packet formats. So need to handle them manually.
            AppendIBFieldNodes("INDIRECT_BUFFER_CNST",
                            mem_manager,
                            submit_index,
                            va_addr,
                            is_ce_packet,
                            header,
                            packet_node_index);
        }
        else if (type7_header.opcode == Pal::Gfx9::IT_INDIRECT_BUFFER)
        {
            // IT_INDIRECT_BUFFER aliases IT_COND_INDIRECT_BUFFER, but have different packet
            // formats. So need to handle them manually.
            AppendIBFieldNodes("INDIRECT_BUFFER",
                            mem_manager,
                            submit_index,
                            va_addr,
                            is_ce_packet,
                            header,
                            packet_node_index);
        }
        else if (type7_header.opcode == Pal::Gfx9::IT_LOAD_UCONFIG_REG ||
                type7_header.opcode == Pal::Gfx9::IT_LOAD_CONTEXT_REG ||
                type7_header.opcode == Pal::Gfx9::IT_LOAD_SH_REG)
        {
            uint32_t reg_space_start = Pal::Gfx9::UCONFIG_SPACE_START;
            if (type7_header.opcode == Pal::Gfx9::IT_LOAD_CONTEXT_REG)
                reg_space_start = Pal::Gfx9::CONTEXT_SPACE_START;
            if (type7_header.opcode == Pal::Gfx9::IT_LOAD_SH_REG)
                reg_space_start = Pal::Gfx9::PERSISTENT_SPACE_START;
            AppendLoadRegNodes(mem_manager,
                            submit_index,
                            va_addr,
                            reg_space_start,
                            header,
                            packet_node_index);
        }
        else if (type7_header.opcode == Pal::Gfx9::IT_LOAD_CONTEXT_REG_INDEX ||
                type7_header.opcode == Pal::Gfx9::IT_LOAD_SH_REG_INDEX)
        {
            uint32_t reg_space_start = (type7_header.opcode == Pal::Gfx9::IT_LOAD_CONTEXT_REG_INDEX)
        ? Pal::Gfx9::CONTEXT_SPACE_START : Pal::Gfx9::PERSISTENT_SPACE_START;
            AppendLoadRegIndexNodes(mem_manager,
                                    submit_index,
                                    va_addr,
                                    reg_space_start,
                                    header,
                                    packet_node_index);
        }
        else if (type7_header.opcode == Pal::Gfx9::IT_EVENT_WRITE)
        {
            // Event field is special case because there are 2 or 4 DWORD variants of this packet
            // Also, the event_type field is not enumerated in the header, so have to enumerate
            // manually
            const PacketInfo *packet_info_ptr = GetPacketInfo(type7_header.opcode);
            DIVE_ASSERT(packet_info_ptr != nullptr);
            AppendEventWriteFieldNodes(mem_manager,
                                    submit_index,
                                    va_addr,
                                    header,
                                    packet_info_ptr,
                                    packet_node_index);
        }
        else
        */
        if (type7_header.opcode == CP_CONTEXT_REG_BUNCH)
        {
            AppendRegNodes(mem_manager,
                           submit_index,
                           va_addr + sizeof(Pm4Type7Header),  // skip the type7 PM4
                           type7_header.count,
                           packet_node_index);
        }
        else
        {
            // If there are missing packet fields, then output the raw DWORDS directly
            // Some packets, such as CP_LOAD_STATE6_* handle this explicitly elsewhere
            bool append_extra_dwords = (type7_header.opcode != CP_LOAD_STATE6 &&
                                        type7_header.opcode != CP_LOAD_STATE6_GEOM &&
                                        type7_header.opcode != CP_LOAD_STATE6_FRAG);

            const PacketInfo *packet_info_ptr = GetPacketInfo(type7_header.opcode);
            DIVE_ASSERT(packet_info_ptr != nullptr);
            AppendPacketFieldNodes(mem_manager,
                                   submit_index,
                                   va_addr + sizeof(Pm4Type7Header),  // skip the type7 PM4
                                   type7_header.count,
                                   append_extra_dwords,
                                   packet_info_ptr,
                                   packet_node_index);
        }
        return packet_node_index;
    }
    else if (type == Pm4Type::kType4)
    {
        Pm4Type4Header type4_header;
        type4_header.u32All = header;

        std::ostringstream packet_string_stream;
        packet_string_stream << "TYPE4 REGWRITE";
        packet_string_stream << " 0x" << std::hex << type4_header.u32All << std::dec;

        CommandHierarchy::AuxInfo aux_info = CommandHierarchy::AuxInfo::PacketNode(va_addr,
                                                                                   UINT8_MAX,
                                                                                   m_cur_ib_level);

        uint64_t packet_node_index = AddNode(NodeType::kPacketNode,
                                             packet_string_stream.str(),
                                             aux_info);

        AppendRegNodes(mem_manager, submit_index, va_addr, type4_header, packet_node_index);
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
        AddChild(CommandHierarchy::kEngineTopology, reg_node_index, field_node_index);
        AddChild(CommandHierarchy::kSubmitTopology, reg_node_index, field_node_index);
        AddChild(CommandHierarchy::kAllEventTopology, reg_node_index, field_node_index);
        AddChild(CommandHierarchy::kRgpTopology, reg_node_index, field_node_index);
    }
    return reg_node_index;
}

//--------------------------------------------------------------------------------------------------
uint64_t CommandHierarchyCreator::AddSyncEventNode(const IMemoryManager &mem_manager,
                                                   uint32_t              submit_index,
                                                   uint64_t              va_addr,
                                                   SyncType              sync_type)
{
    return UINT64_MAX;
}

//--------------------------------------------------------------------------------------------------
uint32_t CommandHierarchyCreator::GetMarkerSize(const uint8_t *marker_ptr, size_t num_dwords)
{
    return UINT32_MAX;
}

//--------------------------------------------------------------------------------------------------
bool CommandHierarchyCreator::IsBeginDebugMarkerNode(uint64_t node_index)
{
    NodeType node_type = m_command_hierarchy_ptr->GetNodeType(node_index);

    if (node_type == NodeType::kMarkerNode && m_last_user_push_parent_node != UINT64_MAX)
    {
        CommandHierarchy::MarkerType marker_type = m_command_hierarchy_ptr->GetMarkerNodeType(
        node_index);
        if (marker_type == CommandHierarchy::MarkerType::kBeginEnd)
        {
            return true;
        }
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::ParseVulkanCallMarker(char    *marker_ptr,
                                                    uint32_t marker_size,
                                                    uint64_t submit_node_index,
                                                    uint64_t packet_node_index)
{
    return;
}

static const std::string ConvertPrimTypeToStr(pc_di_primtype pt)
{
    std::string s;
    switch (pt)
    {
    case DI_PT_NONE: s = "NONE"; break;
    case DI_PT_POINTLIST_PSIZE: s = "POINTLIST_PSIZE"; break;
    case DI_PT_LINELIST: s = "LINELIST"; break;
    case DI_PT_LINESTRIP: s = "LINESTRIP"; break;
    case DI_PT_TRILIST: s = "TRIANGLELIST"; break;
    case DI_PT_TRIFAN: s = "TRIANGLE_FAN"; break;
    case DI_PT_TRISTRIP: s = "TRIANGLESTRIP"; break;
    case DI_PT_LINELOOP: s = "LINELOOP"; break;
    case DI_PT_RECTLIST: s = "RECTLIST"; break;
    case DI_PT_POINTLIST: s = "POINTLIST"; break;
    case DI_PT_LINE_ADJ: s = "LINE_ADJ"; break;
    case DI_PT_LINESTRIP_ADJ: s = "LINESTRIP_ADJ"; break;
    case DI_PT_TRI_ADJ: s = "TRI_ADJ"; break;
    case DI_PT_TRISTRIP_ADJ: s = "TRISTRIP_ADJ"; break;
    case DI_PT_PATCHES0: s = "PATCHES0"; break;
    case DI_PT_PATCHES1: s = "PATCHES1"; break;
    case DI_PT_PATCHES2: s = "PATCHES2"; break;
    case DI_PT_PATCHES3: s = "PATCHES3"; break;
    case DI_PT_PATCHES4: s = "PATCHES4"; break;
    case DI_PT_PATCHES5: s = "PATCHES5"; break;
    case DI_PT_PATCHES6: s = "PATCHES6"; break;
    case DI_PT_PATCHES7: s = "PATCHES7"; break;
    case DI_PT_PATCHES8: s = "PATCHES8"; break;
    case DI_PT_PATCHES9: s = "PATCHES9"; break;
    case DI_PT_PATCHES10: s = "PATCHES10"; break;
    case DI_PT_PATCHES11: s = "PATCHES11"; break;
    case DI_PT_PATCHES12: s = "PATCHES12"; break;
    case DI_PT_PATCHES13: s = "PATCHES13"; break;
    case DI_PT_PATCHES14: s = "PATCHES14"; break;
    case DI_PT_PATCHES15: s = "PATCHES15"; break;
    case DI_PT_PATCHES16: s = "PATCHES16"; break;
    case DI_PT_PATCHES17: s = "PATCHES17"; break;
    case DI_PT_PATCHES18: s = "PATCHES18"; break;
    case DI_PT_PATCHES19: s = "PATCHES19"; break;
    case DI_PT_PATCHES20: s = "PATCHES20"; break;
    case DI_PT_PATCHES21: s = "PATCHES21"; break;
    case DI_PT_PATCHES22: s = "PATCHES22"; break;
    case DI_PT_PATCHES23: s = "PATCHES23"; break;
    case DI_PT_PATCHES24: s = "PATCHES24"; break;
    case DI_PT_PATCHES25: s = "PATCHES25"; break;
    case DI_PT_PATCHES26: s = "PATCHES26"; break;
    case DI_PT_PATCHES27: s = "PATCHES27"; break;
    case DI_PT_PATCHES28: s = "PATCHES28"; break;
    case DI_PT_PATCHES29: s = "PATCHES29"; break;
    case DI_PT_PATCHES30: s = "PATCHES30"; break;
    case DI_PT_PATCHES31: s = "PATCHES31"; break;
    }
    return s;
}

// TODO(wangra): do not output following properties for now since this will make the text too
// long maybe move this to state tracking tab
// static const std::string ConvertSrcSelToStr(pc_di_src_sel pt)
//{
//     std::string s;
//     switch (pt)
//     {
//     case DI_SRC_SEL_DMA: s = "DMA"; break;
//     case DI_SRC_SEL_IMMEDIATE: s = "Immediate"; break;
//     case DI_SRC_SEL_AUTO_INDEX: s = "Auto Index"; break;
//     case DI_SRC_SEL_AUTO_XFB: s = "Auto XFB"; break;
//     default: DIVE_ASSERT(false); break;
//     }
//     return s;
// }
//
// static const std::string ConvertVisCullModeToStr(pc_di_vis_cull_mode vcm)
//{
//     std::string s;
//     switch (vcm)
//     {
//     case IGNORE_VISIBILITY: s = "Ignore Visibility"; break;
//     case USE_VISIBILITY: s = "Use Visibility"; break;
//     default: DIVE_ASSERT(false); break;
//     }
//     return s;
// }
//
// static const std::string ConvertPatchTypeToStr(a6xx_patch_type pt)
//{
//     std::string s;
//     switch (pt)
//     {
//     case TESS_QUADS: s = "Tessellation Quad"; break;
//     case TESS_TRIANGLES: s = "Tessellation Triangle"; break;
//     case TESS_ISOLINES: s = "Tessellation Isolines"; break;
//     default: DIVE_ASSERT(false); break;
//     }
//     return s;
// }

template<typename VgtDrawInitiatorField>
std::string OutputVgtDrawInitiator(VgtDrawInitiatorField packet)
{
    std::string s = "Primitve Type:" + ConvertPrimTypeToStr(packet.bitfields0.PRIM_TYPE) + ","
    // TODO(wangra): do not output following properties for now since this will make the text too
    // long maybe move this to state tracking tab
    /* + "Source Select:" + ConvertSrcSelToStr(packet.bitfields0.SOURCE_SELECT) + "," +
    "Visibility Cull Mode:" + ConvertVisCullModeToStr(packet.bitfields0.VIS_CULL) +
    "," + "Patch Type:" + ConvertPatchTypeToStr(packet.bitfields0.PATCH_TYPE) +
    "," + "GS Enabled:" + ((packet.bitfields0.GS_ENABLE != 0) ? "True" : "False") +
    "," + "Tessellation Enabled:" +
    ((packet.bitfields0.TESS_ENABLE != 0) ? "True" : "False") + ","*/
    ;
    return s;
}

//--------------------------------------------------------------------------------------------------
std::string CommandHierarchyCreator::GetEventString(const IMemoryManager &mem_manager,
                                                    uint32_t              submit_index,
                                                    uint64_t              va_addr,
                                                    uint32_t              opcode)
{
    std::ostringstream string_stream;
    DIVE_ASSERT(IsDrawDispatchEventOpcode(opcode) ||
                IsBlitEvent(mem_manager, submit_index, va_addr, opcode));

    if (opcode == CP_DRAW_INDX)
    {
        PM4_CP_DRAW_INDX packet;
        DIVE_VERIFY(mem_manager.CopyMemory(&packet, submit_index, va_addr, sizeof(packet)))
        string_stream << "DrawIndexOffset(NumIndices:" << packet.bitfields2.NUM_INDICES << ","
                      << "IndexBase:" << packet.bitfields3.INDX_BASE << ","
                      << "IndexSize:" << packet.bitfields4.INDX_SIZE << ","
                      << "VizQuery:" << packet.bitfields0.VIZ_QUERY << ")";
    }
    else if (opcode == CP_DRAW_INDX)
    {
        PM4_CP_DRAW_INDX packet;
        DIVE_VERIFY(mem_manager.CopyMemory(&packet, submit_index, va_addr, sizeof(packet)))
        string_stream << "DrawIndexOffset(NumIndices:" << packet.bitfields2.NUM_INDICES << ","
                      << "VizQuery:" << packet.bitfields0.VIZ_QUERY << ")";
    }
    else if (opcode == CP_DRAW_INDX_OFFSET)
    {
        PM4_CP_DRAW_INDX_OFFSET packet;
        DIVE_VERIFY(mem_manager.CopyMemory(&packet, submit_index, va_addr, sizeof(packet)))
        string_stream << "DrawIndexOffset(" << OutputVgtDrawInitiator(packet)
                      << "NumInstances:" << packet.bitfields1.NUM_INSTANCES << ","
                      << "NumIndices:" << packet.bitfields2.NUM_INDICES << ","
                      << "FirstIndex:" << packet.bitfields3.FIRST_INDX << ","
                      << "IndexBase:" << packet.bitfields4.INDX_BASE << ","
                      << "IndexSize:" << packet.bitfields5.INDX_SIZE << ","
                      << "MaxIndices:" << packet.bitfields6.MAX_INDICES << ")";
    }
    else if (opcode == CP_DRAW_INDIRECT)
    {
        PM4_CP_DRAW_INDIRECT packet;
        DIVE_VERIFY(mem_manager.CopyMemory(&packet, submit_index, va_addr, sizeof(packet)))
        string_stream << "DrawIndirect(" << OutputVgtDrawInitiator(packet)
                      << "IndirectLo:" << std::hex << "0x" << packet.bitfields1.INDIRECT_LO << ","
                      << "IndirectHi:"
                      << "0x" << packet.bitfields2.INDIRECT_HI << std::dec << ")";
    }
    else if (opcode == CP_DRAW_INDX_INDIRECT)
    {
        PM4_CP_DRAW_INDX_INDIRECT packet;
        DIVE_VERIFY(mem_manager.CopyMemory(&packet, submit_index, va_addr, sizeof(packet)))
        string_stream << "DrawIndexIndirect(" << OutputVgtDrawInitiator(packet)
                      << "IndexBaseLo:" << std::hex << "0x" << packet.bitfields1.INDX_BASE_LO << ","
                      << "IndexBaseHi:"
                      << "0x" << packet.bitfields2.INDX_BASE_HI << std::dec << ","
                      << "MaxIndices:" << packet.bitfields3.MAX_INDICES << ","
                      << "IndirectLo:" << std::hex << "0x" << packet.bitfields4.INDIRECT_LO << ","
                      << "IndirectHi:"
                      << "0x" << packet.bitfields5.INDIRECT_HI << std::dec << ")";
    }
    else if (opcode == CP_DRAW_INDIRECT_MULTI)
    {
        PM4_CP_DRAW_INDIRECT_MULTI_INDIRECT_OP_NORMAL base_packet;
        DIVE_VERIFY(
        mem_manager.CopyMemory(&base_packet, submit_index, va_addr, sizeof(base_packet)));
        if (base_packet.bitfields1.OPCODE == INDIRECT_OP_NORMAL)
        {
            string_stream << "DrawIndirectMulti(" << OutputVgtDrawInitiator(base_packet)
                          << "DrawCount:" << base_packet.DRAW_COUNT << ","
                          << "Indirect:" << std::hex << "0x" << base_packet.INDIRECT << std::dec
                          << ","
                          << "Stride:" << base_packet.STRIDE << ","
                          << "DstOff:" << base_packet.bitfields1.DST_OFF << ")";
        }
        else if (base_packet.bitfields1.OPCODE == INDIRECT_OP_INDEXED)
        {
            PM4_CP_DRAW_INDIRECT_MULTI_INDEXED packet;
            DIVE_VERIFY(mem_manager.CopyMemory(&packet, submit_index, va_addr, sizeof(packet)));
            string_stream << "DrawIndirectMultiIndexed(" << OutputVgtDrawInitiator(base_packet)
                          << "DrawCount:" << packet.DRAW_COUNT << ","
                          << "Index:" << std::hex << "0x" << packet.INDEX << std::dec << ","
                          << "MaxIndices:" << packet.MAX_INDICES << ","
                          << "Indirect:" << std::hex << "0x" << packet.INDIRECT << std::dec << ","
                          << "Stride:" << packet.STRIDE << ","
                          << "DstOff:" << packet.bitfields1.DST_OFF << ")";
        }
        else if (base_packet.bitfields1.OPCODE == INDIRECT_OP_INDIRECT_COUNT)
        {
            PM4_CP_DRAW_INDIRECT_MULTI_INDIRECT packet;
            DIVE_VERIFY(mem_manager.CopyMemory(&packet, submit_index, va_addr, sizeof(packet)));
            string_stream << "DrawIndirectMultiIndirect(" << OutputVgtDrawInitiator(base_packet)
                          << "DrawCount:" << packet.DRAW_COUNT << ","
                          << "Indirect:" << std::hex << "0x" << packet.INDIRECT << std::dec << ","
                          << "IndirectCount:" << packet.INDIRECT_COUNT << ","
                          << "Stride:" << packet.STRIDE << ","
                          << "DstOff:" << packet.bitfields1.DST_OFF << ")";
        }
        else if (base_packet.bitfields1.OPCODE == INDIRECT_OP_INDIRECT_COUNT_INDEXED)
        {
            PM4_CP_DRAW_INDIRECT_MULTI_INDIRECT_INDEXED packet;
            DIVE_VERIFY(mem_manager.CopyMemory(&packet, submit_index, va_addr, sizeof(packet)));
            string_stream << "DrawIndirectMultiIndirectIndexed("
                          << OutputVgtDrawInitiator(base_packet)
                          << "DrawCount:" << packet.DRAW_COUNT << ","
                          << "Index:" << std::hex << "0x" << packet.INDEX << std::dec << ","
                          << "MaxIndices:" << packet.MAX_INDICES << ","
                          << "Indirect:" << std::hex << "0x" << packet.INDIRECT << std::dec << ","
                          << "IndirectCount:" << packet.INDIRECT_COUNT << ","
                          << "Stride:" << packet.STRIDE << ","
                          << "DstOff:" << packet.bitfields1.DST_OFF << ")";
        }
    }
    else if (opcode == CP_DRAW_AUTO)
    {
        // Where is this defined?!?
        string_stream << "DrawAuto";
    }
    else if (opcode == CP_EXEC_CS_INDIRECT)
    {
        PM4_CP_EXEC_CS_INDIRECT packet;
        DIVE_VERIFY(mem_manager.CopyMemory(&packet, submit_index, va_addr, sizeof(packet)));
        string_stream << "ExecCsIndirect(x:" << packet.bitfields3.LOCALSIZEX << ","
                      << "y:" << packet.bitfields3.LOCALSIZEY << ","
                      << "z:" << packet.bitfields3.LOCALSIZEZ << ","
                      << "AddrLo:" << std::hex << "0x" << packet.bitfields1.ADDR_LO << ","
                      << "AddrHi:"
                      << "0x" << packet.bitfields2.ADDR_HI << std::dec << ")";
    }
    else if (opcode == CP_EXEC_CS)
    {
        PM4_CP_EXEC_CS packet;
        DIVE_VERIFY(mem_manager.CopyMemory(&packet, submit_index, va_addr, sizeof(packet)));
        string_stream << "ExecCsIndirect(x:" << packet.bitfields1.NGROUPS_X << ","
                      << "y:" << packet.bitfields2.NGROUPS_Y << ","
                      << "z:" << packet.bitfields3.NGROUPS_Z << ")";
    }
    else if (opcode == CP_BLIT)
    {
        PM4_CP_BLIT packet;
        DIVE_VERIFY(mem_manager.CopyMemory(&packet, submit_index, va_addr, sizeof(packet)));
        std::string op;
        switch (packet.bitfields0.OP)
        {
        case BLIT_OP_FILL: op = "BLIT_OP_FILL"; break;
        case BLIT_OP_COPY: op = "BLIT_OP_COPY"; break;
        case BLIT_OP_SCALE: op = "BLIT_OP_SCALE"; break;
        }
        string_stream << "CpBlit(op:" << op << ","
                      << "srcX1:" << packet.bitfields1.SRC_X1 << ","
                      << "srcY1:" << packet.bitfields1.SRC_Y1 << ","
                      << "srcX2:" << packet.bitfields2.SRC_X2 << ","
                      << "srcY2:" << packet.bitfields2.SRC_Y2 << ","
                      << "dstX1:" << packet.bitfields3.DST_X1 << ","
                      << "dstX1:" << packet.bitfields3.DST_Y1 << ","
                      << "dstX2:" << packet.bitfields4.DST_X2 << ","
                      << "dstX2:" << packet.bitfields4.DST_Y2 << ")";
    }
    else if (opcode == CP_EVENT_WRITE7)
    {
        // Assumed it is a BLIT event (note: renamed to CCU_RESOLVE for a7xx)
        string_stream << "CpEventWrite(type:CCU_RESOLVE)";
    }
    return string_stream.str();
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
    DIVE_ASSERT(dword_count % 2 == 0);
    for (uint32_t i = 0; i < dword_count; i += 2)
    {
        struct
        {
            uint32_t m_reg_offset;
            uint32_t m_reg_value;
        } reg_pair;
        uint64_t pair_addr = va_addr + i * sizeof(uint32_t);
        DIVE_VERIFY(mem_manager.CopyMemory(&reg_pair, submit_index, pair_addr, sizeof(reg_pair)));

        const RegInfo *reg_info_ptr = GetRegInfo(reg_pair.m_reg_offset);

        RegInfo temp = {};
        temp.m_name = "Unknown";
        temp.m_enum_handle = UINT8_MAX;
        if (reg_info_ptr == nullptr)
            reg_info_ptr = &temp;

        DIVE_ASSERT(!reg_info_ptr->m_is_64_bit);

        // Create the register node, as well as all its children nodes that describe the various
        // fields set in the single 32-bit register
        uint64_t reg_node_index = AddRegisterNode(reg_pair.m_reg_offset,
                                                  reg_pair.m_reg_value,
                                                  reg_info_ptr);

        // Add it as child to packet node
        AddChild(CommandHierarchy::kEngineTopology, packet_node_index, reg_node_index);
        AddChild(CommandHierarchy::kSubmitTopology, packet_node_index, reg_node_index);
        AddChild(CommandHierarchy::kAllEventTopology, packet_node_index, reg_node_index);
        AddChild(CommandHierarchy::kRgpTopology, packet_node_index, reg_node_index);
    }
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::AppendRegNodes(const IMemoryManager &mem_manager,
                                             uint32_t              submit_index,
                                             uint64_t              va_addr,
                                             Pm4Type4Header        header,
                                             uint64_t              packet_node_index)
{
    // This version of AppendRegNodes takes in an offset from the header, and expects a contiguous
    // sequence of register values

    // Go through each register set by this packet
    uint32_t offset_in_bytes = 0;
    uint32_t dword = 0;
    while (dword < header.count)
    {
        uint64_t       reg_va_addr = va_addr + sizeof(header) + offset_in_bytes;
        uint32_t       reg_offset = header.offset + dword;
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
        DIVE_VERIFY(mem_manager.CopyMemory(&reg_value, submit_index, reg_va_addr, size_to_read));
        // Create the register node, as well as all its children nodes that describe the various
        // fields set in the single 32-bit register
        uint64_t reg_node_index = AddRegisterNode(reg_offset, reg_value, reg_info_ptr);

        // Add it as child to packet node
        AddChild(CommandHierarchy::kEngineTopology, packet_node_index, reg_node_index);
        AddChild(CommandHierarchy::kSubmitTopology, packet_node_index, reg_node_index);
        AddChild(CommandHierarchy::kAllEventTopology, packet_node_index, reg_node_index);
        AddChild(CommandHierarchy::kRgpTopology, packet_node_index, reg_node_index);

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
            AddChild(CommandHierarchy::kEngineTopology, packet_node_index, array_node_index);
            AddChild(CommandHierarchy::kSubmitTopology, packet_node_index, array_node_index);
            AddChild(CommandHierarchy::kAllEventTopology, packet_node_index, array_node_index);
            AddChild(CommandHierarchy::kRgpTopology, packet_node_index, array_node_index);
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
            DIVE_VERIFY(
            mem_manager.CopyMemory(&dword_value, submit_index, dword_va_addr, sizeof(uint32_t)));

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
            AddChild(CommandHierarchy::kEngineTopology, parent_node_index, field_node_index);
            AddChild(CommandHierarchy::kSubmitTopology, parent_node_index, field_node_index);
            AddChild(CommandHierarchy::kAllEventTopology, parent_node_index, field_node_index);
            AddChild(CommandHierarchy::kRgpTopology, parent_node_index, field_node_index);
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
                DIVE_VERIFY(mem_manager.CopyMemory(&dword_value,
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
                AddChild(CommandHierarchy::kEngineTopology, packet_node_index, field_node_index);
                AddChild(CommandHierarchy::kSubmitTopology, packet_node_index, field_node_index);
                AddChild(CommandHierarchy::kAllEventTopology, packet_node_index, field_node_index);
                AddChild(CommandHierarchy::kRgpTopology, packet_node_index, field_node_index);
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
    DIVE_VERIFY(mem_manager.CopyMemory(&packet, submit_index, va_addr, sizeof(packet)));

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
    case SB6_CS_TEX: cat = StateBlockCat::kTex; break;
    case SB6_VS_SHADER:
    case SB6_HS_SHADER:
    case SB6_DS_SHADER:
    case SB6_GS_SHADER:
    case SB6_FS_SHADER:
    case SB6_CS_SHADER: cat = StateBlockCat::kShader; break;
    case SB6_IBO:
    case SB6_CS_IBO: cat = StateBlockCat::kIbo; break;
    default: DIVE_ASSERT(false); cat = StateBlockCat::kTex;
    }

    uint64_t ext_src_addr = 0;
    bool     bindless = false;
    switch (packet.bitfields0.STATE_SRC)
    {
    case SS6_DIRECT: ext_src_addr = va_addr + sizeof(PM4_CP_LOAD_STATE6); break;
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
    case SS6_UBO: DIVE_ASSERT(false);  // Not used. Not sure what it's for
    }

    auto AppendSharps = [&](const char *sharp_struct_name, uint32_t sharp_struct_size) {
        for (uint32_t i = 0; i < packet.bitfields0.NUM_UNIT; ++i)
        {
            uint64_t          addr = ext_src_addr + i * sharp_struct_size;
            const PacketInfo *packet_info_ptr = GetPacketInfo(0xffffffff, sharp_struct_name);
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
            // NUM_UNIT is in unit of float4s
            // Add 4 dwords per line
            AddConstantsToPacketNode<float>(mem_manager,
                                            ext_src_addr,
                                            packet_node_index,
                                            packet.bitfields0.NUM_UNIT * 4,
                                            submit_index,
                                            4);
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
    default: DIVE_ASSERT(false); break;
    }
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::AppendMemRegNodes(const IMemoryManager &mem_manager,
                                                uint32_t              submit_index,
                                                uint64_t              va_addr,
                                                uint64_t              packet_node_index)
{
    PM4_CP_MEM_TO_REG packet;
    DIVE_VERIFY(mem_manager.CopyMemory(&packet, submit_index, va_addr, sizeof(packet)));

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
    AddChild(CommandHierarchy::kEngineTopology, packet_node_index, reg_node_index);
    AddChild(CommandHierarchy::kSubmitTopology, packet_node_index, reg_node_index);
    AddChild(CommandHierarchy::kAllEventTopology, packet_node_index, reg_node_index);
    AddChild(CommandHierarchy::kRgpTopology, packet_node_index, reg_node_index);

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
                                                         uint64_t set_draw_state_node_index,
                                                         uint32_t header)
{
    Pm4Type7Header *type7_header = (Pm4Type7Header *)&header;

    // Find all the children of the set_draw_state packet, which should contain array indices
    // Using any of the topologies where field nodes are added will work
    uint64_t               index = set_draw_state_node_index;
    std::vector<uint64_t> &children = m_node_children[CommandHierarchy::kSubmitTopology][0][index];

    // Obtain the address of each of the children group IBs
    PM4_CP_SET_DRAW_STATE packet;
    DIVE_VERIFY(mem_manager.CopyMemory(&packet,
                                       submit_index,
                                       va_addr,
                                       (type7_header->count + 1) * sizeof(uint32_t)));

    // Sanity check: The # of children should match the array size
    uint32_t total_size_bytes = (type7_header->count * sizeof(uint32_t));
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
                                          const std::string        &desc,
                                          CommandHierarchy::AuxInfo aux_info,
                                          char                     *metadata_ptr,
                                          uint32_t                  metadata_size)
{
    uint64_t node_index = m_command_hierarchy_ptr->AddNode(type,
                                                           desc,
                                                           aux_info,
                                                           metadata_ptr,
                                                           metadata_size);
    for (uint32_t i = 0; i < CommandHierarchy::kTopologyTypeCount; ++i)
    {
        DIVE_ASSERT(m_node_children[i][0].size() == node_index);
        DIVE_ASSERT(m_node_children[i][1].size() == node_index);
        m_node_children[i][0].resize(m_node_children[i][0].size() + 1);
        m_node_children[i][1].resize(m_node_children[i][1].size() + 1);
    }
    return node_index;
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::AppendEventNodeIndex(uint64_t node_index)
{
    m_command_hierarchy_ptr->m_nodes.m_event_node_indices.push_back(node_index);
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::AddChild(CommandHierarchy::TopologyType type,
                                       uint64_t                       node_index,
                                       uint64_t                       child_node_index)
{
    // Store children info into the temporary m_node_children
    // Use this to create the appropriate topology later
    DIVE_ASSERT(node_index < m_node_children[type][0].size());
    m_node_children[type][0][node_index].push_back(child_node_index);
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::AddSharedChild(CommandHierarchy::TopologyType type,
                                             uint64_t                       node_index,
                                             uint64_t                       child_node_index)
{
    // Store children info into the temporary m_node_children
    // Use this to create the appropriate topology later
    DIVE_ASSERT(node_index < m_node_children[type][1].size());
    m_node_children[type][1][node_index].push_back(child_node_index);
}

//--------------------------------------------------------------------------------------------------
// Remove all children listed in |children_node_indices|.
void CommandHierarchyCreator::RemoveListOfChildren(
CommandHierarchy::TopologyType type,
uint64_t                       node_index,
const std::vector<uint64_t>   &children_node_indices)
{
    if (children_node_indices.empty())
    {
        return;
    }

    auto  &vec = m_node_children[type][0][node_index];
    size_t j = 0;
    size_t k = 0;
    for (size_t i = 0; i < vec.size(); i++)
    {
        if (j < children_node_indices.size() && vec[i] == children_node_indices[j])
        {
            j++;
        }
        else
        {
            vec[k++] = vec[i];
        }
    }

    DIVE_ASSERT(j == children_node_indices.size());

    vec.erase(vec.begin() + vec.size() - children_node_indices.size(), vec.end());
}

//--------------------------------------------------------------------------------------------------
uint64_t CommandHierarchyCreator::GetChildNodeIndex(CommandHierarchy::TopologyType type,
                                                    uint64_t                       node_index,
                                                    uint64_t child_index) const
{
    DIVE_ASSERT(node_index < m_node_children[type][0].size());
    DIVE_ASSERT(child_index < m_node_children[type][0][node_index].size());
    return m_node_children[type][0][node_index][child_index];
}

//--------------------------------------------------------------------------------------------------
uint64_t CommandHierarchyCreator::GetChildCount(CommandHierarchy::TopologyType type,
                                                uint64_t                       node_index) const
{
    DIVE_ASSERT(node_index < m_node_children[type][0].size());
    return m_node_children[type][0][node_index].size();
}

//--------------------------------------------------------------------------------------------------
void CommandHierarchyCreator::CreateTopologies()
{
    // A kVulkanCallTopology is a kAllEventTopology without the following:
    //  kDrawDispatchDmaNode, kSyncNode, kPostambleStateNode, kMarkerNode-kBarrier
    auto FilterOut = [&](size_t node_index) {
        NodeType type = m_command_hierarchy_ptr->GetNodeType(node_index);
        // Filter out all these node types
        if (type == NodeType::kDrawDispatchBlitNode || type == NodeType::kSyncNode ||
            type == NodeType::kPostambleStateNode)
            return true;
        // Also filter out kMarkerNode-kBarrier nodes
        if (type == NodeType::kMarkerNode)
        {
            auto marker_type = m_command_hierarchy_ptr->GetMarkerNodeType(node_index);
            if (marker_type == CommandHierarchy::MarkerType::kBarrier)
                return true;
        }
        return false;
    };
    CommandHierarchy::TopologyType src_topology = CommandHierarchy::kAllEventTopology;
    CommandHierarchy::TopologyType dst_topology = CommandHierarchy::kVulkanCallTopology;
    size_t                         num_nodes = m_node_children[src_topology][0].size();
    DIVE_ASSERT(num_nodes == m_node_children[src_topology][1].size());
    for (size_t node_index = 0; node_index < num_nodes; ++node_index)
    {
        // Ensure topology was not previously filled-in
        DIVE_ASSERT(m_node_children[dst_topology][0][node_index].empty());
        DIVE_ASSERT(m_node_children[dst_topology][1][node_index].empty());

        // Ignore all these node types
        if (FilterOut(node_index))
            continue;

        // Go through primary children of a particular node, and only add non-ignored nodes
        const std::vector<uint64_t> &children = m_node_children[src_topology][0][node_index];
        for (size_t child = 0; child < children.size(); ++child)
        {
            if (!FilterOut(children[child]))
                AddChild(dst_topology, node_index, children[child]);
        }

        // Shared children should remain the same
        const std::vector<uint64_t> &shared = m_node_children[src_topology][1][node_index];
        m_node_children[CommandHierarchy::kVulkanCallTopology][1][node_index] = shared;
    }

    // A kVulkanEventTopology is a kVulkanCallTopology without non-Event Vulkan kMarkerNodes.
    // The shared-children of the non-Event Vulkan kMarkerNodes will be inherited by the "next"
    // Vulkan kMarkerNode encountered
    src_topology = CommandHierarchy::kVulkanCallTopology;
    dst_topology = CommandHierarchy::kVulkanEventTopology;
    num_nodes = m_node_children[src_topology][0].size();
    DIVE_ASSERT(num_nodes == m_node_children[src_topology][1].size());

    for (size_t node_index = 0; node_index < num_nodes; ++node_index)
    {
        // Skip over all Vulkan non-Event nodes
        if (IsVulkanNonEventNode(node_index))
            continue;

        // Go through primary children of a particular node, and only add non-ignored nodes
        const std::vector<uint64_t> &children = m_node_children[src_topology][0][node_index];
        std::vector<uint64_t>        acc_shared;
        for (size_t child = 0; child < children.size(); ++child)
        {
            // Accumulate shared packets from the child node
            uint64_t                     child_index = children[child];
            const std::vector<uint64_t> &shared = m_node_children[src_topology][1][child_index];
            acc_shared.insert(acc_shared.end(), shared.begin(), shared.end());
            if (!IsVulkanNonEventNode(child_index))
            {
                // If it isn't a Vulkan Event node or a Vulkan Non-Event node (ie. a non-Vulkan
                // node, such as a normal marker node, a submit node, etc), then throw away the
                // previous accumulation. For example, the beginning of a submit sometimes has a
                // vkCmdBegin followed by a debug-marker. The PM4 contents of the vkCmdBegin is
                // thrown away, since it isn't part of the debug-marker.
                if (!IsVulkanEventNode(child_index))
                    acc_shared.clear();

                AddChild(dst_topology, node_index, child_index);

                if (acc_shared.empty())
                    m_node_children[dst_topology][1][child_index] = shared;
                else
                    m_node_children[dst_topology][1][child_index] = acc_shared;
                acc_shared.clear();
            }
        }
    }

    // Convert the m_node_children temporary structure into CommandHierarchy's topologies
    for (uint32_t topology = 0; topology < CommandHierarchy::kTopologyTypeCount; ++topology)
    {
        num_nodes = m_node_children[topology][0].size();
        Topology &cur_topology = m_command_hierarchy_ptr->m_topology[topology];
        cur_topology.SetNumNodes(num_nodes);
        for (uint64_t node_index = 0; node_index < num_nodes; ++node_index)
        {
            DIVE_ASSERT(m_node_children[topology][0].size() == m_node_children[topology][1].size());
            cur_topology.AddChildren(node_index, m_node_children[topology][0][node_index]);
            cur_topology.AddSharedChildren(node_index, m_node_children[topology][1][node_index]);
        }
    }
}

//--------------------------------------------------------------------------------------------------
bool CommandHierarchyCreator::EventNodeHelper(uint64_t                      node_index,
                                              std::function<bool(uint32_t)> callback) const
{
    NodeType node_type = m_command_hierarchy_ptr->GetNodeType(node_index);
    if (node_type == NodeType::kMarkerNode)
    {
        CommandHierarchy::MarkerType type = m_command_hierarchy_ptr->GetMarkerNodeType(node_index);
        if (type == CommandHierarchy::MarkerType::kDiveMetadata)
            return callback(m_command_hierarchy_ptr->GetMarkerNodeId(node_index));
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
bool CommandHierarchyCreator::IsVulkanEventNode(uint64_t node_index) const
{
    auto fp = std::bind(&CommandHierarchyCreator::IsVulkanEvent, this, std::placeholders::_1);
    return EventNodeHelper(node_index, fp);
}

//--------------------------------------------------------------------------------------------------
bool CommandHierarchyCreator::IsVulkanNonEventNode(uint64_t node_index) const
{
    auto fp = std::bind(&CommandHierarchyCreator::IsNonVulkanEvent, this, std::placeholders::_1);
    return EventNodeHelper(node_index, fp);
}

//--------------------------------------------------------------------------------------------------
bool CommandHierarchyCreator::IsVulkanEvent(uint32_t cmd_id) const
{
    return false;
}

//--------------------------------------------------------------------------------------------------
template<typename T> struct OutputStream
{
    static void SetupFormat(std::ostringstream &stream) {}
};

template<> struct OutputStream<float>
{
    static void SetupFormat(std::ostringstream &stream)
    {
        stream << std::fixed << std::setw(11) << std::setprecision(6);
    }
};

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
                DIVE_VERIFY(mem_manager.CopyMemory(&value, submit_index, addr, sizeof(T)));
                OutputStream<T>::SetupFormat(string_stream);
                string_stream << value << " ";
            }
        }

        // Add it as child to packet_node
        CommandHierarchy::AuxInfo aux_info = CommandHierarchy::AuxInfo::RegFieldNode(false);
        uint64_t const_node_index = AddNode(NodeType::kFieldNode, string_stream.str(), aux_info);
        AddChild(CommandHierarchy::kEngineTopology, packet_node_index, const_node_index);
        AddChild(CommandHierarchy::kSubmitTopology, packet_node_index, const_node_index);
        AddChild(CommandHierarchy::kAllEventTopology, packet_node_index, const_node_index);
        AddChild(CommandHierarchy::kRgpTopology, packet_node_index, const_node_index);
    }
}
}  // namespace Dive
