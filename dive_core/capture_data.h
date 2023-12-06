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
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include "common.h"
#include "dive_core/common/dive_capture_format.h"
#include "dive_core/common/emulate_pm4.h"
#include "dive_core/common/memory_manager_base.h"
#include "log.h"
#include "progress_tracker.h"

// Forward declarations
struct SqttFileChunkAsicInfo;

namespace Dive
{

class CommandHierarchy;
//--------------------------------------------------------------------------------------------------
class MemoryAllocationInfo
{
public:
    // Find the internal memory allocation data for the given range
    // "Internal" allocations are those made by the driver on behalf of the application
    const MemoryAllocationData *FindInternalAllocation(uint64_t va_addr, uint64_t size) const;

    // Find the global memory allocation data for the given range
    // "Global" allocations are those made by the application via vkAllocateMemory
    const MemoryAllocationData *FindGlobalAllocation(uint64_t va_addr, uint64_t size) const;

    // Find the submit memory allocation data for the given range
    // "Submit" allocations are those memory references passed to kernel during submit time
    const MemoryAllocationData *FindSubmitAllocation(uint32_t submit_index,
                                                     uint64_t va_addr,
                                                     uint64_t size) const;

    // Add memory allocation info. The 'type' parameter indicates which array to add it to
    void AddMemoryAllocations(uint32_t                            submit_index,
                              MemoryAllocationsDataHeader::Type   type,
                              std::vector<MemoryAllocationData> &&allocations);

private:
    struct SubmitAllocations
    {
        uint32_t                          m_submit_index;
        std::vector<MemoryAllocationData> m_allocations;
    };

    // Allocations done by the driver, for resources like load-sh buffers, shader binaries, etc
    std::vector<MemoryAllocationData> m_internal_allocs;

    // Allocations done by the application via vkAllocateMemory
    std::vector<MemoryAllocationData> m_global_allocs;
};

//--------------------------------------------------------------------------------------------------
// Container for the memory data of a memory block
struct MemoryData
{
    uint32_t m_data_size;
    uint8_t *m_data_ptr;
};

//--------------------------------------------------------------------------------------------------
// Handles the loading/storage/caching of all memory blocks in the capture data file
// Assumption is that memory is not re-used from within a submit, but can be re-used
//  in between submits. So a "submit_index" is an important identifier for a memory block.
class MemoryManager : public IMemoryManager
{
public:
    virtual ~MemoryManager();

    // Use an r-value reference instead of normal reference to prevent an extra copy
    // Given the amount of memory potentially in a capture, this can be significant
    void AddMemoryBlock(uint32_t submit_index, uint64_t va_addr, MemoryData &&data);

    // Add memory allocation info to internal MemoryAllocationInfo object
    void AddMemoryAllocations(uint32_t                            submit_index,
                              MemoryAllocationsDataHeader::Type   type,
                              std::vector<MemoryAllocationData> &&allocations);

    // Finalize load. After this, no memory block should be added!
    // same_submit_copy_only - If set, then CopyMemory() will only copy from memory blocks used in
    // the same submit. If not set, then allowed to use any allocations from any submit, with the
    // assumption that there is no overlap in captured memory
    void Finalize(bool same_submit_copy_only, bool duplicate_ib_capture);

    const MemoryAllocationInfo &GetMemoryAllocationInfo() const;

    // Load the given va/size from the memory blocks
    virtual bool CopyMemory(void    *buffer_ptr,
                            uint32_t submit_index,
                            uint64_t va_addr,
                            uint64_t size) const override;

    // Keep grabbing contiguous memory blocks until the callback returns false
    virtual bool GetMemoryOfUnknownSizeViaCallback(uint32_t     submit_index,
                                                   uint64_t     va_addr,
                                                   PfnGetMemory data_callback,
                                                   void        *user_ptr) const override;

    // Given an address, find the maximum contiguous amount of memory accessible from that point
    virtual uint64_t GetMaxContiguousSize(uint32_t submit_index, uint64_t va_addr) const override;

    // Determine if given range is covered by memory blocks
    virtual bool IsValid(uint32_t submit_index, uint64_t addr, uint64_t size) const override;

private:
    struct MemoryBlock
    {
        uint64_t m_va_addr;
        uint32_t m_submit_index;
        uint32_t m_data_size;
        uint8_t *m_data_ptr;
    };

    // mutable variable for caching reasons
    mutable const MemoryBlock *m_last_used_block_ptr = nullptr;

    // Memory blocks containing all the captured memory data
    std::vector<MemoryBlock> m_memory_blocks;

    // All the captured memory allocation info
    MemoryAllocationInfo m_memory_allocations;

