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

// =====================================================================================================================
// The CommandHierarchy class parses and creates a tree of Nodes in the command buffer. The primary
// client for this class is the Model class for the Node and Command Views in the UI.
// =====================================================================================================================

#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "capture_data.h"
#include "capture_event_info.h"
#include "dive_core/common/dive_capture_format.h"
#include "dive_core/common/emulate_pm4.h"
#include "dive_core/common/pm4_packets/pfp_pm4_packets.h"

// Forward declarations
struct PacketInfo;
struct RegInfo;

namespace Dive
{

// Forward declarations
class CaptureData;
class GFRData;
class MemoryManager;
class SubmitInfo;
class ILog;

//--------------------------------------------------------------------------------------------------
enum class NodeType
{
    kRootNode,
    kEngineNode,
    kSubmitNode,
    kIbNode,
    kMarkerNode,
    kDrawDispatchBlitNode,
    kSyncNode,
    kPostambleStateNode,
    kPacketNode,
    kRegNode,
    kFieldNode,
    kPresentNode,
    kRenderMarkerNode
};

//--------------------------------------------------------------------------------------------------
// This is per-node graph topology info.
// This is essentially a tree, but each node can also have a set of "shared" children - these are
// children that have more than 1 parent, and are used for kPacketNodes (ie. these nodes can
// simultaneously be children of kSubmitNodes, kMarkerNodes, kDrawDispatchDmaNode nodes, etc.)
class Topology
{
public:
    const static uint64_t kRootNodeIndex = 0;

    uint64_t GetNumNodes() const;

    // Node-index of parent node
    uint64_t GetParentNodeIndex(uint64_t node_index) const;

    // Index of child w.r.t. to its parent
    uint64_t GetChildIndex(uint64_t node_index) const;

    // Children info
    uint64_t GetNumChildren(uint64_t node_index) const;
    uint64_t GetChildNodeIndex(uint64_t node_index, uint64_t child_index) const;
    uint64_t GetNumSharedChildren(uint64_t node_index) const;
    uint64_t GetSharedChildNodeIndex(uint64_t node_index, uint64_t child_index) const;
    uint64_t GetNextNodeIndex(uint64_t node_index) const;

private:
    friend class CommandHierarchy;
    friend class CommandHierarchyCreator;

    struct ChildrenInfo
    {
        uint64_t m_start_index = UINT64_MAX;
        uint64_t m_num_children = 0;
    };

    // This is a per-node pointer to m_children_list
    // 2 sets of children per node
    std::vector<ChildrenInfo> m_node_children;
    std::vector<ChildrenInfo> m_node_shared_children;

    // Index of parent
    std::vector<uint64_t> m_node_parent;

    // Index of child w.r.t. to its parent
    std::vector<uint64_t> m_node_child_index;

    std::vector<uint64_t> m_children_list;
    std::vector<uint64_t> m_shared_children_list;

    void SetNumNodes(uint64_t num_nodes);
    void AddChildren(uint64_t node_index, const std::vector<uint64_t> &children);
    void AddSharedChildren(uint64_t node_index, const std::vector<uint64_t> &children);
};

//--------------------------------------------------------------------------------------------------
union SyncInfo
{
    struct
    {
        uint32_t m_flush_inv_cb_meta : 1;     // fmask/cmask, but not dcc
        uint32_t m_flush_inv_pixel_data : 1;  // dcc and cb data
        uint32_t m_wb_inv_cbdb : 1;
        uint32_t m_vs_partial_flush : 1;
        uint32_t m_ps_partial_flush : 1;
        uint32_t m_cs_partial_flush : 1;
        uint32_t m_wb_inv_db : 1;
        uint32_t m_vgt_flush : 1;
    } eventwrite;

    struct
    {
        uint32_t m_bottom_of_pipe : 1;
        uint32_t m_cs_done : 1;
        uint32_t m_ps_done : 1;
        uint32_t m_wb_inv_cbdb : 1;
        uint32_t m_write_data_sel : 3;  // ME_RELEASE_MEM_data_sel_enum
    } releasemem;

    struct
    {
        uint32_t m_wb_inv_db : 1;
        uint32_t m_inv_sq_k : 1;
        uint32_t m_inv_sq_i : 1;
        uint32_t m_flush_sq_k : 1;
        uint32_t m_coher_base_set : 1;
        uint32_t m_engine_sel : 1;
    } acquiremem;

    struct
    {
        // For the release/acquire-mem-specific bits
        uint32_t m_releasemem_acquiremem_common : 7;

