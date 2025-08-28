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
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "pm4_capture_data.h"
#include "capture_event_info.h"
#include "dive_core/common/dive_capture_format.h"
#include "dive_core/common/emulate_pm4.h"
#include "dive_core/common/pm4_packets/pfp_pm4_packets.h"
#include "dive_core/stl_replacement.h"

// Forward declarations
struct PacketInfo;
struct RegInfo;

namespace Dive
{

// Forward declarations
class Pm4CaptureData;
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
    kDrawDispatchNode,
    kDrawDispatchBlitNode,
    kSyncNode,
    kPostambleStateNode,
    kPacketNode,
    kRegNode,
    kFieldNode,
    kPresentNode,
    kRenderMarkerNode,
    kGfxrVulkanSubmitNode,
    kGfxrVulkanCommandBufferNode,
    kGfxrVulkanCommandNode,
    kGfxrVulkanCommandArgNode,
    kGfxrVulkanDrawCommandNode,
    kGfxrVulkanRenderPassCommandNode,
    kGfxrBeginDebugUtilsLabelCommandNode,
};

//--------------------------------------------------------------------------------------------------
// This is per-node graph topology info.
class Topology
{
public:
    const static uint64_t kRootNodeIndex = 0;

    virtual uint64_t GetNumNodes() const;

    // Node-index of parent node
    uint64_t GetParentNodeIndex(uint64_t node_index) const;

    // Index of child w.r.t. to its parent
    uint64_t GetChildIndex(uint64_t node_index) const;

    // Children info
    uint64_t GetNumChildren(uint64_t node_index) const;
    uint64_t GetChildNodeIndex(uint64_t node_index, uint64_t child_index) const;
    uint64_t GetNextNodeIndex(uint64_t node_index) const;

protected:
    struct ChildrenInfo
    {
        uint64_t m_start_index = UINT64_MAX;
        uint64_t m_num_children = 0;
    };

    // List of all children for all nodes.

    // m_children_list contains the node_indexes of all the "normal" children throughout the entire
    // command hierarchy for a specific TopologyType (e.g., kSubmitTopology or kAllEventTopology).
    // It's a concatenated list of all direct children, ordered by the parent node they belong to.
    // For example,
    //  if node A has children(C1, C2) and node B has children(C3),
    //  then m_children_list might look like[C1_index, C2_index, C3_index, ...]
    //
    // The m_node_children vector then contains ChildrenInfo structs for each parent node. Each
    // ChildrenInfo struct has a m_start_index and m_num_children. These values tell you where in
    // m_children_list to find the children for a specific parent node.
    DiveVector<uint64_t> m_children_list;

    // This vector points into the m_children_list to define the range of children for a particular
    // node.
    DiveVector<ChildrenInfo> m_node_children;

    // Index of parent
    DiveVector<uint64_t> m_node_parent;

    // Index of child w.r.t. to its parent
    DiveVector<uint64_t> m_node_child_index;

    virtual void SetNumNodes(uint64_t num_nodes);
    void         AddChildren(uint64_t node_index, const DiveVector<uint64_t> &children);

private:
    friend class CommandHierarchy;
    friend class GfxrVulkanCommandHierarchyCreator;
    friend class DiveCommandHierarchyCreator;
};

//--------------------------------------------------------------------------------------------------
// This is per-node graph topology info but each node can also have a set of "shared" children -
// these are children that have more than 1 parent, and are used for kPacketNodes (ie. these nodes
// can simultaneously be children of kSubmitNodes, kMarkerNodes, kDrawDispatchDmaNode nodes, etc.)
class SharedNodeTopology : public Topology
{
public:
    uint64_t GetNumNodes() const override;

    // Used to get list of shared children for a top level root node
    // All children of this top level node share this alternate set of children
    // Each non-top-level node has begin/end indices towards this set of shared children
    uint64_t GetNumSharedChildren(uint64_t node_index) const;
    uint64_t GetSharedChildNodeIndex(uint64_t node_index, uint64_t child_index) const;

