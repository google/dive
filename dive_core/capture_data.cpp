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

#include "capture_data.h"

#include <assert.h>
#include <string.h>  // memcpy
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include "archive.h"
#include "dive_core/command_hierarchy.h"
#include "dive_core/common/common.h"
#include "freedreno_dev_info.h"
#if defined(DIVE_ENABLE_PERFETTO)
#    include "perfetto_trace/trace_reader.h"
#endif
#include "pm4_info.h"

namespace Dive
{

namespace
{
constexpr const uint64_t
kMaxNumMemAlloc = (uint64_t(24) << 30) /
                  (4 * 1024);  // Number of allocation in 4k chunks for 16+8 GiB memory
constexpr const uint32_t kMaxMemAllocSize = 1 << 30;      // 1 GiB
constexpr const uint32_t kMaxStrLen = 100 << 20;          // 100 MiB
constexpr const uint32_t kMaxNumWavesPerBlock = 1 << 20;  // 1 MiB
constexpr const uint32_t kMaxNumSGPRPerWave = 1 << 20;    // 1 MiB
constexpr const uint32_t kMaxNumVGPRPerWave = 1 << 20;    // 1 MiB
}  // namespace

//--------------------------------------------------------------------------------------------------
FileReader::FileReader(const char *file_name) :
    m_file_name(file_name),
    m_handle(std::unique_ptr<struct archive, decltype(&archive_read_free)>(archive_read_new(),
                                                                           &archive_read_free))
{
    DIVE_ASSERT(m_handle != nullptr);
}

//--------------------------------------------------------------------------------------------------
int FileReader::open()
{
    // Enables auto-detection code and decompression support for gzip
    int ret = archive_read_support_filter_gzip(m_handle.get());
    if (ret != ARCHIVE_OK)
    {
        std::cerr << "error archive_read_support_filter_gzip: "
                  << archive_error_string(m_handle.get());
        return ret;
    }

    ret = archive_read_support_filter_none(m_handle.get());
    if (ret != ARCHIVE_OK)
    {
        std::cerr << "error archive_read_support_filter_none: "
                  << archive_error_string(m_handle.get());
        return ret;
    }

    // Enables support for all available formats except the "raw" format
    ret = archive_read_support_format_all(m_handle.get());
    if (ret != ARCHIVE_OK)
    {
        std::cerr << "error archive_read_support_format_all: "
                  << archive_error_string(m_handle.get());
        return ret;
    }

    // The "raw" format handler allows libarchive to be used to read arbitrary data.
    ret = archive_read_support_format_raw(m_handle.get());
    if (ret != ARCHIVE_OK)
    {
        std::cerr << "error archive_read_support_format_raw: "
                  << archive_error_string(m_handle.get());
        return ret;
    }

    ret = archive_read_open_filename(m_handle.get(), m_file_name.c_str(), 10240);
    if (ret != ARCHIVE_OK)
    {
        std::cerr << "error archive_read_open_filename: " << archive_error_string(m_handle.get());
        return ret;
    }
    struct archive_entry *entry;
    ret = archive_read_next_header(m_handle.get(), &entry);
    if (ret != ARCHIVE_OK)
    {
        std::cerr << "error archive_read_next_header: " << archive_error_string(m_handle.get());
    }

    return ret;
}

//--------------------------------------------------------------------------------------------------
int64_t FileReader::read(char *buf, int64_t nbytes)
{
    char   *ptr = buf;
    int64_t ret = 0;
    while (nbytes > 0)
    {
        int64_t n = archive_read_data(m_handle.get(), ptr, nbytes);
        if (n < 0)
        {
            std::cerr << "error archive_read_data: " << archive_error_string(m_handle.get());
            return n;
        }
        if (n == 0)
            break;
        ptr += n;
        nbytes -= n;
        ret += n;
    }
    return ret;
}

//--------------------------------------------------------------------------------------------------
int FileReader::close()
{
    m_handle = nullptr;
    return 0;
}

// =================================================================================================
// MemoryAllocationInfo
// =================================================================================================
const MemoryAllocationData *MemoryAllocationInfo::FindInternalAllocation(uint64_t va_addr,
                                                                         uint64_t size) const
{
    for (uint32_t alloc = 0; alloc < m_internal_allocs.size(); ++alloc)
    {
        const MemoryAllocationData &alloc_data = m_internal_allocs[alloc];
        if ((alloc_data.m_gpu_virt_addr <= va_addr) &&
            ((va_addr + size) <= (alloc_data.m_gpu_virt_addr + alloc_data.m_size)))
        {
            return &alloc_data;
        }
    }
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
const MemoryAllocationData *MemoryAllocationInfo::FindGlobalAllocation(uint64_t va_addr,
                                                                       uint64_t size) const
{
    for (uint32_t alloc = 0; alloc < m_global_allocs.size(); ++alloc)
    {
        const MemoryAllocationData &alloc_data = m_global_allocs[alloc];
        if ((alloc_data.m_gpu_virt_addr <= va_addr) &&
            ((va_addr + size) <= (alloc_data.m_gpu_virt_addr + alloc_data.m_size)))
        {
            return &alloc_data;
        }
    }
    return nullptr;
}

//--------------------------------------------------------------------------------------------------
void MemoryAllocationInfo::AddMemoryAllocations(uint32_t                            submit_index,
                                                MemoryAllocationsDataHeader::Type   type,
                                                std::vector<MemoryAllocationData> &&allocations)
{
    // There can only be 1 single internal and global allocation set, but there can
    // potentially be a submission allocation set *per* submit
    if (type == MemoryAllocationsDataHeader::Type::kInternal)
    {
        DIVE_ASSERT(m_internal_allocs.empty());
        m_internal_allocs = allocations;
    }
    else if (type == MemoryAllocationsDataHeader::Type::kGlobal)
    {
        DIVE_ASSERT(m_global_allocs.empty());
        m_global_allocs = allocations;
    }
}

// =================================================================================================
// MemoryManager
// =================================================================================================
MemoryManager::~MemoryManager()
{
    for (uint32_t i = 0; i < m_memory_blocks.size(); ++i)
    {
        delete[] m_memory_blocks[i].m_data_ptr;
    }
}

//--------------------------------------------------------------------------------------------------
void MemoryManager::AddMemoryBlock(uint32_t submit_index, uint64_t va_addr, MemoryData &&data)
{
    MemoryBlock mem_block;
    mem_block.m_submit_index = submit_index;
    mem_block.m_va_addr = va_addr;
    mem_block.m_data_size = data.m_data_size;
    mem_block.m_data_ptr = data.m_data_ptr;
    m_memory_blocks.push_back(mem_block);

    // Clear the MemoryData since ownership of the data memory has been "moved"
    data.m_data_size = 0;
    data.m_data_ptr = nullptr;
}

//--------------------------------------------------------------------------------------------------
void MemoryManager::AddMemoryAllocations(uint32_t                            submit_index,
                                         MemoryAllocationsDataHeader::Type   type,
                                         std::vector<MemoryAllocationData> &&allocations)
{
    m_memory_allocations.AddMemoryAllocations(submit_index, type, std::move(allocations));
}

//--------------------------------------------------------------------------------------------------
void MemoryManager::Finalize(bool same_submit_copy_only, bool duplicate_ib_capture)
{
    m_same_submit_only = same_submit_copy_only;

    // Sorting required for GetMaxContiguousSize(), GetMemoryOfUnknownSizeViaCallback(), and others
    // Important: Preserve order of equivalent blocks using stable_sort (later blocks have more
    // updated view of memory)
    std::stable_sort(m_memory_blocks.begin(),
                     m_memory_blocks.end(),
                     [&](const MemoryBlock &lhs, const MemoryBlock &rhs) -> bool {
                         // Only if m_same_submit_only does the submit affect the sort order
                         // When this flag is not enabled, then the memory is "flattened" and memory
                         // can be captured from any submit (ie: memory tracker not reset between
                         // submits)
                         if (m_same_submit_only)
                         {
                             // Sort by submit, then by address
                             if (lhs.m_submit_index == rhs.m_submit_index)
                                 return (lhs.m_va_addr < rhs.m_va_addr);
                             return lhs.m_submit_index < rhs.m_submit_index;
                         }
                         return (lhs.m_va_addr < rhs.m_va_addr);
                     });

    // In libwrap, sometimes we re-capture buffers that have been suballocated from. This means the
    // *later* buffers have a more up-to-date view of memory
    if (m_same_submit_only)
    {
        std::vector<MemoryBlock> temp_memory_blocks;

        uint64_t prev_addr = 0;
        uint64_t prev_submit = 0;
        uint32_t prev_size = 0;
        for (uint32_t i = 0; i < m_memory_blocks.size(); ++i)
        {
            const MemoryBlock &memory_block = m_memory_blocks[i];

            if (memory_block.m_submit_index != prev_submit)
            {
                prev_submit = memory_block.m_submit_index;
                temp_memory_blocks.push_back(memory_block);
            }
            else
            {
                if (memory_block.m_va_addr != prev_addr || memory_block.m_data_size != prev_size)
                    temp_memory_blocks.push_back(memory_block);
                else
                {
                    // Replace previous memory block with the more updated current version
                    delete[] temp_memory_blocks.back().m_data_ptr;
                    temp_memory_blocks.back() = m_memory_blocks[i];
                }
            }
            prev_addr = memory_block.m_va_addr;
            prev_size = memory_block.m_data_size;
        }
        m_memory_blocks = std::move(temp_memory_blocks);
    }

#ifndef NDEBUG
    // Sanity check
    //      same_submit_only == true -> Make sure there are no overlaps within same submit
    //      same_submit_only == false -> Make sure there are no overlaps, period
    if (m_same_submit_only)
    {
        uint64_t cur_addr = 0;
        uint64_t cur_submit = 0;
        for (uint32_t i = 0; i < m_memory_blocks.size(); ++i)
        {
            const MemoryBlock &memory_block = m_memory_blocks[i];

            if (memory_block.m_submit_index != cur_submit)
                cur_submit = memory_block.m_submit_index;
            else
                DIVE_ASSERT(memory_block.m_va_addr >= cur_addr);
            cur_addr = memory_block.m_va_addr + memory_block.m_data_size;
        }
    }
    else
    {
        // This will trigger if IBs are captured more than once
        // There might be edge cases where duplicate IB capture can cause memory not to be
        // grabbed properly, especially if those IBs span multiple blocks. But generally isn't a big
        // issue and duplicate_ib_capture only applies to early captures which should be made
        // obsolete very soon
        if (!duplicate_ib_capture)
        {
            uint64_t cur_addr = 0;
            uint32_t cur_submit = 0;
            for (uint32_t i = 0; i < m_memory_blocks.size(); ++i)
            {
                const MemoryBlock &memory_block = m_memory_blocks[i];
                DIVE_ASSERT(memory_block.m_submit_index != cur_submit ||
                            memory_block.m_va_addr >= cur_addr);
                cur_addr = memory_block.m_va_addr + memory_block.m_data_size;
                cur_submit = memory_block.m_submit_index;
            }
        }
    }
#endif
}

//--------------------------------------------------------------------------------------------------
const MemoryAllocationInfo &MemoryManager::GetMemoryAllocationInfo() const
{
    return m_memory_allocations;
}

//--------------------------------------------------------------------------------------------------
bool MemoryManager::CopyMemory(void    *buffer_ptr,
                               uint32_t submit_index,
                               uint64_t va_addr,
                               uint64_t size) const
{
    // Check the last-used block first, because this is the desired block most of the time
    if (m_last_used_block_ptr != nullptr)
    {
        const MemoryBlock &mem_block = *m_last_used_block_ptr;
        uint64_t           mem_block_end_addr = mem_block.m_va_addr + mem_block.m_data_size;
        uint64_t           end_addr = va_addr + size;

        // Can only use the cached block if it fully encompasses the desired region
        bool valid_submit = m_same_submit_only ? (submit_index == mem_block.m_submit_index) : true;
        bool encompasses = (mem_block.m_va_addr <= va_addr) && (end_addr <= mem_block_end_addr);
        if (valid_submit && encompasses)
        {
#ifndef NDEBUG
            if (mem_block.m_data_size >= 16 * 1024 * 1024)
            {
                std::cout << "MemoryManager::CopyMemory data.m_data_size: " << mem_block.m_data_size
                          << " gpu addr:  " << va_addr << std::endl;
            }
#endif
            memcpy(buffer_ptr, (void *)&mem_block.m_data_ptr[va_addr - mem_block.m_va_addr], size);
            return true;
        }
    }

    // Iterate through the memory blocks to find overlapping blocks and do the appropriate memcopies
    uint64_t           amount_copied = 0;
    uint32_t           num_memory_blocks = (uint32_t)m_memory_blocks.size();
    const MemoryBlock *memory_blocks = (num_memory_blocks > 0) ? &m_memory_blocks.front() : nullptr;
    for (uint32_t i = num_memory_blocks - 1; i != UINT32_MAX; --i)
    {
        const MemoryBlock &mem_block = memory_blocks[i];

        uint64_t mem_block_end_addr = mem_block.m_va_addr + mem_block.m_data_size;
        uint64_t end_addr = va_addr + size;
        bool valid_submit = m_same_submit_only ? (submit_index == mem_block.m_submit_index) : true;
        bool overlaps = (va_addr < mem_block_end_addr) && (mem_block.m_va_addr < end_addr);
        if (valid_submit && overlaps)
        {
            m_last_used_block_ptr = &mem_block;
            uint64_t max_start_addr = std::max(va_addr, mem_block.m_va_addr);
            uint64_t min_end_addr = std::min(mem_block_end_addr, end_addr);
            uint64_t src_offset = max_start_addr - mem_block.m_va_addr;
            uint64_t dst_offset = max_start_addr - va_addr;
            uint64_t size_to_copy = min_end_addr - max_start_addr;

            const uint8_t *src_data_ptr = &mem_block.m_data_ptr[0];
            memcpy((uint8_t *)buffer_ptr + dst_offset, src_data_ptr + src_offset, size_to_copy);
            amount_copied += size_to_copy;
#ifndef NDEBUG
            static int64_t max_buffer_size = 0;
            if (max_buffer_size < mem_block.m_data_size)
            {
                max_buffer_size = mem_block.m_data_size;
                std::cout << "max_buffer_size: " << max_buffer_size << std::endl;
            }
#endif
            // Since capture requirement is there's no overlap in m_memory_blocks within the same
            // submit, if the memory is fully captured, then there should be exactly 'size' amount
            // memcpy-ed
            if (amount_copied == size)
                return true;
        }
    }
    return false;
}

//--------------------------------------------------------------------------------------------------
bool MemoryManager::GetMemoryOfUnknownSizeViaCallback(uint32_t     submit_index,
                                                      uint64_t     va_addr,
                                                      PfnGetMemory data_callback,
                                                      void        *user_ptr) const
{
    uint64_t cur_addr = va_addr;

    // Iterate through the memory blocks to find the biggest block that contains the passed-in addr
    // Note: m_same_submit_only => Blocks are sorted by submit, then by address
    //       otherwise they are just sorted by address
    for (uint64_t i = 0; i < m_memory_blocks.size(); ++i)
    {
        const MemoryBlock &mem_block = m_memory_blocks[i];
        bool valid_submit = m_same_submit_only ? (submit_index == mem_block.m_submit_index) : true;
        if (valid_submit)
        {
            if (cur_addr == va_addr)
            {
                // First block just has to contain this address
                uint64_t mem_block_end_addr = mem_block.m_va_addr + mem_block.m_data_size;
                if (mem_block.m_va_addr <= va_addr && va_addr < mem_block_end_addr)
                {
                    void    *data_ptr = mem_block.m_data_ptr + (va_addr - mem_block.m_va_addr);
                    uint64_t size = mem_block_end_addr - va_addr;
                    if (!data_callback(data_ptr, va_addr, size, user_ptr))
                        break;  // Callback indicates no more searching is needed
                    cur_addr = mem_block_end_addr;
                }
            }
            else if (cur_addr == mem_block.m_va_addr)
            {
                if (!data_callback(mem_block.m_data_ptr, cur_addr, mem_block.m_data_size, user_ptr))
                    break;  // Callback indicates no more searching is needed

                // Is contiguous. Update the cur_addr to reflect this block.
                cur_addr = mem_block.m_va_addr + mem_block.m_data_size;
            }
            else if (cur_addr != va_addr)
            {
                // Not contiguous, and found a discountinuity in captured address range
                // So safe to early out instead of continuing the search
                return true;
            }
        }
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
uint64_t MemoryManager::GetMaxContiguousSize(uint32_t submit_index, uint64_t va_addr) const
{
    uint64_t cur_addr = va_addr;

    // Iterate through the memory blocks to find the biggest block that contains the passed-in addr
    // Note: m_same_submit_only => Blocks are sorted by submit, then by address
    //       otherwise they are just sorted by address
    for (uint64_t i = 0; i < m_memory_blocks.size(); ++i)
    {
        const MemoryBlock &mem_block = m_memory_blocks[i];
        bool valid_submit = m_same_submit_only ? (submit_index == mem_block.m_submit_index) : true;
        if (valid_submit)
        {
            if (cur_addr == va_addr)
            {
                // First block just has to contain this address
                uint64_t mem_block_end_addr = mem_block.m_va_addr + mem_block.m_data_size;
                if (mem_block.m_va_addr <= va_addr && va_addr < mem_block_end_addr)
                    cur_addr = mem_block_end_addr;
            }
            else if (cur_addr == mem_block.m_va_addr)
            {
                // Is contiguous. Update the cur_addr to reflect this block.
                cur_addr = mem_block.m_va_addr + mem_block.m_data_size;
            }
            else if (cur_addr != va_addr)
            {
                // Not contiguous, and found a discountinuity in captured address range
                // So safe to early out instead of continuing the search
                return (cur_addr - va_addr);
            }
        }
    }
    return (cur_addr - va_addr);
}

//--------------------------------------------------------------------------------------------------
bool MemoryManager::IsValid(uint32_t submit_index, uint64_t addr, uint64_t size) const
{
    uint64_t max_size = GetMaxContiguousSize(submit_index, addr);
    return (max_size >= size);
}

// =================================================================================================
// SubmitInfo
// =================================================================================================
SubmitInfo::SubmitInfo(EngineType                        engine_type,
                       QueueType                         queue_type,
                       uint8_t                           engine_index,
                       bool                              is_dummy_submit,
                       std::vector<IndirectBufferInfo> &&ibs)
{
    m_engine_type = engine_type;
    m_queue_type = queue_type;
    m_engine_index = engine_index;
    m_is_dummy_submit = is_dummy_submit;
    m_ibs = ibs;
}

//--------------------------------------------------------------------------------------------------
EngineType SubmitInfo::GetEngineType() const
{
    return m_engine_type;
}

//--------------------------------------------------------------------------------------------------
QueueType SubmitInfo::GetQueueType() const
{
    return m_queue_type;
}

//--------------------------------------------------------------------------------------------------
uint8_t SubmitInfo::GetEngineIndex() const
{
    return m_engine_index;
}

//--------------------------------------------------------------------------------------------------
bool SubmitInfo::IsDummySubmit() const
{
    return m_is_dummy_submit;
}

//--------------------------------------------------------------------------------------------------
uint32_t SubmitInfo::GetNumIndirectBuffers() const
{
    return (uint32_t)m_ibs.size();
}

//--------------------------------------------------------------------------------------------------
const IndirectBufferInfo &SubmitInfo::GetIndirectBufferInfo(uint32_t ib_index) const
{
    return m_ibs[ib_index];
}

//--------------------------------------------------------------------------------------------------
const IndirectBufferInfo *SubmitInfo::GetIndirectBufferInfoPtr() const
{
    return &m_ibs[0];
}

//--------------------------------------------------------------------------------------------------
void SubmitInfo::AppendIb(const IndirectBufferInfo &ib)
{
    m_ibs.push_back(ib);
}

// =================================================================================================
// Present Info
// =================================================================================================
PresentInfo::PresentInfo()
{
    m_valid_data = false;
}

//--------------------------------------------------------------------------------------------------
PresentInfo::PresentInfo(EngineType engine_type,
                         QueueType  queue_type,
                         uint32_t   submit_index,
                         bool       full_screen,
                         uint64_t   addr,
                         uint64_t   size,
                         uint32_t   vk_format,
                         uint32_t   vk_color_space)
{
    m_valid_data = true;
    m_engine_type = engine_type;
    m_queue_type = queue_type;
    m_submit_index = submit_index;
    m_full_screen = full_screen;
    m_addr = addr;
    m_size = size;
    m_vk_format = vk_format;
    m_vk_color_space = vk_color_space;
}

//--------------------------------------------------------------------------------------------------
bool PresentInfo::HasValidData() const
{
    return m_valid_data;
}

//--------------------------------------------------------------------------------------------------
EngineType PresentInfo::GetEngineType() const
{
    return m_engine_type;
}

//--------------------------------------------------------------------------------------------------
QueueType PresentInfo::GetQueueType() const
{
    return m_queue_type;
}

//--------------------------------------------------------------------------------------------------
uint32_t PresentInfo::GetSubmitIndex() const
{
    return m_submit_index;
}

//--------------------------------------------------------------------------------------------------
bool PresentInfo::IsFullScreen() const
{
    return m_full_screen;
}

//--------------------------------------------------------------------------------------------------
uint64_t PresentInfo::GetSurfaceAddr() const
{
    return m_addr;
}

//--------------------------------------------------------------------------------------------------
uint64_t PresentInfo::GetSurfaceSize() const
{
    return m_size;
}

//--------------------------------------------------------------------------------------------------
uint32_t PresentInfo::GetSurfaceVkFormat() const
{
    return m_vk_format;
}

//--------------------------------------------------------------------------------------------------
uint32_t PresentInfo::GetSurfaceVkColorSpaceKHR() const
{
    return m_vk_color_space;
}

// =================================================================================================
// RingInfo
// =================================================================================================
RingInfo::RingInfo(QueueType queue_type,
                   uint32_t  queue_index,
                   uint64_t  ring_base_addr,
                   uint32_t  ring_full_size,
                   uint64_t  ring_capture_addr,
                   uint32_t  ring_capture_size,
                   uint64_t  hang_ib_addr,
                   uint64_t  hang_size_left,
                   uint64_t  fence_signaled_addr,
                   uint64_t  fence_emitted_addr)
{
    m_queue_type = queue_type;
    m_queue_index = queue_index;
    m_ring_base_addr = ring_base_addr;
    m_ring_full_size = ring_full_size;
    m_ring_capture_addr = ring_capture_addr;
    m_ring_capture_size = ring_capture_size;
    m_hang_ib_addr = hang_ib_addr;
    m_hang_size_left = hang_size_left;
    m_fence_signaled_addr = fence_signaled_addr;
    m_fence_emitted_addr = fence_emitted_addr;
}

//--------------------------------------------------------------------------------------------------
QueueType RingInfo::GetQueueType() const
{
    return m_queue_type;
}

//--------------------------------------------------------------------------------------------------
uint32_t RingInfo::GetQueueIndex() const
{
    return m_queue_index;
}

//--------------------------------------------------------------------------------------------------
uint64_t RingInfo::GetRingBaseAddress() const
{
    return m_ring_base_addr;
}

//--------------------------------------------------------------------------------------------------
uint32_t RingInfo::GetRingSize() const
{
    return m_ring_full_size;
}

//--------------------------------------------------------------------------------------------------
uint64_t RingInfo::GetRingCaptureAddress() const
{
    return m_ring_capture_addr;
}

//--------------------------------------------------------------------------------------------------
uint32_t RingInfo::GetRingCaptureSize() const
{
    return m_ring_capture_size;
}

//--------------------------------------------------------------------------------------------------
uint64_t RingInfo::GetHungIbAddress() const
{
    return m_hang_ib_addr;
}

//--------------------------------------------------------------------------------------------------
uint64_t RingInfo::GetHungSizeLeft() const
{
    return m_hang_size_left;
}

//--------------------------------------------------------------------------------------------------
uint64_t RingInfo::GetEmittedFenceAddress() const
{
    return m_fence_emitted_addr;
}

//--------------------------------------------------------------------------------------------------
uint64_t RingInfo::GetSignaledFenceAddress() const
{
    return m_fence_signaled_addr;
}

// =================================================================================================
// TextInfo
// =================================================================================================
TextInfo::TextInfo(std::string name, uint64_t size, std::vector<char> &&text)
{
    m_name = std::move(name);
    m_size = size;
    m_text = text;
}

//--------------------------------------------------------------------------------------------------
const std::string &TextInfo::GetName() const
{
    return m_name;
}

//--------------------------------------------------------------------------------------------------
uint64_t TextInfo::GetSize() const
{
    return m_size;
}

//--------------------------------------------------------------------------------------------------
const char *TextInfo::GetText() const
{
    return m_text.data();
}

// =================================================================================================
// WaveInfo
// =================================================================================================
WaveStateInfo::WaveStateInfo(const Dive::WaveState  &state,
                             std::vector<uint32_t> &&sgprs,
                             std::vector<uint32_t> &&vgprs,
                             std::vector<uint32_t> &&ttmps)
{
    m_state = state;
    m_sgprs = sgprs;
    m_vgprs = vgprs;
    m_ttmps = ttmps;
}

//--------------------------------------------------------------------------------------------------
const Dive::WaveState &WaveStateInfo::GetState() const
{
    return m_state;
}

//--------------------------------------------------------------------------------------------------
const std::vector<uint32_t> &WaveStateInfo::GetSGPRs() const
{
    return m_sgprs;
}

//--------------------------------------------------------------------------------------------------
const std::vector<uint32_t> &WaveStateInfo::GetVGPRs() const
{
    return m_vgprs;
}

//--------------------------------------------------------------------------------------------------
const std::vector<uint32_t> &WaveStateInfo::GetTTMPs() const
{
    return m_ttmps;
}

// =================================================================================================
// WaveInfo
// =================================================================================================
WaveInfo::WaveInfo(std::vector<WaveStateInfo> &&waves)
{
    m_waves = waves;
}

//--------------------------------------------------------------------------------------------------
const std::vector<WaveStateInfo> &WaveInfo::GetWaves() const
{
    return m_waves;
}

// =================================================================================================
// RegisterInfo
// =================================================================================================
RegisterInfo::RegisterInfo(std::map<std::string, uint32_t> &&regs)
{
    m_registers = regs;
}

//--------------------------------------------------------------------------------------------------
const std::map<std::string, uint32_t> &RegisterInfo::GetRegisters() const
{
    return m_registers;
}

// =================================================================================================
// CaptureData
// =================================================================================================
CaptureData::CaptureData() :
    m_progress_tracker(NULL),
    m_log_ptr(&LogNull::GetInstance())
{
}

//--------------------------------------------------------------------------------------------------
CaptureData::CaptureData(ILog *log_ptr) :
    m_progress_tracker(NULL),
    m_log_ptr(log_ptr)
{
}

//--------------------------------------------------------------------------------------------------
CaptureData::CaptureData(ProgressTracker *progress_tracker, ILog *log_ptr) :
    m_progress_tracker(progress_tracker),
    m_log_ptr(log_ptr)
{
}

//--------------------------------------------------------------------------------------------------
CaptureData::LoadResult CaptureData::LoadFile(const char *file_name)
{
    std::string file_name_(file_name);
    std::string file_extension = std::filesystem::path(file_name_).extension().generic_string();

    if (file_extension.compare(".dive") == 0)
    {
        return LoadCaptureFile(file_name);
    }
    else if (file_extension.compare(".rd") == 0)
    {
#if defined(DIVE_ENABLE_PERFETTO)
        auto perfetto_trace_path = std::filesystem::path(file_name_ + ".perfetto");
        if (std::filesystem::exists(perfetto_trace_path))
        {
            CaptureData::LoadResult res = LoadPerfettoFile(perfetto_trace_path.c_str());
            if (res != CaptureData::LoadResult::kSuccess)
            {
                return res;
            }
        }
#endif
        return LoadAdrenoRdFile(file_name);
    }
#if defined(DIVE_ENABLE_PERFETTO)
    else if (file_extension.compare(".perfetto") == 0)
    {
        return LoadPerfettoFile(file_name);
    }
#endif
    else
    {
        std::cerr << "Unknown capture type: " << file_name << std::endl;
        return LoadResult::kCorruptData;
    }
}

//--------------------------------------------------------------------------------------------------
std::ostream &operator<<(std::ostream &os, const CaptureData::LoadResult &r)
{
    switch (r)
    {
    case CaptureData::LoadResult::kSuccess: os << "Success"; break;
    case CaptureData::LoadResult::kFileIoError: os << "File IO Error"; break;
    case CaptureData::LoadResult::kCorruptData: os << "Corrupt Data"; break;
    case CaptureData::LoadResult::kVersionError: os << "Version Error"; break;
    default: os << "Unknown"; break;
    }
    return os;
}

//--------------------------------------------------------------------------------------------------
CaptureData::LoadResult CaptureData::LoadCaptureFile(const char *file_name)
{
    // Open the file stream
    std::fstream capture_file(file_name, std::ios::in | std::ios::binary);
    if (!capture_file.is_open())
    {
        std::cerr << "Not able to open: " << file_name << std::endl;
        return LoadResult::kFileIoError;
    }

    auto result = LoadCaptureFile(capture_file);
    if (result != LoadResult::kSuccess)
    {
        std::cerr << "Error reading: " << file_name << " (" << result << ")" << std::endl;
    }
    else
    {
        m_cur_capture_file = std::string(file_name);
    }

    return result;
}

//--------------------------------------------------------------------------------------------------
CaptureData::LoadResult CaptureData::LoadAdrenoRdFile(const char *file_name)
{
    FileReader reader(file_name);
    if (reader.open() != 0)
    {
        std::cerr << "Not able to open: " << file_name << std::endl;
        return LoadResult::kFileIoError;
    }
    auto result = LoadAdrenoRdFile(reader);
    if (result != LoadResult::kSuccess)
    {
        std::cerr << "Error reading: " << file_name << " (" << result << ")" << std::endl;
    }
    else
    {
        m_cur_capture_file = std::string(file_name);
    }

    return result;
}

#if defined(DIVE_ENABLE_PERFETTO)
//--------------------------------------------------------------------------------------------------
CaptureData::LoadResult CaptureData::LoadPerfettoFile(const char *file_name)
{
    std::string name(file_name);
    TraceReader reader(name);
    if (reader.LoadTraceFile())
    {
        return LoadResult::kSuccess;
    }

    return LoadResult::kFileIoError;
}
#endif

//--------------------------------------------------------------------------------------------------
CaptureData::LoadResult CaptureData::LoadCaptureFile(std::istream &capture_file)
{
    // Read file header
    FileHeader file_header;
    if (!capture_file.read((char *)&file_header, sizeof(file_header)))
    {
        return LoadResult::kFileIoError;
    }

    if (file_header.m_file_id != kDiveFileId)
        return LoadResult::kCorruptData;
    if (file_header.m_file_version != kDiveFileVersion)
        return LoadResult::kVersionError;

    BlockInfo block_info;
    while (capture_file.read((char *)&block_info, sizeof(block_info)))
    {
        switch (block_info.m_block_type)
        {
        case BlockType::kCapture:
        {
            // The capture data always begins with some metadata info
            m_data_header = {};
            capture_file.read((char *)&m_data_header, sizeof(m_data_header));
            bool incompatible = ((m_data_header.m_major_version != kCaptureMajorVersion) ||
                                 (m_data_header.m_minor_version > kCaptureMinorVersion));
            // Cannot open version 0.1/0.2.x due to CaptureDataHeader change
            incompatible |= ((m_data_header.m_major_version == 0) &&
                             (m_data_header.m_minor_version == 1 ||
                              m_data_header.m_minor_version == 2));
            if (incompatible)
            {
                std::cerr << "Incompatible capture version " << m_data_header.m_major_version << "."
                          << m_data_header.m_minor_version << std::endl;
                std::cerr << "Supported version: " << kCaptureMajorVersion << "."
                          << kCaptureMinorVersion << std::endl;
                return LoadResult::kVersionError;
            }
            m_capture_type = m_data_header.m_capture_type;
            if (m_progress_tracker)
            {
                m_progress_tracker->sendMessage("Loading memory blocks...");
            }

            // If return false, either encountered a parsing error, or found an unknown block on a
            // capture file with version # <= version supported by the host tool
            if (!LoadCapture(capture_file, m_data_header))
                return LoadResult::kCorruptData;

            break;
        }
        default:
            DIVE_ASSERT(false);  // No other type of parent block supported right now
            return LoadResult::kCorruptData;
        }
    }
    return LoadResult::kSuccess;
}

//--------------------------------------------------------------------------------------------------
CaptureData::LoadResult CaptureData::LoadAdrenoRdFile(FileReader &capture_file)
{
    enum rd_sect_type
    {
        RD_NONE,
        RD_TEST,           /* ascii text */
        RD_CMD,            /* ascii text */
        RD_GPUADDR,        /* u32 gpuaddr, u32 size */
        RD_CONTEXT,        /* raw dump */
        RD_CMDSTREAM,      /* raw dump */
        RD_CMDSTREAM_ADDR, /* gpu addr of cmdstream */
        RD_PARAM,          /* u32 param_type, u32 param_val, u32 bitlen */
        RD_FLUSH,          /* empty, clear previous params */
        RD_PROGRAM,        /* shader program, raw dump */
        RD_VERT_SHADER,
        RD_FRAG_SHADER,
        RD_BUFFER_CONTENTS,
        RD_GPU_ID,
        RD_CHIP_ID,
    };

    struct BlockInfo
    {
        // The first 2 dwords are set to 0xffffffff
        uint32_t m_max_uint32_1;
        uint32_t m_max_uint32_2;

        uint32_t m_block_type;

        // The number of bytes that follow this header.
        uint32_t m_data_size;
    };

    BlockInfo block_info;
    uint64_t  cur_gpu_addr = UINT64_MAX;
    uint32_t  cur_size = UINT32_MAX;
    bool      is_new_submit = false;
    while (capture_file.read((char *)&block_info, sizeof(block_info)) > 0)
    {
        if (block_info.m_max_uint32_1 != 0xffffffff || block_info.m_max_uint32_2 != 0xffffffff)
            return LoadResult::kCorruptData;

        switch (block_info.m_block_type)
        {
        case RD_GPUADDR:
            if (!LoadGpuAddressAndSize(capture_file,
                                       block_info.m_data_size,
                                       &cur_gpu_addr,
                                       &cur_size))
                return LoadResult::kFileIoError;
            is_new_submit = true;
            break;
        case RD_CMDSTREAM_ADDR:
            if (!LoadCmdStreamBlockAdreno(capture_file, block_info.m_data_size, is_new_submit))
                return LoadResult::kFileIoError;
            is_new_submit = false;
            break;
        case RD_BUFFER_CONTENTS:
            // The size read from RD_GPUADDR should match block size exactly
            if (block_info.m_data_size != cur_size)
                return LoadResult::kCorruptData;
            if (!LoadMemoryBlockAdreno(capture_file, cur_gpu_addr, cur_size))
                return LoadResult::kFileIoError;
            break;
        case RD_NONE:
        case RD_TEST:
        case RD_CMD:
        case RD_CONTEXT:
        case RD_CMDSTREAM:
        case RD_PARAM:
        case RD_FLUSH:
        case RD_PROGRAM:
        case RD_VERT_SHADER:
        case RD_FRAG_SHADER:
        {
            std::vector<char> buf(block_info.m_data_size);
            capture_file.read(buf.data(), block_info.m_data_size);
            break;
        }
        case RD_GPU_ID:
        {
            DIVE_ASSERT(block_info.m_data_size == 4);
            uint32_t gpu_id = 0;
            capture_file.read(reinterpret_cast<char *>(&gpu_id), block_info.m_data_size);
            SetGPUID(gpu_id / 100);
        }
        break;
        case RD_CHIP_ID:
        {
            DIVE_ASSERT(block_info.m_data_size == 8);
            fd_dev_id dev_id;
            capture_file.read(reinterpret_cast<char *>(&dev_id.chip_id), block_info.m_data_size);
            auto info = fd_dev_info(&dev_id);
            // It is possible that only RD_GPU_ID is valid, and RD_CHIP_ID contains invalid values
            if (info != nullptr)
            {
                SetGPUID(info->chip);
            }
        }
        break;
        }
    }
    m_memory.Finalize(true, true);
    return LoadResult::kSuccess;
}

//--------------------------------------------------------------------------------------------------
CaptureDataHeader::CaptureType CaptureData::GetCaptureType() const
{
    return m_capture_type;
}

//--------------------------------------------------------------------------------------------------
const MemoryManager &CaptureData::GetMemoryManager() const
{
    return m_memory;
}

//--------------------------------------------------------------------------------------------------
uint32_t CaptureData::GetNumSubmits() const
{
    return (uint32_t)m_submits.size();
}

//--------------------------------------------------------------------------------------------------
const SubmitInfo &CaptureData::GetSubmitInfo(uint32_t submit_index) const
{
    return m_submits[submit_index];
}

//--------------------------------------------------------------------------------------------------
const std::vector<SubmitInfo> &CaptureData::GetSubmits() const
{
    return m_submits;
}

//--------------------------------------------------------------------------------------------------
uint32_t CaptureData::GetNumPresents() const
{
    return (uint32_t)m_presents.size();
}

//--------------------------------------------------------------------------------------------------
const PresentInfo &CaptureData::GetPresentInfo(uint32_t present_index) const
{
    return m_presents[present_index];
}

//--------------------------------------------------------------------------------------------------
uint32_t CaptureData::GetNumRings() const
{
    return (uint32_t)m_rings.size();
}

//--------------------------------------------------------------------------------------------------
const RingInfo &CaptureData::GetRingInfo(uint32_t ring_index) const
{
    return m_rings[ring_index];
}

//--------------------------------------------------------------------------------------------------
const WaveInfo &CaptureData::GetWaveInfo() const
{
    return m_waves;
}

//--------------------------------------------------------------------------------------------------
const RegisterInfo &CaptureData::GetRegisterInfo() const
{
    return m_registers;
}

//--------------------------------------------------------------------------------------------------
bool CaptureData::LoadCapture(std::istream &capture_file, const CaptureDataHeader &data_header)
{
    BlockInfo block_info;
    while (capture_file.read((char *)&block_info, sizeof(block_info)))
    {
        switch (block_info.m_block_type)
        {
        case BlockType::kMemoryAlloc:
            if (!LoadMemoryAllocBlock(capture_file))
                return false;
            break;
        case BlockType::kSubmit:
            if (!LoadSubmitBlock(capture_file))
                return false;
            break;
        case BlockType::kMemoryRaw:
            if (!LoadMemoryBlock(capture_file))
                return false;
            break;
        case BlockType::kPresent:
            if (!LoadPresentBlock(capture_file))
                return false;
            break;
        case BlockType::kWaveState:
            if (!LoadWaveStateBlock(capture_file, data_header))
                return false;
            break;
        case BlockType::kText:
            if (!LoadTextBlock(capture_file))
                return false;
            break;
        case BlockType::kRegisters:
            if (!LoadRegisterBlock(capture_file))
                return false;
            break;
        case BlockType::kVulkanMetadata:
            if (!LoadVulkanMetaDataBlock(capture_file))
                return false;
            break;
        default:
            // It is an error if it is NOT a future version of the capture format AND there are
            // unsupported blocks. The idea is that unsupported blocks are possible due to forward
            // compatibility ONLY.
            bool later_capture = (data_header.m_major_version > kCaptureMajorVersion);
            later_capture |= ((data_header.m_major_version == kCaptureMajorVersion) &&
                              (data_header.m_minor_version > kCaptureMinorVersion));
            later_capture |= ((data_header.m_major_version == kCaptureMajorVersion) &&
                              (data_header.m_minor_version == kCaptureMinorVersion) &&
                              (data_header.m_revision > kCaptureRevision));
            if (!later_capture)
            {
                DIVE_ERROR_MSG("Unsupported block type 0x%x found!\n", block_info.m_block_type);
                DIVE_ASSERT(false);  // Unsupported block type found!
                return false;
            }
            capture_file.seekg(block_info.m_data_size, std::ios::cur);
        }
    }
    Finalize(data_header);
    return true;
}

//--------------------------------------------------------------------------------------------------
bool CaptureData::LoadMemoryAllocBlock(std::istream &capture_file)
{
    MemoryAllocationsDataHeader memory_allocations_header;
    if (!capture_file.read((char *)&memory_allocations_header, sizeof(memory_allocations_header)))
        return false;
    if (memory_allocations_header.m_num_allocations > kMaxNumMemAlloc)
        return false;

    // Load the allocations
    std::vector<MemoryAllocationData> allocations;
    allocations.resize(memory_allocations_header.m_num_allocations);
    uint32_t size = memory_allocations_header.m_num_allocations * sizeof(MemoryAllocationData);
    if (!capture_file.read((char *)&allocations[0], size))
        return false;

    // Add it to memory manager
    uint32_t submit_index = (uint32_t)(m_submits.size());
    m_memory.AddMemoryAllocations(submit_index,
                                  memory_allocations_header.m_type,
                                  std::move(allocations));

    return true;
}

//--------------------------------------------------------------------------------------------------
bool CaptureData::LoadSubmitBlock(std::istream &capture_file)
{
    SubmitDataHeader submit_data_header;
    if (!capture_file.read((char *)&submit_data_header, sizeof(submit_data_header)))
        return false;

    // Load the ib info
    std::vector<IndirectBufferInfo> ibs;
    for (uint32_t ib = 0; ib < submit_data_header.m_num_ibs; ++ib)
    {
        IndirectBufferData ib_data;
        if (!capture_file.read((char *)&ib_data, sizeof(ib_data)))
            return false;

        IndirectBufferInfo ib_info;
        ib_info.m_va_addr = ib_data.m_va_addr;
        ib_info.m_size_in_dwords = ib_data.m_size_in_dwords;
        ib_info.m_skip = false;
        ibs.push_back(ib_info);
    }

    SubmitInfo submit_info((EngineType)submit_data_header.m_engine_type,
                           (QueueType)submit_data_header.m_queue_type,
                           submit_data_header.m_engine_index,
                           submit_data_header.m_is_dummy_submit,
                           std::move(ibs));
    m_submits.push_back(std::move(submit_info));
    return true;
}

//--------------------------------------------------------------------------------------------------
bool CaptureData::LoadMemoryBlock(std::istream &capture_file)
{
    MemoryRawDataHeader memory_raw_data_header;
    if (!capture_file.read((char *)&memory_raw_data_header, sizeof(memory_raw_data_header)))
        return false;

    if (memory_raw_data_header.m_size_in_bytes > kMaxMemAllocSize)
        return false;
    MemoryData raw_memory;
    raw_memory.m_data_size = memory_raw_data_header.m_size_in_bytes;
    raw_memory.m_data_ptr = new uint8_t[raw_memory.m_data_size];
    if (!capture_file.read((char *)raw_memory.m_data_ptr, memory_raw_data_header.m_size_in_bytes))
    {
        delete[] raw_memory.m_data_ptr;
        return false;
    }

    uint32_t submit_index = (uint32_t)(m_submits.size() - 1);

    m_memory.AddMemoryBlock(submit_index, memory_raw_data_header.m_va_addr, std::move(raw_memory));
    return true;
}

//--------------------------------------------------------------------------------------------------
bool CaptureData::LoadPresentBlock(std::istream &capture_file)
{
    PresentData present_data;
    if (!capture_file.read((char *)&present_data, sizeof(present_data)))
        return false;

    uint32_t submit_index = (uint32_t)(m_submits.size() - 1);
    if (present_data.m_valid_data)
    {
        PresentInfo present_info((EngineType)present_data.m_engine_type,
                                 (QueueType)present_data.m_queue_type,
                                 submit_index,
                                 present_data.m_full_screen,
                                 present_data.m_addr,
                                 present_data.m_size,
                                 present_data.m_vk_format,
                                 present_data.m_vk_color_space);
        m_presents.push_back(present_info);
    }
    else
    {
        m_presents.push_back(PresentInfo());
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
bool CaptureData::LoadTextBlock(std::istream &capture_file)
{
    TextBlockHeader text_header;

    if (!capture_file.read((char *)&text_header, sizeof(text_header)))
        return false;

    std::string name;
    if (text_header.m_name_len > kMaxStrLen || text_header.m_size_in_bytes > kMaxMemAllocSize)
        return false;
    name.reserve(text_header.m_name_len);
    if (!std::getline(capture_file, name, '\0'))
        return false;

    std::vector<char> data;
    data.resize(text_header.m_size_in_bytes);

    if (!capture_file.read((char *)data.data(), data.size()))
        return false;

    m_text.push_back(TextInfo(std::move(name), text_header.m_size_in_bytes, std::move(data)));
    return true;
}

//--------------------------------------------------------------------------------------------------
bool CaptureData::LoadWaveStateBlock(std::istream            &capture_file,
                                     const CaptureDataHeader &data_header)
{
    // Only one wave block per capture is expected.
    assert(m_waves.GetWaves().size() == 0);

    // Read chunk header
    WaveStateBlockHeader wave_header;

    if (!capture_file.read((char *)&wave_header, sizeof(wave_header)))
        return false;

    std::vector<WaveStateInfo> waves;
    if (wave_header.m_num_waves > kMaxNumWavesPerBlock)
        return false;
    for (uint32_t i = 0; i < wave_header.m_num_waves; ++i)
    {
        Dive::WaveState state;
        if (!capture_file.read((char *)&state, sizeof(state)))
            return false;

        assert(state.num_threads == 64);
        assert((state.num_vgprs % state.num_threads) == 0);
        if (state.num_sgprs > kMaxNumSGPRPerWave || state.num_vgprs > kMaxNumVGPRPerWave)
        {
            return false;
        }
        std::vector<uint32_t> sgprs(state.num_sgprs);
        if (sgprs.size() > 0)
        {
            if (!capture_file.read((char *)sgprs.data(), state.num_sgprs * sizeof(uint32_t)))
                return false;
        }

        std::vector<uint32_t> vgprs(state.num_vgprs);
        if (vgprs.size() > 0)
        {
            if (!capture_file.read((char *)vgprs.data(), state.num_vgprs * sizeof(uint32_t)))
                return false;
        }

        // If this was captured while a trap handler was active,
        // we should have 16 tmp registers.
        bool has_temps = state.WAVE_STATUS & (1 << 6);

        // Older versions didn't have temp registers captured.
        if (data_header.m_major_version == 0 && data_header.m_minor_version <= 3 &&
            data_header.m_revision < 11)
        {
            has_temps = false;
        }

        std::vector<uint32_t> ttmps;
        if (has_temps)
        {
            ttmps.resize(16);
            if (!capture_file.read((char *)ttmps.data(), 16 * sizeof(uint32_t)))
                return false;
        }

        waves.emplace_back(state, std::move(sgprs), std::move(vgprs), std::move(ttmps));
    }

    m_waves = WaveInfo(std::move(waves));
    return true;
}

//--------------------------------------------------------------------------------------------------
bool CaptureData::LoadRegisterBlock(std::istream &capture_file)
{
    // Only one wave block per capture is expected.
    assert(m_registers.GetRegisters().size() == 0);

    // Read chunk header
    RegisterBlockHeader reg_header;

    if (!capture_file.read((char *)&reg_header, sizeof(reg_header)))
        return false;

    std::map<std::string, uint32_t> regs;
    for (uint32_t i = 0; i < reg_header.m_num_registers; ++i)
    {
        std::string name;
        if (!std::getline(capture_file, name, '\0'))
            return false;

        uint32_t value;
        if (!capture_file.read((char *)&value, sizeof(uint32_t)))
            return false;

        regs.emplace(name, value);
    }

    m_registers = RegisterInfo(std::move(regs));
    return true;
}

bool CaptureData::LoadVulkanMetaDataBlock(std::istream &capture_file)
{
    return true;
}

//--------------------------------------------------------------------------------------------------
bool CaptureData::LoadGpuAddressAndSize(FileReader &capture_file,
                                        uint32_t    block_size,
                                        uint64_t   *gpu_addr,
                                        uint32_t   *size)
{
    assert(block_size >= 2 * sizeof(uint32_t));

    uint32_t dword;
    if (!capture_file.read((char *)&dword, sizeof(uint32_t)))
        return false;
    *gpu_addr = dword;
    if (!capture_file.read((char *)&dword, sizeof(uint32_t)))
        return false;
    *size = dword;

    // It's possible that only the lower 32-bits are written to the file?
    if (block_size > 2 * sizeof(uint32_t))
    {
        if (!capture_file.read((char *)&dword, sizeof(uint32_t)))
            return false;
        *gpu_addr |= ((uint64_t)(dword)) << 32;
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
bool CaptureData::LoadMemoryBlockAdreno(FileReader &capture_file, uint64_t gpu_addr, uint32_t size)
{
    MemoryData raw_memory;
    raw_memory.m_data_size = size;
    raw_memory.m_data_ptr = new uint8_t[raw_memory.m_data_size];
    if (!capture_file.read((char *)raw_memory.m_data_ptr, size))
    {
        delete[] raw_memory.m_data_ptr;
        return false;
    }

    // Unlike with Dive, all memory blocks for a submit come *before* the submit
    uint32_t submit_index = (uint32_t)(m_submits.size());
    m_memory.AddMemoryBlock(submit_index, gpu_addr, std::move(raw_memory));
    return true;
}

//--------------------------------------------------------------------------------------------------
bool CaptureData::LoadCmdStreamBlockAdreno(FileReader &capture_file,
                                           uint32_t    block_size,
                                           bool        create_new_submit)
{
    uint64_t gpu_addr;
    uint32_t size_in_dwords;
    if (!LoadGpuAddressAndSize(capture_file, block_size, &gpu_addr, &size_in_dwords))
        return false;

    IndirectBufferInfo ib_info;
    ib_info.m_va_addr = gpu_addr;
    ib_info.m_size_in_dwords = size_in_dwords;
    ib_info.m_skip = false;
    if (create_new_submit)
    {
        std::vector<IndirectBufferInfo> ibs;
        ibs.push_back(ib_info);

        SubmitInfo submit_info(EngineType::kUniversal,
                               QueueType::kUniversal,
                               0,
                               false,
                               std::move(ibs));
        m_submits.push_back(std::move(submit_info));
    }
    else
    {
        // Append it to the previous submit
        DIVE_ASSERT(!m_submits.empty());
        m_submits.back().AppendIb(ib_info);
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
void CaptureData::Finalize(const CaptureDataHeader &data_header)
{
    // 0.3.2 implemented memory tracking for IBs - ie. no more duplicate capturing
    // Can probably remove this extra check later, since there are very few captures
    // before 0.3.2
    bool duplicate_ib_capture = (data_header.m_major_version == 0) &&
                                ((data_header.m_minor_version < 3) ||
                                 ((data_header.m_minor_version == 3) &&
                                  (data_header.m_revision < 2)));

    // If capture process resets memory tracker in between every submit, then each submit is
    // "self-sufficient" in the sense that all memory referenced by that submit should be
    // captured, even if the same memory content was captured for a previous submit already.
    // This is one way to ensure all referenced memory is captured, since there is no
    // way to check that the memory content of a memory range has changed in between submits
    // (ie: there is no checksum algorithm employed in the capture process). However, this
    // bloats capture file size and affects any profiling code running at the same time, so
    // the reset is only done for certain capture modes.
    bool same_submit_copy_only = data_header.m_reset_memory_tracker;
    m_memory.Finalize(same_submit_copy_only, duplicate_ib_capture);
}

//--------------------------------------------------------------------------------------------------
std::string CaptureData::GetFileFormatVersion() const
{
    std::stringstream os;
    os << m_data_header.m_major_version << "." << m_data_header.m_minor_version << "."
       << m_data_header.m_revision;
    return os.str();
}

}  // namespace Dive
