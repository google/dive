
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

#pragma once
#include <map>
#include <vector>
#include "capture_data.h"
#include "capture_event_info.h"
#include "command_hierarchy.h"
#include "event_state.h"
#include "progress_tracker.h"
#include "gfxr_vulkan_command_hierarchy.h"

namespace Dive
{

//--------------------------------------------------------------------------------------------------
// Metadata that describes information in the capture
struct CaptureMetadata
{
    // Information about the command buffers, represented in a tree hierarchy
    CommandHierarchy m_command_hierarchy;

    // Information about each shader in the capture
    std::vector<Disassembly> m_shaders;

    // Information about each buffer in the capture
    std::vector<BufferInfo> m_buffers;

    // Information about each event in the capture
    std::vector<EventInfo> m_event_info;

    // Register state tracking for each event
    // This is separated from EventInfo to take advantage of code-gen
    EventStateInfo m_event_state;

    // Information about the submits in this capture
    uint64_t m_num_pm4_packets;
};

//--------------------------------------------------------------------------------------------------
// Main container for the capture data as well as associated metadata
class DataCore
{
protected:
    ProgressTracker *m_progress_tracker;

public:
    DataCore() = default;

    DataCore(ProgressTracker *progress_tracker);

    // Load the capture file
    CaptureData::LoadResult LoadCaptureData(const char *file_name);

    // Parse the capture to generate info that describes the capture
    bool ParseCaptureData(bool is_gfxr_capture = false);

    // Create meta data from the captured data
    bool CreateMetaData(bool is_gfxr_capture = false);

    // Get the capture data (includes access to raw command buffers and memory blocks)
    const CaptureData &GetCaptureData() const;
    CaptureData       &GetMutableCaptureData();

    // Get the command-hierarchy, which is a tree view interpretation of the command buffer
    const CommandHierarchy &GetCommandHierarchy() const;

    // Get metadata describing the capture (info obtained by parsing the capture)
    const CaptureMetadata &GetCaptureMetadata() const;

private:
    // Create command hierarchy from the captured data
    bool CreateCommandHierarchy(bool is_gfxr_capture = false);
    // The relatively raw captured data (memory & submit blocks)
    CaptureData m_capture_data;

    // Metadata for the capture data in m_capture_data
    CaptureMetadata m_capture_metadata;
};

#if defined(ENABLE_CAPTURE_BUFFERS)
//--------------------------------------------------------------------------------------------------
// Shader reflector callback class, for use by the CaptureMetadataCreator
class SRDCallbacks : public IShaderReflectorCallbacks
{
public:
    // Callbacks on each SRD table used by the shader. The SRD table is a buffer that contains 1 or
    // more SRDs, each of which might be a different type
    virtual bool OnSRDTable(ShaderStage shader_stage,
                            uint64_t    va_addr,
                            uint64_t    size,
                            void       *user_ptr)
    {
        return true;
    }

    virtual bool OnSRDTable(ShaderStage shader_stage,
                            void       *data_ptr,
                            uint64_t    va_addr,
                            uint64_t    size,
                            void       *user_ptr)
    {
        return true;
    }
};
#endif

//--------------------------------------------------------------------------------------------------
// Handles creation of much of the metadata "info" from the capture
class CaptureMetadataCreator : public EmulateCallbacksBase
{
public:
    CaptureMetadataCreator(CaptureMetadata &capture_metadata);
    ~CaptureMetadataCreator();

    virtual void OnSubmitStart(uint32_t submit_index, const SubmitInfo &submit_info) override;
    virtual void OnSubmitEnd(uint32_t submit_index, const SubmitInfo &submit_info) override;

    const EmulateStateTracker &GetStateTracker() const { return m_state_tracker; }

    // Callbacks
    virtual bool OnIbStart(uint32_t                  submit_index,
                           uint32_t                  ib_index,
                           const IndirectBufferInfo &ib_info,
                           IbType                    type) override;

    virtual bool OnIbEnd(uint32_t                  submit_index,
                         uint32_t                  ib_index,
                         const IndirectBufferInfo &ib_info) override;

    virtual bool OnPacket(const IMemoryManager &mem_manager,
                          uint32_t              submit_index,
                          uint32_t              ib_index,
                          uint64_t              va_addr,
                          Pm4Header             header) override;

private:
    bool HandleShaders(const IMemoryManager &mem_manager, uint32_t submit_index, uint32_t opcode);
    void FillEventStateInfo(EventStateInfo::Iterator event_state_it);
    void FillInputAssemblyState(EventStateInfo::Iterator event_state_it);
    void FillTessellationState(EventStateInfo::Iterator event_state_it);
    void FillViewportState(EventStateInfo::Iterator event_state_it);
    void FillRasterizerState(EventStateInfo::Iterator event_state_it);
    void FillMultisamplingState(EventStateInfo::Iterator event_state_it);
    void FillDepthState(EventStateInfo::Iterator event_state_it);
    void FillColorBlendState(EventStateInfo::Iterator event_state_it);
    void FillHardwareSpecificStates(EventStateInfo::Iterator event_state_it);

    // Map from shader address to shader index (in m_capture_metadata.m_shaders)
    std::map<uint64_t, uint32_t> m_shader_addrs;

    // Map from buffer address to buffer index (in m_capture_metadata.m_buffers)
    std::map<uint64_t, uint32_t> m_buffer_addrs;

    CaptureMetadata &m_capture_metadata;
    RenderModeType   m_current_render_mode = RenderModeType::kUnknown;

#if defined(ENABLE_CAPTURE_BUFFERS)
    // SRDCallbacks is a friend class, since it is essentially doing part of
    // CaptureMetadataCreator's work and is only a separate class due to the callback nature of SRD
    // reflection
    friend class SRDCallbacks;
#endif
};

}  // namespace Dive