    // Indicate the being & end index of the shared children in the top-level node that belong to
    // the given node
    uint64_t GetStartSharedChildNodeIndex(uint64_t node_index) const;
    uint64_t GetEndSharedChildNodeIndex(uint64_t node_index) const;

    // All descendant nodes from the same root node have the same set of shared children
    // This returns that common root top level node
    uint64_t GetSharedChildRootNodeIndex(uint64_t node_index) const;

private:
    friend class CommandHierarchy;
    friend class CommandHierarchyCreator;
    friend class DiveCommandHierarchyCreator;

    // List of all children for shared nodes.

    // m_shared_children_indices contains the node_indexes of all the "shared" children throughout
    // the entire command hierarchy for a specific TopologyType. Similar to m_children_list, it's a
    // flat, concatenated list of all nodes that are designated as "shared children". These are
    // typically kPacketNodes that can logically appear under multiple different parent nodes or
    // contexts. The m_node_shared_children vector, similarly points into m_shared_children_indices
    // to define the range of shared children belonging to a particular node.
    DiveVector<uint64_t> m_shared_children_indices;

    // This vector points into the m_shared_children_indices to define the range of shared children
    // for a particular node.
    DiveVector<ChildrenInfo> m_node_shared_children;

    // For each non-root node, indicate where the shared children start/end are, and
    // what the top level root node is
    DiveVector<uint64_t> m_start_shared_child;
    DiveVector<uint64_t> m_end_shared_child;
    DiveVector<uint64_t> m_root_node_index;

    void SetNumNodes(uint64_t num_nodes) override;
    void AddSharedChildren(uint64_t node_index, const DiveVector<uint64_t> &children);
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

    // The topologies are layed out such that the "normal" children contain non-packet nodes
    // and the "shared children" contain packet nodes. The difference lies in what is in
    // the "normal" children arrays:
    //  The Submit hierarchy contains kRootNode -> kSubmitNodes -> kIbNodes
    //  The Event hierarchy contains kRootNode -> kSubmitNodes/kPresentNodes -> (EventNodes)
    //      Where (EventNodes) == kMarkerNode/kDrawDispatchDmaNode/kSyncNode/kPostambleStateNode
    //  Note that all except kRootNode & kPresentNodes can have kPacketNodes as shared children
    const SharedNodeTopology &GetSubmitHierarchyTopology() const;
    const SharedNodeTopology &GetAllEventHierarchyTopology() const;

    NodeType    GetNodeType(uint64_t node_index) const;
    const char *GetNodeDesc(uint64_t node_index) const;

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

    // GetEventIndex returns sequence number for Event/Sync Nodes, 0 if not exist.
    size_t GetEventIndex(uint64_t node_index) const;

    // For kBinningPassOnly
    // - Keep Binning Pass
    // - Exclude all Tile&Resolve Passes (0 - N)
    // For kFirstTilePassOnly
    // - Keep Tile Pass 0 (1st Tile&Resolve Pass after the Binning Pass)
    // - Exclude all nodes in Binning Pass
    // - Exclude Tile Pass 1 to N
    // For kBinningAndFirstTilePass
    // - Keep Binning Pass
    // - Keep Tile Pass 0 (1st Tile&Resolve Pass after the Binning Pass)
    // - Exclude Tile Pass 1 to N
    enum FilterListType : uint32_t
    {
        kBinningPassOnly,
        kFirstTilePassOnly,
        kBinningAndFirstTilePass,
        kFilterListTypeCount
    };

    const std::unordered_set<uint64_t> &GetFilterExcludeIndices(FilterListType filter_type) const
    {
        return m_filter_exclude_indices_list[filter_type];
    }

private:
    friend class CommandHierarchyCreator;
    friend class GfxrVulkanCommandHierarchyCreator;
    friend class DiveCommandHierarchyCreator;

    enum TopologyType
    {
        kSubmitTopology,
        kAllEventTopology,
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
        DiveVector<NodeType>    m_node_type;
        DiveVector<std::string> m_description;
        DiveVector<AuxInfo>     m_aux_info;
        DiveVector<uint64_t>    m_event_node_indices;