        // common to releasemem and acquiremem
        uint32_t m_wb_inv_l1l2 : 1;
        uint32_t m_wb_inv_l2 : 1;
        uint32_t m_wb_l2 : 1;
        uint32_t m_inv_l2 : 1;
        uint32_t m_inv_l2_md : 1;
        uint32_t m_inv_l1 : 1;
        uint32_t m_wb_inv_cbdata : 1;
    } common;
    uint32_t m_u32All;
};
static_assert(sizeof(SyncInfo) == sizeof(uint32_t), "Unexpected size!");

//--------------------------------------------------------------------------------------------------
class CommandHierarchy
{
public:
    enum class MarkerType
    {
        kBeginEnd,      // vkCmdDebugMarkerBegin / vkCmdDebugMarkerEnd
        kInsert,        // vkCmdDebugMarkerInsert
        kRgpInternal,   // RGP internal metadata
        kDiveMetadata,  // Dive NOP-based metadata
        kBarrier,       // Barrier node
        kCount
    };

    CommandHierarchy();
    ~CommandHierarchy();

    inline size_t size() const { return m_nodes.m_node_type.size(); }

    // The topologies are layed out such that children1 contain non-packet nodes
    // and children2 contain packet nodes. The difference lies in what is in children1 arrays:
    //  The Engine hierarchy contains kRootNode -> kEngineNodes -> kSubmitNodes -> kIbNodes
    //  The Submit hierarchy contains kRootNode -> kSubmitNodes -> kIbNodes
    //  The Event hierarchy contains kRootNode -> kSubmitNodes/kPresentNodes -> (EventNodes)
    //      Where (EventNodes) == kMarkerNode/kDrawDispatchDmaNode/kSyncNode/kPostambleStateNode
    //  Note that all except kRootNode & kPresentNodes can have kPacketNodes as children2
    const Topology &GetEngineHierarchyTopology() const;
    const Topology &GetSubmitHierarchyTopology() const;
    const Topology &GetVulkanDrawEventHierarchyTopology() const;
    const Topology &GetVulkanEventHierarchyTopology() const;
    const Topology &GetAllEventHierarchyTopology() const;
    const Topology &GetRgpHierarchyTopology() const;

    NodeType    GetNodeType(uint64_t node_index) const;
    const char *GetNodeDesc(uint64_t node_index) const;

    const std::vector<uint8_t> &GetMetadata(uint64_t node_index) const;

    Dive::EngineType GetSubmitNodeEngineType(uint64_t node_index) const;
    uint32_t         GetSubmitNodeIndex(uint64_t node_index) const;
    uint8_t          GetIbNodeIndex(uint64_t node_index) const;
    Dive::IbType     GetIbNodeType(uint64_t node_index) const;
    uint32_t         GetIbNodeSizeInDwords(uint64_t node_index) const;
    bool             GetIbNodeIsFullyCaptured(uint64_t node_index) const;
    MarkerType       GetMarkerNodeType(uint64_t node_index) const;
    uint32_t         GetMarkerNodeId(uint64_t node_index) const;
    uint32_t         GetEventNodeId(uint64_t node_index) const;
    uint64_t         GetPacketNodeAddr(uint64_t node_index) const;
    uint8_t          GetPacketNodeOpcode(uint64_t node_index) const;
    uint8_t          GetPacketNodeIbLevel(uint64_t node_index) const;
    bool             GetRegFieldNodeIsCe(uint64_t node_index) const;
    SyncType         GetSyncNodeSyncType(uint64_t node_index) const;
    SyncInfo         GetSyncNodeSyncInfo(uint64_t node_index) const;
    bool             HasVulkanMarkers() const { return m_has_vulkan_marker; }

    // GetEventIndex returns sequence number for Event/Sync Nodes, 0 if not exist.
    size_t GetEventIndex(uint64_t node_index) const;

private:
    friend class CommandHierarchyCreator;

    enum TopologyType
    {
        kEngineTopology,
        kSubmitTopology,
        kVulkanEventTopology,
        kVulkanCallTopology,
        kAllEventTopology,
        kRgpTopology,
        kTopologyTypeCount
    };

    static const uint8_t kMaxNumIbsBits = 6;
    static_assert((1 << kMaxNumIbsBits) >= EmulatePM4::kMaxNumIbsPerSubmit, "Not enough bits!");

    union AuxInfo
    {
        struct
        {
            Dive::EngineType m_engine_type;
            uint32_t         m_submit_index;
        } submit_node;

        struct
        {
            uint32_t m_ib_type : 8;  // IbType
            uint32_t m_ib_index : kMaxNumIbsBits;
            uint32_t m_fully_captured : 1;
            uint32_t : 17;
            uint32_t m_size_in_dwords;
        } ib_node;