    // If set, then only memory blocks from same submit are considered
    // Otherwise, all previous submits are considered as well
    bool m_same_submit_only = true;
};

//--------------------------------------------------------------------------------------------------
class SubmitInfo
{
public:
    SubmitInfo(EngineType                        engine_type,
               QueueType                         queue_type,
               uint8_t                           engine_index,
               bool                              is_dummy_submit,
               std::vector<IndirectBufferInfo> &&ibs);
    EngineType                GetEngineType() const;
    QueueType                 GetQueueType() const;
    uint8_t                   GetEngineIndex() const;
    bool                      IsDummySubmit() const;
    uint32_t                  GetNumIndirectBuffers() const;
    const IndirectBufferInfo &GetIndirectBufferInfo(uint32_t ib_index) const;
    const IndirectBufferInfo *GetIndirectBufferInfoPtr() const;
    void                      AppendIb(const IndirectBufferInfo &ib);

private:
    EngineType                      m_engine_type;
    QueueType                       m_queue_type;
    uint8_t                         m_engine_index;
    bool                            m_is_dummy_submit;
    std::vector<IndirectBufferInfo> m_ibs;
};

//--------------------------------------------------------------------------------------------------
class PresentInfo
{
public:
    PresentInfo();
    PresentInfo(EngineType engine_type,
                QueueType  queue_type,
                uint32_t   submit_index,
                bool       full_screen,
                uint64_t   addr,
                uint64_t   size,
                uint32_t   vk_format,
                uint32_t   vk_color_space);
    bool       HasValidData() const;
    EngineType GetEngineType() const;
    QueueType  GetQueueType() const;
    uint32_t   GetSubmitIndex() const;
    bool       IsFullScreen() const;
    uint64_t   GetSurfaceAddr() const;
    uint64_t   GetSurfaceSize() const;
    uint32_t   GetSurfaceVkFormat() const;
    uint32_t   GetSurfaceVkColorSpaceKHR() const;

private:
    bool       m_valid_data;
    uint32_t   m_submit_index;  // After what index in CaptureData::m_submits was there a present
    EngineType m_engine_type;
    QueueType  m_queue_type;
    bool       m_full_screen;
    uint64_t   m_addr;
    uint64_t   m_size;
    uint32_t   m_vk_format;       // VkFormat of the presented surface
    uint32_t   m_vk_color_space;  // VkColorSpaceKHR of the presented surface
};

//--------------------------------------------------------------------------------------------------
class RingInfo
{
public:
    RingInfo(QueueType queue_type,
             uint32_t  queue_index,
             uint64_t  ring_base_addr,
             uint32_t  ring_full_size,
             uint64_t  ring_capture_start_addr,
             uint32_t  ring_capture_size,
             uint64_t  hang_ib_addr,
             uint64_t  hang_size_left,
             uint64_t  fence_signaled_addr,
             uint64_t  fence_emitted_addr);

    RingInfo(){};

    QueueType GetQueueType() const;
    uint32_t  GetQueueIndex() const;
    uint64_t  GetRingBaseAddress() const;
    uint32_t  GetRingSize() const;
    uint64_t  GetRingCaptureAddress() const;
    uint32_t  GetRingCaptureSize() const;
    uint64_t  GetHungIbAddress() const;
    uint64_t  GetHungSizeLeft() const;
    uint64_t  GetEmittedFenceAddress() const;
    uint64_t  GetSignaledFenceAddress() const;

private:
    QueueType m_queue_type;
    uint32_t  m_queue_index;
    uint64_t  m_ring_base_addr;
    uint32_t  m_ring_full_size;
    uint64_t  m_ring_capture_addr;
    uint32_t  m_ring_capture_size;
    uint64_t  m_hang_ib_addr;
    uint64_t  m_hang_size_left;
    uint64_t  m_fence_signaled_addr;
    uint64_t  m_fence_emitted_addr;
};

//--------------------------------------------------------------------------------------------------
class TextInfo
{
public:
    TextInfo(std::string name, uint64_t size, std::vector<char> &&data);

    const std::string &GetName() const;
    uint64_t           GetSize() const;
    const char        *GetText() const;

private:
    std::string       m_name;
    uint64_t          m_size;
    std::vector<char> m_text;
};

//--------------------------------------------------------------------------------------------------
// State for a single wave, including SGPRS and VGPRS
class WaveStateInfo
{
public:
    WaveStateInfo(const Dive::WaveState  &state,
                  std::vector<uint32_t> &&sgprs,
                  std::vector<uint32_t> &&vgprs,
                  std::vector<uint32_t> &&ttmps);