        uint64_t AddNode(NodeType type, std::string &&desc, AuxInfo aux_info);
        uint64_t AddGfxrNode(NodeType type, std::string &&desc);
    };

    // Add a node and returns index of the added node
    uint64_t AddNode(NodeType type, std::string &&desc, AuxInfo aux_info);
    // Add a gfxr node and returns index of the added node
    uint64_t AddGfxrNode(NodeType type, std::string &&desc);
    void     AddToFilterExcludeIndexList(uint64_t index, FilterListType filter_mode)
    {
        m_filter_exclude_indices_list[filter_mode].insert(index);
    }

    Nodes                        m_nodes;
    std::unordered_set<uint64_t> m_filter_exclude_indices_list[kFilterListTypeCount];
    SharedNodeTopology           m_topology[kTopologyTypeCount];
};

//--------------------------------------------------------------------------------------------------
class CommandHierarchyCreator : public EmulateCallbacksBase
{
public:
    CommandHierarchyCreator(CommandHierarchy     &command_hierarchy,
                            const Pm4CaptureData &capture_data);
    // If flatten_chain_nodes set to true, then chain nodes are children of the top-most
    // root ib or call ib node, and never a child of another chain node. This prevents a
    // deep tree of chain nodes when a capture chains together tons of IBs.
    // Optional: Passing a reserve_size will allow the creator to pre-reserve the memory needed and
    // potentially speed up the creation
    bool CreateTrees(bool flatten_chain_nodes, std::optional<uint64_t> reserve_size);

    bool CreateTrees(const Pm4CaptureData   &capture_data,
                     bool                    flatten_chain_nodes,
                     std::optional<uint64_t> reserve_size);

    bool CreateTrees(bool                    flatten_chain_nodes,
                     bool                    createTopologies,
                     std::optional<uint64_t> reserve_size);