        struct
        {
            MarkerType m_type;
            uint32_t   m_id;  // VkCmdID, for kDiveMetadata, or VkBarrierMarkerId for kBarrier

        } marker_node;

        struct
        {
            uint32_t m_event_id;
        } event_node;

        struct
        {
            uint64_t m_addr : 48;
            uint64_t m_opcode : 8;
            uint64_t m_ib_level : 3;
            uint64_t m_reserved : 5;
        } packet_node;

        struct
        {
            bool m_is_ce_packet;
        } reg_field_node;

        struct
        {
            uint32_t m_sync_type : 4;  // SyncType
            SyncInfo m_sync_info;
        } sync_node;

        uint64_t m_u64All;

        AuxInfo(uint64_t val);
        static AuxInfo SubmitNode(Dive::EngineType engine_type, uint32_t submit_index);
        static AuxInfo IbNode(uint32_t ib_index,
                              IbType   ib_type,
                              uint32_t size_in_dwords,
                              bool     fully_captured);
        static AuxInfo PacketNode(uint64_t addr, uint8_t opcode, uint8_t ib_level);
        static AuxInfo RegFieldNode(bool is_ce_packet);
        static AuxInfo EventNode(uint32_t event_id);
        static AuxInfo MarkerNode(MarkerType type, uint32_t id = 0);
        static AuxInfo SyncNode(SyncType type, SyncInfo sync_info);
    };
    static_assert(sizeof(AuxInfo) == sizeof(uint64_t), "Unexpected size!");

    // This is information about each node and contains no topology information
    // Arranged in structure-of-arrays for better locality
    struct Nodes
    {
        std::vector<NodeType>    m_node_type;
        std::vector<std::string> m_description;
        std::vector<AuxInfo>     m_aux_info;
        std::vector<uint64_t>    m_event_node_indices;

        // Used mostly to cache Vulkan argument metadata for Vulkan marker nodes
        std::vector<std::vector<uint8_t>> m_metadata;

        uint64_t AddNode(NodeType           type,
                         const std::string &desc,
                         AuxInfo            aux_info,
                         char              *metadata_ptr,
                         uint32_t           metadata_size);
    };

    // Add a node and returns index of the added node
    uint64_t AddNode(NodeType           type,
                     const std::string &desc,
                     AuxInfo            aux_info,
                     char              *metadata_ptr,
                     uint32_t           metadata_size);

    Nodes    m_nodes;
    Topology m_topology[kTopologyTypeCount];
    bool     m_has_vulkan_marker = false;
};

//--------------------------------------------------------------------------------------------------
class CommandHierarchyCreator : public IEmulateCallbacks
{
public:
    CommandHierarchyCreator(EmulateStateTracker &state_tracker);
    // If flatten_chain_nodes set to true, then chain nodes are children of the top-most
    // root ib or call ib node, and never a child of another chain node. This prevents a
    // deep tree of chain nodes when a capture chains together tons of IBs.
    bool CreateTrees(CommandHierarchy  *command_hierarchy_ptr,
                     const CaptureData &capture_data,
                     bool               flatten_chain_nodes,
                     ILog              *log_ptr);

    // This is used to create a command-hierarchy out of a PM4 universal stream (ie: single IB)
    bool CreateTrees(CommandHierarchy *command_hierarchy_ptr,
                     EngineType        engine_type,
                     QueueType         queue_type,
                     uint32_t         *command_dwords,
                     uint32_t          size_in_dwords,
                     ILog             *log_ptr);

    virtual bool OnIbStart(uint32_t                  submit_index,
                           uint32_t                  ib_index,
                           const IndirectBufferInfo &ib_info,
                           IbType                    type) override;

    virtual bool OnIbEnd(uint32_t                  submit_index,
                         uint32_t                  ib_index,
                         const IndirectBufferInfo &ib_info) override;

    virtual bool OnPacket(const IMemoryManager &mem_manager,
                          uint32_t              submit_index,
                          uint32_t              ib_index,
                          uint64_t              va_addr,
                          Pm4Type               type,
                          uint32_t              header) override;

private:
    union Type3Ordinal2
    {
        struct
        {
            uint32_t reg_offset : 16;
            uint32_t reserved1 : 12;
            uint32_t index : 4;
        } bitfields2;
        uint32_t ordinal2;
    };

