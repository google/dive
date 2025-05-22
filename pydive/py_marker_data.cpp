/*
 Copyright 2020 Google LLC

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

#include "dive_core/marker_data.h"
#include "py_common.h"

using namespace pybind11::literals;

namespace Dive
{
void py_marker_data(py::module &m)
{

    //--------------------------------------------------------------------------------------------------
    py::class_<StringInfo>(m, "StringInfo")
    .def("get", [](const StringInfo &self, py::array_t<uint32_t> ids) -> py::array {
        auto r = ids.unchecked<1>();

        size_t n = r.shape(0);
        size_t elem_len = 0;

        for (uint32_t i = 0; i < n; ++i)
        {
            StringId id(r(i));
            if (!self.IsValidId(id))
                throw std::out_of_range("StringId out of range");
            elem_len = std::max(elem_len, strlen(self.Get(id)));
        }

        auto p = new char[n * elem_len];
        for (uint32_t i = 0; i < n; ++i)
            strncpy(p + elem_len * i, self.Get(StringId(r(i))), elem_len);
        py::dtype             dtype("S" + std::to_string(elem_len));
        std::array<size_t, 1> shape{ n };
        std::array<size_t, 1> stride{ elem_len };
        py::capsule           capsule(p, [](void *p) { delete[] (char *)p; });
        return py::array(dtype, shape, stride, p, capsule);
    });

    //--------------------------------------------------------------------------------------------------
    py::class_<MarkerData>(m, "MarkerData")
    .def("events",
         static_cast<const EventMarkerInfo &(MarkerData::*)() const>(&MarkerData::Events),
         py::return_value_policy::reference_internal)
    .def("command_buffers",
         static_cast<const CommandBufferMarkerInfo &(MarkerData::*)() const>(
         &MarkerData::CommandBuffers),
         py::return_value_policy::reference_internal)
    .def("barriers",
         static_cast<const BarrierMarkerInfo &(MarkerData::*)() const>(&MarkerData::Barriers),
         py::return_value_policy::reference_internal)
    .def("labels",
         static_cast<const LabelMarkerInfo &(MarkerData::*)() const>(&MarkerData::Labels),
         py::return_value_policy::reference_internal)
    .def("layout_transitions",
         &MarkerData::LayoutTransitions,
         py::return_value_policy::reference_internal)
    .def("pipeline_binds", &MarkerData::PipelineBinds, py::return_value_policy::reference_internal)
    .def("sub_events",
         static_cast<const SubEventInfo &(MarkerData::*)() const>(&MarkerData::SubEvents),
         py::return_value_policy::reference_internal)
    .def("strings", &MarkerData::Strings, py::return_value_policy::reference_internal)
    .def("get_label_strings",
         [](const MarkerData &self, py::array_t<LabelMarkerId::basic_type> ids) {
             auto   r = ids.unchecked<1>();
             size_t n = r.shape(0);
             if (n > 1)
             {
                 // non-standard strides are not supported
                 DIVE_ASSERT(r.data(1) - r.data(0) == 1);
             }
             size_t max_width = 0;
             std::unique_ptr<const char[]>
             p = self.GetFixedWidthFullLabelStrings(reinterpret_cast<const LabelMarkerId *>(
                                                    r.data(0)),
                                                    (uint32_t)n,
                                                    "/",
                                                    0,
                                                    &max_width);
             py::dtype             dtype("|S" + std::to_string(max_width));
             std::array<size_t, 1> shape{ n };
             std::array<size_t, 1> stride{ max_width };
             py::capsule           capsule(p.get(), [](void *p) { delete[] (char *)p; });
             return py::array(dtype, shape, stride, p.release(), capsule);
         });
}
}  // namespace Dive