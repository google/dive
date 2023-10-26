
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
#include "data_core.h"
#include <assert.h>
#include "pm4_info.h"

namespace Dive
{

// =================================================================================================
// DataCore
// =================================================================================================
DataCore::DataCore(ILog *log_ptr) :
    m_progress_tracker(NULL),
    m_capture_data(log_ptr),
    m_log_ptr(log_ptr)
{
}

//--------------------------------------------------------------------------------------------------
DataCore::DataCore(ProgressTracker *progress_tracker, ILog *log_ptr) :
    m_progress_tracker(progress_tracker),
    m_capture_data(log_ptr),
    m_log_ptr(log_ptr)
{
}

//--------------------------------------------------------------------------------------------------
CaptureData::LoadResult DataCore::LoadCaptureData(const char *file_name)
{
    m_capture_data = CaptureData(m_progress_tracker,
                                 m_log_ptr);  // Clear any previously loaded data
    m_capture_metadata = CaptureMetadata();
    return m_capture_data.LoadFile(file_name);
}

//--------------------------------------------------------------------------------------------------
bool DataCore::ParseCaptureData()
{
    if (m_progress_tracker)
    {
        m_progress_tracker->sendMessage("Processing command buffers...");
    }

    // Command hierarchy tree creation
    CommandHierarchyCreator cmd_hier_creator;
    if (!cmd_hier_creator.CreateTrees(&m_capture_metadata.m_command_hierarchy,
                                      m_capture_data,
                                      true,
                                      m_log_ptr))
        return false;

    CaptureMetadataCreator metadata_creator(m_capture_metadata);
    for (uint32_t submit_index = 0; submit_index < m_capture_data.GetNumSubmits(); ++submit_index)
    {
        const Dive::SubmitInfo &submit_info = m_capture_data.GetSubmitInfo(submit_index);
        metadata_creator.OnSubmitStart(submit_index, submit_info);

        if (submit_info.IsDummySubmit())
        {
            metadata_creator.OnSubmitEnd(submit_index, submit_info);
            continue;
        }

        // Only gfx or compute engine types are parsed
        if ((submit_info.GetEngineType() != Dive::EngineType::kUniversal) &&
            (submit_info.GetEngineType() != Dive::EngineType::kCompute))
        {
            metadata_creator.OnSubmitEnd(submit_index, submit_info);
            continue;
        }

        // Assumption: A switch_buffer packet is added in between submits, which means the ce/de
        // counters will be reset in between submits. Therefore no need to track
        // markers/draws/counters across submit boundaries.
        EmulatePM4 emu;
        if (!emu.ExecuteSubmit(metadata_creator,
                               m_capture_data.GetMemoryManager(),
                               submit_index,
                               submit_info.GetNumIndirectBuffers(),
                               submit_info.GetIndirectBufferInfoPtr()))
            return false;

        metadata_creator.OnSubmitEnd(submit_index, submit_info);
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
const CaptureData &DataCore::GetCaptureData() const
{
    return m_capture_data;
}

CaptureData &DataCore::GetMutableCaptureData()
{
    return m_capture_data;
}

//--------------------------------------------------------------------------------------------------
const CommandHierarchy &DataCore::GetCommandHierarchy() const
{
    return m_capture_metadata.m_command_hierarchy;
}

//--------------------------------------------------------------------------------------------------
const CaptureMetadata &DataCore::GetCaptureMetadata() const
{
    return m_capture_metadata;
}

// =================================================================================================
// CaptureMetadataCreator
// =================================================================================================
CaptureMetadataCreator::CaptureMetadataCreator(CaptureMetadata &capture_metadata) :
    m_capture_metadata(capture_metadata)
{
    m_state_tracker.Reset();
}

//--------------------------------------------------------------------------------------------------
CaptureMetadataCreator::~CaptureMetadataCreator() {}

//--------------------------------------------------------------------------------------------------
void CaptureMetadataCreator::OnSubmitStart(uint32_t submit_index, const SubmitInfo &submit_info) {}

//--------------------------------------------------------------------------------------------------
void CaptureMetadataCreator::OnSubmitEnd(uint32_t submit_index, const SubmitInfo &submit_info) {}

//--------------------------------------------------------------------------------------------------
bool CaptureMetadataCreator::OnIbStart(uint32_t                  submit_index,
                                       uint32_t                  ib_index,
                                       const IndirectBufferInfo &ib_info,
                                       IbType                    type)
{
    return true;
}

//--------------------------------------------------------------------------------------------------
bool CaptureMetadataCreator::OnIbEnd(uint32_t                  submit_index,
                                     uint32_t                  ib_index,
                                     const IndirectBufferInfo &ib_info)
{
    return true;
}

//--------------------------------------------------------------------------------------------------
bool CaptureMetadataCreator::OnPacket(const IMemoryManager &mem_manager,
                                      uint32_t              submit_index,
                                      uint32_t              ib_index,
                                      uint64_t              va_addr,
                                      Pm4Type               type,
                                      uint32_t              header)
{
    if (!m_state_tracker.OnPacket(mem_manager, submit_index, ib_index, va_addr, type, header))
        return false;

    if (type != Pm4Type::kType7)
        return true;

    Pm4Type7Header *type7_header = (Pm4Type7Header *)&header;

    if (IsDrawDispatchBlitSyncEvent(mem_manager, submit_index, va_addr, type7_header->opcode))
    {
        // Add a new event to the EventInfo metadata array
        EventInfo event_info;
        event_info.m_submit_index = submit_index;
        if (IsDrawEventOpcode(type7_header->opcode))
            event_info.m_type = EventInfo::EventType::kDraw;
        else if (IsBlitEvent(mem_manager, submit_index, va_addr, type7_header->opcode))
            event_info.m_type = EventInfo::EventType::kBlit;
        else if (IsDispatchEventOpcode(type7_header->opcode))
            event_info.m_type = EventInfo::EventType::kDispatch;
        else
        {
            DIVE_ASSERT(GetSyncType(mem_manager, submit_index, va_addr, type7_header->opcode) !=
                        SyncType::kNone);  // Sanity check
            event_info.m_type = EventInfo::EventType::kSync;
        }

        EventStateInfo::Iterator it = m_capture_metadata.m_event_state.Add();

        m_capture_metadata.m_event_info.push_back(event_info);

        // Parse and add the shader(s) info to the metadata
        if (event_info.m_type == EventInfo::EventType::kDraw ||
            event_info.m_type == EventInfo::EventType::kDispatch)
        {
            FillEventStateInfo(it);

            if (!HandleShaders(mem_manager, submit_index, type7_header->opcode))
                return false;
        }

#if defined(ENABLE_CAPTURE_BUFFERS)
        // Parse descriptor tables, descriptors, and descriptor contents (ie: textures,
        // buffers, etc)
        ShaderReflector sr;
        SRDCallbacks    srd_callbacks;
        if (!sr.Process(srd_callbacks,
                        mem_manager,
                        m_state_tracker,
                        m_constant_engine_emu,
                        submit_index,
                        header.opcode,
                        this))
            return false;
#endif
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
bool CaptureMetadataCreator::HandleShaders(const IMemoryManager &mem_manager,
                                           uint32_t              submit_index,
                                           uint32_t              opcode)
{
    for (uint32_t shader = 0; shader < Dive::kShaderStageCount; ++shader)
    {
        bool is_dispatch = IsDispatchEventOpcode(opcode);
        bool is_valid_shader = is_dispatch && (shader == (uint32_t)ShaderStage::kShaderStageCs);
        is_valid_shader |= !is_dispatch && (shader != (uint32_t)ShaderStage::kShaderStageCs);

        EventInfo &cur_event_info = m_capture_metadata.m_event_info.back();
        uint64_t   addr = m_state_tracker.GetCurShaderAddr((ShaderStage)shader);
        if (is_valid_shader && (addr != UINT64_MAX))
        {
            // Has it been added already? If so, just add the index
            if (m_shader_addrs[shader].find(addr) != m_shader_addrs[shader].end())
            {
                cur_event_info.m_shader_indices[shader] = m_shader_addrs[shader][addr];
            }
            else
            {
                uint64_t max_size = mem_manager.GetMaxContiguousSize(submit_index, addr);
                uint8_t *data_ptr = new uint8_t[max_size];
                if (!mem_manager.CopyMemory(data_ptr, submit_index, addr, max_size))
                    return false;

                ShaderInfo shader_info;
                shader_info.m_addr = addr;
                shader_info.m_disassembly.Init(data_ptr,
                                               shader_info.m_addr,
                                               max_size,
                                               &cur_event_info.m_metadata_log);
                delete[] data_ptr;

                // Can only get the actual size after parsing the shader
                shader_info.m_size = shader_info.m_disassembly.GetShaderSize();

                uint32_t shader_index = (uint32_t)m_capture_metadata.m_shaders[shader].size();
                m_capture_metadata.m_shaders[shader].push_back(shader_info);
                m_shader_addrs[shader].insert(std::make_pair(addr, shader_index));

                // Add the shader index to the EventInfo
                cur_event_info.m_shader_indices[shader] = shader_index;
            }
        }
        else
        {
            // Shader stage not used, so add UINT32_MAX
            cur_event_info.m_shader_indices[shader] = UINT32_MAX;
        }
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
void CaptureMetadataCreator::FillEventStateInfo(EventStateInfo::Iterator event_state_it)
{
    // Vulkan states
    FillInputAssemblyState(event_state_it);
    FillTessellationState(event_state_it);
    FillViewportState(event_state_it);
    FillRasterizerState(event_state_it);
    FillMultisamplingState(event_state_it);
    FillDepthState(event_state_it);
    FillColorBlendState(event_state_it);

    // Hardware-specific non-Vulkan states
    FillHardwareSpecificStates(event_state_it);
}

//--------------------------------------------------------------------------------------------------
void CaptureMetadataCreator::FillInputAssemblyState(EventStateInfo::Iterator event_state_it)
{
    // note that primtive topology is no longer set in the context regs,
    // it is set as part of the drawcall pm4 (see tu_draw_initiator)
    uint32_t pc_primitive_cntl_reg_offset = GetRegOffsetByName("PC_PRIMITIVE_CNTL_0");
    if (m_state_tracker.IsRegSet(pc_primitive_cntl_reg_offset))
    {
        PC_PRIMITIVE_CNTL_0 primitive_raster_cntl;
        primitive_raster_cntl.u32All = m_state_tracker.GetRegValue(pc_primitive_cntl_reg_offset);
        event_state_it->SetPrimRestartEnabled(primitive_raster_cntl.bitfields.PRIMITIVE_RESTART ==
                                              1);
    }
}

//--------------------------------------------------------------------------------------------------
void CaptureMetadataCreator::FillTessellationState(EventStateInfo::Iterator event_state_it)
{
    uint32_t pc_hs_input_size_reg_offset = GetRegOffsetByName("PC_HS_INPUT_SIZE");
    if (m_state_tracker.IsRegSet(pc_hs_input_size_reg_offset))
    {
        PC_HS_INPUT_SIZE pc_hs_input_size;
        pc_hs_input_size.u32All = m_state_tracker.GetRegValue(pc_hs_input_size_reg_offset);
        event_state_it->SetPatchControlPoints(pc_hs_input_size.bitfields.SIZE);
    }
}

//--------------------------------------------------------------------------------------------------
void CaptureMetadataCreator::FillViewportState(EventStateInfo::Iterator event_state_it)
{
    // Check if viewport is set. Up to 16 of them can be set.
    uint32_t viewport_id = 0;
    uint32_t viewport_reg_start = GetRegOffsetByName("GRAS_CL_VPORT0_XOFFSET");
    DIVE_ASSERT(viewport_reg_start != kInvalidRegOffset);
    while (viewport_id < 16 && m_state_tracker.IsRegSet(viewport_reg_start + 6 * viewport_id))
    {
        GRAS_CL_VPORT_XOFFSET xOffset;
        GRAS_CL_VPORT_XSCALE  xScale;
        GRAS_CL_VPORT_YOFFSET yOffset;
        GRAS_CL_VPORT_YSCALE  yScale;
        GRAS_CL_VPORT_ZOFFSET zOffset;
        GRAS_CL_VPORT_ZSCALE  zScale;

        // Assumption: These are all set together. All-or-nothing.
        uint32_t viewport_reg = viewport_reg_start + 6 * viewport_id;
        xOffset.u32All = m_state_tracker.GetRegValue(viewport_reg);
        xScale.u32All = m_state_tracker.GetRegValue(viewport_reg + 1);
        yOffset.u32All = m_state_tracker.GetRegValue(viewport_reg + 2);
        yScale.u32All = m_state_tracker.GetRegValue(viewport_reg + 3);
        zOffset.u32All = m_state_tracker.GetRegValue(viewport_reg + 4);
        zScale.u32All = m_state_tracker.GetRegValue(viewport_reg + 5);

        VkViewport viewport;
        viewport.x = xOffset.f32All - xScale.f32All;
        viewport.width = xScale.f32All * 2.0f;
        viewport.y = yOffset.f32All - yScale.f32All;
        viewport.height = yScale.f32All * 2.0f;

        // AFAIK, z is always set to ZeroToOne, so the minDepth can be set to zOffset
        // See UniversalCmdBuffer::ValidateViewports() and DepthRange::NegativeOneToOne
        viewport.minDepth = zOffset.f32All;
        viewport.maxDepth = zScale.f32All + viewport.minDepth;
        event_state_it->SetViewport(viewport_id, viewport);

        ++viewport_id;
    }

    // Scissor
    // Check if scissor is set. Up to 16 of them can be set.
    uint16_t scissor_id = 0;
    // TODO(wangra): there is also GRAS_SC_SCREEN_SCISSOR0_TL
    uint32_t scissor_reg_start = GetRegOffsetByName("GRAS_SC_VIEWPORT_SCISSOR0_TL");
    DIVE_ASSERT(scissor_reg_start != kInvalidRegOffset);
    while (scissor_id < 16 && m_state_tracker.IsRegSet(scissor_reg_start + 2 * scissor_id))
    {
        // Assumption: These are all set together. All-or-nothing.
        GRAS_SC_VIEWPORT_SCISSOR_TL tl;
        GRAS_SC_VIEWPORT_SCISSOR_BR br;
        uint32_t                    scissor_reg = scissor_reg_start + 2 * scissor_id;
        tl.u32All = m_state_tracker.GetRegValue(scissor_reg);
        br.u32All = m_state_tracker.GetRegValue(scissor_reg + 1);

        // Note: The scissor rects set in these registers are modified by the ICD so that they do
        // not exceed viewport regions. So the exact scissor as set in Vulkan may be lost.
        // See UniversalCmdBuffer::BuildScissorRectImage
        VkRect2D rect;
        rect.offset.x = tl.bitfields.X;
        rect.offset.y = tl.bitfields.Y;
        rect.extent.width = br.bitfields.X - tl.bitfields.X;
        rect.extent.height = br.bitfields.Y - tl.bitfields.Y;
        event_state_it->SetScissor(scissor_id, rect);

        ++scissor_id;
    }
}

//--------------------------------------------------------------------------------------------------
void CaptureMetadataCreator::FillRasterizerState(EventStateInfo::Iterator event_state_it)
{
    uint32_t gras_cl_cntl_reg_offset = GetRegOffsetByName("GRAS_CL_CNTL");
    if (m_state_tracker.IsRegSet(gras_cl_cntl_reg_offset))
    {
        GRAS_CL_CNTL gras_cl_clip_cntl;
        gras_cl_clip_cntl.u32All = m_state_tracker.GetRegValue(gras_cl_cntl_reg_offset);
        event_state_it->SetDepthClampEnabled(gras_cl_clip_cntl.bitfields.Z_CLAMP_ENABLE != 1);
    }

    uint32_t pc_raster_cntl_reg_offset = GetRegOffsetByName("PC_RASTER_CNTL");
    if (m_state_tracker.IsRegSet(pc_raster_cntl_reg_offset))
    {
        PC_RASTER_CNTL pc_raster_cntl;
        pc_raster_cntl.u32All = m_state_tracker.GetRegValue(pc_raster_cntl_reg_offset);
        event_state_it->SetRasterizerDiscardEnabled(pc_raster_cntl.bitfields.DISCARD == 1);
    }

    // TODO(wangra): Should we also check VPC_POLYGON_MODE and VPC_POLYGON_MODE2?
    // what is the difference between PC and VPC?
    uint32_t pc_polygon_mode_reg_offset = GetRegOffsetByName("PC_POLYGON_MODE");
    if (m_state_tracker.IsRegSet(pc_polygon_mode_reg_offset))
    {
        PC_POLYGON_MODE pc_polygon_mode;
        pc_polygon_mode.u32All = m_state_tracker.GetRegValue(pc_polygon_mode_reg_offset);
        switch (pc_polygon_mode.bitfields.MODE)
        {
        case POLYMODE6_TRIANGLES: event_state_it->SetPolygonMode(VK_POLYGON_MODE_FILL); break;
        case POLYMODE6_LINES: event_state_it->SetPolygonMode(VK_POLYGON_MODE_LINE); break;
        case POLYMODE6_POINTS: event_state_it->SetPolygonMode(VK_POLYGON_MODE_POINT); break;
        default: DIVE_ASSERT(false);
        };
    }

    uint32_t gras_su_cntl_reg_offset = GetRegOffsetByName("GRAS_SU_CNTL");
    if (m_state_tracker.IsRegSet(gras_su_cntl_reg_offset))
    {
        GRAS_SU_CNTL gras_su_cntl;
        gras_su_cntl.u32All = m_state_tracker.GetRegValue(gras_su_cntl_reg_offset);
        VkCullModeFlags cull_mode = 0;
        cull_mode |= (gras_su_cntl.bitfields.CULL_FRONT) ? VK_CULL_MODE_FRONT_BIT : 0;
        cull_mode |= (gras_su_cntl.bitfields.CULL_BACK) ? VK_CULL_MODE_BACK_BIT : 0;
        event_state_it->SetCullMode(cull_mode);

        VkFrontFace front_face = VK_FRONT_FACE_CLOCKWISE;
        if (gras_su_cntl.bitfields.FRONT_CW == 0)
            front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        event_state_it->SetFrontFace(front_face);

        event_state_it->SetDepthBiasEnabled(gras_su_cntl.bitfields.POLY_OFFSET == 1);
        event_state_it->SetLineWidth(gras_su_cntl.bitfields.LINEHALFWIDTH * 2.f);
    }

    uint32_t gras_su_poly_offset_offset_reg_offset = GetRegOffsetByName(
    "GRAS_SU_POLY_OFFSET_OFFSET");
    if (m_state_tracker.IsRegSet(gras_su_poly_offset_offset_reg_offset))
    {
        GRAS_SU_POLY_OFFSET_OFFSET gras_su_poly_offset_offset;
        gras_su_poly_offset_offset.u32All = m_state_tracker.GetRegValue(
        gras_su_poly_offset_offset_reg_offset);
        event_state_it->SetDepthBiasConstantFactor(gras_su_poly_offset_offset.f32All);
    }

    uint32_t gras_su_poly_offset_clamp_reg_offset = GetRegOffsetByName(
    "GRAS_SU_POLY_OFFSET_OFFSET_CLAMP");
    if (m_state_tracker.IsRegSet(gras_su_poly_offset_clamp_reg_offset))
    {
        GRAS_SU_POLY_OFFSET_OFFSET_CLAMP gras_su_poly_offset_clamp;
        gras_su_poly_offset_clamp.u32All = m_state_tracker.GetRegValue(
        gras_su_poly_offset_clamp_reg_offset);
        event_state_it->SetDepthBiasClamp(gras_su_poly_offset_clamp.f32All);
    }

    uint32_t gras_su_poly_offset_scale_reg_offset = GetRegOffsetByName("GRAS_SU_POLY_OFFSET_SCALE");
    if (m_state_tracker.IsRegSet(gras_su_poly_offset_scale_reg_offset))
    {
        GRAS_SU_POLY_OFFSET_SCALE gras_su_poly_offset_scale;
        gras_su_poly_offset_scale.u32All = m_state_tracker.GetRegValue(
        gras_su_poly_offset_scale_reg_offset);
        event_state_it->SetDepthBiasSlopeFactor(gras_su_poly_offset_scale.f32All);
    }
}

//--------------------------------------------------------------------------------------------------
void CaptureMetadataCreator::FillMultisamplingState(EventStateInfo::Iterator event_state_it)
{
    uint32_t gras_ras_msaa_cntl_reg_offset = GetRegOffsetByName("GRAS_RAS_MSAA_CNTL");
    if (m_state_tracker.IsRegSet(gras_ras_msaa_cntl_reg_offset))
    {
        GRAS_RAS_MSAA_CNTL gras_ras_msaa_cntl;
        gras_ras_msaa_cntl.u32All = m_state_tracker.GetRegValue(gras_ras_msaa_cntl_reg_offset);
        uint32_t ras_samples = 1 << gras_ras_msaa_cntl.bitfields.SAMPLES;
        event_state_it->SetRasterizationSamples((VkSampleCountFlagBits)(ras_samples));
    }

    // TODO(wangra): do we need to check SP_BLEND_CNTL?
    uint32_t rb_blend_cntl_reg_offset = GetRegOffsetByName("RB_BLEND_CNTL");
    if (m_state_tracker.IsRegSet(rb_blend_cntl_reg_offset))
    {
        RB_BLEND_CNTL rb_blend_cntl;
        rb_blend_cntl.u32All = m_state_tracker.GetRegValue(rb_blend_cntl_reg_offset);
        bool enabled = (rb_blend_cntl.bitfields.ALPHA_TO_COVERAGE != 1);
        event_state_it->SetAlphaToCoverageEnabled(enabled);
    }
}

//--------------------------------------------------------------------------------------------------
void CaptureMetadataCreator::FillDepthState(EventStateInfo::Iterator event_state_it)
{
    uint32_t rb_depth_cntl_reg_offset = GetRegOffsetByName("RB_DEPTH_CNTL");
    if (m_state_tracker.IsRegSet(rb_depth_cntl_reg_offset))
    {
        RB_DEPTH_CNTL rb_depth_cntl;
        rb_depth_cntl.u32All = m_state_tracker.GetRegValue(rb_depth_cntl_reg_offset);
        event_state_it->SetDepthTestEnabled(rb_depth_cntl.bitfields.Z_TEST_ENABLE);
        event_state_it->SetDepthWriteEnabled(rb_depth_cntl.bitfields.Z_WRITE_ENABLE);
        // Be careful!!! Here we assume the enum `VkCompareOp` matches exactly `adreno_compare_func`
        event_state_it->SetDepthCompareOp(static_cast<VkCompareOp>(rb_depth_cntl.bitfields.ZFUNC));
        event_state_it->SetDepthBoundsTestEnabled(rb_depth_cntl.bitfields.Z_BOUNDS_ENABLE == 1);

        {
            uint32_t rb_z_bounds_min_reg_offset = GetRegOffsetByName("RB_Z_BOUNDS_MIN");
            if (m_state_tracker.IsRegSet(rb_z_bounds_min_reg_offset))
            {
                RB_Z_BOUNDS_MIN rb_z_bounds_min;
                rb_z_bounds_min.u32All = m_state_tracker.GetRegValue(rb_z_bounds_min_reg_offset);
                event_state_it->SetMinDepthBounds(rb_z_bounds_min.f32All);
            }
            uint32_t rb_z_bounds_max_reg_offset = GetRegOffsetByName("RB_Z_BOUNDS_MAX");
            if (m_state_tracker.IsRegSet(rb_z_bounds_max_reg_offset))
            {
                RB_Z_BOUNDS_MAX rb_z_bounds_max;
                rb_z_bounds_max.u32All = m_state_tracker.GetRegValue(rb_z_bounds_max_reg_offset);
                event_state_it->SetMaxDepthBounds(rb_z_bounds_max.f32All);
            }
        }
    }

    uint32_t rb_stencil_cntl_reg_offset = GetRegOffsetByName("RB_STENCIL_CONTROL");
    if (m_state_tracker.IsRegSet(rb_stencil_cntl_reg_offset))
    {
        RB_STENCIL_CONTROL rb_stencil_cntl;
        rb_stencil_cntl.u32All = m_state_tracker.GetRegValue(rb_stencil_cntl_reg_offset);

        const bool is_stencil_front_enabled = (rb_stencil_cntl.bitfields.STENCIL_ENABLE == 1);
        // TODO(wangra): we are missing the funciton to set if stencil is enabled for back face
        // const bool is_stencil_back_enabled = (rb_stencil_cntl.bitfields.STENCIL_ENABLE_BF == 1);
        event_state_it->SetStencilTestEnabled(is_stencil_front_enabled);
        VkStencilOpState front;
        VkStencilOpState back;

        // Be careful!!! Here we assume the enum `VkStencilOp` matches exactly `adreno_stencil_op`
        front.failOp = static_cast<VkStencilOp>(rb_stencil_cntl.bitfields.FAIL);
        front.passOp = static_cast<VkStencilOp>(rb_stencil_cntl.bitfields.ZPASS);
        front.depthFailOp = static_cast<VkStencilOp>(rb_stencil_cntl.bitfields.ZFAIL);
        // Be careful!!! Here we assume the enum `VkCompareOp` matches exactly `adreno_compare_func`
        front.compareOp = static_cast<VkCompareOp>(rb_stencil_cntl.bitfields.FUNC);

        back.failOp = static_cast<VkStencilOp>(rb_stencil_cntl.bitfields.FAIL_BF);
        back.passOp = static_cast<VkStencilOp>(rb_stencil_cntl.bitfields.ZPASS_BF);
        back.depthFailOp = static_cast<VkStencilOp>(rb_stencil_cntl.bitfields.ZFAIL_BF);
        // Be careful!!! Here we assume the enum `VkCompareOp` matches exactly `adreno_compare_func`
        back.compareOp = static_cast<VkCompareOp>(rb_stencil_cntl.bitfields.FUNC_BF);

        uint32_t rb_stencilref_reg_offset = GetRegOffsetByName("RB_STENCILREF");
        if (m_state_tracker.IsRegSet(rb_stencilref_reg_offset))
        {
            RB_STENCILREF rb_stencilref;
            rb_stencilref.u32All = m_state_tracker.GetRegValue(rb_stencilref_reg_offset);
            front.reference = rb_stencilref.bitfields.REF;
            back.reference = rb_stencilref.bitfields.BFREF;
        }

        uint32_t rb_stencilmask_reg_offset = GetRegOffsetByName("RB_STENCILMASK");
        if (m_state_tracker.IsRegSet(rb_stencilmask_reg_offset))
        {
            RB_STENCILMASK rb_stencilmask;
            rb_stencilmask.u32All = m_state_tracker.GetRegValue(rb_stencilmask_reg_offset);
            front.compareMask = rb_stencilmask.bitfields.MASK;
            back.compareMask = rb_stencilmask.bitfields.BFMASK;
        }
        uint32_t rb_stencilwrmask_reg_offset = GetRegOffsetByName("RB_STENCILWRMASK");
        if (m_state_tracker.IsRegSet(rb_stencilwrmask_reg_offset))
        {
            RB_STENCILWRMASK rb_stencilwrmask;
            rb_stencilwrmask.u32All = m_state_tracker.GetRegValue(rb_stencilwrmask_reg_offset);
            front.writeMask = rb_stencilwrmask.bitfields.WRMASK;
            back.writeMask = rb_stencilwrmask.bitfields.BFWRMASK;
        }
        event_state_it->SetStencilOpStateFront(front);
        event_state_it->SetStencilOpStateBack(back);
    }
}

//--------------------------------------------------------------------------------------------------
void CaptureMetadataCreator::FillColorBlendState(EventStateInfo::Iterator event_state_it)
{
    uint32_t rt_id = 0;
    uint32_t rb_mrt_reg_start = GetRegOffsetByName("RB_MRT0_CONTROL");
    DIVE_ASSERT(rb_mrt_reg_start != kInvalidRegOffset);
    constexpr uint32_t kElemCount = 8;
    while (rt_id < 8 && m_state_tracker.IsRegSet(rb_mrt_reg_start + kElemCount * rt_id))
    {
        RB_MRT_CONTROL       rb_mrt_control;
        RB_MRT_BLEND_CONTROL rb_mrt_blend_control;

        // Assumption: These are all set together. All-or-nothing.
        const uint32_t mrt_reg = rb_mrt_reg_start + kElemCount * rt_id;
        rb_mrt_control.u32All = m_state_tracker.GetRegValue(mrt_reg);
        rb_mrt_blend_control.u32All = m_state_tracker.GetRegValue(mrt_reg + 1);

        VkPipelineColorBlendAttachmentState attach;
        attach.blendEnable = (rb_mrt_control.bitfields.BLEND == 1) ? VK_TRUE : VK_FALSE;

        // Be careful!!! Here we assume the enum `VkBlendFactor` matches exactly
        // `adreno_rb_blend_factor`
        attach.srcColorBlendFactor = static_cast<VkBlendFactor>(
        rb_mrt_blend_control.bitfields.RGB_SRC_FACTOR);
        // Be careful!!! Here we assume the enum `VkBlendFactor` matches exactly
        // `adreno_rb_blend_factor`
        attach.dstColorBlendFactor = static_cast<VkBlendFactor>(
        rb_mrt_blend_control.bitfields.RGB_DEST_FACTOR);
        // Be careful!!! Here we assume the enum `VkBlendOp` matches exactly `a3xx_rb_blend_opcode`
        attach.colorBlendOp = static_cast<VkBlendOp>(
        rb_mrt_blend_control.bitfields.RGB_BLEND_OPCODE);
        // Be careful!!! Here we assume the enum `VkBlendFactor` matches exactly
        // `adreno_rb_blend_factor`
        attach.srcAlphaBlendFactor = static_cast<VkBlendFactor>(
        rb_mrt_blend_control.bitfields.ALPHA_SRC_FACTOR);
        // Be careful!!! Here we assume the enum `VkBlendFactor` matches exactly
        // `adreno_rb_blend_factor`
        attach.dstAlphaBlendFactor = static_cast<VkBlendFactor>(
        rb_mrt_blend_control.bitfields.ALPHA_DEST_FACTOR);
        // Be careful!!! Here we assume the enum `VkBlendOp` matches exactly `a3xx_rb_blend_opcode`
        attach.alphaBlendOp = static_cast<VkBlendOp>(
        rb_mrt_blend_control.bitfields.ALPHA_BLEND_OPCODE);
        event_state_it->SetAttachment(rt_id, attach);

        ++rt_id;
    }

    // TODO(wangra): rop code should be set per MR
    // if (m_state_tracker.IsContextStateSet(mmCB_COLOR_CONTROL))
    //{
    //    CB_COLOR_CONTROL cb_color_control;
    //    cb_color_control.u32All = m_state_tracker.GetContextRegData(mmCB_COLOR_CONTROL);

    //    VkLogicOp op = VK_LOGIC_OP_MAX_ENUM;
    //    switch (cb_color_control.bits.ROP3)
    //    {
    //    case 0x00: op = VK_LOGIC_OP_CLEAR; break;
    //    case 0x88: op = VK_LOGIC_OP_AND; break;
    //    case 0x44: op = VK_LOGIC_OP_AND_REVERSE; break;
    //    case 0xCC: op = VK_LOGIC_OP_COPY; break;
    //    case 0x22: op = VK_LOGIC_OP_AND_INVERTED; break;
    //    case 0xAA: op = VK_LOGIC_OP_NO_OP; break;
    //    case 0x66: op = VK_LOGIC_OP_XOR; break;
    //    case 0xEE: op = VK_LOGIC_OP_OR; break;
    //    case 0x11: op = VK_LOGIC_OP_NOR; break;
    //    case 0x99: op = VK_LOGIC_OP_EQUIVALENT; break;
    //    case 0x55: op = VK_LOGIC_OP_INVERT; break;
    //    case 0xDD: op = VK_LOGIC_OP_OR_REVERSE; break;
    //    case 0x33: op = VK_LOGIC_OP_COPY_INVERTED; break;
    //    case 0xBB: op = VK_LOGIC_OP_OR_INVERTED; break;
    //    case 0x77: op = VK_LOGIC_OP_NAND; break;
    //    case 0xFF: op = VK_LOGIC_OP_SET; break;
    //    };
    //    event_state_it->SetLogicOp(op);

    //    // Setting op to COPY is equivalent to disabling the logic-op, since it's the default
    //    // behavior. In fact, from PM4 perspective, the 2 are indistinguishable
    //    event_state_it->SetLogicOpEnabled(op != VK_LOGIC_OP_COPY);
    //}

    // Assumption: The ICD sets all of the color channels together. So it is enough
    // to check whether just 1 of them is set or not. It's all or nothing.
    uint32_t rb_blend_red_reg_offset = GetRegOffsetByName("RB_BLEND_RED_F32");
    if (m_state_tracker.IsRegSet(rb_blend_red_reg_offset))
    {
        uint32_t rb_blend_red_green_offset = GetRegOffsetByName("RB_BLEND_GREEN_F32");
        uint32_t rb_blend_red_blue_offset = GetRegOffsetByName("RB_BLEND_BLUE_F32");
        uint32_t rb_blend_red_alpha_offset = GetRegOffsetByName("RB_BLEND_ALPHA_F32");

        RB_BLEND_RED_F32   rb_blend_red;
        RB_BLEND_GREEN_F32 rb_blend_green;
        RB_BLEND_BLUE_F32  rb_blend_blue;
        RB_BLEND_ALPHA_F32 rb_blend_alpha;
        rb_blend_red.u32All = m_state_tracker.GetRegValue(rb_blend_red_reg_offset);
        rb_blend_green.u32All = m_state_tracker.GetRegValue(rb_blend_red_green_offset);
        rb_blend_blue.u32All = m_state_tracker.GetRegValue(rb_blend_red_blue_offset);
        rb_blend_alpha.u32All = m_state_tracker.GetRegValue(rb_blend_red_alpha_offset);
        event_state_it->SetBlendConstant(0, rb_blend_red.f32All);
        event_state_it->SetBlendConstant(1, rb_blend_green.f32All);
        event_state_it->SetBlendConstant(2, rb_blend_blue.f32All);
        event_state_it->SetBlendConstant(3, rb_blend_alpha.f32All);
    }
}

//--------------------------------------------------------------------------------------------------
void CaptureMetadataCreator::FillHardwareSpecificStates(EventStateInfo::Iterator event_state_it) {}

}  // namespace Dive