    // This is used to create a command-hierarchy out of a PM4 universal stream (ie: single IB)
    bool CreateTrees(EngineType             engine_type,
                     QueueType              queue_type,
                     std::vector<uint32_t> &command_dwords,
                     uint32_t               size_in_dwords);

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
                          Pm4Header             header) override;

    void CreateTopologies();

    virtual void OnSubmitStart(uint32_t submit_index, const SubmitInfo &submit_info) override;
    virtual void OnSubmitEnd(uint32_t submit_index, const SubmitInfo &submit_info) override;

    const DiveVector<DiveVector<uint64_t>> &GetNodeChildren(uint64_t type, size_t sub_index) const
    {
        return m_node_children[type][sub_index];
    }

    const DiveVector<uint64_t> &GetNodeStartSharedChildren(uint64_t type) const
    {
        return m_node_start_shared_children[type];
    }

    const DiveVector<uint64_t> &GetNodeEndSharedChildren(uint64_t type) const
    {
        return m_node_end_shared_children[type];
    }

    const DiveVector<uint64_t> &GetNodeRootNodeIndices(uint64_t type) const
    {
        return m_node_root_node_indices[type];
    }

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

    // The type of child nodes.
    enum ChildrenNodeType
    {
        kSingleParentNodeChildren,
        kSharedNodeChildren,
        kChildrenNodeTypeCount
    };

    uint64_t AddPacketNode(const IMemoryManager &mem_manager,
                           uint32_t              submit_index,
                           uint64_t              va_addr,
                           bool                  is_ce_packet,
                           Pm4Header             header);
    uint64_t AddRegisterNode(uint32_t reg, uint64_t reg_value, const RegInfo *reg_info_ptr);

    bool IsBeginDebugMarkerNode(uint64_t node_index);

    uint32_t GetMarkerSize(const uint8_t *marker_ptr, size_t num_dwords);

    void     AppendRegNodes(const IMemoryManager &mem_manager,
                            uint32_t              submit_index,
                            uint64_t              va_addr,
                            Pm4Header             header,
                            uint64_t              packet_node_index);
    void     AppendRegNodes(const IMemoryManager &mem_manager,
                            uint32_t              submit_index,
                            uint64_t              va_addr,
                            uint32_t              dword_count,
                            uint64_t              packet_node_index);
    void     AppendContextRegRmwNodes(const IMemoryManager        &mem_manager,
                                      uint32_t                     submit_index,
                                      uint64_t                     va_addr,
                                      const PM4_PFP_TYPE_3_HEADER &header,
                                      uint64_t                     packet_node_index);
    void     AppendIBFieldNodes(const char                  *suffix,
                                const IMemoryManager        &mem_manager,
                                uint32_t                     submit_index,
                                uint64_t                     va_addr,
                                bool                         is_ce_packet,
                                const PM4_PFP_TYPE_3_HEADER &header,
                                uint64_t                     packet_node_index);
    void     AppendLoadRegNodes(const IMemoryManager        &mem_manager,
                                uint32_t                     submit_index,
                                uint64_t                     va_addr,
                                uint32_t                     reg_space_start,
                                const PM4_PFP_TYPE_3_HEADER &header,
                                uint64_t                     packet_node_index);
    void     AppendLoadRegIndexNodes(const IMemoryManager        &mem_manager,
                                     uint32_t                     submit_index,
                                     uint64_t                     va_addr,
                                     uint32_t                     reg_space_start,
                                     const PM4_PFP_TYPE_3_HEADER &header,
                                     uint64_t                     packet_node_index);
    void     AppendEventWriteFieldNodes(const IMemoryManager        &mem_manager,
                                        uint32_t                     submit_index,
                                        uint64_t                     va_addr,
                                        const PM4_PFP_TYPE_3_HEADER &header,
                                        const PacketInfo            *packet_info_ptr,
                                        uint64_t                     packet_node_index);
    void     AppendPacketFieldNodes(const IMemoryManager &mem_manager,
                                    uint32_t              submit_index,
                                    uint64_t              va_addr,
                                    uint32_t              dword_count,
                                    bool                  append_extra_dwords,
                                    const PacketInfo     *packet_info_ptr,
                                    uint64_t              packet_node_index,
                                    const char           *prefix = "");
    void     AppendLoadStateExtBufferNode(const IMemoryManager &mem_manager,
                                          uint32_t              submit_index,
                                          uint64_t              va_addr,
                                          uint64_t              packet_node_index);
    void     AppendMemRegNodes(const IMemoryManager &mem_manager,
                               uint32_t              submit_index,
                               uint64_t              va_addr,
                               uint64_t              packet_node_index);
    void     CacheSetDrawStateGroupInfo(const IMemoryManager &mem_manager,
                                        uint32_t              submit_index,
                                        uint64_t              va_addr,
                                        uint64_t              set_draw_state_node_index,
                                        Pm4Header             header);
    uint64_t AddNode(NodeType type, std::string &&desc, CommandHierarchy::AuxInfo aux_info = 0);

    void AppendEventNodeIndex(uint64_t node_index);

    void AddChild(CommandHierarchy::TopologyType type,
                  uint64_t                       node_index,
                  uint64_t                       child_node_index);
    void AddSharedChild(CommandHierarchy::TopologyType type,
                        uint64_t                       node_index,
                        uint64_t                       child_node_index);
    void SetStartSharedChildrenNodeIndex(CommandHierarchy::TopologyType type,
                                         uint64_t                       node_index,
                                         uint64_t                       shared_child_node_index);
    void SetEndSharedChildrenNodeIndex(CommandHierarchy::TopologyType type,
                                       uint64_t                       node_index,
                                       uint64_t                       shared_child_node_index);
    void SetSharedChildRootNodeIndex(CommandHierarchy::TopologyType type,
                                     uint64_t                       node_index,
                                     uint64_t                       root_node_index);

    uint64_t GetChildNodeIndex(CommandHierarchy::TopologyType type,
                               uint64_t                       node_index,
                               uint64_t                       child_index) const;
    uint64_t GetChildCount(CommandHierarchy::TopologyType type, uint64_t node_index) const;

    bool EventNodeHelper(uint64_t node_index, std::function<bool(uint32_t)> callback) const;

    template<typename T>
    void AddConstantsToPacketNode(const IMemoryManager &mem_manager,
                                  uint64_t              ext_src_addr,
                                  uint64_t              packet_node_index,
                                  uint32_t              num_dwords,
                                  uint32_t              submit_index,
                                  uint32_t              value_count_per_row);

    struct SetDrawStateGroupInfo
    {
        uint64_t m_group_node_index;
        uint64_t m_group_addr;
    };

    CommandHierarchy     &m_command_hierarchy;  // Reference to class being created
    const Pm4CaptureData &m_capture_data;

    // Parsing State
    DiveVector<uint64_t>
    m_cmd_begin_packet_node_indices;  // Potential packets node for vkBeginCommandBuffer
    DiveVector<uint64_t>
    m_cmd_begin_event_node_indices;  // Potential event node for vkBeginCommandBuffer
    static constexpr uint64_t kInvalidRenderMarkerIndex = UINT64_MAX;
    uint64_t m_render_marker_index = kInvalidRenderMarkerIndex;  // Current render marker index,
                                                                 // there is no nested render
                                                                 // marker, so no need to use stack
    uint64_t m_last_user_push_parent_node = UINT64_MAX;
    // second level secondary command buffer
    std::unordered_map<int, std::unordered_map<uint64_t, uint64_t>>
    m_node_parent_info;  // Node parent index table, used to find which events need to be moved for
                         // VkBeginCommandBuffer.

    DiveVector<uint64_t> m_ib_stack;          // Tracks current IB stack
    DiveVector<uint64_t> m_renderpass_stack;  // render pass marker begin/end stack

    // Cache the most recent cp_set_draw_state node, to append IBs to later
    SetDrawStateGroupInfo m_group_info[EmulatePM4::kMaxPendingIbs] = {};
    uint32_t              m_group_info_size = 0;

    // Cache the most recent cp_start_bin, to be appended to later with prefix and common packets
    uint64_t m_start_bin_node_index = UINT64_MAX;

    // Cache the most recent fixed stride draw table, to be appended to later
    uint64_t m_draw_table_node_index = UINT64_MAX;

    uint32_t m_num_events = 0;  // Number of events so far

    bool m_new_event_start = true;
    bool m_new_ib_start = true;
    bool m_tracking_first_tile_pass_start = false;

    // Stack of shared child node that begins the current ibs/pass/events
    // Need a stack because IBs and pass/events can be stacked
    DiveVector<uint64_t> m_start_node_stack[CommandHierarchy::kTopologyTypeCount];

    uint64_t m_last_added_node_index;

    uint64_t m_cur_submit_node_index = 0;     // Current submit node being processed
    uint64_t m_cur_ib_packet_node_index = 0;  // Current ib packet node being processed
    uint8_t  m_cur_ib_level = 0;
    uint64_t m_shared_node_ib_parent_stack[EmulatePM4::kTotalIbLevels] = {};

    // Flattening is the process of forcing chain ib nodes to only ever be child to non-chain ib
    // nodes, even when daisy chaining across multiple chain ib nodes. This makes the topology
    // simpler.
    bool m_flatten_chain_nodes = false;

    // Range of shared children associated with each non-top-level node, per topology
    DiveVector<uint64_t> m_node_start_shared_children[CommandHierarchy::kTopologyTypeCount];
    DiveVector<uint64_t> m_node_end_shared_children[CommandHierarchy::kTopologyTypeCount];
    DiveVector<uint64_t> m_node_root_node_indices[CommandHierarchy::kTopologyTypeCount];

    // This is a list of child indices per node, ie. topology info
    // Once parsing is complete, we will create a topology from this
    // There are 2 sets of children per node, per topology. The second set of children nodes can
    // have more than 1 parent each
    DiveVector<DiveVector<uint64_t>> m_node_children[CommandHierarchy::kTopologyTypeCount]
                                                    [kChildrenNodeTypeCount];
};

}  // namespace Dive
