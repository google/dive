/*
 Copyright 2020 Google LLC

 Licensed under the Apache License, Version 2.0 (the \"License\";
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an \"AS IS\" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

#include <functional>

#include "dive_core/command_hierarchy.h"
#include "dive_core/pm4_info.h"
#include "py_common.h"

using namespace pybind11::literals;

namespace Dive
{
//--------------------------------------------------------------------------------------------------
// GenArrayForNodeTypes constructs a numpy array for all nodes, in the CommandHierarchy, where the
// value of element `i` depends on the NodeType of node `i`.
//   - If the node type of `i` is in `types`, then the value of array element `i` is given by
//   calling the CommandHierachy member function `gen(i)`.
//   - If the node type of `i` is not in `types`, then the value of array element `i` is
//   `default_val`.
template<typename T, typename U>
py::array_t<T> GenArrayForNodeTypes(const CommandHierarchy&         h,
                                    std::initializer_list<NodeType> types,
                                    U (CommandHierarchy::*gen)(uint64_t) const,
                                    U default_val)
{
    py::array_t<T> result(h.size());
    auto           r = result.template mutable_unchecked<1>();
    for (uint64_t i = 0; i < h.size(); ++i)
    {
        bool match = false;
        auto t_i = h.GetNodeType(i);
        for (auto t : types)
        {
            if (t == t_i)
            {
                match = true;
                break;
            }
        }
        if (match)
            r(i) = (T)((h.*gen)(i));
        else
            r(i) = (T)default_val;
    }
    return result;
}

void py_command_hierarchy(py::module& m)
{

    //--------------------------------------------------------------------------------------------------
    py::class_<CommandHierarchy>(m, "CommandHierarchy")
    .def(py::init([](CaptureData& capture_data) {
        static bool is_pm4_info_initialized = false;
        if (!is_pm4_info_initialized)
        {
            Pm4InfoInit();
            is_pm4_info_initialized = true;
        }
        CommandHierarchy        self;
        CommandHierarchyCreator creator;
        DIVE_VERIFY(creator.CreateTrees(&self, &capture_data.GetMarkers(), capture_data, true));
        return self;
    }))
    .def("engine_hierarchy_topology",
         &CommandHierarchy::GetEngineHierarchyTopology,
         py::return_value_policy::reference_internal)
    .def("submit_hierarchy_topology",
         &CommandHierarchy::GetSubmitHierarchyTopology,
         py::return_value_policy::reference_internal)
    .def("event_hierarchy_topology",
         &CommandHierarchy::GetAllEventHierarchyTopology,
         py::return_value_policy::reference_internal)
    .def("rgp_hierarchy_topology",
         &CommandHierarchy::GetRgpHierarchyTopology,
         py::return_value_policy::reference_internal)
    .def("to_dataframe", [](const CommandHierarchy& self) {
        using namespace std::placeholders;
        auto       pd = py::module::import("pandas");
        py::object DataFrame = pd.attr("DataFrame");
        return DataFrame(
        py::
        dict("type"_a = GenArray<int>(self, &CommandHierarchy::GetNodeType),
             "desc"_a = GenStrArray(self.size(),
                                    std::bind(&CommandHierarchy::GetNodeDesc, &self, _1)),
             "submit_node_engine_type"_a = GenArrayForNodeTypes<
             uint8_t>(self,
                      { NodeType::kSubmitNode },
                      &CommandHierarchy::GetSubmitNodeEngineType,
                      EngineType::kCount),
             "submit_node_is_fully_captured"_a = GenArrayForNodeTypes<
             bool>(self,
                   { NodeType::kSubmitNode },
                   &CommandHierarchy::GetSubmitNodeIsFullyCaptured,
                   false),
             "submit_node_index"_a = GenArrayForNodeTypes<
             uint32_t>(self,
                       { NodeType::kSubmitNode },
                       &CommandHierarchy::GetSubmitNodeIndex,
                       UINT32_MAX),
             "ib_node_index"_a = GenArrayForNodeTypes<uint8_t>(self,
                                                               { NodeType::kIbNode },
                                                               &CommandHierarchy::GetIbNodeIndex,
                                                               (uint8_t)UINT8_MAX),
             "ib_node_type"_a = GenArrayForNodeTypes<uint8_t>(self,
                                                              { NodeType::kIbNode },
                                                              &CommandHierarchy::GetIbNodeType,
                                                              IbType(3)),
             "ib_node_size_in_dwords"_a = GenArrayForNodeTypes<
             uint32_t>(self,
                       { NodeType::kIbNode },
                       &CommandHierarchy::GetIbNodeSizeInDwords,
                       UINT32_MAX),
             "marker_node_type"_a = GenArrayForNodeTypes<
             uint8_t>(self,
                      { NodeType::kMarkerNode },
                      &CommandHierarchy::GetMarkerNodeType,
                      CommandHierarchy::MarkerType::kCount),
             "event_node_id"_a = GenArrayForNodeTypes<uint32_t>(self,
                                                                { NodeType::kDrawDispatchDmaNode },
                                                                &CommandHierarchy::GetEventNodeId,
                                                                UINT32_MAX),
             "event_marker_id"_a = GenArrayForNodeTypes<
             uint32_t>(self,
                       { NodeType::kDrawDispatchDmaNode },
                       &CommandHierarchy::GetEventMarkerId,
                       EventMarkerId()),
             "packet_node_addr"_a = GenArrayForNodeTypes<
             uint64_t>(self,
                       { NodeType::kPacketNode },
                       &CommandHierarchy::GetPacketNodeAddr,
                       UINT64_MAX),
             "packet_node_opcode"_a = GenArrayForNodeTypes<
             uint8_t>(self,
                      { NodeType::kPacketNode },
                      &CommandHierarchy::GetPacketNodeOpcode,
                      (uint8_t)UINT8_MAX),
             "packet_node_is_ce"_a = GenArrayForNodeTypes<
             bool>(self, { NodeType::kPacketNode }, &CommandHierarchy::GetPacketNodeIsCe, false),
             "reg_field_node_is_ce"_a = GenArrayForNodeTypes<
             bool>(self,
                   { NodeType::kRegNode, NodeType::kFieldNode },
                   &CommandHierarchy::GetRegFieldNodeIsCe,
                   false)));
    });

    //--------------------------------------------------------------------------------------------------
    py::class_<Topology>(m, "Topology")
    .def("parent",
         [](const Topology& self) {
             return GenArray<uint64_t>(self, self.GetNumNodes(), &Topology::GetParentNodeIndex);
         })
    .def("shared", [](const Topology& self) {
        uint64_t num_shared_children = 0;
        for (uint64_t node = 0; node < self.GetNumNodes(); ++node)
            num_shared_children += self.GetNumSharedChildren(node);

        py::array_t<uint64_t> parent(num_shared_children);
        auto                  r_parent = parent.template mutable_unchecked<1>();
        py::array_t<uint64_t> child(num_shared_children);
        auto                  r_child = child.template mutable_unchecked<1>();
        uint64_t              shared_index = 0;
        for (uint64_t node = 0; node < self.GetNumNodes(); ++node)
        {
            uint64_t node_shared_children = self.GetNumSharedChildren(node);
            for (uint64_t child_index = 0; child_index < node_shared_children; ++child_index)
            {
                r_parent(shared_index) = node;
                r_child(shared_index) = self.GetSharedChildNodeIndex(node, child_index);
                ++shared_index;
            }
        }
        DIVE_ASSERT(shared_index == num_shared_children);

        auto       pd = py::module::import("pandas");
        py::object DataFrame = pd.attr("DataFrame");
        return DataFrame(py::dict("parent"_a = parent, "child"_a = child));
    });
}
}  // namespace Dive