
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
    GfxrVulkanCommandHierarchyCreator &m_gfxr_command_hierarchy_creator;
    CommandHierarchyCreator           &m_pm4_command_hierarchy_creator;

    bool m_flatten_chain_nodes = false;

    uint32_t m_num_events = 0;
};

}  // namespace Dive
