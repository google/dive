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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING! WARNING!
//
// This code has been generated automatically by generateSOAs.py. Do not hand-modify this code.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "dive_core/marker_types.h"

#include "dive_core/vulkan_call_info.h"

#include "py_common.h"

using namespace pybind11::literals;

namespace Dive
{
void py_marker_types(py::module &m)
{
    py::class_<EventMarkerInfo>(m, "EventMarkerInfo")
    .def("api_type",
         [](const EventMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint8_t>::type>({ self.size() },
                                        { sizeof(numpy_type<uint8_t>::type) },
                                        reinterpret_cast<const numpy_type<uint8_t>::type *>(
                                        self.ApiTypePtr()));
         })
    .def("has_thread_dims",
         [](const EventMarkerInfo &self) {
             return py::array_t<
             numpy_type<bool>::type>({ self.size() },
                                     { sizeof(numpy_type<bool>::type) },
                                     reinterpret_cast<const numpy_type<bool>::type *>(
                                     self.HasThreadDimsPtr()));
         })
    .def("orig_command_buffer",
         [](const EventMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.OrigCommandBufferPtr()));
         })
    .def("vertex_offset_reg_idx",
         [](const EventMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint8_t>::type>({ self.size() },
                                        { sizeof(numpy_type<uint8_t>::type) },
                                        reinterpret_cast<const numpy_type<uint8_t>::type *>(
                                        self.VertexOffsetRegIdxPtr()));
         })
    .def("instance_offset_reg_idx",
         [](const EventMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint8_t>::type>({ self.size() },
                                        { sizeof(numpy_type<uint8_t>::type) },
                                        reinterpret_cast<const numpy_type<uint8_t>::type *>(
                                        self.InstanceOffsetRegIdxPtr()));
         })
    .def("draw_index_reg_idx",
         [](const EventMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint8_t>::type>({ self.size() },
                                        { sizeof(numpy_type<uint8_t>::type) },
                                        reinterpret_cast<const numpy_type<uint8_t>::type *>(
                                        self.DrawIndexRegIdxPtr()));
         })
    .def("command_index",
         [](const EventMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.CommandIndexPtr()));
         })
    .def("thread_x",
         [](const EventMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.ThreadXPtr()));
         })
    .def("thread_y",
         [](const EventMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.ThreadYPtr()));
         })
    .def("thread_z",
         [](const EventMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.ThreadZPtr()));
         })
    .def("command_buffer",
         [](const EventMarkerInfo &self) {
             return py::array_t<numpy_type<
             CommandBufferMarkerId>::type>({ self.size() },
                                           { sizeof(numpy_type<CommandBufferMarkerId>::type) },
                                           reinterpret_cast<
                                           const numpy_type<CommandBufferMarkerId>::type *>(
                                           self.CommandBufferPtr()));
         })
    .def("gfx_pipe_line",
         [](const EventMarkerInfo &self) {
             return py::array_t<numpy_type<
             PipelineBindMarkerId>::type>({ self.size() },
                                          { sizeof(numpy_type<PipelineBindMarkerId>::type) },
                                          reinterpret_cast<
                                          const numpy_type<PipelineBindMarkerId>::type *>(
                                          self.GfxPipeLinePtr()));
         })
    .def("cs_pipe_line",
         [](const EventMarkerInfo &self) {
             return py::array_t<numpy_type<
             PipelineBindMarkerId>::type>({ self.size() },
                                          { sizeof(numpy_type<PipelineBindMarkerId>::type) },
                                          reinterpret_cast<
                                          const numpy_type<PipelineBindMarkerId>::type *>(
                                          self.CsPipeLinePtr()));
         })
    .def("bound_pipe_line",
         [](const EventMarkerInfo &self) {
             return py::array_t<numpy_type<
             PipelineBindMarkerId>::type>({ self.size() },
                                          { sizeof(numpy_type<PipelineBindMarkerId>::type) },
                                          reinterpret_cast<
                                          const numpy_type<PipelineBindMarkerId>::type *>(
                                          self.BoundPipeLinePtr()));
         })
    .def("index",
         [](const EventMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.IndexPtr()));
         })
    .def("label",
         [](const EventMarkerInfo &self) {
             return py::array_t<numpy_type<
             LabelMarkerId>::type>({ self.size() },
                                   { sizeof(numpy_type<LabelMarkerId>::type) },
                                   reinterpret_cast<const numpy_type<LabelMarkerId>::type *>(
                                   self.LabelPtr()));
         })
    .def("vulkan_call",
         [](const EventMarkerInfo &self) {
             return py::array_t<numpy_type<
             VulkanCallId>::type>({ self.size() },
                                  { sizeof(numpy_type<VulkanCallId>::type) },
                                  reinterpret_cast<const numpy_type<VulkanCallId>::type *>(
                                  self.VulkanCallPtr()));
         })
    .def("start_cycle",
         [](const EventMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint64_t>::type>({ self.size(), kNumSe },
                                         { sizeof(numpy_type<uint64_t>::type) * kNumSe,
                                           sizeof(numpy_type<uint64_t>::type) * kNumSe },
                                         reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                         self.StartCyclePtr()));
         })
    .def("end_cycle",
         [](const EventMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint64_t>::type>({ self.size(), kNumSe },
                                         { sizeof(numpy_type<uint64_t>::type) * kNumSe,
                                           sizeof(numpy_type<uint64_t>::type) * kNumSe },
                                         reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                         self.EndCyclePtr()));
         })
    .def("stage_start_cycle",
         [](const EventMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint64_t>::type>({ self.size(), kNumSe, kShaderStageCount },
                                         { sizeof(numpy_type<uint64_t>::type) * kNumSe *
                                           kShaderStageCount,
                                           sizeof(numpy_type<uint64_t>::type) * kNumSe *
                                           kShaderStageCount,
                                           sizeof(numpy_type<uint64_t>::type) * kShaderStageCount },
                                         reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                         self.StageStartCyclePtr()));
         })
    .def("stage_end_cycle",
         [](const EventMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint64_t>::type>({ self.size(), kNumSe, kShaderStageCount },
                                         { sizeof(numpy_type<uint64_t>::type) * kNumSe *
                                           kShaderStageCount,
                                           sizeof(numpy_type<uint64_t>::type) * kNumSe *
                                           kShaderStageCount,
                                           sizeof(numpy_type<uint64_t>::type) * kShaderStageCount },
                                         reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                         self.StageEndCyclePtr()));
         })
    .def("packet_node",
         [](const EventMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint64_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint64_t>::type) },
                                         reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                         self.PacketNodePtr()));
         })
    .def("event_node",
         [](const EventMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint64_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint64_t>::type) },
                                         reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                         self.EventNodePtr()));
         })
    .def("index_in_stream",
         [](const EventMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.IndexInStreamPtr()));
         })
    .def("barrier",
         [](const EventMarkerInfo &self) {
             return py::array_t<numpy_type<
             BarrierMarkerId>::type>({ self.size() },
                                     { sizeof(numpy_type<BarrierMarkerId>::type) },
                                     reinterpret_cast<const numpy_type<BarrierMarkerId>::type *>(
                                     self.BarrierPtr()));
         })
    .def("layout_transition",
         [](const EventMarkerInfo &self) {
             return py::array_t<numpy_type<
             LayoutTransitionMarkerId>::type>({ self.size() },
                                              { sizeof(
                                              numpy_type<LayoutTransitionMarkerId>::type) },
                                              reinterpret_cast<
                                              const numpy_type<LayoutTransitionMarkerId>::type *>(
                                              self.LayoutTransitionPtr()));
         })

    .def("to_dataframe", [](const EventMarkerInfo &self) {
        py::object DataFrame = py::module::import("pandas").attr("DataFrame");
        py::dict   dict;
        dict["api_type"] = py::array_t<
        numpy_type<uint8_t>::type>({ self.size() },
                                   { sizeof(numpy_type<uint8_t>::type) },
                                   reinterpret_cast<const numpy_type<uint8_t>::type *>(
                                   self.ApiTypePtr()));
        dict["has_thread_dims"] = py::array_t<
        numpy_type<bool>::type>({ self.size() },
                                { sizeof(numpy_type<bool>::type) },
                                reinterpret_cast<const numpy_type<bool>::type *>(
                                self.HasThreadDimsPtr()));
        dict["orig_command_buffer"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.OrigCommandBufferPtr()));
        dict["vertex_offset_reg_idx"] = py::array_t<
        numpy_type<uint8_t>::type>({ self.size() },
                                   { sizeof(numpy_type<uint8_t>::type) },
                                   reinterpret_cast<const numpy_type<uint8_t>::type *>(
                                   self.VertexOffsetRegIdxPtr()));
        dict["instance_offset_reg_idx"] = py::array_t<
        numpy_type<uint8_t>::type>({ self.size() },
                                   { sizeof(numpy_type<uint8_t>::type) },
                                   reinterpret_cast<const numpy_type<uint8_t>::type *>(
                                   self.InstanceOffsetRegIdxPtr()));
        dict["draw_index_reg_idx"] = py::array_t<
        numpy_type<uint8_t>::type>({ self.size() },
                                   { sizeof(numpy_type<uint8_t>::type) },
                                   reinterpret_cast<const numpy_type<uint8_t>::type *>(
                                   self.DrawIndexRegIdxPtr()));
        dict["command_index"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.CommandIndexPtr()));
        dict["thread_x"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.ThreadXPtr()));
        dict["thread_y"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.ThreadYPtr()));
        dict["thread_z"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.ThreadZPtr()));
        dict["command_buffer"] = py::array_t<
        numpy_type<CommandBufferMarkerId>::type>({ self.size() },
                                                 { sizeof(
                                                 numpy_type<CommandBufferMarkerId>::type) },
                                                 reinterpret_cast<
                                                 const numpy_type<CommandBufferMarkerId>::type *>(
                                                 self.CommandBufferPtr()));
        dict["gfx_pipe_line"] = py::array_t<
        numpy_type<PipelineBindMarkerId>::type>({ self.size() },
                                                { sizeof(numpy_type<PipelineBindMarkerId>::type) },
                                                reinterpret_cast<
                                                const numpy_type<PipelineBindMarkerId>::type *>(
                                                self.GfxPipeLinePtr()));
        dict["cs_pipe_line"] = py::array_t<
        numpy_type<PipelineBindMarkerId>::type>({ self.size() },
                                                { sizeof(numpy_type<PipelineBindMarkerId>::type) },
                                                reinterpret_cast<
                                                const numpy_type<PipelineBindMarkerId>::type *>(
                                                self.CsPipeLinePtr()));
        dict["bound_pipe_line"] = py::array_t<
        numpy_type<PipelineBindMarkerId>::type>({ self.size() },
                                                { sizeof(numpy_type<PipelineBindMarkerId>::type) },
                                                reinterpret_cast<
                                                const numpy_type<PipelineBindMarkerId>::type *>(
                                                self.BoundPipeLinePtr()));
        dict["index"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.IndexPtr()));
        dict["label"] = py::array_t<
        numpy_type<LabelMarkerId>::type>({ self.size() },
                                         { sizeof(numpy_type<LabelMarkerId>::type) },
                                         reinterpret_cast<const numpy_type<LabelMarkerId>::type *>(
                                         self.LabelPtr()));
        dict["vulkan_call"] = py::array_t<
        numpy_type<VulkanCallId>::type>({ self.size() },
                                        { sizeof(numpy_type<VulkanCallId>::type) },
                                        reinterpret_cast<const numpy_type<VulkanCallId>::type *>(
                                        self.VulkanCallPtr()));
        for (uint32_t se = 0; se < kNumSe; ++se)
        {
            std::ostringstream key;
            key << "start_cycle"
                << "_" << se;
            dict[py::cast(key.str())] = py::array_t<
            numpy_type<uint64_t>::type>({ self.size() },
                                        { sizeof(numpy_type<uint64_t>::type) * kNumSe },
                                        reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                        self.StartCyclePtr(EventMarkerId(0), se)));
        }
        for (uint32_t se = 0; se < kNumSe; ++se)
        {
            std::ostringstream key;
            key << "end_cycle"
                << "_" << se;
            dict[py::cast(key.str())] = py::array_t<
            numpy_type<uint64_t>::type>({ self.size() },
                                        { sizeof(numpy_type<uint64_t>::type) * kNumSe },
                                        reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                        self.EndCyclePtr(EventMarkerId(0), se)));
        }
        for (uint32_t se = 0; se < kNumSe; ++se)
            for (uint32_t stage = 0; stage < kShaderStageCount; ++stage)
            {
                std::ostringstream key;
                key << "stage_start_cycle"
                    << "_" << se << "_" << stage;
                dict[py::cast(key.str())] = py::array_t<
                numpy_type<uint64_t>::type>({ self.size() },
                                            { sizeof(numpy_type<uint64_t>::type) * kNumSe *
                                              kShaderStageCount },
                                            reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                            self.StageStartCyclePtr(EventMarkerId(0),
                                                                    se,
                                                                    static_cast<ShaderStage>(
                                                                    stage))));
            }
        for (uint32_t se = 0; se < kNumSe; ++se)
            for (uint32_t stage = 0; stage < kShaderStageCount; ++stage)
            {
                std::ostringstream key;
                key << "stage_end_cycle"
                    << "_" << se << "_" << stage;
                dict[py::cast(key.str())] = py::array_t<
                numpy_type<uint64_t>::type>({ self.size() },
                                            { sizeof(numpy_type<uint64_t>::type) * kNumSe *
                                              kShaderStageCount },
                                            reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                            self.StageEndCyclePtr(EventMarkerId(0),
                                                                  se,
                                                                  static_cast<ShaderStage>(
                                                                  stage))));
            }
        dict["packet_node"] = py::array_t<
        numpy_type<uint64_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint64_t>::type) },
                                    reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                    self.PacketNodePtr()));
        dict["event_node"] = py::array_t<
        numpy_type<uint64_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint64_t>::type) },
                                    reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                    self.EventNodePtr()));
        dict["index_in_stream"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.IndexInStreamPtr()));
        dict["barrier"] = py::array_t<numpy_type<
        BarrierMarkerId>::type>({ self.size() },
                                { sizeof(numpy_type<BarrierMarkerId>::type) },
                                reinterpret_cast<const numpy_type<BarrierMarkerId>::type *>(
                                self.BarrierPtr()));
        dict["layout_transition"] = py::array_t<numpy_type<
        LayoutTransitionMarkerId>::type>({ self.size() },
                                         { sizeof(numpy_type<LayoutTransitionMarkerId>::type) },
                                         reinterpret_cast<
                                         const numpy_type<LayoutTransitionMarkerId>::type *>(
                                         self.LayoutTransitionPtr()));

        return DataFrame(dict);
    });
    py::class_<CommandBufferMarkerInfo>(m, "CommandBufferMarkerInfo")
    .def("orig_command_buffer_id",
         [](const CommandBufferMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.OrigCommandBufferIdPtr()));
         })
    .def("queue_family_index",
         [](const CommandBufferMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint8_t>::type>({ self.size() },
                                        { sizeof(numpy_type<uint8_t>::type) },
                                        reinterpret_cast<const numpy_type<uint8_t>::type *>(
                                        self.QueueFamilyIndexPtr()));
         })
    .def("queue_index",
         [](const CommandBufferMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint8_t>::type>({ self.size() },
                                        { sizeof(numpy_type<uint8_t>::type) },
                                        reinterpret_cast<const numpy_type<uint8_t>::type *>(
                                        self.QueueIndexPtr()));
         })
    .def("device",
         [](const CommandBufferMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint64_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint64_t>::type) },
                                         reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                         self.DevicePtr()));
         })
    .def("queue_flags",
         [](const CommandBufferMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.QueueFlagsPtr()));
         })
    .def("first_event",
         [](const CommandBufferMarkerInfo &self) {
             return py::array_t<numpy_type<
             EventMarkerId>::type>({ self.size() },
                                   { sizeof(numpy_type<EventMarkerId>::type) },
                                   reinterpret_cast<const numpy_type<EventMarkerId>::type *>(
                                   self.FirstEventPtr()));
         })
    .def("event_count",
         [](const CommandBufferMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.EventCountPtr()));
         })
    .def("first_barrier",
         [](const CommandBufferMarkerInfo &self) {
             return py::array_t<numpy_type<
             BarrierMarkerId>::type>({ self.size() },
                                     { sizeof(numpy_type<BarrierMarkerId>::type) },
                                     reinterpret_cast<const numpy_type<BarrierMarkerId>::type *>(
                                     self.FirstBarrierPtr()));
         })
    .def("barrier_count",
         [](const CommandBufferMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.BarrierCountPtr()));
         })
    .def("start_label",
         [](const CommandBufferMarkerInfo &self) {
             return py::array_t<numpy_type<
             LabelMarkerId>::type>({ self.size() },
                                   { sizeof(numpy_type<LabelMarkerId>::type) },
                                   reinterpret_cast<const numpy_type<LabelMarkerId>::type *>(
                                   self.StartLabelPtr()));
         })
    .def("end_label",
         [](const CommandBufferMarkerInfo &self) {
             return py::array_t<numpy_type<
             LabelMarkerId>::type>({ self.size() },
                                   { sizeof(numpy_type<LabelMarkerId>::type) },
                                   reinterpret_cast<const numpy_type<LabelMarkerId>::type *>(
                                   self.EndLabelPtr()));
         })
    .def("first_label",
         [](const CommandBufferMarkerInfo &self) {
             return py::array_t<numpy_type<
             LabelMarkerId>::type>({ self.size() },
                                   { sizeof(numpy_type<LabelMarkerId>::type) },
                                   reinterpret_cast<const numpy_type<LabelMarkerId>::type *>(
                                   self.FirstLabelPtr()));
         })
    .def("label_count",
         [](const CommandBufferMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.LabelCountPtr()));
         })
    .def("parent",
         [](const CommandBufferMarkerInfo &self) {
             return py::array_t<numpy_type<
             CommandBufferMarkerId>::type>({ self.size() },
                                           { sizeof(numpy_type<CommandBufferMarkerId>::type) },
                                           reinterpret_cast<
                                           const numpy_type<CommandBufferMarkerId>::type *>(
                                           self.ParentPtr()));
         })
    .def("first_pipeline_bind",
         [](const CommandBufferMarkerInfo &self) {
             return py::array_t<numpy_type<
             PipelineBindMarkerId>::type>({ self.size() },
                                          { sizeof(numpy_type<PipelineBindMarkerId>::type) },
                                          reinterpret_cast<
                                          const numpy_type<PipelineBindMarkerId>::type *>(
                                          self.FirstPipelineBindPtr()));
         })
    .def("pipeline_bind_count",
         [](const CommandBufferMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.PipelineBindCountPtr()));
         })
    .def("first_vulkan_call",
         [](const CommandBufferMarkerInfo &self) {
             return py::array_t<numpy_type<
             VulkanCallId>::type>({ self.size() },
                                  { sizeof(numpy_type<VulkanCallId>::type) },
                                  reinterpret_cast<const numpy_type<VulkanCallId>::type *>(
                                  self.FirstVulkanCallPtr()));
         })
    .def("vulkan_call_count",
         [](const CommandBufferMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.VulkanCallCountPtr()));
         })
    .def("start_cycle",
         [](const CommandBufferMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint64_t>::type>({ self.size(), kNumSe },
                                         { sizeof(numpy_type<uint64_t>::type) * kNumSe,
                                           sizeof(numpy_type<uint64_t>::type) * kNumSe },
                                         reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                         self.StartCyclePtr()));
         })
    .def("end_cycle",
         [](const CommandBufferMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint64_t>::type>({ self.size(), kNumSe },
                                         { sizeof(numpy_type<uint64_t>::type) * kNumSe,
                                           sizeof(numpy_type<uint64_t>::type) * kNumSe },
                                         reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                         self.EndCyclePtr()));
         })

    .def("to_dataframe", [](const CommandBufferMarkerInfo &self) {
        py::object DataFrame = py::module::import("pandas").attr("DataFrame");
        py::dict   dict;
        dict["orig_command_buffer_id"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.OrigCommandBufferIdPtr()));
        dict["queue_family_index"] = py::array_t<
        numpy_type<uint8_t>::type>({ self.size() },
                                   { sizeof(numpy_type<uint8_t>::type) },
                                   reinterpret_cast<const numpy_type<uint8_t>::type *>(
                                   self.QueueFamilyIndexPtr()));
        dict["queue_index"] = py::array_t<
        numpy_type<uint8_t>::type>({ self.size() },
                                   { sizeof(numpy_type<uint8_t>::type) },
                                   reinterpret_cast<const numpy_type<uint8_t>::type *>(
                                   self.QueueIndexPtr()));
        dict["device"] = py::array_t<
        numpy_type<uint64_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint64_t>::type) },
                                    reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                    self.DevicePtr()));
        dict["queue_flags"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.QueueFlagsPtr()));
        dict["first_event"] = py::array_t<
        numpy_type<EventMarkerId>::type>({ self.size() },
                                         { sizeof(numpy_type<EventMarkerId>::type) },
                                         reinterpret_cast<const numpy_type<EventMarkerId>::type *>(
                                         self.FirstEventPtr()));
        dict["event_count"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.EventCountPtr()));
        dict["first_barrier"] = py::array_t<numpy_type<
        BarrierMarkerId>::type>({ self.size() },
                                { sizeof(numpy_type<BarrierMarkerId>::type) },
                                reinterpret_cast<const numpy_type<BarrierMarkerId>::type *>(
                                self.FirstBarrierPtr()));
        dict["barrier_count"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.BarrierCountPtr()));
        dict["start_label"] = py::array_t<
        numpy_type<LabelMarkerId>::type>({ self.size() },
                                         { sizeof(numpy_type<LabelMarkerId>::type) },
                                         reinterpret_cast<const numpy_type<LabelMarkerId>::type *>(
                                         self.StartLabelPtr()));
        dict["end_label"] = py::array_t<
        numpy_type<LabelMarkerId>::type>({ self.size() },
                                         { sizeof(numpy_type<LabelMarkerId>::type) },
                                         reinterpret_cast<const numpy_type<LabelMarkerId>::type *>(
                                         self.EndLabelPtr()));
        dict["first_label"] = py::array_t<
        numpy_type<LabelMarkerId>::type>({ self.size() },
                                         { sizeof(numpy_type<LabelMarkerId>::type) },
                                         reinterpret_cast<const numpy_type<LabelMarkerId>::type *>(
                                         self.FirstLabelPtr()));
        dict["label_count"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.LabelCountPtr()));
        dict["parent"] = py::array_t<
        numpy_type<CommandBufferMarkerId>::type>({ self.size() },
                                                 { sizeof(
                                                 numpy_type<CommandBufferMarkerId>::type) },
                                                 reinterpret_cast<
                                                 const numpy_type<CommandBufferMarkerId>::type *>(
                                                 self.ParentPtr()));
        dict["first_pipeline_bind"] = py::array_t<
        numpy_type<PipelineBindMarkerId>::type>({ self.size() },
                                                { sizeof(numpy_type<PipelineBindMarkerId>::type) },
                                                reinterpret_cast<
                                                const numpy_type<PipelineBindMarkerId>::type *>(
                                                self.FirstPipelineBindPtr()));
        dict["pipeline_bind_count"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.PipelineBindCountPtr()));
        dict["first_vulkan_call"] = py::array_t<
        numpy_type<VulkanCallId>::type>({ self.size() },
                                        { sizeof(numpy_type<VulkanCallId>::type) },
                                        reinterpret_cast<const numpy_type<VulkanCallId>::type *>(
                                        self.FirstVulkanCallPtr()));
        dict["vulkan_call_count"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.VulkanCallCountPtr()));
        for (uint32_t se = 0; se < kNumSe; ++se)
        {
            std::ostringstream key;
            key << "start_cycle"
                << "_" << se;
            dict[py::cast(key.str())] = py::array_t<
            numpy_type<uint64_t>::type>({ self.size() },
                                        { sizeof(numpy_type<uint64_t>::type) * kNumSe },
                                        reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                        self.StartCyclePtr(CommandBufferMarkerId(0), se)));
        }
        for (uint32_t se = 0; se < kNumSe; ++se)
        {
            std::ostringstream key;
            key << "end_cycle"
                << "_" << se;
            dict[py::cast(key.str())] = py::array_t<
            numpy_type<uint64_t>::type>({ self.size() },
                                        { sizeof(numpy_type<uint64_t>::type) * kNumSe },
                                        reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                        self.EndCyclePtr(CommandBufferMarkerId(0), se)));
        }

        return DataFrame(dict);
    });
    py::class_<BarrierMarkerInfo>(m, "BarrierMarkerInfo")
    .def("orig_command_buffer",
         [](const BarrierMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.OrigCommandBufferPtr()));
         })
    .def("driver_reason",
         [](const BarrierMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.DriverReasonPtr()));
         })
    .def("flags",
         [](const BarrierMarkerInfo &self) {
             return py::array_t<
             numpy_type<BarrierMarkerFlags>::type>({ self.size() },
                                                   { sizeof(numpy_type<BarrierMarkerFlags>::type) },
                                                   reinterpret_cast<
                                                   const numpy_type<BarrierMarkerFlags>::type *>(
                                                   self.FlagsPtr()));
         })
    .def("command_buffer",
         [](const BarrierMarkerInfo &self) {
             return py::array_t<numpy_type<
             CommandBufferMarkerId>::type>({ self.size() },
                                           { sizeof(numpy_type<CommandBufferMarkerId>::type) },
                                           reinterpret_cast<
                                           const numpy_type<CommandBufferMarkerId>::type *>(
                                           self.CommandBufferPtr()));
         })
    .def("start_index",
         [](const BarrierMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.StartIndexPtr()));
         })
    .def("end_index",
         [](const BarrierMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.EndIndexPtr()));
         })
    .def("label",
         [](const BarrierMarkerInfo &self) {
             return py::array_t<numpy_type<
             LabelMarkerId>::type>({ self.size() },
                                   { sizeof(numpy_type<LabelMarkerId>::type) },
                                   reinterpret_cast<const numpy_type<LabelMarkerId>::type *>(
                                   self.LabelPtr()));
         })
    .def("first_layout_transition",
         [](const BarrierMarkerInfo &self) {
             return py::array_t<numpy_type<
             LayoutTransitionMarkerId>::type>({ self.size() },
                                              { sizeof(
                                              numpy_type<LayoutTransitionMarkerId>::type) },
                                              reinterpret_cast<
                                              const numpy_type<LayoutTransitionMarkerId>::type *>(
                                              self.FirstLayoutTransitionPtr()));
         })
    .def("layout_transition_count",
         [](const BarrierMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.LayoutTransitionCountPtr()));
         })
    .def("first_event",
         [](const BarrierMarkerInfo &self) {
             return py::array_t<numpy_type<
             EventMarkerId>::type>({ self.size() },
                                   { sizeof(numpy_type<EventMarkerId>::type) },
                                   reinterpret_cast<const numpy_type<EventMarkerId>::type *>(
                                   self.FirstEventPtr()));
         })
    .def("event_count",
         [](const BarrierMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.EventCountPtr()));
         })
    .def("start_index_in_stream",
         [](const BarrierMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.StartIndexInStreamPtr()));
         })
    .def("end_index_in_stream",
         [](const BarrierMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.EndIndexInStreamPtr()));
         })
    .def("barrier_node",
         [](const BarrierMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint64_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint64_t>::type) },
                                         reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                         self.BarrierNodePtr()));
         })
    .def("vulkan_call",
         [](const BarrierMarkerInfo &self) {
             return py::array_t<numpy_type<
             VulkanCallId>::type>({ self.size() },
                                  { sizeof(numpy_type<VulkanCallId>::type) },
                                  reinterpret_cast<const numpy_type<VulkanCallId>::type *>(
                                  self.VulkanCallPtr()));
         })
    .def("start_cycle",
         [](const BarrierMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint64_t>::type>({ self.size(), kNumSe },
                                         { sizeof(numpy_type<uint64_t>::type) * kNumSe,
                                           sizeof(numpy_type<uint64_t>::type) * kNumSe },
                                         reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                         self.StartCyclePtr()));
         })
    .def("end_cycle",
         [](const BarrierMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint64_t>::type>({ self.size(), kNumSe },
                                         { sizeof(numpy_type<uint64_t>::type) * kNumSe,
                                           sizeof(numpy_type<uint64_t>::type) * kNumSe },
                                         reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                         self.EndCyclePtr()));
         })

    .def("to_dataframe", [](const BarrierMarkerInfo &self) {
        py::object DataFrame = py::module::import("pandas").attr("DataFrame");
        py::dict   dict;
        dict["orig_command_buffer"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.OrigCommandBufferPtr()));
        dict["driver_reason"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.DriverReasonPtr()));
        dict["flags"] = py::array_t<numpy_type<
        BarrierMarkerFlags>::type>({ self.size() },
                                   { sizeof(numpy_type<BarrierMarkerFlags>::type) },
                                   reinterpret_cast<const numpy_type<BarrierMarkerFlags>::type *>(
                                   self.FlagsPtr()));
        dict["command_buffer"] = py::array_t<
        numpy_type<CommandBufferMarkerId>::type>({ self.size() },
                                                 { sizeof(
                                                 numpy_type<CommandBufferMarkerId>::type) },
                                                 reinterpret_cast<
                                                 const numpy_type<CommandBufferMarkerId>::type *>(
                                                 self.CommandBufferPtr()));
        dict["start_index"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.StartIndexPtr()));
        dict["end_index"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.EndIndexPtr()));
        dict["label"] = py::array_t<
        numpy_type<LabelMarkerId>::type>({ self.size() },
                                         { sizeof(numpy_type<LabelMarkerId>::type) },
                                         reinterpret_cast<const numpy_type<LabelMarkerId>::type *>(
                                         self.LabelPtr()));
        dict["first_layout_transition"] = py::array_t<numpy_type<
        LayoutTransitionMarkerId>::type>({ self.size() },
                                         { sizeof(numpy_type<LayoutTransitionMarkerId>::type) },
                                         reinterpret_cast<
                                         const numpy_type<LayoutTransitionMarkerId>::type *>(
                                         self.FirstLayoutTransitionPtr()));
        dict["layout_transition_count"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.LayoutTransitionCountPtr()));
        dict["first_event"] = py::array_t<
        numpy_type<EventMarkerId>::type>({ self.size() },
                                         { sizeof(numpy_type<EventMarkerId>::type) },
                                         reinterpret_cast<const numpy_type<EventMarkerId>::type *>(
                                         self.FirstEventPtr()));
        dict["event_count"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.EventCountPtr()));
        dict["start_index_in_stream"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.StartIndexInStreamPtr()));
        dict["end_index_in_stream"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.EndIndexInStreamPtr()));
        dict["barrier_node"] = py::array_t<
        numpy_type<uint64_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint64_t>::type) },
                                    reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                    self.BarrierNodePtr()));
        dict["vulkan_call"] = py::array_t<
        numpy_type<VulkanCallId>::type>({ self.size() },
                                        { sizeof(numpy_type<VulkanCallId>::type) },
                                        reinterpret_cast<const numpy_type<VulkanCallId>::type *>(
                                        self.VulkanCallPtr()));
        for (uint32_t se = 0; se < kNumSe; ++se)
        {
            std::ostringstream key;
            key << "start_cycle"
                << "_" << se;
            dict[py::cast(key.str())] = py::array_t<
            numpy_type<uint64_t>::type>({ self.size() },
                                        { sizeof(numpy_type<uint64_t>::type) * kNumSe },
                                        reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                        self.StartCyclePtr(BarrierMarkerId(0), se)));
        }
        for (uint32_t se = 0; se < kNumSe; ++se)
        {
            std::ostringstream key;
            key << "end_cycle"
                << "_" << se;
            dict[py::cast(key.str())] = py::array_t<
            numpy_type<uint64_t>::type>({ self.size() },
                                        { sizeof(numpy_type<uint64_t>::type) * kNumSe },
                                        reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                        self.EndCyclePtr(BarrierMarkerId(0), se)));
        }

        return DataFrame(dict);
    });
    py::class_<LabelMarkerInfo>(m, "LabelMarkerInfo")
    .def("string",
         [](const LabelMarkerInfo &self) {
             return py::array_t<
             numpy_type<StringId>::type>({ self.size() },
                                         { sizeof(numpy_type<StringId>::type) },
                                         reinterpret_cast<const numpy_type<StringId>::type *>(
                                         self.StringPtr()));
         })
    .def("parent",
         [](const LabelMarkerInfo &self) {
             return py::array_t<numpy_type<
             LabelMarkerId>::type>({ self.size() },
                                   { sizeof(numpy_type<LabelMarkerId>::type) },
                                   reinterpret_cast<const numpy_type<LabelMarkerId>::type *>(
                                   self.ParentPtr()));
         })
    .def("start_command_buffer",
         [](const LabelMarkerInfo &self) {
             return py::array_t<numpy_type<
             CommandBufferMarkerId>::type>({ self.size() },
                                           { sizeof(numpy_type<CommandBufferMarkerId>::type) },
                                           reinterpret_cast<
                                           const numpy_type<CommandBufferMarkerId>::type *>(
                                           self.StartCommandBufferPtr()));
         })
    .def("end_command_buffer",
         [](const LabelMarkerInfo &self) {
             return py::array_t<numpy_type<
             CommandBufferMarkerId>::type>({ self.size() },
                                           { sizeof(numpy_type<CommandBufferMarkerId>::type) },
                                           reinterpret_cast<
                                           const numpy_type<CommandBufferMarkerId>::type *>(
                                           self.EndCommandBufferPtr()));
         })
    .def("start_index",
         [](const LabelMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.StartIndexPtr()));
         })
    .def("end_index",
         [](const LabelMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.EndIndexPtr()));
         })
    .def("first_event",
         [](const LabelMarkerInfo &self) {
             return py::array_t<numpy_type<
             EventMarkerId>::type>({ self.size() },
                                   { sizeof(numpy_type<EventMarkerId>::type) },
                                   reinterpret_cast<const numpy_type<EventMarkerId>::type *>(
                                   self.FirstEventPtr()));
         })
    .def("event_count",
         [](const LabelMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.EventCountPtr()));
         })
    .def("first_barrier",
         [](const LabelMarkerInfo &self) {
             return py::array_t<numpy_type<
             BarrierMarkerId>::type>({ self.size() },
                                     { sizeof(numpy_type<BarrierMarkerId>::type) },
                                     reinterpret_cast<const numpy_type<BarrierMarkerId>::type *>(
                                     self.FirstBarrierPtr()));
         })
    .def("barrier_count",
         [](const LabelMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.BarrierCountPtr()));
         })
    .def("end_cycle_events",
         [](const LabelMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint64_t>::type>({ self.size(), kNumSe },
                                         { sizeof(numpy_type<uint64_t>::type) * kNumSe,
                                           sizeof(numpy_type<uint64_t>::type) * kNumSe },
                                         reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                         self.EndCycleEventsPtr()));
         })
    .def("label_node",
         [](const LabelMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint64_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint64_t>::type) },
                                         reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                         self.LabelNodePtr()));
         })
    .def("start_cycle",
         [](const LabelMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint64_t>::type>({ self.size(), kNumSe },
                                         { sizeof(numpy_type<uint64_t>::type) * kNumSe,
                                           sizeof(numpy_type<uint64_t>::type) * kNumSe },
                                         reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                         self.StartCyclePtr()));
         })
    .def("end_cycle",
         [](const LabelMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint64_t>::type>({ self.size(), kNumSe },
                                         { sizeof(numpy_type<uint64_t>::type) * kNumSe,
                                           sizeof(numpy_type<uint64_t>::type) * kNumSe },
                                         reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                         self.EndCyclePtr()));
         })

    .def("to_dataframe", [](const LabelMarkerInfo &self) {
        py::object DataFrame = py::module::import("pandas").attr("DataFrame");
        py::dict   dict;
        dict["string"] = py::array_t<
        numpy_type<StringId>::type>({ self.size() },
                                    { sizeof(numpy_type<StringId>::type) },
                                    reinterpret_cast<const numpy_type<StringId>::type *>(
                                    self.StringPtr()));
        dict["parent"] = py::array_t<
        numpy_type<LabelMarkerId>::type>({ self.size() },
                                         { sizeof(numpy_type<LabelMarkerId>::type) },
                                         reinterpret_cast<const numpy_type<LabelMarkerId>::type *>(
                                         self.ParentPtr()));
        dict["start_command_buffer"] = py::array_t<
        numpy_type<CommandBufferMarkerId>::type>({ self.size() },
                                                 { sizeof(
                                                 numpy_type<CommandBufferMarkerId>::type) },
                                                 reinterpret_cast<
                                                 const numpy_type<CommandBufferMarkerId>::type *>(
                                                 self.StartCommandBufferPtr()));
        dict["end_command_buffer"] = py::array_t<
        numpy_type<CommandBufferMarkerId>::type>({ self.size() },
                                                 { sizeof(
                                                 numpy_type<CommandBufferMarkerId>::type) },
                                                 reinterpret_cast<
                                                 const numpy_type<CommandBufferMarkerId>::type *>(
                                                 self.EndCommandBufferPtr()));
        dict["start_index"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.StartIndexPtr()));
        dict["end_index"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.EndIndexPtr()));
        dict["first_event"] = py::array_t<
        numpy_type<EventMarkerId>::type>({ self.size() },
                                         { sizeof(numpy_type<EventMarkerId>::type) },
                                         reinterpret_cast<const numpy_type<EventMarkerId>::type *>(
                                         self.FirstEventPtr()));
        dict["event_count"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.EventCountPtr()));
        dict["first_barrier"] = py::array_t<numpy_type<
        BarrierMarkerId>::type>({ self.size() },
                                { sizeof(numpy_type<BarrierMarkerId>::type) },
                                reinterpret_cast<const numpy_type<BarrierMarkerId>::type *>(
                                self.FirstBarrierPtr()));
        dict["barrier_count"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.BarrierCountPtr()));
        for (uint32_t se = 0; se < kNumSe; ++se)
        {
            std::ostringstream key;
            key << "end_cycle_events"
                << "_" << se;
            dict[py::cast(key.str())] = py::array_t<
            numpy_type<uint64_t>::type>({ self.size() },
                                        { sizeof(numpy_type<uint64_t>::type) * kNumSe },
                                        reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                        self.EndCycleEventsPtr(LabelMarkerId(0), se)));
        }
        dict["label_node"] = py::array_t<
        numpy_type<uint64_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint64_t>::type) },
                                    reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                    self.LabelNodePtr()));
        for (uint32_t se = 0; se < kNumSe; ++se)
        {
            std::ostringstream key;
            key << "start_cycle"
                << "_" << se;
            dict[py::cast(key.str())] = py::array_t<
            numpy_type<uint64_t>::type>({ self.size() },
                                        { sizeof(numpy_type<uint64_t>::type) * kNumSe },
                                        reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                        self.StartCyclePtr(LabelMarkerId(0), se)));
        }
        for (uint32_t se = 0; se < kNumSe; ++se)
        {
            std::ostringstream key;
            key << "end_cycle"
                << "_" << se;
            dict[py::cast(key.str())] = py::array_t<
            numpy_type<uint64_t>::type>({ self.size() },
                                        { sizeof(numpy_type<uint64_t>::type) * kNumSe },
                                        reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                        self.EndCyclePtr(LabelMarkerId(0), se)));
        }

        return DataFrame(dict);
    });
    py::class_<LayoutTransitionMarkerInfo>(m, "LayoutTransitionMarkerInfo")
    .def("flags",
         [](const LayoutTransitionMarkerInfo &self) {
             return py::array_t<
             numpy_type<LayoutTransitionMarkerFlags>::
             type>({ self.size() },
                   { sizeof(numpy_type<LayoutTransitionMarkerFlags>::type) },
                   reinterpret_cast<const numpy_type<LayoutTransitionMarkerFlags>::type *>(
                   self.FlagsPtr()));
         })
    .def("index",
         [](const LayoutTransitionMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.IndexPtr()));
         })
    .def("barrier",
         [](const LayoutTransitionMarkerInfo &self) {
             return py::array_t<numpy_type<
             BarrierMarkerId>::type>({ self.size() },
                                     { sizeof(numpy_type<BarrierMarkerId>::type) },
                                     reinterpret_cast<const numpy_type<BarrierMarkerId>::type *>(
                                     self.BarrierPtr()));
         })
    .def("label",
         [](const LayoutTransitionMarkerInfo &self) {
             return py::array_t<numpy_type<
             LabelMarkerId>::type>({ self.size() },
                                   { sizeof(numpy_type<LabelMarkerId>::type) },
                                   reinterpret_cast<const numpy_type<LabelMarkerId>::type *>(
                                   self.LabelPtr()));
         })
    .def("first_event",
         [](const LayoutTransitionMarkerInfo &self) {
             return py::array_t<numpy_type<
             EventMarkerId>::type>({ self.size() },
                                   { sizeof(numpy_type<EventMarkerId>::type) },
                                   reinterpret_cast<const numpy_type<EventMarkerId>::type *>(
                                   self.FirstEventPtr()));
         })
    .def("event_count",
         [](const LayoutTransitionMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.EventCountPtr()));
         })

    .def("to_dataframe", [](const LayoutTransitionMarkerInfo &self) {
        py::object DataFrame = py::module::import("pandas").attr("DataFrame");
        py::dict   dict;
        dict["flags"] = py::array_t<numpy_type<
        LayoutTransitionMarkerFlags>::type>({ self.size() },
                                            { sizeof(
                                            numpy_type<LayoutTransitionMarkerFlags>::type) },
                                            reinterpret_cast<
                                            const numpy_type<LayoutTransitionMarkerFlags>::type *>(
                                            self.FlagsPtr()));
        dict["index"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.IndexPtr()));
        dict["barrier"] = py::array_t<numpy_type<
        BarrierMarkerId>::type>({ self.size() },
                                { sizeof(numpy_type<BarrierMarkerId>::type) },
                                reinterpret_cast<const numpy_type<BarrierMarkerId>::type *>(
                                self.BarrierPtr()));
        dict["label"] = py::array_t<
        numpy_type<LabelMarkerId>::type>({ self.size() },
                                         { sizeof(numpy_type<LabelMarkerId>::type) },
                                         reinterpret_cast<const numpy_type<LabelMarkerId>::type *>(
                                         self.LabelPtr()));
        dict["first_event"] = py::array_t<
        numpy_type<EventMarkerId>::type>({ self.size() },
                                         { sizeof(numpy_type<EventMarkerId>::type) },
                                         reinterpret_cast<const numpy_type<EventMarkerId>::type *>(
                                         self.FirstEventPtr()));
        dict["event_count"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.EventCountPtr()));

        return DataFrame(dict);
    });
    py::class_<PipelineBindMarkerInfo>(m, "PipelineBindMarkerInfo")
    .def("bind_point",
         [](const PipelineBindMarkerInfo &self) {
             return py::array_t<numpy_type<
             MarkerPipelineBindPoint>::type>({ self.size() },
                                             { sizeof(numpy_type<MarkerPipelineBindPoint>::type) },
                                             reinterpret_cast<
                                             const numpy_type<MarkerPipelineBindPoint>::type *>(
                                             self.BindPointPtr()));
         })
    .def("api_pso_hash",
         [](const PipelineBindMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint64_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint64_t>::type) },
                                         reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                         self.ApiPsoHashPtr()));
         })
    .def("command_buffer",
         [](const PipelineBindMarkerInfo &self) {
             return py::array_t<numpy_type<
             CommandBufferMarkerId>::type>({ self.size() },
                                           { sizeof(numpy_type<CommandBufferMarkerId>::type) },
                                           reinterpret_cast<
                                           const numpy_type<CommandBufferMarkerId>::type *>(
                                           self.CommandBufferPtr()));
         })
    .def("index",
         [](const PipelineBindMarkerInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.IndexPtr()));
         })
    .def("label",
         [](const PipelineBindMarkerInfo &self) {
             return py::array_t<numpy_type<
             LabelMarkerId>::type>({ self.size() },
                                   { sizeof(numpy_type<LabelMarkerId>::type) },
                                   reinterpret_cast<const numpy_type<LabelMarkerId>::type *>(
                                   self.LabelPtr()));
         })

    .def("to_dataframe", [](const PipelineBindMarkerInfo &self) {
        py::object DataFrame = py::module::import("pandas").attr("DataFrame");
        py::dict   dict;
        dict["bind_point"] = py::array_t<numpy_type<
        MarkerPipelineBindPoint>::type>({ self.size() },
                                        { sizeof(numpy_type<MarkerPipelineBindPoint>::type) },
                                        reinterpret_cast<
                                        const numpy_type<MarkerPipelineBindPoint>::type *>(
                                        self.BindPointPtr()));
        dict["api_pso_hash"] = py::array_t<
        numpy_type<uint64_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint64_t>::type) },
                                    reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                    self.ApiPsoHashPtr()));
        dict["command_buffer"] = py::array_t<
        numpy_type<CommandBufferMarkerId>::type>({ self.size() },
                                                 { sizeof(
                                                 numpy_type<CommandBufferMarkerId>::type) },
                                                 reinterpret_cast<
                                                 const numpy_type<CommandBufferMarkerId>::type *>(
                                                 self.CommandBufferPtr()));
        dict["index"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.IndexPtr()));
        dict["label"] = py::array_t<
        numpy_type<LabelMarkerId>::type>({ self.size() },
                                         { sizeof(numpy_type<LabelMarkerId>::type) },
                                         reinterpret_cast<const numpy_type<LabelMarkerId>::type *>(
                                         self.LabelPtr()));

        return DataFrame(dict);
    });
    py::class_<SubEventInfo>(m, "SubEventInfo")
    .def("initiator",
         [](const SubEventInfo &self) {
             return py::array_t<numpy_type<
             InitiatorState>::type>({ self.size() },
                                    { sizeof(numpy_type<InitiatorState>::type) },
                                    reinterpret_cast<const numpy_type<InitiatorState>::type *>(
                                    self.InitiatorPtr()));
         })
    .def("event",
         [](const SubEventInfo &self) {
             return py::array_t<numpy_type<
             EventMarkerId>::type>({ self.size() },
                                   { sizeof(numpy_type<EventMarkerId>::type) },
                                   reinterpret_cast<const numpy_type<EventMarkerId>::type *>(
                                   self.EventPtr()));
         })
    .def("barrier",
         [](const SubEventInfo &self) {
             return py::array_t<numpy_type<
             BarrierMarkerId>::type>({ self.size() },
                                     { sizeof(numpy_type<BarrierMarkerId>::type) },
                                     reinterpret_cast<const numpy_type<BarrierMarkerId>::type *>(
                                     self.BarrierPtr()));
         })
    .def("layout_transition",
         [](const SubEventInfo &self) {
             return py::array_t<numpy_type<
             LayoutTransitionMarkerId>::type>({ self.size() },
                                              { sizeof(
                                              numpy_type<LayoutTransitionMarkerId>::type) },
                                              reinterpret_cast<
                                              const numpy_type<LayoutTransitionMarkerId>::type *>(
                                              self.LayoutTransitionPtr()));
         })
    .def("initiator_register",
         [](const SubEventInfo &self) {
             return py::array_t<
             numpy_type<uint16_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint16_t>::type) },
                                         reinterpret_cast<const numpy_type<uint16_t>::type *>(
                                         self.InitiatorRegisterPtr()));
         })
    .def("initiator_data",
         [](const SubEventInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.InitiatorDataPtr()));
         })
    .def("hardware_context",
         [](const SubEventInfo &self) {
             return py::array_t<
             numpy_type<uint8_t>::type>({ self.size() },
                                        { sizeof(numpy_type<uint8_t>::type) },
                                        reinterpret_cast<const numpy_type<uint8_t>::type *>(
                                        self.HardwareContextPtr()));
         })
    .def("initiator_cycle",
         [](const SubEventInfo &self) {
             return py::array_t<
             numpy_type<uint64_t>::type>({ self.size(), kNumSe },
                                         { sizeof(numpy_type<uint64_t>::type) * kNumSe,
                                           sizeof(numpy_type<uint64_t>::type) * kNumSe },
                                         reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                         self.InitiatorCyclePtr()));
         })
    .def("context_roll_cycle",
         [](const SubEventInfo &self) {
             return py::array_t<
             numpy_type<uint64_t>::type>({ self.size(), kNumSe },
                                         { sizeof(numpy_type<uint64_t>::type) * kNumSe,
                                           sizeof(numpy_type<uint64_t>::type) * kNumSe },
                                         reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                         self.ContextRollCyclePtr()));
         })
    .def("num_indices",
         [](const SubEventInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.NumIndicesPtr()));
         })
    .def("num_instances",
         [](const SubEventInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.NumInstancesPtr()));
         })
    .def("dim_x",
         [](const SubEventInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.DimXPtr()));
         })
    .def("dim_y",
         [](const SubEventInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.DimYPtr()));
         })
    .def("dim_z",
         [](const SubEventInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.DimZPtr()));
         })
    .def("num_thread_x",
         [](const SubEventInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.NumThreadXPtr()));
         })
    .def("num_thread_y",
         [](const SubEventInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.NumThreadYPtr()));
         })
    .def("num_thread_z",
         [](const SubEventInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.NumThreadZPtr()));
         })
    .def("vgprs",
         [](const SubEventInfo &self) {
             return py::array_t<
             numpy_type<uint16_t>::type>({ self.size(), kShaderStageCount },
                                         { sizeof(numpy_type<uint16_t>::type) * kShaderStageCount,
                                           sizeof(numpy_type<uint16_t>::type) * kShaderStageCount },
                                         reinterpret_cast<const numpy_type<uint16_t>::type *>(
                                         self.VGPRsPtr()));
         })
    .def("sgprs",
         [](const SubEventInfo &self) {
             return py::array_t<
             numpy_type<uint16_t>::type>({ self.size(), kShaderStageCount },
                                         { sizeof(numpy_type<uint16_t>::type) * kShaderStageCount,
                                           sizeof(numpy_type<uint16_t>::type) * kShaderStageCount },
                                         reinterpret_cast<const numpy_type<uint16_t>::type *>(
                                         self.SGPRsPtr()));
         })

    .def("to_dataframe", [](const SubEventInfo &self) {
        py::object DataFrame = py::module::import("pandas").attr("DataFrame");
        py::dict   dict;
        dict["initiator"] = py::array_t<numpy_type<
        InitiatorState>::type>({ self.size() },
                               { sizeof(numpy_type<InitiatorState>::type) },
                               reinterpret_cast<const numpy_type<InitiatorState>::type *>(
                               self.InitiatorPtr()));
        dict["event"] = py::array_t<
        numpy_type<EventMarkerId>::type>({ self.size() },
                                         { sizeof(numpy_type<EventMarkerId>::type) },
                                         reinterpret_cast<const numpy_type<EventMarkerId>::type *>(
                                         self.EventPtr()));
        dict["barrier"] = py::array_t<numpy_type<
        BarrierMarkerId>::type>({ self.size() },
                                { sizeof(numpy_type<BarrierMarkerId>::type) },
                                reinterpret_cast<const numpy_type<BarrierMarkerId>::type *>(
                                self.BarrierPtr()));
        dict["layout_transition"] = py::array_t<numpy_type<
        LayoutTransitionMarkerId>::type>({ self.size() },
                                         { sizeof(numpy_type<LayoutTransitionMarkerId>::type) },
                                         reinterpret_cast<
                                         const numpy_type<LayoutTransitionMarkerId>::type *>(
                                         self.LayoutTransitionPtr()));
        dict["initiator_register"] = py::array_t<
        numpy_type<uint16_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint16_t>::type) },
                                    reinterpret_cast<const numpy_type<uint16_t>::type *>(
                                    self.InitiatorRegisterPtr()));
        dict["initiator_data"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.InitiatorDataPtr()));
        dict["hardware_context"] = py::array_t<
        numpy_type<uint8_t>::type>({ self.size() },
                                   { sizeof(numpy_type<uint8_t>::type) },
                                   reinterpret_cast<const numpy_type<uint8_t>::type *>(
                                   self.HardwareContextPtr()));
        for (uint32_t se = 0; se < kNumSe; ++se)
        {
            std::ostringstream key;
            key << "initiator_cycle"
                << "_" << se;
            dict[py::cast(key.str())] = py::array_t<
            numpy_type<uint64_t>::type>({ self.size() },
                                        { sizeof(numpy_type<uint64_t>::type) * kNumSe },
                                        reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                        self.InitiatorCyclePtr(SubEventId(0), se)));
        }
        for (uint32_t se = 0; se < kNumSe; ++se)
        {
            std::ostringstream key;
            key << "context_roll_cycle"
                << "_" << se;
            dict[py::cast(key.str())] = py::array_t<
            numpy_type<uint64_t>::type>({ self.size() },
                                        { sizeof(numpy_type<uint64_t>::type) * kNumSe },
                                        reinterpret_cast<const numpy_type<uint64_t>::type *>(
                                        self.ContextRollCyclePtr(SubEventId(0), se)));
        }
        dict["num_indices"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.NumIndicesPtr()));
        dict["num_instances"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.NumInstancesPtr()));
        dict["dim_x"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.DimXPtr()));
        dict["dim_y"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.DimYPtr()));
        dict["dim_z"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.DimZPtr()));
        dict["num_thread_x"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.NumThreadXPtr()));
        dict["num_thread_y"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.NumThreadYPtr()));
        dict["num_thread_z"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.NumThreadZPtr()));
        for (uint32_t stage = 0; stage < kShaderStageCount; ++stage)
        {
            std::ostringstream key;
            key << "vgprs"
                << "_" << stage;
            dict[py::cast(key.str())] = py::array_t<
            numpy_type<uint16_t>::type>({ self.size() },
                                        { sizeof(numpy_type<uint16_t>::type) * kShaderStageCount },
                                        reinterpret_cast<const numpy_type<uint16_t>::type *>(
                                        self.VGPRsPtr(SubEventId(0),
                                                      static_cast<ShaderStage>(stage))));
        }
        for (uint32_t stage = 0; stage < kShaderStageCount; ++stage)
        {
            std::ostringstream key;
            key << "sgprs"
                << "_" << stage;
            dict[py::cast(key.str())] = py::array_t<
            numpy_type<uint16_t>::type>({ self.size() },
                                        { sizeof(numpy_type<uint16_t>::type) * kShaderStageCount },
                                        reinterpret_cast<const numpy_type<uint16_t>::type *>(
                                        self.SGPRsPtr(SubEventId(0),
                                                      static_cast<ShaderStage>(stage))));
        }

        return DataFrame(dict);
    });
    py::class_<VulkanCallInfo>(m, "VulkanCallInfo")
    .def("command_buffer",
         [](const VulkanCallInfo &self) {
             return py::array_t<numpy_type<
             CommandBufferMarkerId>::type>({ self.size() },
                                           { sizeof(numpy_type<CommandBufferMarkerId>::type) },
                                           reinterpret_cast<
                                           const numpy_type<CommandBufferMarkerId>::type *>(
                                           self.CommandBufferPtr()));
         })
    .def("first_event",
         [](const VulkanCallInfo &self) {
             return py::array_t<numpy_type<
             EventMarkerId>::type>({ self.size() },
                                   { sizeof(numpy_type<EventMarkerId>::type) },
                                   reinterpret_cast<const numpy_type<EventMarkerId>::type *>(
                                   self.FirstEventPtr()));
         })
    .def("event_count",
         [](const VulkanCallInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.EventCountPtr()));
         })
    .def("first_barrier",
         [](const VulkanCallInfo &self) {
             return py::array_t<numpy_type<
             BarrierMarkerId>::type>({ self.size() },
                                     { sizeof(numpy_type<BarrierMarkerId>::type) },
                                     reinterpret_cast<const numpy_type<BarrierMarkerId>::type *>(
                                     self.FirstBarrierPtr()));
         })
    .def("barrier_count",
         [](const VulkanCallInfo &self) {
             return py::array_t<
             numpy_type<uint32_t>::type>({ self.size() },
                                         { sizeof(numpy_type<uint32_t>::type) },
                                         reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                         self.BarrierCountPtr()));
         })
    .def("cmd_id",
         [](const VulkanCallInfo &self) {
             return py::array_t<
             numpy_type<VKCmdID>::type>({ self.size() },
                                        { sizeof(numpy_type<VKCmdID>::type) },
                                        reinterpret_cast<const numpy_type<VKCmdID>::type *>(
                                        self.CmdIdPtr()));
         })
    .def("argument_data",
         [](const VulkanCallInfo &self) {
             return py::array_t<
             numpy_type<uintptr_t>::type>({ self.size() },
                                          { sizeof(numpy_type<uintptr_t>::type) },
                                          reinterpret_cast<const numpy_type<uintptr_t>::type *>(
                                          self.ArgumentDataPtr()));
         })
    .def("to_dataframe", [](const VulkanCallInfo &self) {
        py::object DataFrame = py::module::import("pandas").attr("DataFrame");
        py::dict   dict;
        dict["command_buffer"] = py::array_t<
        numpy_type<CommandBufferMarkerId>::type>({ self.size() },
                                                 { sizeof(
                                                 numpy_type<CommandBufferMarkerId>::type) },
                                                 reinterpret_cast<
                                                 const numpy_type<CommandBufferMarkerId>::type *>(
                                                 self.CommandBufferPtr()));
        dict["first_event"] = py::array_t<
        numpy_type<EventMarkerId>::type>({ self.size() },
                                         { sizeof(numpy_type<EventMarkerId>::type) },
                                         reinterpret_cast<const numpy_type<EventMarkerId>::type *>(
                                         self.FirstEventPtr()));
        dict["event_count"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.EventCountPtr()));
        dict["first_barrier"] = py::array_t<numpy_type<
        BarrierMarkerId>::type>({ self.size() },
                                { sizeof(numpy_type<BarrierMarkerId>::type) },
                                reinterpret_cast<const numpy_type<BarrierMarkerId>::type *>(
                                self.FirstBarrierPtr()));
        dict["barrier_count"] = py::array_t<
        numpy_type<uint32_t>::type>({ self.size() },
                                    { sizeof(numpy_type<uint32_t>::type) },
                                    reinterpret_cast<const numpy_type<uint32_t>::type *>(
                                    self.BarrierCountPtr()));
        dict["cmd_id"] = py::array_t<
        numpy_type<VKCmdID>::type>({ self.size() },
                                   { sizeof(numpy_type<VKCmdID>::type) },
                                   reinterpret_cast<const numpy_type<VKCmdID>::type *>(
                                   self.CmdIdPtr()));
        dict["argument_data"] = py::array_t<
        numpy_type<uintptr_t>::type>({ self.size() },
                                     { sizeof(numpy_type<uintptr_t>::type) },
                                     reinterpret_cast<const numpy_type<uintptr_t>::type *>(
                                     self.ArgumentDataPtr()));
        return DataFrame(dict);
    });
}
}  // namespace Dive
