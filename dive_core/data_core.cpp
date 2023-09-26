
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
void CaptureMetadataCreator::FillInputAssemblyState(EventStateInfo::Iterator event_state_it) {}

//--------------------------------------------------------------------------------------------------
void CaptureMetadataCreator::FillTessellationState(EventStateInfo::Iterator event_state_it) {}

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
        xOffset.u32All = m_state_tracker.GetRegValue(viewport_reg + 1);
        xScale.u32All = m_state_tracker.GetRegValue(viewport_reg);
        yOffset.u32All = m_state_tracker.GetRegValue(viewport_reg + 3);
        yScale.u32All = m_state_tracker.GetRegValue(viewport_reg + 2);
        zOffset.u32All = m_state_tracker.GetRegValue(viewport_reg + 5);
        zScale.u32All = m_state_tracker.GetRegValue(viewport_reg + 4);

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
void CaptureMetadataCreator::FillRasterizerState(EventStateInfo::Iterator event_state_it) {}

//--------------------------------------------------------------------------------------------------
void CaptureMetadataCreator::FillMultisamplingState(EventStateInfo::Iterator event_state_it) {}

//--------------------------------------------------------------------------------------------------
void CaptureMetadataCreator::FillDepthState(EventStateInfo::Iterator event_state_it) {}

//--------------------------------------------------------------------------------------------------
void CaptureMetadataCreator::FillColorBlendState(EventStateInfo::Iterator event_state_it) {}

//--------------------------------------------------------------------------------------------------
void CaptureMetadataCreator::FillHardwareSpecificStates(EventStateInfo::Iterator event_state_it) {}

}  // namespace Dive