    virtual void OnSubmitStart(uint32_t submit_index, const SubmitInfo &submit_info) override;
    virtual void OnSubmitEnd(uint32_t submit_index, const SubmitInfo &submit_info) override;
    uint64_t     AddPacketNode(const IMemoryManager &mem_manager,
                               uint32_t              submit_index,
                               uint64_t              va_addr,
                               bool                  is_ce_packet,
                               Pm4Type               type,
                               uint32_t              header);
    uint64_t     AddRegisterNode(uint32_t reg, uint64_t reg_value, const RegInfo *reg_info_ptr);
    uint64_t     AddSyncEventNode(const IMemoryManager &mem_manager,
                                  uint32_t              submit_index,
                                  uint64_t              va_addr,
                                  SyncType              sync_event);

    void ParseVulkanCallMarker(char    *marker_ptr,
                               uint32_t marker_size,
                               uint64_t submit_node_index,
                               uint64_t packet_node_index);

    bool IsBeginDebugMarkerNode(uint64_t node_index);

    uint32_t GetMarkerSize(const uint8_t *marker_ptr, size_t num_dwords);

    std::string GetEventString(const IMemoryManager &mem_manager,
                               uint32_t              submit_index,
                               uint64_t              va_addr,
                               uint32_t              opcode);
    void        AppendRegNodes(const IMemoryManager &mem_manager,
                               uint32_t              submit_index,
                               uint64_t              va_addr,
                               Pm4Type4Header        header,
                               uint64_t              packet_node_index);
    void        AppendRegNodes(const IMemoryManager &mem_manager,
                               uint32_t              submit_index,
                               uint64_t              va_addr,
                               uint32_t              dword_count,
                               uint64_t              packet_node_index);
    void        AppendContextRegRmwNodes(const IMemoryManager        &mem_manager,
                                         uint32_t                     submit_index,
                                         uint64_t                     va_addr,
                                         const PM4_PFP_TYPE_3_HEADER &header,
                                         uint64_t                     packet_node_index);
    void        AppendIBFieldNodes(const char                  *suffix,
                                   const IMemoryManager        &mem_manager,
                                   uint32_t                     submit_index,
                                   uint64_t                     va_addr,
                                   bool                         is_ce_packet,
                                   const PM4_PFP_TYPE_3_HEADER &header,
                                   uint64_t                     packet_node_index);
    void        AppendLoadRegNodes(const IMemoryManager        &mem_manager,
                                   uint32_t                     submit_index,
                                   uint64_t                     va_addr,
                                   uint32_t                     reg_space_start,
                                   const PM4_PFP_TYPE_3_HEADER &header,
                                   uint64_t                     packet_node_index);
    void        AppendLoadRegIndexNodes(const IMemoryManager        &mem_manager,
                                        uint32_t                     submit_index,
                                        uint64_t                     va_addr,
                                        uint32_t                     reg_space_start,
                                        const PM4_PFP_TYPE_3_HEADER &header,
                                        uint64_t                     packet_node_index);
    void        AppendEventWriteFieldNodes(const IMemoryManager        &mem_manager,
                                           uint32_t                     submit_index,
                                           uint64_t                     va_addr,
                                           const PM4_PFP_TYPE_3_HEADER &header,
                                           const PacketInfo            *packet_info_ptr,
                                           uint64_t                     packet_node_index);
    void        AppendPacketFieldNodes(const IMemoryManager &mem_manager,
                                       uint32_t              submit_index,
                                       uint64_t              va_addr,
                                       uint32_t              dword_count,
                                       bool                  append_extra_dwords,
                                       const PacketInfo     *packet_info_ptr,
                                       uint64_t              packet_node_index,
                                       const char           *prefix = "");
    void        AppendLoadStateExtBufferNode(const IMemoryManager &mem_manager,
                                             uint32_t              submit_index,
                                             uint64_t              va_addr,
                                             uint64_t              packet_node_index);
    void        AppendMemRegNodes(const IMemoryManager &mem_manager,
                                  uint32_t              submit_index,
                                  uint64_t              va_addr,
                                  uint64_t              packet_node_index);
    void        CacheSetDrawStateGroupInfo(const IMemoryManager &mem_manager,
                                           uint32_t              submit_index,
                                           uint64_t              va_addr,
                                           uint64_t              set_draw_state_node_index,
                                           uint32_t              header);

    uint64_t AddNode(NodeType                  type,
                     const std::string        &desc,
                     CommandHierarchy::AuxInfo aux_info = 0,
                     char                     *metadata_ptr = nullptr,
                     uint32_t                  metadata_size = 0);

    void AppendEventNodeIndex(uint64_t node_index);

