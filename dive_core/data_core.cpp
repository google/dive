
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
#include "dive_core/common/shader_reflector.h"

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
    static const uint8_t kNumUserDataRegisters = 32;
    static const uint8_t kNumUserDataRegistersCompute = 16;
    const uint32_t       ce_ram_size = 48 * 1024;  // Estimate. Driver actually uses less than this.
    uint32_t buffer_size = EmulateConstantEngine::CalculateInternalBufferSize(ce_ram_size);
    m_ce_buffer_ptr = new uint8_t[buffer_size];

    m_state_tracker.Init(kNumUserDataRegisters, kNumUserDataRegistersCompute);
    m_constant_engine_emu.Init(m_ce_buffer_ptr, ce_ram_size);
}

//--------------------------------------------------------------------------------------------------
CaptureMetadataCreator::~CaptureMetadataCreator()
{
    if (m_ce_buffer_ptr != nullptr)
        delete[] m_ce_buffer_ptr;
}

//--------------------------------------------------------------------------------------------------
void CaptureMetadataCreator::OnSubmitStart(uint32_t submit_index, const SubmitInfo &submit_info) {}

//--------------------------------------------------------------------------------------------------
void CaptureMetadataCreator::OnSubmitEnd(uint32_t submit_index, const SubmitInfo &submit_info)
{
    // Have to reset at the end of submits because IsDrawDispatchDmaSyncEvent() uses size of the
    // arrays to determine the Event type, and packets should not carry across submit boundaries for
    // parsing purposes
    m_opcodes.clear();
    m_addrs.clear();
}

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
bool CaptureMetadataCreator::OnDcbPacket(const IMemoryManager        &mem_manager,
                                         uint32_t                     submit_index,
                                         uint32_t                     ib_index,
                                         uint64_t                     va_addr,
                                         const PM4_PFP_TYPE_3_HEADER &header)
{
    if (!m_state_tracker.OnDcbPacket(mem_manager, submit_index, ib_index, va_addr, header))
        return false;

    m_opcodes.push_back(header.opcode);
    m_addrs.push_back(va_addr);

    if (IsDrawDispatchDmaSyncEvent(mem_manager, submit_index, m_opcodes, m_addrs))
    {
        // Add a new event to the EventInfo metadata array
        EventInfo event_info;
        event_info.m_submit_index = submit_index;
        if (IsDrawEventOpcode(header.opcode))
            event_info.m_type = EventInfo::EventType::kDraw;
        else if (IsDmaEventOpcode(header.opcode))
            event_info.m_type = EventInfo::EventType::kDma;
        else if (IsDispatchEventOpcode(header.opcode))
            event_info.m_type = EventInfo::EventType::kDispatch;
        else
        {
            DIVE_ASSERT(GetSyncType(mem_manager, submit_index, m_opcodes, m_addrs) !=
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

            if (!HandleShaders(mem_manager, submit_index, header.opcode))
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
        // Have to reset between Events because IsDrawDispatchDmaSyncEvent() uses size of the
        // arrays to determine the Event type
        m_opcodes.clear();
        m_addrs.clear();
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
bool CaptureMetadataCreator::OnCcbPacket(const IMemoryManager        &mem_manager,
                                         uint32_t                     submit_index,
                                         uint32_t                     ib_index,
                                         uint64_t                     va_addr,
                                         const PM4_PFP_TYPE_3_HEADER &header)
{
    if (!m_state_tracker.OnCcbPacket(mem_manager, submit_index, ib_index, va_addr, header))
        return false;
    if (!m_constant_engine_emu.OnCcbPacket(mem_manager, submit_index, ib_index, va_addr, header))
        return false;
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
void CaptureMetadataCreator::FillViewportState(EventStateInfo::Iterator event_state_it) {}

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
