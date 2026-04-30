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

#include "dive_core/gfxr_vulkan_command_hierarchy.h"

#include <functional>
#include <iostream>
#include <string_view>
#include <unordered_map>

#include "absl/strings/numbers.h"
#include "dive_core/dive_strings.h"

namespace Dive
{

// =================================================================================================
// GfxrVulkanCommandHierarchyCreator
// =================================================================================================
GfxrVulkanCommandHierarchyCreator::GfxrVulkanCommandHierarchyCreator(
    CommandHierarchy& command_hierarchy, const GfxrCaptureData& capture_data)
    : m_command_hierarchy(command_hierarchy), m_capture_data(capture_data)
{
}

GfxrVulkanCommandHierarchyCreator::~GfxrVulkanCommandHierarchyCreator() {}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandHierarchyCreator::ConditionallyAddChild(uint64_t node_index)
{
    // Check if the command node should be a child of a command buffer or debug utils/renderpass
    // node
    if (m_cur_parent_node_index_stack.empty())
    {
        AddChild(CommandHierarchy::TopologyType::kAllEventTopology, m_cur_command_buffer_node_index,
                 node_index);
    }
    else
    {
        AddChild(CommandHierarchy::TopologyType::kAllEventTopology,
                 m_cur_parent_node_index_stack.top(), node_index);
    }
}

std::string GfxrVulkanCommandHierarchyCreator::GetValueStr(const nlohmann::ordered_json& node,
                                                           const std::string& key)
{
    if (node.contains(key))
    {
        const auto& val = node[key];
        if (val.is_string())
        {
            return val.get<std::string>();
        }
        return val.dump();
    }
    return "0";
}

void GfxrVulkanCommandHierarchyCreator::AppendDrawCallSummary(std::ostringstream& s,
                                                              const nlohmann::ordered_json& args,
                                                              const std::string& primary_key)
{
    if (args.contains(primary_key))
    {
        s << "(" << primary_key << ": " << GetValueStr(args, primary_key);
        if (args.contains("instanceCount"))
        {
            std::string instance_count = GetValueStr(args, "instanceCount");
            if (instance_count != "1")
            {
                s << ", instanceCount: " << instance_count;
            }
        }
        s << ")";
    }
}

void GfxrVulkanCommandHierarchyCreator::AppendEnumSummary(std::ostringstream& s,
                                                          const nlohmann::ordered_json& args,
                                                          const std::string& key,
                                                          const std::string& prefix)
{
    if (args.contains(key))
    {
        std::string val = GetValueStr(args, key);
        size_t prefix_pos = val.find(prefix);
        if (prefix_pos != std::string::npos)
        {
            val.erase(prefix_pos, prefix.length());
        }
        s << "(" << val << ")";
    }
}

std::string GfxrVulkanCommandHierarchyCreator::GetCommandSummary(const std::string& cmd_name,
                                                                 const nlohmann::ordered_json& args)
{
    std::ostringstream s;

    using SummaryHandler = std::function<void(GfxrVulkanCommandHierarchyCreator*,
                                              std::ostringstream&, const nlohmann::ordered_json&)>;

    static const std::unordered_map<std::string_view, SummaryHandler> handlers = {
        {"vkCmdDrawIndexed",
         [](auto* self, auto& s, const auto& args) {
             self->AppendDrawCallSummary(s, args, "indexCount");
         }},
        {"vkCmdDraw",
         [](auto* self, auto& s, const auto& args) {
             self->AppendDrawCallSummary(s, args, "vertexCount");
         }},
        {"vkCmdSetViewport",
         [](auto* self, auto& s, const auto& args) {
             if (args.contains("pViewports") && args["pViewports"].is_array() &&
                 !args["pViewports"].empty())
             {
                 const auto& vp = args["pViewports"][0];
                 s << "(x:" << self->GetValueStr(vp, "x") << ", y:" << self->GetValueStr(vp, "y")
                   << ", width:" << self->GetValueStr(vp, "width")
                   << ", height:" << self->GetValueStr(vp, "height") << ")";
             }
         }},
        {"vkCmdSetScissor",
         [](auto* self, auto& s, const auto& args) {
             if (args.contains("pScissors") && args["pScissors"].is_array() &&
                 !args["pScissors"].empty())
             {
                 const auto& sc = args["pScissors"][0];
                 if (sc.contains("offset") && sc.contains("extent"))
                 {
                     const auto& offset = sc["offset"];
                     const auto& extent = sc["extent"];
                     s << "(x:" << self->GetValueStr(offset, "x")
                       << ", y:" << self->GetValueStr(offset, "y")
                       << ", width:" << self->GetValueStr(extent, "width")
                       << ", height:" << self->GetValueStr(extent, "height") << ")";
                 }
             }
         }},
        {"vkCmdDispatch",
         [](auto* self, auto& s, const auto& args) {
             if (args.contains("groupCountX"))
             {
                 s << "(x:" << self->GetValueStr(args, "groupCountX")
                   << ", y:" << self->GetValueStr(args, "groupCountY")
                   << ", z:" << self->GetValueStr(args, "groupCountZ") << ")";
             }
         }},
        {"vkCmdDrawMultiEXT",
         [](auto* self, auto& s, const auto& args) {
             self->AppendDrawCallSummary(s, args, "drawCount");
         }},
        {"vkCmdDrawMultiIndexedEXT",
         [](auto* self, auto& s, const auto& args) {
             self->AppendDrawCallSummary(s, args, "drawCount");
         }},
        {"vkCmdDrawIndirect",
         [](auto* self, auto& s, const auto& args) {
             if (args.contains("drawCount"))
             {
                 std::string draw_count = self->GetValueStr(args, "drawCount");
                 if (draw_count != "1")
                 {
                     s << "(drawCount: " << draw_count << ")";
                 }
             }
         }},
        {"vkCmdDrawIndexedIndirect",
         [](auto* self, auto& s, const auto& args) {
             if (args.contains("drawCount"))
             {
                 std::string draw_count = self->GetValueStr(args, "drawCount");
                 if (draw_count != "1")
                 {
                     s << "(drawCount: " << draw_count << ")";
                 }
             }
         }},
        {"vkCmdBindPipeline",
         [](auto* self, auto& s, const auto& args) {
             self->AppendEnumSummary(s, args, "pipelineBindPoint", "VK_PIPELINE_BIND_POINT_");
         }},
        {"vkCmdBindDescriptorSets",
         [](auto* self, auto& s, const auto& args) {
             if (args.contains("firstSet") && args.contains("descriptorSetCount"))
             {
                 s << "(firstSet: " << self->GetValueStr(args, "firstSet")
                   << ", setCount: " << self->GetValueStr(args, "descriptorSetCount") << ")";
             }
         }},
        {"vkCmdBindVertexBuffers",
         [](auto* self, auto& s, const auto& args) {
             if (args.contains("firstBinding") && args.contains("bindingCount"))
             {
                 s << "(firstBinding: " << self->GetValueStr(args, "firstBinding")
                   << ", bindingCount: " << self->GetValueStr(args, "bindingCount") << ")";
             }
         }},
        {"vkCmdBindIndexBuffer", [](auto* self, auto& s, const auto& args) {
             self->AppendEnumSummary(s, args, "indexType", "VK_INDEX_TYPE_");
         }}};

    auto it = handlers.find(std::string_view(cmd_name));
    if (it != handlers.end())
    {
        it->second(this, s, args);
    }

    return s.str();
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandHierarchyCreator::OnCommand(
    const DiveAnnotationProcessor::VulkanCommandInfo& vk_cmd_info, uint64_t draw_call_count,
    std::vector<uint64_t>& render_pass_draw_call_counts)
{
    const std::string& vulkan_cmd_name = vk_cmd_info.name;
    const nlohmann::ordered_json& vulkan_cmd_args = vk_cmd_info.args;
    std::ostringstream vk_cmd_string_stream;
    vk_cmd_string_stream << vulkan_cmd_name;
    vk_cmd_string_stream << GetCommandSummary(vulkan_cmd_name, vulkan_cmd_args);

    if (vulkan_cmd_name == "vkBeginCommandBuffer")
    {
        vk_cmd_string_stream << ", Draw Call Count: " << draw_call_count;
        uint64_t cmd_buffer_index =
            AddNode(NodeType::kGfxrVulkanBeginCommandBufferNode, vk_cmd_string_stream.str());
        m_cur_command_buffer_node_index = cmd_buffer_index;
        GetArgs(vulkan_cmd_args, m_cur_command_buffer_node_index);
        AddChild(CommandHierarchy::TopologyType::kAllEventTopology, m_cur_submit_node_index,
                 cmd_buffer_index);
    }
    else if (vulkan_cmd_name == "vkEndCommandBuffer")
    {
        uint64_t cmd_buffer_index =
            AddNode(NodeType::kGfxrVulkanEndCommandBufferNode, vk_cmd_string_stream.str());

        GetArgs(vulkan_cmd_args, cmd_buffer_index);
        AddChild(CommandHierarchy::TopologyType::kAllEventTopology, m_cur_command_buffer_node_index,
                 cmd_buffer_index);
    }
    else if (vulkan_cmd_name.find("BeginDebugUtilsLabelEXT") != std::string::npos)
    {
        std::string label_name = vulkan_cmd_args["pLabelInfo"]["pLabelName"];

        uint64_t begin_debug_utils_label_cmd_index =
            AddNode(NodeType::kGfxrBeginDebugUtilsLabelCommandNode, label_name.c_str());
        GetArgs(vulkan_cmd_args, begin_debug_utils_label_cmd_index);
        ConditionallyAddChild(begin_debug_utils_label_cmd_index);
        m_cur_parent_node_index_stack.push(begin_debug_utils_label_cmd_index);
    }
    else if (vulkan_cmd_name.find("EndDebugUtilsLabelEXT") != std::string::npos)
    {
        if (!m_cur_parent_node_index_stack.empty() &&
            m_command_hierarchy.GetNodeType(m_cur_parent_node_index_stack.top()) ==
                NodeType::kGfxrBeginDebugUtilsLabelCommandNode)
        {
            // Remove the corresponding begin debug utils node from the stack
            m_cur_parent_node_index_stack.pop();
        }
    }
    else if (vulkan_cmd_name.find("vkCmdDraw") != std::string::npos ||
             vulkan_cmd_name.find("vkCmdDispatch") != std::string::npos)
    {
        uint64_t vk_cmd_index =
            AddNode(NodeType::kGfxrVulkanDrawCommandNode, vk_cmd_string_stream.str());
        GetArgs(vulkan_cmd_args, vk_cmd_index);
        ConditionallyAddChild(vk_cmd_index);
    }
    else if (vulkan_cmd_name.find("vkCmdBeginRenderPass") != std::string::npos)
    {
        if (!render_pass_draw_call_counts.empty())
        {
            draw_call_count = render_pass_draw_call_counts.front();
            render_pass_draw_call_counts.erase(render_pass_draw_call_counts.begin());
        }
        vk_cmd_string_stream << ", Draw Call Count: " << draw_call_count;
        uint64_t vk_cmd_index =
            AddNode(NodeType::kGfxrVulkanBeginRenderPassCommandNode, vk_cmd_string_stream.str());
        GetArgs(vulkan_cmd_args, vk_cmd_index);
        ConditionallyAddChild(vk_cmd_index);
        m_cur_parent_node_index_stack.push(vk_cmd_index);
    }
    else if (vulkan_cmd_name.find("vkCmdEndRenderPass") != std::string::npos)
    {
        uint64_t vk_cmd_index =
            AddNode(NodeType::kGfxrVulkanEndRenderPassCommandNode, vk_cmd_string_stream.str());
        GetArgs(vulkan_cmd_args, vk_cmd_index);
        ConditionallyAddChild(vk_cmd_index);
        if (!m_cur_parent_node_index_stack.empty())
        {
            // Remove the corresponding vkCmdBeginRenderPass node from the stack
            m_cur_parent_node_index_stack.pop();
        }
    }
    else
    {
        NodeType node_type = NodeType::kGfxrVulkanCommandNode;
        if (vulkan_cmd_name.find("vkCmdCopyBuffer") != std::string::npos)
        {
            node_type = NodeType::kGfxrVulkanCopyBufferCommandNode;
        }
        else if (vulkan_cmd_name.find("vkCmdClearAttachments") != std::string::npos)
        {
            node_type = NodeType::kGfxrVulkanClearAttachmentsCommandNode;
        }
        else if (vulkan_cmd_name.find("vkCmdClearColorImage") != std::string::npos)
        {
            node_type = NodeType::kGfxrVulkanClearColorImageCommandNode;
        }
        else if (vulkan_cmd_name.find("vkCmdClearDepthStencilImage") != std::string::npos)
        {
            node_type = NodeType::kGfxrVulkanClearDepthStencilImageCommandNode;
        }
        else if (vulkan_cmd_name.find("vkCmdResolveImage") != std::string::npos)
        {
            node_type = NodeType::kGfxrVulkanResolveImageCommandNode;
        }

        uint64_t vk_cmd_index = AddNode(node_type, vk_cmd_string_stream.str());
        GetArgs(vulkan_cmd_args, vk_cmd_index);
        ConditionallyAddChild(vk_cmd_index);
    }
}

//--------------------------------------------------------------------------------------------------
bool GfxrVulkanCommandHierarchyCreator::ProcessVkCmds(
    const std::vector<DiveAnnotationProcessor::VulkanCommandInfo>& vkCmds, uint64_t draw_call_count,
    const std::vector<uint64_t>& render_pass_draw_call_counts)
{
    std::vector<uint64_t> mutable_render_pass_draw_call_counts = render_pass_draw_call_counts;

    for (uint32_t i = 0; i < vkCmds.size(); ++i)
    {
        DiveAnnotationProcessor::VulkanCommandInfo vk_cmd_info = vkCmds[i];
        OnCommand(vk_cmd_info, draw_call_count, mutable_render_pass_draw_call_counts);
    }

    // Ensure the parent node index stack is cleared
    while (!m_cur_parent_node_index_stack.empty())
    {
        m_cur_parent_node_index_stack.pop();
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool GfxrVulkanCommandHierarchyCreator::ProcessGfxrSubmits(const GfxrCaptureData& capture_data)
{
    // Add frame node
    uint64_t frame_root_node_index = AddNode(NodeType::kGfxrRootFrameNode, "Frame");
    AddChild(CommandHierarchy::kAllEventTopology, Topology::kRootNodeIndex, frame_root_node_index);

    const auto& submits = capture_data.GetGfxrSubmits();
    for (uint32_t submit_index = 0; submit_index < submits.size(); ++submit_index)
    {
        const DiveAnnotationProcessor::SubmitInfo& submit_info = *submits[submit_index];

        OnGfxrSubmit(submit_index, submit_info);

        std::vector<uint64_t> empty_render_pass_counts;
        if (!ProcessVkCmds(submit_info.none_cmd_vk_commands, 0, empty_render_pass_counts))
        {
            return false;
        }

        const auto& cmd_handles = submit_info.vk_command_buffer_handles;
        for (const auto& handle : cmd_handles)
        {
            const auto& draw_call_counts = capture_data.GetDrawCallCounts(handle);
            if (!ProcessVkCmds(capture_data.GetGfxrCommandBuffers(handle),
                               draw_call_counts.begin_command_buffer_draw_call_count,
                               draw_call_counts.render_pass_draw_call_counts))
            {
                return false;
            }
        }
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
bool GfxrVulkanCommandHierarchyCreator::CreateTrees(bool used_in_mixed_command_hierarchy)
{
    m_used_in_mixed_command_hierarchy = used_in_mixed_command_hierarchy;
    // Clear/Reset internal data structures, just in case
    ClearCreatedDiveIndices();
    if (!m_used_in_mixed_command_hierarchy)
    {
        m_command_hierarchy = CommandHierarchy();

        // Add a dummy root node for easier management
        uint64_t root_node_index = AddNode(NodeType::kRootNode, "");
        DIVE_VERIFY(root_node_index == Topology::kRootNodeIndex);

        if (!ProcessGfxrSubmits(m_capture_data))
        {
            return false;
        }

        // Convert the info in m_gfxr_node_children into GfxrVulkanCommandHierarchy's topologies
        CreateTopologies();
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
uint64_t GfxrVulkanCommandHierarchyCreator::AddNode(NodeType type, std::string&& desc)
{
    uint64_t node_index = m_command_hierarchy.AddGfxrNode(type, std::move(desc));

    if (m_used_in_mixed_command_hierarchy)
    {
        uint64_t local_node_index = m_node_children[CommandHierarchy::kAllEventTopology].size();
        m_dive_indices_to_local_indices_map[node_index] = local_node_index;
        m_node_children[CommandHierarchy::kAllEventTopology].resize(local_node_index + 1);
    }
    else
    {
        DIVE_ASSERT(m_node_children[CommandHierarchy::kAllEventTopology].size() == node_index);
        m_node_children[CommandHierarchy::kAllEventTopology].resize(
            m_node_children[CommandHierarchy::kAllEventTopology].size() + 1);
    }

    return node_index;
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandHierarchyCreator::AddChild(CommandHierarchy::TopologyType type,
                                                 uint64_t node_index, uint64_t child_node_index)
{
    if (m_used_in_mixed_command_hierarchy)
    {
        if (m_dive_indices_to_local_indices_map.find(node_index) !=
            m_dive_indices_to_local_indices_map.end())
        {
            node_index = m_dive_indices_to_local_indices_map.at(node_index);
        }
        DIVE_ASSERT(node_index < m_node_children[type].size());
        m_node_children[type][node_index].push_back(child_node_index);
    }
    else
    {
        DIVE_ASSERT(node_index < m_node_children[type].size());
        m_node_children[type][node_index].push_back(child_node_index);
    }
}

void GfxrVulkanCommandHierarchyCreator::GetArgs(const nlohmann::ordered_json& json_args,
                                                uint64_t curr_index)
{
    // This block processes key-value pairs where keys represent field names
    // and values can be objects, arrays, or primitives.
    if (json_args.is_object())
    {
        for (auto const& [key, val] : json_args.items())
        {
            if (val.is_object())
            {
                // If the value is another object, create a new node for it
                // and recursively process it.
                uint64_t object_node_index =
                    AddNode(NodeType::kGfxrVulkanCommandArgNode, key.c_str());
                AddChild(CommandHierarchy::TopologyType::kAllEventTopology, curr_index,
                         object_node_index);

                GetArgs(val, object_node_index);
            }
            else if (val.is_array())
            {
                // If the value is an array, create a new node for the array
                // and then iterate through its elements.
                uint64_t array_node_index =
                    AddNode(NodeType::kGfxrVulkanCommandArgNode, key.c_str());
                AddChild(CommandHierarchy::TopologyType::kAllEventTopology, curr_index,
                         array_node_index);
                for (size_t i = 0; i < val.size(); ++i)
                {
                    const auto& element = val[i];
                    if (element.is_object())
                    {
                        // If an array element is an object, recursively process it.
                        GetArgs(element, array_node_index);
                    }
                    else if (element.is_array())
                    {
                        // If an array element is a nested array,
                        // create a node for it and recursively process it.
                        uint64_t nested_array_node_index = AddNode(
                            NodeType::kGfxrVulkanCommandArgNode, "element_" + std::to_string(i));
                        AddChild(CommandHierarchy::TopologyType::kAllEventTopology,
                                 array_node_index, nested_array_node_index);
                        GetArgs(element, nested_array_node_index);
                    }
                    else
                    {
                        // If an array element is a primitive,
                        // create a node containing its string representation.
                        uint64_t arg_index =
                            AddNode(NodeType::kGfxrVulkanCommandArgNode, element.dump());
                        AddChild(CommandHierarchy::TopologyType::kAllEventTopology,
                                 array_node_index, arg_index);
                    }
                }
            }
            else
            {
                // If the value is a primitive,
                // create a node containing the "key:value" pair.
                uint64_t vk_cmd_arg_index =
                    AddNode(NodeType::kGfxrVulkanCommandArgNode, key + ":" + val.dump());
                AddChild(CommandHierarchy::TopologyType::kAllEventTopology, curr_index,
                         vk_cmd_arg_index);
            }
        }
    }
    // This block processes each element of an array.
    else if (json_args.is_array())
    {
        for (size_t i = 0; i < json_args.size(); ++i)
        {
            const auto& element = json_args[i];
            if (element.is_object() || element.is_array())
            {
                // If an array element is an object or another array,
                // recursively process it, and associate it with the current parent node.
                GetArgs(element, curr_index);
            }
            else
            {
                // If an array element is a primitive, create a node for its string representation.
                uint64_t arg_index = AddNode(NodeType::kGfxrVulkanCommandArgNode, element.dump());
                AddChild(CommandHierarchy::TopologyType::kAllEventTopology, curr_index, arg_index);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandHierarchyCreator::OnGfxrSubmit(
    uint32_t submit_index, const DiveAnnotationProcessor::SubmitInfo& submit_info)
{
    std::ostringstream submit_string_stream;
    submit_string_stream << submit_info.name << ": " << submit_index;
    submit_string_stream << ", Command Buffer Count: "
                         << std::to_string(submit_info.vk_command_buffer_handles.size());
    // Create submit node
    uint64_t submit_node_index =
        AddNode(NodeType::kGfxrVulkanSubmitNode, submit_string_stream.str());

    // Add submit node to the other topologies as children to the root node
    AddChild(CommandHierarchy::kAllEventTopology, Topology::kRootNodeIndex, submit_node_index);
    m_cur_submit_node_index = submit_node_index;
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandHierarchyCreator::CreateTopologies()
{
    uint64_t total_num_children[CommandHierarchy::kAllEventTopology] = {};

    // Convert the m_node_children temporary structure into CommandHierarchy's All Event topology
    size_t num_nodes = m_node_children[CommandHierarchy::kAllEventTopology].size();
    Topology& cur_topology = m_command_hierarchy.m_topology[CommandHierarchy::kAllEventTopology];
    cur_topology.SetNumNodes(num_nodes);

    if (total_num_children[0] == 0)
    {
        for (uint64_t node_index = 0; node_index < num_nodes; ++node_index)
        {
            auto& node_children = m_node_children[CommandHierarchy::kAllEventTopology];
            total_num_children[0] += node_children[node_index].size();
        }
    }

    cur_topology.m_children_list.reserve(total_num_children[0]);

    for (uint64_t node_index = 0; node_index < num_nodes; ++node_index)
    {
        cur_topology.AddChildren(node_index,
                                 m_node_children[CommandHierarchy::kAllEventTopology][node_index]);
    }
}
}  // namespace Dive
