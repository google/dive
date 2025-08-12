
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
#include "dive_core/common/emulate_pm4.h"
#include "dive_core/dive_capture_data.h"
#include "dive_core/gfxr_vulkan_command_hierarchy.h"

namespace Dive
{

class MixedNodeTopology : public Topology
{
public:
    uint64_t GetNumNodes() const override;

    // Used to get list of shared children for a top level root node
    // All children of this top level node share this alternate set of children
    // Each non-top-level node has begin/end indices towards this set of shared children
    uint64_t GetNumSharedChildren(uint64_t node_index) const;
    uint64_t GetSharedChildNodeIndex(uint64_t node_index, uint64_t child_index) const;

    // Indicate the begin & end index of the shared children in the top-level node that belong to
    // the given node
    uint64_t GetStartSharedChildNodeIndex(uint64_t node_index) const;
    uint64_t GetEndSharedChildNodeIndex(uint64_t node_index) const;

    // All descendant nodes from the same root node have the same set of shared children
    // This returns that common root top level node
    uint64_t GetSharedChildRootNodeIndex(uint64_t node_index) const;

private:
    friend class CommandHierarchy;
    friend class CommandHierarchyCreator;

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
    DiveVector<uint64_t> m_start_shared_children;
    DiveVector<uint64_t> m_end_shared_children;
    DiveVector<uint64_t> m_root_node_indices;

    void SetNumNodes(uint64_t num_nodes) override;
    void AddSharedChildren(uint64_t node_index, const DiveVector<uint64_t> &children);
};

class DiveCommandHierarchyCreator
{
public:
    DiveCommandHierarchyCreator(CommandHierarchy                  &command_hierarchy,
                                GfxrVulkanCommandHierarchyCreator &gfxr_command_hierarchy_creator,
                                CommandHierarchyCreator           &pm4_command_hierarchy_creator);

    bool ProcessDiveSubmits(
    const DiveVector<SubmitInfo>                                            &submits,
    const IMemoryManager                                                    &mem_manager,
    const std::vector<std::unique_ptr<DiveAnnotationProcessor::SubmitInfo>> &gfxr_submits);

    // If flatten_chain_nodes set to true, then chain nodes are children of the top-most
    // root ib or call ib node, and never a child of another chain node. This prevents a
    // deep tree of chain nodes when a capture chains together tons of IBs.
    // Optional: Passing a reserve_size will allow the creator to pre-reserve the memory needed and
    // potentially speed up the creation
    bool CreateTrees(DiveCaptureData        &dive_capture_data,
                     bool                    flatten_chain_nodes,
                     std::optional<uint64_t> reserve_size);

    void OnCommand(uint32_t submit_index, DiveAnnotationProcessor::VulkanCommandInfo vk_cmd_info);

    void OnGfxrSubmit(uint32_t                                   submit_index,
                      const DiveAnnotationProcessor::SubmitInfo &submit_info);

    void CreateTopologies();

private:
    friend class CommandHierarchyCreator;
    friend class GfxrVulkanCommandHierarchyCreator;

    CommandHierarchy                  &m_command_hierarchy;
    const Pm4CaptureData              *m_capture_data_ptr = nullptr;
    const GfxrCaptureData             *m_gfxr_capture_data_ptr = nullptr;
    GfxrVulkanCommandHierarchyCreator &m_gfxr_command_hierarchy_creator;
    CommandHierarchyCreator           &m_pm4_command_hierarchy_creator;

    bool m_flatten_chain_nodes = false;

    uint32_t m_num_events = 0;
};

}  // namespace Dive
