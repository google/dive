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

#include <cstring>

#include "event_state.h"

namespace Dive
{

template<>
void EventStateInfoT<EventStateInfo_CONFIG>::Reserve(
typename EventStateInfo::Id::basic_type new_cap)
{
    if (new_cap <= m_cap)
        return;

    // Round up to next aligned capacity. The address of each field array is:
    //     `m_buffer + field_offset * cap`
    // - `m_buffer` satisfies the alignment of any basic type (from operator new)
    // - `field_offset` has no alignment constraints at all (fields are tightly packed).
    // - `cap`  must be aligned to ensure that `field_offset * cap` is aligned
    new_cap = (new_cap + kAlignment - 1) & ~(kAlignment - 1);

    // Allocate enough memory to store `new_cap` number of elements
    size_t num_bytes = new_cap * kElemSize;
    size_t is_set_num_bytes = (new_cap * kNumFields) / 8 + 1;
    m_is_set_buffer.resize(is_set_num_bytes, 0);

    // Allocate new buffer as an array of `max_align_t`, to make sure the buffer
    // is sufficiently aligned for the type of any possible field.
    auto new_buffer = std::unique_ptr<std::max_align_t[]>(
    new std::max_align_t[(num_bytes + sizeof(std::max_align_t) - 1) / sizeof(std::max_align_t)]);

    auto old_topology_ptr = TopologyPtr();
    auto old_prim_restart_enabled_ptr = PrimRestartEnabledPtr();
    auto old_patch_control_points_ptr = PatchControlPointsPtr();
    auto old_viewport_ptr = ViewportPtr();
    auto old_scissor_ptr = ScissorPtr();
    auto old_depth_clamp_enabled_ptr = DepthClampEnabledPtr();
    auto old_rasterizer_discard_enabled_ptr = RasterizerDiscardEnabledPtr();
    auto old_polygon_mode_ptr = PolygonModePtr();
    auto old_cull_mode_ptr = CullModePtr();
    auto old_front_face_ptr = FrontFacePtr();
    auto old_depth_bias_enabled_ptr = DepthBiasEnabledPtr();
    auto old_depth_bias_constant_factor_ptr = DepthBiasConstantFactorPtr();
    auto old_depth_bias_clamp_ptr = DepthBiasClampPtr();
    auto old_depth_bias_slope_factor_ptr = DepthBiasSlopeFactorPtr();
    auto old_line_width_ptr = LineWidthPtr();
    auto old_rasterization_samples_ptr = RasterizationSamplesPtr();
    auto old_sample_shading_enabled_ptr = SampleShadingEnabledPtr();
    auto old_min_sample_shading_ptr = MinSampleShadingPtr();
    auto old_sample_mask_ptr = SampleMaskPtr();
    auto old_alpha_to_coverage_enabled_ptr = AlphaToCoverageEnabledPtr();
    auto old_depth_test_enabled_ptr = DepthTestEnabledPtr();
    auto old_depth_write_enabled_ptr = DepthWriteEnabledPtr();
    auto old_depth_compare_op_ptr = DepthCompareOpPtr();
    auto old_depth_bounds_test_enabled_ptr = DepthBoundsTestEnabledPtr();
    auto old_stencil_test_enabled_ptr = StencilTestEnabledPtr();
    auto old_stencil_op_state_front_ptr = StencilOpStateFrontPtr();
    auto old_stencil_op_state_back_ptr = StencilOpStateBackPtr();
    auto old_min_depth_bounds_ptr = MinDepthBoundsPtr();
    auto old_max_depth_bounds_ptr = MaxDepthBoundsPtr();
    auto old_logic_op_enabled_ptr = LogicOpEnabledPtr();
    auto old_logic_op_ptr = LogicOpPtr();
    auto old_attachment_ptr = AttachmentPtr();
    auto old_blend_constant_ptr = BlendConstantPtr();
    auto old_lrz_enabled_ptr = LRZEnabledPtr();
    auto old_lrz_write_ptr = LRZWritePtr();
    auto old_lrz_dir_status_ptr = LRZDirStatusPtr();
    auto old_lrz_dir_write_ptr = LRZDirWritePtr();
    auto old_z_test_mode_ptr = ZTestModePtr();
    auto old_bin_w_ptr = BinWPtr();
    auto old_bin_h_ptr = BinHPtr();
    auto old_render_mode_ptr = RenderModePtr();
    auto old_buffers_location_ptr = BuffersLocationPtr();
    auto old_thread_size_ptr = ThreadSizePtr();
    auto old_enable_all_helper_lanes_ptr = EnableAllHelperLanesPtr();
    auto old_enable_partial_helper_lanes_ptr = EnablePartialHelperLanesPtr();

    // `old_buffer` keeps the old buffer from being deallocated before we have
    // copied the data into the new buffer. The old buffer will be deallocated
    // once `old_buffer` goes out of scope.
    auto old_buffer(std::move(m_buffer));

    m_buffer = std::move(new_buffer);
    m_cap = new_cap;

    // Copy all of the data from the old buffer to the new buffer
    static_assert(std::is_trivially_copyable<uint32_t>::value,
                  "Field type must be trivially copyable");
    memcpy(TopologyPtr(), old_topology_ptr, kTopologySize * m_size);
    static_assert(std::is_trivially_copyable<bool>::value, "Field type must be trivially copyable");
    memcpy(PrimRestartEnabledPtr(), old_prim_restart_enabled_ptr, kPrimRestartEnabledSize * m_size);
    static_assert(std::is_trivially_copyable<uint32_t>::value,
                  "Field type must be trivially copyable");
    memcpy(PatchControlPointsPtr(), old_patch_control_points_ptr, kPatchControlPointsSize * m_size);
    static_assert(std::is_trivially_copyable<VkViewport>::value,
                  "Field type must be trivially copyable");
    memcpy(ViewportPtr(), old_viewport_ptr, kViewportSize * m_size);
    static_assert(std::is_trivially_copyable<VkRect2D>::value,
                  "Field type must be trivially copyable");
    memcpy(ScissorPtr(), old_scissor_ptr, kScissorSize * m_size);
    static_assert(std::is_trivially_copyable<bool>::value, "Field type must be trivially copyable");
    memcpy(DepthClampEnabledPtr(), old_depth_clamp_enabled_ptr, kDepthClampEnabledSize * m_size);
    static_assert(std::is_trivially_copyable<bool>::value, "Field type must be trivially copyable");
    memcpy(RasterizerDiscardEnabledPtr(),
           old_rasterizer_discard_enabled_ptr,
           kRasterizerDiscardEnabledSize * m_size);
    static_assert(std::is_trivially_copyable<VkPolygonMode>::value,
                  "Field type must be trivially copyable");
    memcpy(PolygonModePtr(), old_polygon_mode_ptr, kPolygonModeSize * m_size);
    static_assert(std::is_trivially_copyable<VkCullModeFlags>::value,
                  "Field type must be trivially copyable");
    memcpy(CullModePtr(), old_cull_mode_ptr, kCullModeSize * m_size);
    static_assert(std::is_trivially_copyable<VkFrontFace>::value,
                  "Field type must be trivially copyable");
    memcpy(FrontFacePtr(), old_front_face_ptr, kFrontFaceSize * m_size);
    static_assert(std::is_trivially_copyable<bool>::value, "Field type must be trivially copyable");
    memcpy(DepthBiasEnabledPtr(), old_depth_bias_enabled_ptr, kDepthBiasEnabledSize * m_size);
    static_assert(std::is_trivially_copyable<float>::value,
                  "Field type must be trivially copyable");
    memcpy(DepthBiasConstantFactorPtr(),
           old_depth_bias_constant_factor_ptr,
           kDepthBiasConstantFactorSize * m_size);
    static_assert(std::is_trivially_copyable<float>::value,
                  "Field type must be trivially copyable");
    memcpy(DepthBiasClampPtr(), old_depth_bias_clamp_ptr, kDepthBiasClampSize * m_size);
    static_assert(std::is_trivially_copyable<float>::value,
                  "Field type must be trivially copyable");
    memcpy(DepthBiasSlopeFactorPtr(),
           old_depth_bias_slope_factor_ptr,
           kDepthBiasSlopeFactorSize * m_size);
    static_assert(std::is_trivially_copyable<float>::value,
                  "Field type must be trivially copyable");
    memcpy(LineWidthPtr(), old_line_width_ptr, kLineWidthSize * m_size);
    static_assert(std::is_trivially_copyable<VkSampleCountFlagBits>::value,
                  "Field type must be trivially copyable");
    memcpy(RasterizationSamplesPtr(),
           old_rasterization_samples_ptr,
           kRasterizationSamplesSize * m_size);
    static_assert(std::is_trivially_copyable<bool>::value, "Field type must be trivially copyable");
    memcpy(SampleShadingEnabledPtr(),
           old_sample_shading_enabled_ptr,
           kSampleShadingEnabledSize * m_size);
    static_assert(std::is_trivially_copyable<float>::value,
                  "Field type must be trivially copyable");
    memcpy(MinSampleShadingPtr(), old_min_sample_shading_ptr, kMinSampleShadingSize * m_size);
    static_assert(std::is_trivially_copyable<VkSampleMask>::value,
                  "Field type must be trivially copyable");
    memcpy(SampleMaskPtr(), old_sample_mask_ptr, kSampleMaskSize * m_size);
    static_assert(std::is_trivially_copyable<bool>::value, "Field type must be trivially copyable");
    memcpy(AlphaToCoverageEnabledPtr(),
           old_alpha_to_coverage_enabled_ptr,
           kAlphaToCoverageEnabledSize * m_size);
    static_assert(std::is_trivially_copyable<bool>::value, "Field type must be trivially copyable");
    memcpy(DepthTestEnabledPtr(), old_depth_test_enabled_ptr, kDepthTestEnabledSize * m_size);
    static_assert(std::is_trivially_copyable<bool>::value, "Field type must be trivially copyable");
    memcpy(DepthWriteEnabledPtr(), old_depth_write_enabled_ptr, kDepthWriteEnabledSize * m_size);
    static_assert(std::is_trivially_copyable<VkCompareOp>::value,
                  "Field type must be trivially copyable");
    memcpy(DepthCompareOpPtr(), old_depth_compare_op_ptr, kDepthCompareOpSize * m_size);
    static_assert(std::is_trivially_copyable<bool>::value, "Field type must be trivially copyable");
    memcpy(DepthBoundsTestEnabledPtr(),
           old_depth_bounds_test_enabled_ptr,
           kDepthBoundsTestEnabledSize * m_size);
    static_assert(std::is_trivially_copyable<bool>::value, "Field type must be trivially copyable");
    memcpy(StencilTestEnabledPtr(), old_stencil_test_enabled_ptr, kStencilTestEnabledSize * m_size);
    static_assert(std::is_trivially_copyable<VkStencilOpState>::value,
                  "Field type must be trivially copyable");
    memcpy(StencilOpStateFrontPtr(),
           old_stencil_op_state_front_ptr,
           kStencilOpStateFrontSize * m_size);
    static_assert(std::is_trivially_copyable<VkStencilOpState>::value,
                  "Field type must be trivially copyable");
    memcpy(StencilOpStateBackPtr(),
           old_stencil_op_state_back_ptr,
           kStencilOpStateBackSize * m_size);
    static_assert(std::is_trivially_copyable<float>::value,
                  "Field type must be trivially copyable");
    memcpy(MinDepthBoundsPtr(), old_min_depth_bounds_ptr, kMinDepthBoundsSize * m_size);
    static_assert(std::is_trivially_copyable<float>::value,
                  "Field type must be trivially copyable");
    memcpy(MaxDepthBoundsPtr(), old_max_depth_bounds_ptr, kMaxDepthBoundsSize * m_size);
    static_assert(std::is_trivially_copyable<bool>::value, "Field type must be trivially copyable");
    memcpy(LogicOpEnabledPtr(), old_logic_op_enabled_ptr, kLogicOpEnabledSize * m_size);
    static_assert(std::is_trivially_copyable<VkLogicOp>::value,
                  "Field type must be trivially copyable");
    memcpy(LogicOpPtr(), old_logic_op_ptr, kLogicOpSize * m_size);
    static_assert(std::is_trivially_copyable<VkPipelineColorBlendAttachmentState>::value,
                  "Field type must be trivially copyable");
    memcpy(AttachmentPtr(), old_attachment_ptr, kAttachmentSize * m_size);
    static_assert(std::is_trivially_copyable<float>::value,
                  "Field type must be trivially copyable");
    memcpy(BlendConstantPtr(), old_blend_constant_ptr, kBlendConstantSize * m_size);
    static_assert(std::is_trivially_copyable<bool>::value, "Field type must be trivially copyable");
    memcpy(LRZEnabledPtr(), old_lrz_enabled_ptr, kLRZEnabledSize * m_size);
    static_assert(std::is_trivially_copyable<bool>::value, "Field type must be trivially copyable");
    memcpy(LRZWritePtr(), old_lrz_write_ptr, kLRZWriteSize * m_size);
    static_assert(std::is_trivially_copyable<a6xx_lrz_dir_status>::value,
                  "Field type must be trivially copyable");
    memcpy(LRZDirStatusPtr(), old_lrz_dir_status_ptr, kLRZDirStatusSize * m_size);
    static_assert(std::is_trivially_copyable<bool>::value, "Field type must be trivially copyable");
    memcpy(LRZDirWritePtr(), old_lrz_dir_write_ptr, kLRZDirWriteSize * m_size);
    static_assert(std::is_trivially_copyable<a6xx_ztest_mode>::value,
                  "Field type must be trivially copyable");
    memcpy(ZTestModePtr(), old_z_test_mode_ptr, kZTestModeSize * m_size);
    static_assert(std::is_trivially_copyable<uint32_t>::value,
                  "Field type must be trivially copyable");
    memcpy(BinWPtr(), old_bin_w_ptr, kBinWSize * m_size);
    static_assert(std::is_trivially_copyable<uint32_t>::value,
                  "Field type must be trivially copyable");
    memcpy(BinHPtr(), old_bin_h_ptr, kBinHSize * m_size);
    static_assert(std::is_trivially_copyable<a6xx_render_mode>::value,
                  "Field type must be trivially copyable");
    memcpy(RenderModePtr(), old_render_mode_ptr, kRenderModeSize * m_size);
    static_assert(std::is_trivially_copyable<a6xx_buffers_location>::value,
                  "Field type must be trivially copyable");
    memcpy(BuffersLocationPtr(), old_buffers_location_ptr, kBuffersLocationSize * m_size);
    static_assert(std::is_trivially_copyable<a6xx_threadsize>::value,
                  "Field type must be trivially copyable");
    memcpy(ThreadSizePtr(), old_thread_size_ptr, kThreadSizeSize * m_size);
    static_assert(std::is_trivially_copyable<bool>::value, "Field type must be trivially copyable");
    memcpy(EnableAllHelperLanesPtr(),
           old_enable_all_helper_lanes_ptr,
           kEnableAllHelperLanesSize * m_size);
    static_assert(std::is_trivially_copyable<bool>::value, "Field type must be trivially copyable");
    memcpy(EnablePartialHelperLanesPtr(),
           old_enable_partial_helper_lanes_ptr,
           kEnablePartialHelperLanesSize * m_size);

    // Update the debug-only ponters to the arrays
#ifndef NDEBUG
    DBG_topology = TopologyPtr();
    DBG_prim_restart_enabled = PrimRestartEnabledPtr();
    DBG_patch_control_points = PatchControlPointsPtr();
    DBG_viewport = ViewportPtr();
    DBG_scissor = ScissorPtr();
    DBG_depth_clamp_enabled = DepthClampEnabledPtr();
    DBG_rasterizer_discard_enabled = RasterizerDiscardEnabledPtr();
    DBG_polygon_mode = PolygonModePtr();
    DBG_cull_mode = CullModePtr();
    DBG_front_face = FrontFacePtr();
    DBG_depth_bias_enabled = DepthBiasEnabledPtr();
    DBG_depth_bias_constant_factor = DepthBiasConstantFactorPtr();
    DBG_depth_bias_clamp = DepthBiasClampPtr();
    DBG_depth_bias_slope_factor = DepthBiasSlopeFactorPtr();
    DBG_line_width = LineWidthPtr();
    DBG_rasterization_samples = RasterizationSamplesPtr();
    DBG_sample_shading_enabled = SampleShadingEnabledPtr();
    DBG_min_sample_shading = MinSampleShadingPtr();
    DBG_sample_mask = SampleMaskPtr();
    DBG_alpha_to_coverage_enabled = AlphaToCoverageEnabledPtr();
    DBG_depth_test_enabled = DepthTestEnabledPtr();
    DBG_depth_write_enabled = DepthWriteEnabledPtr();
    DBG_depth_compare_op = DepthCompareOpPtr();
    DBG_depth_bounds_test_enabled = DepthBoundsTestEnabledPtr();
    DBG_stencil_test_enabled = StencilTestEnabledPtr();
    DBG_stencil_op_state_front = StencilOpStateFrontPtr();
    DBG_stencil_op_state_back = StencilOpStateBackPtr();
    DBG_min_depth_bounds = MinDepthBoundsPtr();
    DBG_max_depth_bounds = MaxDepthBoundsPtr();
    DBG_logic_op_enabled = LogicOpEnabledPtr();
    DBG_logic_op = LogicOpPtr();
    DBG_attachment = AttachmentPtr();
    DBG_blend_constant = BlendConstantPtr();
    DBG_lrz_enabled = LRZEnabledPtr();
    DBG_lrz_write = LRZWritePtr();
    DBG_lrz_dir_status = LRZDirStatusPtr();
    DBG_lrz_dir_write = LRZDirWritePtr();
    DBG_z_test_mode = ZTestModePtr();
    DBG_bin_w = BinWPtr();
    DBG_bin_h = BinHPtr();
    DBG_render_mode = RenderModePtr();
    DBG_buffers_location = BuffersLocationPtr();
    DBG_thread_size = ThreadSizePtr();
    DBG_enable_all_helper_lanes = EnableAllHelperLanesPtr();
    DBG_enable_partial_helper_lanes = EnablePartialHelperLanesPtr();
#endif
}

template<> EventStateInfo::Iterator EventStateInfoT<EventStateInfo_CONFIG>::Add()
{
    if (m_size >= m_cap)
    {
        auto new_cap = m_cap > 0 ? m_cap * 2 : static_cast<typename Id::basic_type>(kAlignment);
        if (new_cap <= m_cap)
        {
            // capacity has overflowed the `Id` type.
            DIVE_ASSERT(false);
            return end();
        }
        else
        {
            Reserve(new_cap);
        }
    }

    new (TopologyPtr(Id(m_size))) uint32_t();
    new (PrimRestartEnabledPtr(Id(m_size))) bool();
    new (PatchControlPointsPtr(Id(m_size))) uint32_t();
    for (uint32_t viewport = 0; viewport < 16; ++viewport)
    {
        new (ViewportPtr(Id(m_size), viewport)) VkViewport();
    }
    for (uint32_t scissor = 0; scissor < 16; ++scissor)
    {
        new (ScissorPtr(Id(m_size), scissor)) VkRect2D();
    }
    new (DepthClampEnabledPtr(Id(m_size))) bool();
    new (RasterizerDiscardEnabledPtr(Id(m_size))) bool();
    new (PolygonModePtr(Id(m_size))) VkPolygonMode();
    new (CullModePtr(Id(m_size))) VkCullModeFlags();
    new (FrontFacePtr(Id(m_size))) VkFrontFace();
    new (DepthBiasEnabledPtr(Id(m_size))) bool();
    new (DepthBiasConstantFactorPtr(Id(m_size))) float();
    new (DepthBiasClampPtr(Id(m_size))) float();
    new (DepthBiasSlopeFactorPtr(Id(m_size))) float();
    new (LineWidthPtr(Id(m_size))) float();
    new (RasterizationSamplesPtr(Id(m_size))) VkSampleCountFlagBits();
    new (SampleShadingEnabledPtr(Id(m_size))) bool();
    new (MinSampleShadingPtr(Id(m_size))) float();
    new (SampleMaskPtr(Id(m_size))) VkSampleMask();
    new (AlphaToCoverageEnabledPtr(Id(m_size))) bool();
    new (DepthTestEnabledPtr(Id(m_size))) bool();
    new (DepthWriteEnabledPtr(Id(m_size))) bool();
    new (DepthCompareOpPtr(Id(m_size))) VkCompareOp();
    new (DepthBoundsTestEnabledPtr(Id(m_size))) bool();
    new (StencilTestEnabledPtr(Id(m_size))) bool();
    new (StencilOpStateFrontPtr(Id(m_size))) VkStencilOpState();
    new (StencilOpStateBackPtr(Id(m_size))) VkStencilOpState();
    new (MinDepthBoundsPtr(Id(m_size))) float();
    new (MaxDepthBoundsPtr(Id(m_size))) float();
    for (uint32_t attachment = 0; attachment < 8; ++attachment)
    {
        new (LogicOpEnabledPtr(Id(m_size), attachment)) bool();
    }
    for (uint32_t attachment = 0; attachment < 8; ++attachment)
    {
        new (LogicOpPtr(Id(m_size), attachment)) VkLogicOp();
    }
    for (uint32_t attachment = 0; attachment < 8; ++attachment)
    {
        new (AttachmentPtr(Id(m_size), attachment)) VkPipelineColorBlendAttachmentState();
    }
    for (uint32_t channel = 0; channel < 4; ++channel)
    {
        new (BlendConstantPtr(Id(m_size), channel)) float();
    }
    new (LRZEnabledPtr(Id(m_size))) bool();
    new (LRZWritePtr(Id(m_size))) bool();
    new (LRZDirStatusPtr(Id(m_size))) a6xx_lrz_dir_status();
    new (LRZDirWritePtr(Id(m_size))) bool();
    new (ZTestModePtr(Id(m_size))) a6xx_ztest_mode();
    new (BinWPtr(Id(m_size))) uint32_t();
    new (BinHPtr(Id(m_size))) uint32_t();
    new (RenderModePtr(Id(m_size))) a6xx_render_mode();
    new (BuffersLocationPtr(Id(m_size))) a6xx_buffers_location();
    new (ThreadSizePtr(Id(m_size))) a6xx_threadsize();
    new (EnableAllHelperLanesPtr(Id(m_size))) bool();
    new (EnablePartialHelperLanesPtr(Id(m_size))) bool();

    Id id(m_size);
    m_size += 1;
    return find(id);
}

template<>
void EventStateInfoRefT<EventStateInfo_CONFIG>::assign(
const EventStateInfo                         &other_obj,
EventStateInfoRefT<EventStateInfo_CONFIG>::Id other_id) const
{
    DIVE_ASSERT(IsValid());
    DIVE_ASSERT(other_obj.IsValidId(other_id));
    SetTopology(other_obj.Topology(other_id));
    SetPrimRestartEnabled(other_obj.PrimRestartEnabled(other_id));
    SetPatchControlPoints(other_obj.PatchControlPoints(other_id));
    memcpy(m_obj_ptr->ViewportPtr(m_id),
           other_obj.ViewportPtr(other_id),
           EventStateInfo::kViewportSize);
    memcpy(m_obj_ptr->ScissorPtr(m_id),
           other_obj.ScissorPtr(other_id),
           EventStateInfo::kScissorSize);
    SetDepthClampEnabled(other_obj.DepthClampEnabled(other_id));
    SetRasterizerDiscardEnabled(other_obj.RasterizerDiscardEnabled(other_id));
    SetPolygonMode(other_obj.PolygonMode(other_id));
    SetCullMode(other_obj.CullMode(other_id));
    SetFrontFace(other_obj.FrontFace(other_id));
    SetDepthBiasEnabled(other_obj.DepthBiasEnabled(other_id));
    SetDepthBiasConstantFactor(other_obj.DepthBiasConstantFactor(other_id));
    SetDepthBiasClamp(other_obj.DepthBiasClamp(other_id));
    SetDepthBiasSlopeFactor(other_obj.DepthBiasSlopeFactor(other_id));
    SetLineWidth(other_obj.LineWidth(other_id));
    SetRasterizationSamples(other_obj.RasterizationSamples(other_id));
    SetSampleShadingEnabled(other_obj.SampleShadingEnabled(other_id));
    SetMinSampleShading(other_obj.MinSampleShading(other_id));
    SetSampleMask(other_obj.SampleMask(other_id));
    SetAlphaToCoverageEnabled(other_obj.AlphaToCoverageEnabled(other_id));
    SetDepthTestEnabled(other_obj.DepthTestEnabled(other_id));
    SetDepthWriteEnabled(other_obj.DepthWriteEnabled(other_id));
    SetDepthCompareOp(other_obj.DepthCompareOp(other_id));
    SetDepthBoundsTestEnabled(other_obj.DepthBoundsTestEnabled(other_id));
    SetStencilTestEnabled(other_obj.StencilTestEnabled(other_id));
    SetStencilOpStateFront(other_obj.StencilOpStateFront(other_id));
    SetStencilOpStateBack(other_obj.StencilOpStateBack(other_id));
    SetMinDepthBounds(other_obj.MinDepthBounds(other_id));
    SetMaxDepthBounds(other_obj.MaxDepthBounds(other_id));
    memcpy(m_obj_ptr->LogicOpEnabledPtr(m_id),
           other_obj.LogicOpEnabledPtr(other_id),
           EventStateInfo::kLogicOpEnabledSize);
    memcpy(m_obj_ptr->LogicOpPtr(m_id),
           other_obj.LogicOpPtr(other_id),
           EventStateInfo::kLogicOpSize);
    memcpy(m_obj_ptr->AttachmentPtr(m_id),
           other_obj.AttachmentPtr(other_id),
           EventStateInfo::kAttachmentSize);
    memcpy(m_obj_ptr->BlendConstantPtr(m_id),
           other_obj.BlendConstantPtr(other_id),
           EventStateInfo::kBlendConstantSize);
    SetLRZEnabled(other_obj.LRZEnabled(other_id));
    SetLRZWrite(other_obj.LRZWrite(other_id));
    SetLRZDirStatus(other_obj.LRZDirStatus(other_id));
    SetLRZDirWrite(other_obj.LRZDirWrite(other_id));
    SetZTestMode(other_obj.ZTestMode(other_id));
    SetBinW(other_obj.BinW(other_id));
    SetBinH(other_obj.BinH(other_id));
    SetRenderMode(other_obj.RenderMode(other_id));
    SetBuffersLocation(other_obj.BuffersLocation(other_id));
    SetThreadSize(other_obj.ThreadSize(other_id));
    SetEnableAllHelperLanes(other_obj.EnableAllHelperLanes(other_id));
    SetEnablePartialHelperLanes(other_obj.EnablePartialHelperLanes(other_id));
}

template<>
void EventStateInfoRefT<EventStateInfo_CONFIG>::swap(const EventStateInfoRef &other) const
{
    DIVE_ASSERT(m_obj_ptr != nullptr);
    DIVE_ASSERT(m_obj_ptr->IsValidId(m_id));
    DIVE_ASSERT(other.m_obj_ptr != nullptr);
    DIVE_ASSERT(other.m_obj_ptr->IsValidId(other.m_id));
    {
        auto val = Topology();
        SetTopology(other.Topology());
        other.SetTopology(val);
    }
    {
        auto val = PrimRestartEnabled();
        SetPrimRestartEnabled(other.PrimRestartEnabled());
        other.SetPrimRestartEnabled(val);
    }
    {
        auto val = PatchControlPoints();
        SetPatchControlPoints(other.PatchControlPoints());
        other.SetPatchControlPoints(val);
    }
    {
        VkViewport val[EventStateInfo::kViewportArrayCount];
        auto      *ptr = m_obj_ptr->ViewportPtr(m_id);
        auto      *other_ptr = other.m_obj_ptr->ViewportPtr(other.m_id);
        memcpy(val, ptr, EventStateInfo::kViewportSize);
        memcpy(ptr, other_ptr, EventStateInfo::kViewportSize);
        memcpy(other_ptr, val, EventStateInfo::kViewportSize);
    }
    {
        VkRect2D val[EventStateInfo::kScissorArrayCount];
        auto    *ptr = m_obj_ptr->ScissorPtr(m_id);
        auto    *other_ptr = other.m_obj_ptr->ScissorPtr(other.m_id);
        memcpy(val, ptr, EventStateInfo::kScissorSize);
        memcpy(ptr, other_ptr, EventStateInfo::kScissorSize);
        memcpy(other_ptr, val, EventStateInfo::kScissorSize);
    }
    {
        auto val = DepthClampEnabled();
        SetDepthClampEnabled(other.DepthClampEnabled());
        other.SetDepthClampEnabled(val);
    }
    {
        auto val = RasterizerDiscardEnabled();
        SetRasterizerDiscardEnabled(other.RasterizerDiscardEnabled());
        other.SetRasterizerDiscardEnabled(val);
    }
    {
        auto val = PolygonMode();
        SetPolygonMode(other.PolygonMode());
        other.SetPolygonMode(val);
    }
    {
        auto val = CullMode();
        SetCullMode(other.CullMode());
        other.SetCullMode(val);
    }
    {
        auto val = FrontFace();
        SetFrontFace(other.FrontFace());
        other.SetFrontFace(val);
    }
    {
        auto val = DepthBiasEnabled();
        SetDepthBiasEnabled(other.DepthBiasEnabled());
        other.SetDepthBiasEnabled(val);
    }
    {
        auto val = DepthBiasConstantFactor();
        SetDepthBiasConstantFactor(other.DepthBiasConstantFactor());
        other.SetDepthBiasConstantFactor(val);
    }
    {
        auto val = DepthBiasClamp();
        SetDepthBiasClamp(other.DepthBiasClamp());
        other.SetDepthBiasClamp(val);
    }
    {
        auto val = DepthBiasSlopeFactor();
        SetDepthBiasSlopeFactor(other.DepthBiasSlopeFactor());
        other.SetDepthBiasSlopeFactor(val);
    }
    {
        auto val = LineWidth();
        SetLineWidth(other.LineWidth());
        other.SetLineWidth(val);
    }
    {
        auto val = RasterizationSamples();
        SetRasterizationSamples(other.RasterizationSamples());
        other.SetRasterizationSamples(val);
    }
    {
        auto val = SampleShadingEnabled();
        SetSampleShadingEnabled(other.SampleShadingEnabled());
        other.SetSampleShadingEnabled(val);
    }
    {
        auto val = MinSampleShading();
        SetMinSampleShading(other.MinSampleShading());
        other.SetMinSampleShading(val);
    }
    {
        auto val = SampleMask();
        SetSampleMask(other.SampleMask());
        other.SetSampleMask(val);
    }
    {
        auto val = AlphaToCoverageEnabled();
        SetAlphaToCoverageEnabled(other.AlphaToCoverageEnabled());
        other.SetAlphaToCoverageEnabled(val);
    }
    {
        auto val = DepthTestEnabled();
        SetDepthTestEnabled(other.DepthTestEnabled());
        other.SetDepthTestEnabled(val);
    }
    {
        auto val = DepthWriteEnabled();
        SetDepthWriteEnabled(other.DepthWriteEnabled());
        other.SetDepthWriteEnabled(val);
    }
    {
        auto val = DepthCompareOp();
        SetDepthCompareOp(other.DepthCompareOp());
        other.SetDepthCompareOp(val);
    }
    {
        auto val = DepthBoundsTestEnabled();
        SetDepthBoundsTestEnabled(other.DepthBoundsTestEnabled());
        other.SetDepthBoundsTestEnabled(val);
    }
    {
        auto val = StencilTestEnabled();
        SetStencilTestEnabled(other.StencilTestEnabled());
        other.SetStencilTestEnabled(val);
    }
    {
        auto val = StencilOpStateFront();
        SetStencilOpStateFront(other.StencilOpStateFront());
        other.SetStencilOpStateFront(val);
    }
    {
        auto val = StencilOpStateBack();
        SetStencilOpStateBack(other.StencilOpStateBack());
        other.SetStencilOpStateBack(val);
    }
    {
        auto val = MinDepthBounds();
        SetMinDepthBounds(other.MinDepthBounds());
        other.SetMinDepthBounds(val);
    }
    {
        auto val = MaxDepthBounds();
        SetMaxDepthBounds(other.MaxDepthBounds());
        other.SetMaxDepthBounds(val);
    }
    {
        bool  val[EventStateInfo::kLogicOpEnabledArrayCount];
        auto *ptr = m_obj_ptr->LogicOpEnabledPtr(m_id);
        auto *other_ptr = other.m_obj_ptr->LogicOpEnabledPtr(other.m_id);
        memcpy(val, ptr, EventStateInfo::kLogicOpEnabledSize);
        memcpy(ptr, other_ptr, EventStateInfo::kLogicOpEnabledSize);
        memcpy(other_ptr, val, EventStateInfo::kLogicOpEnabledSize);
    }
    {
        VkLogicOp val[EventStateInfo::kLogicOpArrayCount];
        auto     *ptr = m_obj_ptr->LogicOpPtr(m_id);
        auto     *other_ptr = other.m_obj_ptr->LogicOpPtr(other.m_id);
        memcpy(val, ptr, EventStateInfo::kLogicOpSize);
        memcpy(ptr, other_ptr, EventStateInfo::kLogicOpSize);
        memcpy(other_ptr, val, EventStateInfo::kLogicOpSize);
    }
    {
        VkPipelineColorBlendAttachmentState val[EventStateInfo::kAttachmentArrayCount];
        auto                               *ptr = m_obj_ptr->AttachmentPtr(m_id);
        auto                               *other_ptr = other.m_obj_ptr->AttachmentPtr(other.m_id);
        memcpy(val, ptr, EventStateInfo::kAttachmentSize);
        memcpy(ptr, other_ptr, EventStateInfo::kAttachmentSize);
        memcpy(other_ptr, val, EventStateInfo::kAttachmentSize);
    }
    {
        float val[EventStateInfo::kBlendConstantArrayCount];
        auto *ptr = m_obj_ptr->BlendConstantPtr(m_id);
        auto *other_ptr = other.m_obj_ptr->BlendConstantPtr(other.m_id);
        memcpy(val, ptr, EventStateInfo::kBlendConstantSize);
        memcpy(ptr, other_ptr, EventStateInfo::kBlendConstantSize);
        memcpy(other_ptr, val, EventStateInfo::kBlendConstantSize);
    }
    {
        auto val = LRZEnabled();
        SetLRZEnabled(other.LRZEnabled());
        other.SetLRZEnabled(val);
    }
    {
        auto val = LRZWrite();
        SetLRZWrite(other.LRZWrite());
        other.SetLRZWrite(val);
    }
    {
        auto val = LRZDirStatus();
        SetLRZDirStatus(other.LRZDirStatus());
        other.SetLRZDirStatus(val);
    }
    {
        auto val = LRZDirWrite();
        SetLRZDirWrite(other.LRZDirWrite());
        other.SetLRZDirWrite(val);
    }
    {
        auto val = ZTestMode();
        SetZTestMode(other.ZTestMode());
        other.SetZTestMode(val);
    }
    {
        auto val = BinW();
        SetBinW(other.BinW());
        other.SetBinW(val);
    }
    {
        auto val = BinH();
        SetBinH(other.BinH());
        other.SetBinH(val);
    }
    {
        auto val = RenderMode();
        SetRenderMode(other.RenderMode());
        other.SetRenderMode(val);
    }
    {
        auto val = BuffersLocation();
        SetBuffersLocation(other.BuffersLocation());
        other.SetBuffersLocation(val);
    }
    {
        auto val = ThreadSize();
        SetThreadSize(other.ThreadSize());
        other.SetThreadSize(val);
    }
    {
        auto val = EnableAllHelperLanes();
        SetEnableAllHelperLanes(other.EnableAllHelperLanes());
        other.SetEnableAllHelperLanes(val);
    }
    {
        auto val = EnablePartialHelperLanes();
        SetEnablePartialHelperLanes(other.EnablePartialHelperLanes());
        other.SetEnablePartialHelperLanes(val);
    }
}

}  // namespace Dive