    void     AddChild(CommandHierarchy::TopologyType type,
                      uint64_t                       node_index,
                      uint64_t                       child_node_index);
    void     AddSharedChild(CommandHierarchy::TopologyType type,
                            uint64_t                       node_index,
                            uint64_t                       child_node_index);
    void     RemoveListOfChildren(CommandHierarchy::TopologyType type,
                                  uint64_t                       node_index,
                                  const std::vector<uint64_t>   &children_node_indices);
    uint64_t GetChildNodeIndex(CommandHierarchy::TopologyType type,
                               uint64_t                       node_index,
                               uint64_t                       child_index) const;
    uint64_t GetChildCount(CommandHierarchy::TopologyType type, uint64_t node_index) const;
    void     CreateTopologies();
    bool     IsVulkanEventNode(uint64_t node_index) const;
    bool     IsVulkanNonEventNode(uint64_t node_index) const;

    bool EventNodeHelper(uint64_t node_index, std::function<bool(uint32_t)> callback) const;
    bool IsVulkanEvent(uint32_t cmd_id) const;
    bool IsNonVulkanEvent(uint32_t cmd_id) const { return !IsVulkanEvent(cmd_id); }

    template<typename T>
    void AddConstantsToPacketNode(const IMemoryManager &mem_manager,
                                  uint64_t              ext_src_addr,
                                  uint64_t              packet_node_index,
                                  uint32_t              num_dwords,
                                  uint32_t              submit_index,
                                  uint32_t              value_count_per_row);

    struct Packets
    {
        std::vector<uint64_t> m_packet_node_indices;  // Packets added since last event

        void Add(uint32_t opcode, uint64_t addr, uint64_t packet_node_index)
        {
            m_packet_node_indices.push_back(packet_node_index);
        }
        void Clear() { m_packet_node_indices.clear(); }
        void Pop()
        {
            DIVE_ASSERT(!m_packet_node_indices.empty());
            m_packet_node_indices.pop_back();
        }
    };

    struct SetDrawStateGroupInfo
    {
        uint64_t m_group_node_index;
        uint64_t m_group_addr;
    };

    CommandHierarchy  *m_command_hierarchy_ptr = nullptr;  // Pointer to class being created
    const CaptureData *m_capture_data_ptr = nullptr;

    // Parsing State
    std::vector<uint64_t>
    m_cmd_begin_packet_node_indices;  // Potential packets node for vkBeginCommandBuffer
    std::vector<uint64_t>
    m_cmd_begin_event_node_indices;  // Potential event node for vkBeginCommandBuffer
    static constexpr uint64_t kInvalidRenderMarkerIndex = UINT64_MAX;
    uint64_t m_render_marker_index = kInvalidRenderMarkerIndex;  // Current render marker index,
                                                                 // there is no nested render
                                                                 // marker, so no need to use stack
    uint64_t              m_last_user_push_parent_node = UINT64_MAX;
    std::vector<uint64_t> m_vulkan_cmd_stack;  // Command buffer levels, first level is primary and
                                               // second level secondary command buffer
    std::unordered_map<int, std::unordered_map<uint64_t, uint64_t>>
    m_node_parent_info;  // Node parent index table, used to find which events need to be moved for
                         // VkBeginCommandBuffer.

    std::vector<uint64_t> m_ib_stack;          // Tracks current IB stack
    std::vector<uint64_t> m_renderpass_stack;  // render pass marker begin/end stack

    // Cache the most recent cp_set_draw_state node, to append IBs to later
    SetDrawStateGroupInfo m_group_info[EmulatePM4::kMaxPendingIbs] = {};
    uint32_t              m_group_info_size = 0;

    uint32_t m_num_events = 0;  // Number of events so far
    Packets  m_packets;         // Packets added since last event

    uint64_t m_cur_submit_node_index = 0;     // Current submit node being processed
    uint64_t m_cur_ib_packet_node_index = 0;  // Current ib packet node being processed
    uint8_t  m_cur_ib_level = 0;
    uint64_t m_shared_node_ib_parent_stack[EmulatePM4::kTotalIbLevels] = {};

    // Flattening is the process of forcing chain ib nodes to only ever be child to non-chain ib
    // nodes, even when daisy chaining across multiple chain ib nodes. This makes the topology
    // simpler.
    bool m_flatten_chain_nodes = false;

    // This is a list of child indices per node, ie. topology info
    // Once parsing is complete, we will create a topology from this
    // There are 2 sets of children per node, per topology. The second set of children nodes can
    // have more than 1 parent each
    std::vector<std::vector<uint64_t>> m_node_children[CommandHierarchy::kTopologyTypeCount][2];

    ILog *m_log_ptr = nullptr;

    EmulateStateTracker &m_state_tracker;
};

}  // namespace Dive
