
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
#include <optional>
#include "pm4_info.h"

namespace Dive
{

// =================================================================================================
// DataCore
// =================================================================================================

//--------------------------------------------------------------------------------------------------
DataCore::DataCore(ProgressTracker *progress_tracker) :
    m_progress_tracker(progress_tracker)
{
}

//--------------------------------------------------------------------------------------------------
CaptureData::LoadResult DataCore::LoadDiveCaptureData(const std::string &file_name)
{
    std::filesystem::path rd_file_path(file_name);
    rd_file_path.replace_extension(".rd");
    m_capture_metadata = CaptureMetadata();
    return m_dive_capture_data.LoadFiles(rd_file_path.string(), file_name);
}

//--------------------------------------------------------------------------------------------------
CaptureData::LoadResult DataCore::LoadPm4CaptureData(const std::string &file_name)
{

    m_pm4_capture_data = Pm4CaptureData(m_progress_tracker);  // Clear any previously loaded data
    m_capture_metadata = CaptureMetadata();
    return m_pm4_capture_data.LoadCaptureFile(file_name);
}

//--------------------------------------------------------------------------------------------------
CaptureData::LoadResult DataCore::LoadGfxrCaptureData(const std::string &file_name)
{
    m_gfxr_capture_data = GfxrCaptureData();
    return m_gfxr_capture_data.LoadCaptureFile(file_name);
}

//--------------------------------------------------------------------------------------------------
bool DataCore::CreateDiveCommandHierarchy()
{
    std::unique_ptr<EmulateStateTracker> state_tracker(new EmulateStateTracker);

    // Optional: Reserve the internal vectors based on the number of pm4 packets in the capture
    // This is an educated guess that each PM4 packet results in x number of associated
    // field/register nodes. Overguessing means more memory used during creation. Underguessing
    // means more allocations. For big captures, this is easily in the multi-millions, so
    // pre-reserving the space is a signficiant performance win
    uint64_t reserve_size = m_capture_metadata.m_num_pm4_packets * 10;

    // Command hierarchy tree creation

    DiveCommandHierarchyCreator cmd_hier_creator(m_capture_metadata.m_command_hierarchy);
    if (!cmd_hier_creator.CreateTrees(m_capture_metadata.m_command_hierarchy,
                                      m_dive_capture_data,
                                      true,
                                      reserve_size))
    {
        return false;
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
bool DataCore::CreatePm4CommandHierarchy()
{
    std::unique_ptr<EmulateStateTracker> state_tracker(new EmulateStateTracker);

    // Optional: Reserve the internal vectors based on the number of pm4 packets in the capture
    // This is an educated guess that each PM4 packet results in x number of associated
    // field/register nodes. Overguessing means more memory used during creation. Underguessing
    // means more allocations. For big captures, this is easily in the multi-millions, so
    // pre-reserving the space is a signficiant performance win
    uint64_t reserve_size = m_capture_metadata.m_num_pm4_packets * 10;

    // Command hierarchy tree creation
    CommandHierarchyCreator cmd_hier_creator(m_capture_metadata.m_command_hierarchy,
                                             m_pm4_capture_data);
    if (!cmd_hier_creator.CreateTrees(m_pm4_capture_data, true, reserve_size))
    {
        return false;
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
bool DataCore::CreateGfxrCommandHierarchy()
{
    GfxrVulkanCommandHierarchyCreator vk_cmd_creator(m_capture_metadata.m_command_hierarchy,
                                                     m_gfxr_capture_data);
    if (!vk_cmd_creator.CreateTrees(false))
    {
        return false;
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
bool DataCore::CreateDiveMetaData()
{
    CaptureMetadataCreator metadata_creator(m_capture_metadata);
    if (!metadata_creator
         .ProcessSubmits(m_dive_capture_data.GetPm4CaptureData().GetSubmits(),
                         m_dive_capture_data.GetPm4CaptureData().GetMemoryManager()))
    {
        return false;
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
bool DataCore::CreatePm4MetaData()
{
    CaptureMetadataCreator metadata_creator(m_capture_metadata);
    if (!metadata_creator.ProcessSubmits(m_pm4_capture_data.GetSubmits(),
                                         m_pm4_capture_data.GetMemoryManager()))
    {
        return false;
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
bool DataCore::ParseDiveCaptureData()
{
    if (m_progress_tracker)
    {
        m_progress_tracker->sendMessage("Processing command buffers...");
    }

    if (!CreateDiveMetaData())
    {
        return false;
    }

    if (!CreateDiveCommandHierarchy())
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool DataCore::ParsePm4CaptureData()
{
    if (m_progress_tracker)
    {
        m_progress_tracker->sendMessage("Processing command buffers...");
    }

    if (!CreatePm4MetaData())
    {
        return false;
    }

    if (!CreatePm4CommandHierarchy())
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
bool DataCore::ParseGfxrCaptureData()
{
    if (m_progress_tracker)
    {
        m_progress_tracker->sendMessage("Processing gfxr commands...");
    }

    if (!CreateGfxrCommandHierarchy())
    {
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
const Pm4CaptureData &DataCore::GetPm4CaptureData() const
{
    return m_pm4_capture_data;
}

//--------------------------------------------------------------------------------------------------
Pm4CaptureData &DataCore::GetMutablePm4CaptureData()
{
    return m_pm4_capture_data;
}

//--------------------------------------------------------------------------------------------------
const GfxrCaptureData &DataCore::GetGfxrCaptureData() const
{
    return m_gfxr_capture_data;
}

//--------------------------------------------------------------------------------------------------
GfxrCaptureData &DataCore::GetMutableGfxrCaptureData()
{
    return m_gfxr_capture_data;
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
    m_capture_metadata.m_num_pm4_packets = 0;
}

//--------------------------------------------------------------------------------------------------
CaptureMetadataCreator::~CaptureMetadataCreator() {}

//--------------------------------------------------------------------------------------------------
void CaptureMetadataCreator::OnSubmitStart(uint32_t submit_index, const SubmitInfo &submit_info)
{
    m_state_tracker.Reset();
    m_current_render_mode = RenderModeType::kUnknown;
}

//--------------------------------------------------------------------------------------------------
void CaptureMetadataCreator::OnSubmitEnd(uint32_t submit_index, const SubmitInfo &submit_info) {}

//--------------------------------------------------------------------------------------------------
bool CaptureMetadataCreator::OnIbStart(uint32_t                  submit_index,
                                       uint32_t                  ib_index,
                                       const IndirectBufferInfo &ib_info,
                                       IbType                    type)
{
    EmulateCallbacksBase::OnIbStart(submit_index, ib_index, ib_info, type);
    return true;
}

//--------------------------------------------------------------------------------------------------
bool CaptureMetadataCreator::OnIbEnd(uint32_t                  submit_index,
                                     uint32_t                  ib_index,
                                     const IndirectBufferInfo &ib_info)
{
    EmulateCallbacksBase::OnIbEnd(submit_index, ib_index, ib_info);
    return true;
}

//--------------------------------------------------------------------------------------------------
bool CaptureMetadataCreator::OnPacket(const IMemoryManager &mem_manager,
                                      uint32_t              submit_index,
                                      uint32_t              ib_index,
                                      uint64_t              va_addr,
                                      Pm4Header             header)
{
    m_capture_metadata.m_num_pm4_packets++;
    if (!EmulateCallbacksBase::OnPacket(mem_manager, submit_index, ib_index, va_addr, header))
        return false;

    if (header.type != 7)
        return true;

    Pm4Type7Header *type7_header = (Pm4Type7Header *)&header;

    if (type7_header->opcode == CP_SET_MARKER)
    {
        PM4_CP_SET_MARKER packet;
        DIVE_VERIFY(mem_manager.RetrieveMemoryData(&packet, submit_index, va_addr, sizeof(packet)));
        // as mentioned in adreno_pm4.xml, only b0-b3 are considered when b8 is not set
        DIVE_ASSERT((packet.u32All0 & 0x100) == 0);
        a6xx_marker marker = static_cast<a6xx_marker>(packet.u32All0 & 0xf);

        // TODO(wangra): find a way to remove the duplicatation in CommandHierarchyCreator::OnPacket
        switch (marker)
        {
            // This is emitted at the begining of the render pass if tiled rendering mode is
            // disabled
        case RM6_BYPASS:
            m_current_render_mode = RenderModeType::kDirect;
            break;
            // This is emitted at the begining of the binning pass, although the binning pass
            // could be missing even in tiled rendering mode
        case RM6_BINNING:
            m_current_render_mode = RenderModeType::kBinning;
            break;
            // This is emitted at the begining of the tiled rendering pass
        case RM6_GMEM:
            m_current_render_mode = RenderModeType::kTiled;
            break;
            // This is emitted at the end of the tiled rendering pass
        case RM6_ENDVIS:
            // should be paired with RM6_GMEM only if RM6_BINNING exist, end of tiled mode
            m_current_render_mode = RenderModeType::kUnknown;
            break;
            // This is emitted at the begining of the resolve pass
        case RM6_RESOLVE:
            m_current_render_mode = RenderModeType::kResolve;
            break;
            // This is emitted for each dispatch
        case RM6_COMPUTE:
            m_current_render_mode = RenderModeType::kDispatch;
            break;
        // This seems to be the end of Resolve Pass
        case RM6_YIELD:
            // should be paired with RM6_RESOLVE, end of resolve pass
            m_current_render_mode = RenderModeType::kUnknown;
            break;
        case RM6_BLIT2DSCALE:
        case RM6_IB1LIST_START:
        case RM6_IB1LIST_END:
        default:
            m_current_render_mode = RenderModeType::kUnknown;
            break;
        }
    }

    if (Util::IsEvent(mem_manager, submit_index, va_addr, type7_header->opcode, m_state_tracker))
    {
        // Add a new event to the EventInfo metadata array
        EventInfo event_info = {};
        event_info.m_submit_index = submit_index;
        if (IsDrawEventOpcode(type7_header->opcode))
        {
            event_info.m_type = EventInfo::EventType::kDraw;
            event_info.m_num_indices = Util::GetIndexCount(mem_manager,
                                                           submit_index,
                                                           va_addr,
                                                           *type7_header);
        }
        else if (IsDispatchEventOpcode(type7_header->opcode))
            event_info.m_type = EventInfo::EventType::kDispatch;
        else if (type7_header->opcode == CP_BLIT)
            event_info.m_type = EventInfo::EventType::kBlit;
        else
        {
            SyncType sync_type = Util::GetSyncType(mem_manager,
                                                   submit_index,
                                                   va_addr,
                                                   type7_header->opcode,
                                                   m_state_tracker);

            switch (sync_type)
            {
            case SyncType::kGmemToSysMemResolve:
                event_info.m_type = EventInfo::EventType::kGmemToSysmemResolve;
                break;
            case SyncType::kGmemToSysMemResolveAndClearGmem:
                event_info.m_type = EventInfo::EventType::kGmemToSysMemResolveAndClearGmem;
                break;
            case SyncType::kClearGmem:
                event_info.m_type = EventInfo::EventType::kClearGmem;
                break;
            case SyncType::kSysMemToGmemResolve:
                event_info.m_type = EventInfo::EventType::kSysmemToGmemResolve;
                break;
            case SyncType::kWaitMemWrites:
                event_info.m_type = EventInfo::EventType::kWaitMemWrites;
                break;
            case SyncType::kWaitForIdle:
                event_info.m_type = EventInfo::EventType::kWaitForIdle;
                break;
            case SyncType::kWaitForMe:
                event_info.m_type = EventInfo::EventType::kWaitForMe;
                break;
            case SyncType::kEventWriteStart:
                event_info.m_type = EventInfo::EventType::kEventWriteStart;
                break;
            case SyncType::kEventWriteEnd:
                event_info.m_type = EventInfo::EventType::kEventWriteEnd;
                break;
            default:
                DIVE_ASSERT(false);  // Sanity check
                break;
            }
        }

        EventStateInfo::Iterator it = m_capture_metadata.m_event_state.Add();

        event_info.m_render_mode = m_current_render_mode;
        event_info.m_str = Util::GetEventString(mem_manager,
                                                submit_index,
                                                va_addr,
                                                *type7_header,
                                                m_state_tracker);

        m_capture_metadata.m_event_info.push_back(event_info);

        // Parse and add the shader(s) info to the metadata
        if (event_info.m_type == EventInfo::EventType::kDraw ||
            event_info.m_type == EventInfo::EventType::kDispatch)
        {
            if (event_info.m_type == EventInfo::EventType::kDraw)
            {
                FillEventStateInfo(it);
            }

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

        for (uint32_t enable_index = 0; enable_index < kShaderEnableBitCount; ++enable_index)
        {
            uint32_t enable_mask = 1u << enable_index;

            uint64_t addr = m_state_tracker.GetCurShaderAddr((ShaderStage)shader, enable_mask);

            // TODO(wangra): need to investigate why `addr` could be 0 here
            if (is_valid_shader && (addr != UINT64_MAX) && (addr != 0))
            {
                // Check if we've already seen a shader at this address, in which case we just need
                // to reference the existing shader.
                if (m_shader_addrs.find(addr) != m_shader_addrs.end())
                {
                    uint32_t shader_index = m_shader_addrs[addr];
                    {
                        // Check if this event already has a reference to this shader, in which case
                        // we just add to the existing reference's enable mask.
                        bool found = false;
                        for (auto &reference : cur_event_info.m_shader_references)
                        {
                            if (reference.m_shader_index == shader_index)
                            {
                                reference.m_enable_mask |= enable_mask;
                                found = true;
                            }
                        }
                        if (found)
                            continue;
                    }
                    ShaderReference reference;
                    reference.m_shader_index = shader_index;
                    reference.m_stage = (ShaderStage)shader;
                    reference.m_enable_mask = enable_mask;
                    cur_event_info.m_shader_references.push_back(reference);
                }
                else
                {
                    // We haven't seen this shader address before, so we need to create the shader
                    // info.
                    uint32_t    shader_index = (uint32_t)m_capture_metadata.m_shaders.size();
                    Disassembly shader_info(mem_manager,
                                            submit_index,
                                            addr,
                                            &cur_event_info.m_metadata_log);
                    m_capture_metadata.m_shaders.push_back(shader_info);
                    m_shader_addrs.insert(std::make_pair(addr, shader_index));

                    // Add the shader index to the EventInfo
                    ShaderReference reference;
                    reference.m_shader_index = m_shader_addrs[addr];
                    reference.m_stage = (ShaderStage)shader;
                    reference.m_enable_mask = enable_mask;
                    cur_event_info.m_shader_references.push_back(reference);
                }
            }
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
        case POLYMODE6_TRIANGLES:
            event_state_it->SetPolygonMode(VK_POLYGON_MODE_FILL);
            break;
        case POLYMODE6_LINES:
            event_state_it->SetPolygonMode(VK_POLYGON_MODE_LINE);
            break;
        case POLYMODE6_POINTS:
            event_state_it->SetPolygonMode(VK_POLYGON_MODE_POINT);
            break;
        default:
            DIVE_ASSERT(false);
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
    uint32_t rb_mrt_ctl_reg_start = GetRegOffsetByName("RB_MRT0_CONTROL");
    DIVE_ASSERT(rb_mrt_ctl_reg_start != kInvalidRegOffset);
    constexpr uint32_t kElemCount = 8;
    while (rt_id < 8 && m_state_tracker.IsRegSet(rb_mrt_ctl_reg_start + kElemCount * rt_id))
    {
        RB_MRT_CONTROL       rb_mrt_control;
        RB_MRT_BLEND_CONTROL rb_mrt_blend_control;

        // Assumption: These are all set together. All-or-nothing.
        const uint32_t mrt_reg = rb_mrt_ctl_reg_start + kElemCount * rt_id;
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

        const bool logic_op_enabled = (rb_mrt_control.bitfields.ROP_ENABLE == 1);
        // Be careful!!! Here we assume the enum `VkLogicOp` matches exactly `a3xx_rop_code`
        const VkLogicOp op = static_cast<VkLogicOp>(rb_mrt_control.bitfields.ROP_CODE);
        event_state_it->SetLogicOpEnabled(rt_id, logic_op_enabled);
        event_state_it->SetLogicOp(rt_id, op);

        ++rt_id;
    }

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
void CaptureMetadataCreator::FillHardwareSpecificStates(EventStateInfo::Iterator event_state_it)
{
    // LRZ related
    uint32_t gras_lrz_cntl_reg_offset = GetRegOffsetByName("GRAS_LRZ_CNTL");
    if (m_state_tracker.IsRegSet(gras_lrz_cntl_reg_offset))
    {
        GRAS_LRZ_CNTL gras_lrz_cntl;
        gras_lrz_cntl.u32All = m_state_tracker.GetRegValue(gras_lrz_cntl_reg_offset);
        const auto                bitfields = gras_lrz_cntl.bitfields;
        const bool                lrz_enabled = bitfields.ENABLE;
        const bool                lrz_write = bitfields.LRZ_WRITE;
        const a6xx_lrz_dir_status lrz_dir_status = bitfields.DIR;
        const bool                lrz_dir_write = bitfields.DIR_WRITE;

        event_state_it->SetLRZEnabled(lrz_enabled);
        if (lrz_enabled)
        {
            event_state_it->SetLRZWrite(lrz_write);
            event_state_it->SetLRZDirStatus(lrz_dir_status);
            event_state_it->SetLRZDirWrite(lrz_dir_write);
        }
    }

    uint32_t gras_su_depth_plane_cntl_reg_offset = GetRegOffsetByName("GRAS_SU_DEPTH_PLANE_CNTL");
    if (m_state_tracker.IsRegSet(gras_su_depth_plane_cntl_reg_offset))
    {
        GRAS_SU_DEPTH_PLANE_CNTL gras_su_depth_plane_cntl;
        gras_su_depth_plane_cntl.u32All = m_state_tracker.GetRegValue(
        gras_su_depth_plane_cntl_reg_offset);
        a6xx_ztest_mode ztest_mode = gras_su_depth_plane_cntl.bitfields.Z_MODE;
        event_state_it->SetZTestMode(ztest_mode);
    }

    // binning related
    uint32_t gras_bin_cntl_reg_offset = GetRegOffsetByName("GRAS_BIN_CONTROL");
    if (m_state_tracker.IsRegSet(gras_bin_cntl_reg_offset))
    {
        const RegInfo  *reg_info = GetRegInfo(gras_bin_cntl_reg_offset);
        const RegField *reg_field_w = GetRegFieldByName("BINW", reg_info);
        DIVE_ASSERT(reg_field_w != nullptr);
        const RegField *reg_field_h = GetRegFieldByName("BINH", reg_info);
        DIVE_ASSERT(reg_field_h != nullptr);
        GRAS_BIN_CONTROL gras_bin_cntl;
        gras_bin_cntl.u32All = m_state_tracker.GetRegValue(gras_bin_cntl_reg_offset);
        const auto             bitfields = gras_bin_cntl.bitfields;
        const uint32_t         binw = bitfields.BINW << reg_field_w->m_shr;
        const uint32_t         binh = bitfields.BINH << reg_field_h->m_shr;
        const a6xx_render_mode render_mode = bitfields.RENDER_MODE;

        event_state_it->SetBinW(binw);
        event_state_it->SetBinH(binh);
        event_state_it->SetRenderMode(render_mode);

        // this is only available on a6xx
        if (IsFieldEnabled(GetRegFieldByName("BUFFERS_LOCATION", reg_info)))
        {
            const a6xx_buffers_location buffers_location = bitfields.BUFFERS_LOCATION;
            event_state_it->SetBuffersLocation(buffers_location);
        }
    }
    uint32_t gras_sc_window_scissor_tl_reg_offset = GetRegOffsetByName("GRAS_SC_WINDOW_SCISSOR_TL");
    if (m_state_tracker.IsRegSet(gras_sc_window_scissor_tl_reg_offset))
    {
        GRAS_SC_WINDOW_SCISSOR_TL gras_sc_window_scissor_tl;
        gras_sc_window_scissor_tl.u32All = m_state_tracker.GetRegValue(
        gras_sc_window_scissor_tl_reg_offset);
        event_state_it->SetWindowScissorTLX(gras_sc_window_scissor_tl.bitfields.X);
        event_state_it->SetWindowScissorTLY(gras_sc_window_scissor_tl.bitfields.Y);
    }
    uint32_t gras_sc_window_scissor_br_reg_offset = GetRegOffsetByName("GRAS_SC_WINDOW_SCISSOR_BR");
    if (m_state_tracker.IsRegSet(gras_sc_window_scissor_br_reg_offset))
    {
        GRAS_SC_WINDOW_SCISSOR_BR gras_sc_window_scissor_br;
        gras_sc_window_scissor_br.u32All = m_state_tracker.GetRegValue(
        gras_sc_window_scissor_br_reg_offset);
        event_state_it->SetWindowScissorBRX(gras_sc_window_scissor_br.bitfields.X);
        event_state_it->SetWindowScissorBRY(gras_sc_window_scissor_br.bitfields.Y);
    }

    // helper lane related
    uint32_t sp_fs_ctrl_reg_offset = GetRegOffsetByName("SP_FS_CTRL_REG0");
    if (m_state_tracker.IsRegSet(sp_fs_ctrl_reg_offset))
    {
        SP_FS_CTRL_REG0 sp_fs_ctrl;
        sp_fs_ctrl.u32All = m_state_tracker.GetRegValue(sp_fs_ctrl_reg_offset);
        const auto            bitfields = sp_fs_ctrl.bitfields;
        const a6xx_threadsize thread_size = bitfields.THREADSIZE;
        const bool            enable_all_helper_lanes = bitfields.LODPIXMASK;
        const bool            enable_partial_helper_lanes = bitfields.PIXLODENABLE;

        event_state_it->SetThreadSize(thread_size);
        event_state_it->SetEnableAllHelperLanes(enable_all_helper_lanes);
        event_state_it->SetEnablePartialHelperLanes(enable_partial_helper_lanes);
    }

    // UBWC
    // in the case of BUF_INFO and RB_DEPTH_BUFFER_INFO, A6xx and A7xx+ variants share the same
    // register but with different bitfields, so we cannot use IsFieldEnabled() to check if the
    // field is enabled or not for current GPU
    uint32_t           rt_id = 0;
    constexpr uint32_t kElemCount = 8;
    uint32_t           rb_mrt_buf_info_reg_start = GetRegOffsetByName("RB_MRT0_BUF_INFO");
    DIVE_ASSERT(rb_mrt_buf_info_reg_start != kInvalidRegOffset);
    while (rt_id < 8 && m_state_tracker.IsRegSet(rb_mrt_buf_info_reg_start + kElemCount * rt_id))
    {
        RB_MRT_BUF_INFO rb_mrt_buf_info;
        const uint32_t  mrt_reg = rb_mrt_buf_info_reg_start + kElemCount * rt_id;
        rb_mrt_buf_info.u32All = m_state_tracker.GetRegValue(mrt_reg);
        event_state_it->SetUBWCEnabled(rt_id, rb_mrt_buf_info.bitfields.COLOR_TILE_MODE == TILE6_3);
        if (GetGPUVariantType() >= kA7XX)
        {
            event_state_it->SetUBWCLosslessEnabled(rt_id,
                                                   rb_mrt_buf_info.bitfields.LOSSLESSCOMPEN == 1);
        }
        ++rt_id;
    }

    if (GetGPUVariantType() >= kA7XX)
    {
        uint32_t rb_depth_buf_info_offset = GetRegOffsetByName("RB_DEPTH_BUFFER_INFO");
        DIVE_ASSERT(rb_depth_buf_info_offset != kInvalidRegOffset);
        if (m_state_tracker.IsRegSet(rb_depth_buf_info_offset))
        {
            RB_DEPTH_BUFFER_INFO depth_buffer_info;
            depth_buffer_info.u32All = m_state_tracker.GetRegValue(rb_depth_buf_info_offset);
            event_state_it->SetUBWCEnabledOnDS(depth_buffer_info.bitfields.TILEMODE == TILE6_3);
            event_state_it->SetUBWCLosslessEnabledOnDS(depth_buffer_info.bitfields.LOSSLESSCOMPEN ==
                                                       1);
        }
    }
}

}  // namespace Dive