    const Dive::WaveState       &GetState() const;
    const std::vector<uint32_t> &GetSGPRs() const;
    const std::vector<uint32_t> &GetVGPRs() const;
    const std::vector<uint32_t> &GetTTMPs() const;

private:
    Dive::WaveState       m_state;
    std::vector<uint32_t> m_sgprs;
    std::vector<uint32_t> m_vgprs;
    std::vector<uint32_t> m_ttmps;
};

//--------------------------------------------------------------------------------------------------
class WaveInfo
{
public:
    explicit WaveInfo(std::vector<WaveStateInfo> &&waves);
    WaveInfo(){};

    const std::vector<WaveStateInfo> &GetWaves() const;

private:
    std::vector<WaveStateInfo> m_waves;
};

//--------------------------------------------------------------------------------------------------
class RegisterInfo
{
public:
    explicit RegisterInfo(std::map<std::string, uint32_t> &&regs);
    RegisterInfo(){};

    const std::map<std::string, uint32_t> &GetRegisters() const;

private:
    std::map<std::string, uint32_t> m_registers;
};

//--------------------------------------------------------------------------------------------------
class CaptureData
{
public:
    enum class LoadResult
    {
        kSuccess,
        kFileIoError,
        kCorruptData,
        kVersionError
    };
    CaptureData();
    CaptureData(ILog *log_ptr);
    CaptureData(ProgressTracker *progress_tracker, ILog *log_ptr);
    virtual ~CaptureData() = default;

    LoadResult LoadFile(const char *file_name);

    CaptureDataHeader::CaptureType          GetCaptureType() const;
    const MemoryManager                    &GetMemoryManager() const;
    uint32_t                                GetNumSubmits() const;
    const SubmitInfo                       &GetSubmitInfo(uint32_t submit_index) const;
    uint32_t                                GetNumPresents() const;
    const PresentInfo                      &GetPresentInfo(uint32_t present_index) const;
    uint32_t                                GetNumRings() const;
    const RingInfo                         &GetRingInfo(uint32_t ring_index) const;
    const WaveInfo                         &GetWaveInfo() const;
    const RegisterInfo                     &GetRegisterInfo() const;
    inline uint32_t                         GetNumText() const { return (uint32_t)m_text.size(); }
    inline const TextInfo                  &GetText(uint32_t index) const { return m_text[index]; }
    inline ILog                            &GetLog() const { return *m_log_ptr; }
    inline const VulkanMetadataBlockHeader &GetVulkanMetadataVersion() const
    {
        return m_vulkan_metadata_header;
    }
    const std::vector<SubmitInfo> &GetSubmits() const;

    CaptureData &operator=(CaptureData &&) = default;

    LoadResult LoadCaptureFile(std::istream &capture_file);
    LoadResult LoadAdrenoRdFile(std::istream &capture_file);

    bool        HasPm4Data() const { return m_submits.size() > 0; }
    std::string GetFileFormatVersion() const;

private:
    LoadResult LoadCaptureFile(const char *file_name);
    LoadResult LoadAdrenoRdFile(const char *file_name);
    bool       LoadCapture(std::istream &capture_file, const CaptureDataHeader &data_header);
    bool       LoadMemoryAllocBlock(std::istream &capture_file);
    bool       LoadSubmitBlock(std::istream &capture_file);
    bool       LoadMemoryBlock(std::istream &capture_file);
    bool       LoadPresentBlock(std::istream &capture_file);
    bool       LoadTextBlock(std::istream &capture_file);
    bool       LoadWaveStateBlock(std::istream &capture_file, const CaptureDataHeader &data_header);
    bool       LoadRegisterBlock(std::istream &capture_file);
    bool       LoadVulkanMetaDataBlock(std::istream &capture_file);

    // Adreno-specific load functions
    bool LoadGpuAddressAndSize(std::istream &capture_file,
                               uint32_t      block_size,
                               uint64_t     *gpu_addr,
                               uint32_t     *size);
    bool LoadMemoryBlockAdreno(std::istream &capture_file, uint64_t gpu_addr, uint32_t size);
    bool LoadCmdStreamBlockAdreno(std::istream &capture_file,
                                  uint32_t      block_size,
                                  bool          create_new_submit);

    void Finalize(const CaptureDataHeader &data_header);

    CaptureDataHeader::CaptureType m_capture_type;
    std::vector<SubmitInfo>        m_submits;
    std::vector<PresentInfo>       m_presents;  // More than 1 if multi-frame capture
    std::vector<RingInfo>          m_rings;
    std::vector<TextInfo>          m_text;
    WaveInfo                       m_waves;
    RegisterInfo                   m_registers;
    VulkanMetadataBlockHeader      m_vulkan_metadata_header;
    MemoryManager                  m_memory;
    ProgressTracker               *m_progress_tracker;
    ILog                          *m_log_ptr;
    std::string                    m_cur_capture_file;
    CaptureDataHeader              m_data_header;
};

}  // namespace Dive
