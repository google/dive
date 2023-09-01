/*
 Copyright 2023 Google LLC

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

// Warning: This is a common file that is shared with the Dive GUI tool!
// clang-format off
#pragma once
#include <stdint.h>

#define DIVE_MAKE4CC(c0, c1, c2, c3)          \
    ((uint32_t)(unsigned char)(c0)        |   \
    ((uint32_t)(unsigned char)(c1) << 8)  |   \
    ((uint32_t)(unsigned char)(c2) << 16) |   \
    ((uint32_t)(unsigned char)(c3) << 24) )

namespace Dive
{

constexpr uint32_t kDiveFileId = DIVE_MAKE4CC('D', 'I', 'V', 'E');
constexpr uint32_t kDiveFileVersion = 1;    // Only update this if the block format (ie: BlockInfo) needs to be modified

// Tool version: <Major>.<Minor>.<Rev>
// <Major> is incremented for major events (eg: public release) that ALSO obsoletes the capture format
// <Minor> is incremented when capture format changes and can no longer be opened by older versions of tool
// <Rev> is incremented as needed but are still compatible with older versions of tool
// Note: There is no ability to increment a major version # but still retain compatibility with older tool
constexpr uint16_t kCaptureMajorVersion = 0;
constexpr uint16_t kCaptureMinorVersion = 4;
constexpr uint16_t kCaptureRevision = 1;
// 0.1: Initial proof-of-concept version (before <rev> was added)
// 0.2.0: Increase EngineType/QueueType bits, use SQTT-style markers, update Present-Data/Memory-Header
// 0.3.0: Added bits to CaptureDataHeader to indicate capture method (defer captures, etc)
// 0.3.1: Removed the capturing of SRDs and associated resources
// 0.3.2: IB capturing now go through memory-tracker: no more duplicate capturing. Inst trace enabled.
// 0.3.3: PM4 data now captured *after* SQTT data, and gfx pipeline flushed before SQTT capture
// 0.3.4: Ring, File and Register block types added.
// 0.3.5: Wave state block added.
// 0.3.6: Removed submit memory allocation type
// 0.3.7: Added Dive::MemoryAllocationData::VaPartition::CaptureReplay, added signaled and emitted fence information to Ring,
// 0.3.8: Added Dive::BlockType::kVulkanMetadata to support vulkan metadata versioning.
// 0.3.9: Added Device ID and revision to capture header, fix Capture block to have correct size
// 0.3.10: Added SubmitDataHeader::m_engine_index to record the Vulkan queue index for each submission
// 0.3.11: Added VGPR and TTMP registers added, not really a format change but new functionality.
// 0.3.11: Updated VulkanMetadataBlockHeader to capture more vulkan parameters.
// 0.3.12: Fix issues with IB caching and overwriting. Not a format change, but affects what mem blocks are captured
// 0.3.13: RGP chunk now contains record caches for pipeline hash correlation. Not a format change.
// 0.3.14: Marked m_skip_capture obsolete. Overwritten command buffer will instead have no memory blocks backing them.
// 0.3.15: Added Dive::BlockType::kPerfCounters to support perfcounters.
// 0.4.0: SDK release. Also: 0.3.14 breaks forward compability, because older tools still rely on the m_skip_capture bit
// 0.4.1: Fixed uninitialized class member |m_perf_counter_count| in the capture class. And add interface for layer to get the version number.

//--------------------------------------------------------------------------------------------------
struct FileHeader
{
    uint32_t m_file_id      = kDiveFileId;
    uint32_t m_file_version = kDiveFileVersion;
};

enum class BlockType : uint32_t
{
    kCapture        =   DIVE_MAKE4CC('C', 'A', 'P', 'T'),    // The parent block that contains metadata + the entire capture
    kMemoryAlloc    =   DIVE_MAKE4CC('A', 'L', 'L', 'C'),    // Memory allocations info
    kSubmit         =   DIVE_MAKE4CC('S', 'U', 'B', 'M'),    // Contains info for the submit and each IB
    kMemoryRaw      =   DIVE_MAKE4CC('M', 'E', 'M', 'R'),    // Raw (ie: uncompressed) memory block
    kRgp            =   DIVE_MAKE4CC('R', 'G', 'P', 'D'),    // Rgp data (includes SQTT and/or SPM)
    kPresent        =   DIVE_MAKE4CC('P', 'R', 'S', 'T'),    // A present event has occurred
    kRing           =   DIVE_MAKE4CC('R', 'I', 'N', 'G'),    // A ring descriptor
    kText           =   DIVE_MAKE4CC('T', 'E', 'X', 'T'),    // An embedded named text block
    kRegisters      =   DIVE_MAKE4CC('R', 'E', 'G', 'S'),    // Register state block
    kWaveState      =   DIVE_MAKE4CC('W', 'A', 'V', 'E'),    // Wave state block for each active wave
    kVulkanMetadata =   DIVE_MAKE4CC('V', 'U', 'L', 'K'),    // Vulkan metadata info
    kPerfCounters   =   DIVE_MAKE4CC('P', 'E', 'R', 'F')     // Perf counters
};

// Specifies a category of GPU engine.  Each category corresponds directly to a hardware engine.
// There may be multiple engines available for a given type.
enum class EngineType : uint8_t {
    kUniversal,             // Corresponds to the graphics hardware engine (a.k.a. graphics ring a.k.a 3D)
    kCompute,               // Corresponds to asynchronous compute engines (ACE)
    kDma,                   // Corresponds to SDMA engines.
    kTimer,                 // Virtual engine that only supports inserting sleeps, for implementing frame-pacing.
    kOther,
    kNone,                  // Invalid/uninitialized engine type
    kCount
};

// Specifies a category of GPU work.  Each queue type only supports specific types of work.
enum class QueueType : uint8_t {
    kUniversal, // Supports graphics commands (draws), compute commands (dispatches), and copy commands
    kCompute,   // Supports compute commands (dispatches), and copy commands
    kDma,       // Supports copy commands
    kTimer,     // Virtual engine that only supports inserting sleeps, used for implementing frame pacing. Software-only queue
    kOther,
    kNone,      // Invalid/uninitialized queue type
    kCount
};

//--------------------------------------------------------------------------------------------------
struct BlockInfo
{
    BlockType m_block_type;

    // The number of bytes that follow this header.
    uint64_t m_data_size;

    BlockInfo() {}
    BlockInfo(BlockType type, uint64_t data_size) : m_block_type(type), m_data_size(data_size) {}
};

//--------------------------------------------------------------------------------------------------
// BlockType::kCapture header
struct CaptureDataHeader
{
    enum class CaptureType : uint8_t
    {
        kSingleFrame,
        kBeginEndRange,
        kCrashDump,
    };
    uint16_t    m_major_version  = kCaptureMajorVersion;
    uint16_t    m_minor_version  = kCaptureMinorVersion;
    uint16_t    m_revision       = kCaptureRevision;
    CaptureType m_capture_type;
    uint32_t    m_pal_version;

    uint32_t    m_capture_pm4           : 1;
    uint32_t    m_capture_sqtt          : 1;
    uint32_t    m_capture_spm           : 1;
    uint32_t    m_defer_capture         : 1;
    uint32_t    m_enable_inst_trace     : 1;
    uint32_t    m_reset_memory_tracker  : 1;
    uint32_t                            : 26;

    // GPU device ID and revision
    uint16_t    m_device_id;
    uint16_t    m_device_revision;

    // Reserve some DWORDs for possible future enhancements without breaking forward compatibility
    uint32_t    m_reserved0;
    uint32_t    m_reserved1;
    uint32_t    m_reserved2;

    // ... Followed by the rest of the capture
};

//--------------------------------------------------------------------------------------------------
// BlockType::kMemoryAlloc header
struct MemoryAllocationsDataHeader
{
    enum class Type { kInternal, kGlobal };
    uint32_t    m_num_allocations;
    Type        m_type;

    // ... Followed by m_num_allocations x MemoryAllocationData
};

//--------------------------------------------------------------------------------------------------
struct MemoryAllocationData
{
    enum class GpuHeap : uint8_t
    {
        GpuHeapLocal         = 0x0,  // Local heap visible to the CPU.
        GpuHeapInvisible     = 0x1,  // Local heap not visible to the CPU.
        GpuHeapGartUswc      = 0x2,  // GPU-accessible uncached system memory.
        GpuHeapGartCacheable = 0x3,  // GPU-accessible cached system memory.
        GpuHeapCount
    };

    enum class MType : uint8_t
    {
        Default = 0,       // The kernel should use its default MTYPE.
        CachedNoncoherent, // Cache reads and writes without worrying about CPU coherency.
        CachedCoherent,    // Cache reads and writes while maintaining CPU coherency.
        Uncached,          // Always read and write through the cache.
        Count
    };

    // Enumerates the internal virtual address space partitions used for supporting multiple VA ranges.
    enum class VaPartition : uint8_t
    {
        Default,                // SEE: VaRange::Default
        DefaultBackup,          // Some platforms don't have enough virtual address space to use 4 GB sections for
                                // DescriptorTable and ShadowDescriptorTable, so we split the default VaRange into
                                // two pieces so that the other VA ranges will fit better.
        DescriptorTable,        // SEE: VaRange::DescriptorTable
        ShadowDescriptorTable,  // SEE: VaRange::ShadowDescriptorTable
        Svm,                    // SEE: VaRange::Svm
        Prt,                    // Some platforms require a specific VA range in order to properly setup HW for PRTs.
                                // To simplify client support, no corresponding VA range exists and this partition
                                // will be chosen instead of the default when required.
        CaptureReplay,          // SEE: VaRange::CaptureReplay
        Count,
    };

    // Specifies Base Level priority per GPU memory allocation as a hint to the memory manager in the event it needs to
    // select allocations to page out of their preferred heaps.
    enum class GpuMemPriority : uint8_t
    {
        Unused    = 0x0,    // Indicates that the allocation is not currently being used at all, and should be the first
                            // choice to be paged out.
        VeryLow   = 0x1,    // Lowest priority to keep in its preferred heap.
        Low       = 0x2,    // Low priority to keep in its preferred heap.
        Normal    = 0x3,    // Normal priority to keep in its preferred heap.
        High      = 0x4,    // High priority to keep in its preferred heap (e.g., render targets).
        VeryHigh  = 0x5,    // Highest priority to keep in its preferred heap.  Last choice to be paged out (e.g., page
                            // tables, displayable allocations).
        Count
    };

    // From GpuMemoryDesc
    uint64_t m_gpu_virt_addr;                   // GPU virtual address of the GPU memory allocation.
    uint64_t m_size;                            // Size of the GPU memory allocation, in bytes.
    uint64_t m_alignment;                       // Required GPU virtual address alignment, in bytes.
    uint64_t m_surface_bus_addr;                // Bus Address of SDI memory surface and marker. These will not be initialized
    uint64_t m_marker_bus_addr;                 // until the memory is made resident. Client needs to call
                                                // InitBusAddressableGpuMemory() to query and update before this is valid.

    uint32_t m_preferred_heap           : 8;	// The preferred heap of the GPU memory.
    uint32_t m_is_virtual               : 1;    // GPU memory is not backed by physical memory and must be remapped before the
                                                // GPU can safely access it. Will also be set for sdiExternal allocations. See
                                                // GpuMemoryCreateFlags::sdiExternal
    uint32_t m_is_peer                  : 1;    // GPU memory object was created with @ref IDevice::OpenPeerGpuMemory.
    uint32_t m_is_shared                : 1;    // GPU memory object was created either with
                                                // @ref IDevice::OpenExternalSharedGpuMemory or OpenSharedGpuMemory.
                                                // This IGpuMemory references memory created either by another process or another
                                                // device with the exception of peer access.
    uint32_t m_is_external              : 1;    // GPU memory object was created with @ref IDevice::OpenExternalSharedGpuMemory.
                                                // This IGpuMemory references memory that was created either by another process
                                                // or by a device that doesn't support sharedMemory with this object's device
                                                // (i.e., MDA sharing on Windows).
    uint32_t m_is_svm_alloc             : 1;    // GPU memory is allocated in system memory.
                                                // Valid only when IOMMUv2 is supported
    uint32_t m_is_executable            : 1;    // GPU memory is used for execution. Valid only when IOMMUv2 is supported
    uint32_t m_is_extern_phys           : 1;    // GPU memory is External Physical memory

    // From GpuMemoryFlags
    uint32_t m_is_pinned                : 1;    // GPU memory was pinned for GPU access from CPU memory.
    uint32_t m_is_flippable             : 1;    // GPU memory can be used by flip presents.
    uint32_t m_is_stereo                : 1;    // GPU memory will be used for a stereoscopic surface.
    uint32_t m_is_client                : 1;    // GPU memory is requested by the client.
    uint32_t m_is_shareable             : 1;    // GPU memory can be shared with other Device's without needing peer
                                                // transfers.
    uint32_t m_is_interprocess          : 1;    // GPU memory is visible to other processes (they may choose to open it).
    uint32_t m_is_page_directory        : 1;    // GPU memory will be used for a Page Directory.
    uint32_t m_is_page_table_block      : 1;    // GPU memory will be used for a Page Table Block.
    uint32_t m_is_cmd_allocator         : 1;    // GPU memory is owned by an ICmdAllocator.
    uint32_t m_is_udma_buffer           : 1;    // GPU memory will be used for a UDMA buffer containing GPU commands which
                                                // are contained within a command buffer.
    uint32_t m_is_unmap_info_buffer     : 1;    // GPU memory will be used for GPU DMA writeback of unmapped VA ranges.
    uint32_t m_is_history_buffer        : 1;    // GPU memory will be used for a history buffer containing GPU timestamps.
    uint32_t m_is_xdma_buffer           : 1;    // GPU memory will be used for an XDMA cache buffer for transferring data
                                                // between GPU's in a multi-GPU configuration.
    uint32_t m_is_turbo_sync_surface    : 1;    // GPU memory is private swapchain primary allocation for TurboSync.
    uint32_t m_is_always_resident       : 1;    // GPU memory needs to always be resident for proper operation. Not all
                                                // platforms care about this flag.
    uint32_t m_was_buddy_allocated      : 1;    // GPU memory was allocated by a buddy allocator to be used for
                                                // suballocating smaller memory blocks.
    uint32_t m_is_local_only            : 1;    // GPU memory doesn't prefer nonlocal heaps.
    uint32_t m_is_non_local_only        : 1;    // GPU memory doesn't prefer local heaps.
    uint32_t m_is_cpu_visible           : 1;    // GPU memory is CPU accessible via Map().
    uint32_t m_is_private_screen        : 1;    // GPU memory is bound to a private screen image.
    uint32_t m_is_user_queue            : 1;    // GPU memory is used for an user queue.
    uint32_t m_is_globally_coherent     : 1;    // GPU memory is globally coherent.
    uint32_t m_is_timestamp             : 1;    // GPU memory will be used for KMD timestamp writeback.
    uint32_t m_is_global_gpu_va         : 1;    // GPU memory virtual address must be visible to all devices.
    uint32_t m_is_gpu_va_pre_reserved   : 1;    // GPU memory virtual address is provided by client and was previously
                                                // reserved by other object. Current object should not free this VA.
    uint32_t m_is_typed_buffer          : 1;    // GPU memory is bound to a typed buffer.
    uint32_t m_is_accessed_physically   : 1;    // GPU memory can be accessed physically (physical engine like MM video).
    uint32_t m_is_bus_addressable       : 1;    // GPU memory is Bus Addressable memory
    uint32_t m_is_auto_priority         : 1;    // GPU memory priority is to be managed automatically
    uint32_t m_is_peer_writable         : 1;    // GPU memory can be open as peer memory and be writable
    uint32_t m_is_mappped_to_peer_memory: 1;    // GPU memory is remapped to at least one peer physical memory.
    uint32_t m_is_restricted_content    : 1;    // GPU memory is protected content
    uint32_t m_is_restricted_access     : 1;    // GPU memory is restricted shared access resource
    uint32_t m_is_cross_adapter         : 1;    // GPU memory is shared cross-adapter resource
    uint32_t m_queue_alloc              : 1;    // Whether this allocation is queue-specific and not in the global list
    uint32_t m_submit_alloc             : 1;    // Whether this is a submit allocation
    uint32_t                            : 13;

    // From GpuMemory
    MType           m_mtype;

    // Priority of the memory allocation: serves as a hint to the operating system of how important it is to keep
    // this memory object in its preferred heap(s).
    GpuMemPriority  m_priority;

    VaPartition     m_va_partition;
    uint8_t         m_heap_count;
    GpuHeap         m_heaps[(uint32_t)GpuHeap::GpuHeapCount];
};

//--------------------------------------------------------------------------------------------------
// BlockType::kSubmit header
struct SubmitDataHeader
{
    uint8_t    m_num_ibs;                   // The max is bounded by Pal::Amdgpu::MaxIbsPerSubmit, which is 16 currently
    uint8_t    m_engine_value         : 4;  // The original Pal::EngineType value
    uint8_t    m_queue_value          : 3;  // The original Pal::QueueType value
    uint8_t    m_is_dummy_submit      : 1;  // amdgpu won't give us a new fence value unless the submission has at least one command buffer.
                                            // so a "dummy" submit that does nothing is needed
    EngineType m_engine_type;
    QueueType  m_queue_type;
    uint16_t   m_engine_index : 3;
    uint16_t   m_padding      : 13;

    // ... Followed by m_num_ibs x IndirectBufferData
};

//--------------------------------------------------------------------------------------------------
struct IndirectBufferData
{
    uint64_t m_va_addr;
    uint32_t m_size_in_dwords;
    uint16_t m_is_constant_engine     : 1;  // This IB should be submitted to CE
    uint16_t m_is_preemption_enabled  : 1;  // Preamble flag, which means the IB could be dropped if no context switch
    uint16_t m_drop_if_same_context   : 1;  // Preempt flag, IB should set Pre_enb bit if PREEMPT flag detected
    uint16_t m_skip_capture_OBSOLETE  : 1;  // (Obsolete)IB was not captured because it was overwritten at time of capture (ie: deferred-capturing)
    uint16_t                          : 12;
};

//--------------------------------------------------------------------------------------------------
// BlockType::kMemoryRaw header
struct MemoryRawDataHeader
{
    uint64_t m_va_addr;
    uint32_t m_size_in_bytes;
    uint32_t m_reserved;

    // ... Followed by raw memory data
};

//--------------------------------------------------------------------------------------------------
// BlockType::kPresent body
struct PresentData
{
    uint8_t m_valid_data             : 1;  // Whether the rest of the fields are valid or not
    uint8_t m_engine_type            : 3;  // 0: Universal, 1: Compute, 2: Exclusive-Compute, 3: Dma, 4: Timer: 5: HP-Universal, 6: HP-Graphics
    uint8_t m_queue_type             : 2;  // 0: Universal, 1: Compute, 2: Dma, 3: Timer
    uint8_t m_full_screen            : 1;
    uint8_t                          : 1;
    uint64_t m_addr;
    uint64_t m_size;
    uint32_t m_vk_format;
    uint32_t m_vk_color_space;
};

//-------------------------------------------------------------------------------------------------
// BlockType::kRgp body
struct RgpDataHeader
{
    // Reserve some DWORDs for future use
    uint32_t    m_reserved0;
    uint32_t    m_reserved1;
    uint32_t    m_reserved2;
    uint32_t    m_reserved3;
};


//-------------------------------------------------------------------------------------------------
// BlockType::kRing body
struct RingData
{
    uint64_t m_va_addr;             // Ring VA
    uint32_t m_size_in_bytes;       // Total size of ring buffer

    QueueType m_queue_type;
    uint8_t m_queue_index;
    uint16_t m_reserved;

    uint32_t m_rptr;                // Ring read pointer (offset)
    uint32_t m_wptr;                // Ring write pointer (offset)
    uint32_t m_driver_wptr;         // Ring driver write pointer (offset)

    uint32_t m_signaled_fence;      // Last signalled fence by the GPU
    uint32_t m_emitted_fence;       // Last emitted fence by the KMD
    uint64_t m_signaled_fence_va;   // Address of the signaled fence packet
    uint64_t m_emitted_fence_va;    // Address of the emitted fence packet

    // Note: we always capture the full ring, but only parse the section of interest betwen 2 fences.
    uint32_t m_ring_captured_size;  // Size of the ring that was captured

    uint64_t m_rb_va;               // Address of Ring read ptr
    uint32_t m_rb_bytes_remaining;  // Size left in Ring

    uint64_t m_ib1_va;              // Address of IB1 or 0 if none
    uint32_t m_ib1_bytes_remaining; // Size left in IB1

    uint64_t m_ib2_va;              // Address of IB2 or 0 if none
    uint32_t m_ib2_bytes_remaining; // Size left in IB2
};

//-------------------------------------------------------------------------------------------------
// BlockType::kText body
struct TextBlockHeader
{
    uint32_t m_name_len;
    uint64_t m_size_in_bytes;
    uint32_t m_reserved0;

    // Followed by:
    // char name[m_name_len] // null-ternimated string
    // uint8_t data[m_size_in_bytes]
};

//-------------------------------------------------------------------------------------------------
// BlockType::kRegister body
struct RegisterBlockHeader
{
    uint32_t m_num_registers;
    uint32_t m_reserved0;
    uint32_t m_reserved1;
    uint32_t m_reserved2;

    // Followed by:
    // { char *name, uint32_t } pairs
};

//-------------------------------------------------------------------------------------------------
// BlockType::kWaveState body
struct WaveStateBlockHeader
{
    uint32_t m_num_waves;
    uint32_t m_reserved0;
    uint32_t m_reserved1;
    uint32_t m_reserved2;

    // Followed by array of WaveState and SGPR and VGPR data:
    // { WaveState, uint32_t[num_sgprs], uint32_t[num_vgprs] }[m_num_waves]
    //
};

//--------------------------------------------------------------------------------------------------
enum class CounterType : uint8_t {
    kSqttCounters,
    kLegacyCountersPerDraw,
    kLegacyCountersPerRenderPass
};

//--------------------------------------------------------------------------------------------------
// BlockType::kPerfCounters header
struct PerfCountersBlockHeader
{
    uint32_t m_num_counters : 24;
    uint32_t m_counter_type : 8;  // CounterType
    // ... Followed by raw memory data
};

//-------------------------------------------------------------------------------------------------
// Contains the execution state for a single wave
// It does not contain SGPR or VGPR values
struct WaveState
{
    uint32_t num_threads; // 64 for Vega, variable for Navi
    uint32_t num_sgprs; // num_sgprs DWORDS
    uint32_t num_vgprs; // num_thread * num_vgprs DWORDS

    /*
    [3:0] wave id
    [5:4] simd id
    [7:6] pipe id
    [11:8] cu id
    [12] sh id
    [14:13] se id
    [19:16] tg id
    [23:20] vm id
    [26:24] queue id
    [29:27] state id
    [31:30] me_id
    */
    uint32_t HW_ID;
    uint32_t WAVE_STATUS;
    uint32_t PC_HI;
    uint32_t PC_LO;
    uint32_t EXEC_HI;
    uint32_t EXEC_LO;
    uint32_t INST_DW0;
    uint32_t INST_DW1;
    uint32_t GPRALLOC;
    uint32_t LDSALLOC;
    uint32_t TRAPSTS;
    uint32_t IBSTS;
    uint32_t TBA_HI;
    uint32_t TBA_LO;
    uint32_t TMA_HI;
    uint32_t TMA_LO;
    uint32_t IB_DBG0;
    uint32_t M0;
};

//-------------------------------------------------------------------------------------------------
// BlockType::kVulkanMetadata body
struct VulkanMetadataBlockHeader
{
    uint32_t m_major_version : 16;
    uint32_t m_minor_version : 16;
    uint32_t m_num_metadata;    // Update: Use the |m_reserved0|  as m_num_metadata to counts how many VulkanMetadata in this block. 
    uint32_t m_reserved1;
    uint32_t m_reserved2;
};

struct VulkanMetadataHeader
{
    uint32_t type;  // The type of the blob, used to determine how to encode/decode the blob.
    uint32_t size;
};

} // Dive
// clang-format on
