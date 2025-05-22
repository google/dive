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

#include "dive_core/capture_data.h"
#include "py_common.h"

using namespace pybind11::literals;

namespace Dive
{
void py_capture_data(py::module &m)
{

    //--------------------------------------------------------------------------------------------------
    auto CaptureData_ = py::class_<CaptureData>(m, "CaptureData", R"pbdoc(
        A loaded Dive capture
    )pbdoc")
                        .def(py::init<>())
                        .def("load_file", &CaptureData::LoadFile, R"pbdoc(
                            Load a dive capture file into self
                        )pbdoc")
                        .def("get_memory_manager",
                             &CaptureData::GetMemoryManager,
                             py::return_value_policy::reference_internal)
                        .def("get_num_submits", &CaptureData::GetNumSubmits)
                        .def(
                        "get_submit_info",
                        [](const CaptureData &self, uint32_t submit_index) {
                            if (submit_index >= self.GetNumSubmits())
                                throw std::out_of_range("submit_index out of range");
                            return self.GetSubmitInfo(submit_index);
                        },
                        py::return_value_policy::reference_internal)
                        .def("get_num_presents", &CaptureData::GetNumPresents)
                        .def(
                        "get_present_info",
                        [](const CaptureData &self, uint32_t present_index) {
                            if (present_index >= self.GetNumPresents())
                                throw std::out_of_range("present_index out of range");
                            return self.GetPresentInfo(present_index);
                        },
                        py::return_value_policy::reference_internal)
                        .def("get_num_rings", &CaptureData::GetNumRings)
                        .def(
                        "get_ring_info",
                        [](const CaptureData &self, uint32_t ring_index) {
                            if (ring_index >= self.GetNumRings())
                                throw std::out_of_range("ring_index out of range");
                            return self.GetRingInfo(ring_index);
                        },
                        py::return_value_policy::reference_internal)
                        .def("get_wave_info",
                             &CaptureData::GetWaveInfo,
                             py::return_value_policy::reference_internal)
                        .def("get_register_info",
                             &CaptureData::GetRegisterInfo,
                             py::return_value_policy::reference_internal)
                        .def("get_markers",
                             static_cast<const MarkerData &(CaptureData::*)() const>(
                             &CaptureData::GetMarkers),
                             py::return_value_policy::reference_internal)
                        .def("get_num_shader_engines", &CaptureData::GetNumShaderEngines);

    //--------------------------------------------------------------------------------------------------
    py::enum_<CaptureData::LoadResult>(CaptureData_, "LoadResult")
    .value("kSuccess", CaptureData::LoadResult::kSuccess)
    .value("kFileIoError", CaptureData::LoadResult::kFileIoError)
    .value("kCorruptData", CaptureData::LoadResult::kCorruptData)
    .value("kVersionError", CaptureData::LoadResult::kVersionError);

    //--------------------------------------------------------------------------------------------------
    py::class_<SubmitInfo>(m, "SubmitInfo")
    .def("get_engine_type", &SubmitInfo::GetEngineType)
    .def("get_queue_type", &SubmitInfo::GetQueueType)
    .def("get_engine_index", &SubmitInfo::GetEngineIndex)
    .def("is_dummy_submit", &SubmitInfo::IsDummySubmit)
    .def("get_num_indirect_buffers", &SubmitInfo::GetNumIndirectBuffers)
    .def(
    "get_indirect_buffer_info",
    [](const SubmitInfo &self, uint32_t ib_index) {
        if (ib_index >= self.GetNumIndirectBuffers())
            throw std::out_of_range("ib_index out of range");
        return self.GetIndirectBufferInfo(ib_index);
    },
    py::return_value_policy::reference_internal);

    //--------------------------------------------------------------------------------------------------
    py::enum_<EngineType>(m, "EngineType")
    .value("kUniversal", EngineType::kUniversal)
    .value("kCompute", EngineType::kCompute)
    .value("kDma", EngineType::kDma)
    .value("kTimer", EngineType::kTimer)
    .value("kOther", EngineType::kOther)
    .value("kCount", EngineType::kCount);

    //--------------------------------------------------------------------------------------------------
    py::enum_<QueueType>(m, "QueueType")
    .value("kUniversal", QueueType::kUniversal)
    .value("kCompute", QueueType::kCompute)
    .value("kDma", QueueType::kDma)
    .value("kTimer", QueueType::kTimer)
    .value("kOther", QueueType::kOther)
    .value("kCount", QueueType::kCount);

    //--------------------------------------------------------------------------------------------------
    py::class_<IndirectBufferInfo>(m, "IndirectBufferInfo")
    .def_readonly("va_addr", &IndirectBufferInfo::m_va_addr)
    .def_readonly("size_in_dwords", &IndirectBufferInfo::m_size_in_dwords)
    .def_readonly("vmid", &IndirectBufferInfo::m_vmid)
    .def_readonly("is_constant_engine", &IndirectBufferInfo::m_is_constant_engine)
    .def_readonly("skip", &IndirectBufferInfo::m_skip);

    //--------------------------------------------------------------------------------------------------
    py::class_<MemoryManager>(m, "MemoryManager")
    .def("is_ring_memory", &MemoryManager::IsRingMemory);
}
}  // namespace Dive