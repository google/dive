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

#include "hover_help_model.h"
#include <QTextDocument>
#include <sstream>
#include <string>
#include "dive_core/command_hierarchy.h"
#include "dive_core/common.h"
#include "dive_core/common/gpudefs.h"
#include "dive_core/common/pm4_packets/me_pm4_packets.h"
#include "dive_core/data_core.h"

//--------------------------------------------------------------------------------------------------
HoverHelp::HoverHelp()
{
    // Placeholder event/barrier name functions, so that release builds wont crash if a hover help
    // with an event name somehow gets called before the SQTT data is ready.
    m_event_name_fn = [](uint32_t) -> std::string {
        DIVE_ASSERT(false);
        return "???";
    };
    m_barrier_name_fn = [](uint32_t) -> std::string {
        DIVE_ASSERT(false);
        return "???";
    };
    m_barrier_params_fn = [](uint32_t) -> std::string {
        DIVE_ASSERT(false);
        return "???";
    };
    m_label_name_fn = [](uint32_t) -> std::string {
        DIVE_ASSERT(false);
        return "???";
    };
}

//--------------------------------------------------------------------------------------------------
void HoverHelp::SetCurItem(Item        item,
                           uint32_t    param1,
                           uint32_t    param2,
                           uint32_t    param3,
                           const char* custom_string_ptr)
{
    std::string custom_string;
    if (custom_string_ptr != nullptr)
        custom_string = custom_string_ptr;
    // Return if new parameters as same as previous parameters
    if (m_prev_item == item && m_prev_param1 == param1 && m_prev_param2 == param2 &&
        m_prev_param3 == param3 && m_prev_custom_string == custom_string)
    {
        return;
    }

// Macro out the Switch beg/end to avoid clang-format from indenting the CASE macros
#define SWITCH()  \
    switch (item) \
    {
#define END_SWITCH() \
    }                \
    ;
#define CASE(index, ...)                                        \
    case Item::index:                                           \
        cur_string_size = snprintf(NULL, 0, __VA_ARGS__) + 1;   \
        cur_string.resize(cur_string_size);                     \
        snprintf(&cur_string[0], cur_string_size, __VA_ARGS__); \
        break;
#define CASE_CUSTOM(index, func)                   \
    case Item::index:                              \
        cur_string = func(param1, param2, param3); \
        break;
#define CASE_EMPTY(index) \
    case Item::index: break;

    std::string cur_string;
    int64_t     cur_string_size;
    const char* kShaderStageNames[Dive::kShaderStageCount] = {
        "Cs",  // kShaderStageCs
        "Gs",  // kShaderStageGs
        "Hs",  // kShaderStageHs
        "Ps",  // kShaderStagePs
        "Vs",  // kShaderStageVs
    };
    SWITCH()
    CASE_EMPTY(kNone);
    CASE(kSqttSeView,
         "This visualization shows Event shader stage occupancy on each of the 4 Shader Engines. "
         "Occupancy does not represent utilization, ie. an occupying stage can be idle for most or "
         "part of its duration");
    CASE(kSqttCuView,
         "This visualization shows Event shader stage occupancy for Shader Engine %d. Occupancy "
         "does not represent utilization, ie. an occupying stage can be idle for most or part of "
         "its duration",
         param1);
    CASE(kSqttSimdView,
         "This visualization shows Event shader stage occupancy for Shader Engine %d, Compute Unit "
         "%d. Occupancy does not represent utilization, ie. an occupying stage can be idle for "
         "most or part of its duration",
         param1,
         param2);
    CASE(kSqttSlotView,
         "This visualization shows wavefront occupancy for Shader Engine %d, Compute Unit %d, Simd "
         "%d. Occupancy does not represent utilization, ie. an occupying wavefront can be idle "
         "for most or part of its duration. There is instruction buffer \"slots\" to hold the PC "
         "for up to 10 wavefronts per Simd, which means up to 10 wavefronts can be occupying a "
         "single Simd at once",
         param1,
         param2,
         param3);
    CASE(kContextView,
         "This visualization shows which of the 7 contexts each graphics Event occupies. This is "
         "a useful view to visualize where the context roll stalls occur. See "
         "https://gpuopen.com/learn/understanding-gpu-context-rolls/ for more information about "
         "contexts and context rolls");
    CASE(kBarChartView,
         "Provides various bar charts to help quickly identify areas of interest. Provides the "
         "option to visually \"serialize\" the Events so they do not overlap and are easier to "
         "see");
    CASE(kSqttSeText,
         "This row shows the occupancy time of each Shader Stage for each Event scheduled on "
         "Shader Engine %d. Each block in this row represents the start time of the earliest "
         "wavefront and the end time of the last wavefront for a particular occupying Event",
         param1);
    CASE(kSqttCuText,
         "This row shows the occupancy time of each Shader Stage for each Event scheduled on "
         "Compute Unit %d. Each block in this row represents the start time of the earliest "
         "wavefront and the end time of the last wavefront for a particular occupying Event",
         param1);
    CASE(kSqttSimdText,
         "This row shows the occupancy time of each Shader Stage for each Event scheduled on "
         "Simd %d. Each block in this row represents the start time of the earliest "
         "wavefront and the end time of the last wavefront for a particular occupying Event",
         param1);
    CASE(kSqttSlotText,
         "This row shows the duration and presence of wavefronts scheduled on slot %d of the "
         "current Simd",
         param1);
    CASE(kSqttContextText,
         "This row shows the duration and presence of Events scheduled on hardware context %d",
         param1);
    CASE(kSqttCuViewSummarySe,
         "This row shows Event occupancy for Shader Engine %d. The start and end times for each "
         "Event in this row is the min/max of the start and end times of each Event over all the "
         "Compute Units below",
         param1);
    CASE(kSqttSimdViewSummaryCu,
         "This row shows Event occupancy for Compute Unit %d. The start and end times for each "
         "Event in this row is the min/max of the start and end times of each Event over all the "
         "Simds below",
         param1);
    CASE(kSqttSlotViewSummarySimd,
         "This row shows Event occupancy for Simd %d. The start and end times for each "
         "Event in this row is the min/max of the start and end times of all Event wavefronts "
         "scheduled over all the Slots below",
         param1);
    CASE(kSqttInstTraceCheckbox,
         "Enable/disable showing instruction trace - which shows when each shader instruction is "
         "scheduled for each wavefront it is enabled on. Instruction trace is only available on "
         "the first Simd of the first enabled Compute Unit (Simd0 of Compute Unit %d) on each "
         "Shader Engine",
         param1);
    CASE(kSqttZoomToFitPushButton,
         "Zooms so that the current selection fills the view. Also centers the view on the current "
         "selection");
    CASE(kSqttZoomFullPushButton, "Zooms out fully so that the full capture fills the view");
    CASE(kSqttRuler,
         "The x-axis represents time in shader core cycles. The shader clock attempts to run at a "
         "fixed %d Mhz during profiling mode, but can still fluctuate depending on power and heat "
         "characteristics on the GPU. During non-profiling mode, the frequency can fluctuate "
         "anywhere between %d-%d Mhz depending on the part of the frame being processed",
         param1,
         param2,
         param3);
    CASE(kSqttSeRect,
         "This represents occupancy time of all %s shader stage wavefronts for Event %s on Shader "
         "Engine %d. This is derived from the min/max of all Event %s %s shader stage wavefront "
         "occupancy times across all Shader Engine %d Compute Units",
         kShaderStageNames[param1],
         m_event_name_fn(param2).c_str(),
         param3,
         m_event_name_fn(param2).c_str(),
         kShaderStageNames[param1],
         param3);
    CASE(kSqttCuRect,
         "This represents occupancy time of all %s shader stage wavefronts for Event %s on "
         "Compute Unit %d of the current Shader Engine. This is derived from the min/max of all "
         "Event %s %s shader stage wavefront occupancy times across all Compute Units %d Simds on "
         "the current Shader Engine",
         kShaderStageNames[param1],
         m_event_name_fn(param2).c_str(),
         param3,
         m_event_name_fn(param2).c_str(),
         kShaderStageNames[param1],
         param3);
    CASE(kSqttSimdRect,
         "This represents occupancy time of all %s shader stage wavefronts for Event %s on "
         "Simd %d of the current Compute unit. This is derived from the min/max of all "
         "Event %s %s shader stage wavefront occupancy times across all Simd %d Slots on "
         "the current Compute Unit",
         kShaderStageNames[param1],
         m_event_name_fn(param2).c_str(),
         param3,
         m_event_name_fn(param2).c_str(),
         kShaderStageNames[param1],
         param3);
    CASE(kSqttSlotRect,
         "This represents occupancy time of a single wavefront (out of %d) of Event %s occupying "
         "Slot %d of the current Simd. Wavefront occupancy can include periods when wavefronts "
         "from this Event are idle and waiting to be scheduled, so the duration does not represent "
         "how long the Event takes to process",
         param1,
         m_event_name_fn(param2).c_str(),
         param3);
    CASE(kSqttGap,
         "This is a gap of %d idle cycles on the GPU where no Event is being scheduled and no "
         "wavefronts are occupying the hardware. This is likely due to a GPU stall waiting on the "
         "CPU (ie. GPU starving for work).<br><br>A gap <b>may</b> be the result of the Dive "
         "capture process: there is an added GPU pipeline flush that occurs at the beginning and "
         "end of the frame during the capture process. Any intra-frame GPU work is forced to "
         "finish at the frame boundary, which can affect frame timing layout and sometimes results "
         "in gaps as the CPU tries to catch up to the GPU",
         param1);
    CASE(kSqttDisabledCu,
         "The given Compute Unit is disabled for this GPU. There are only %d Compute Units active "
         "per Shader Engine",
         param1);
    CASE(kContextStalledEvent,
         "This Event's start time has been stalled due to all hardware contexts being occupied "
         "when it was trying to be scheduled. This is because this Event requires an open context "
         "for its context roll. See https://gpuopen.com/learn/understanding-gpu-context-rolls/ "
         "for more information about contexts and context rolls");
    CASE(kNavigatorGfxText,
         "This row shows the duration and presence of Events that have wavefronts scheduled on the "
         "universal engine (aka main Vulkan graphics queue) of the GPU. The start point of each "
         "Event is the min(Event-start on each Shader Engine), and the end point of each "
         "Event is the max(Event-end on each Shader Engine)");
    CASE(kNavigatorAceText,
         "This row shows the duration and presence of Events that have wavefronts scheduled on "
         "asynchronous compute engine (ACE) %c of the GPU. The start point of each Event is the "
         "min(Event-start on each Shader Engine), and the end point of each Event is the "
         "max(Event-end on each Shader Engine). Note that Vulkan queues do not always map 1:1 with "
         "the ACE engines, since the driver can choose to schedule submits from different compute "
         "Vulkan queues to the same ACE queue",
         'A' + param1);
    CASE(kNavigatorEvent,
         "This block represents the duration of event %s. The Event starts on the earliest Event "
         "initiation time across all Shader Engines and ends when the last shader stage completes "
         "across all the Shader Engines. The wavefront occupancy range for each Shader Stage is "
         "represented as individual colored sub-blocks. Please note that this represents a "
         "summarized view of the Event across all Shader Engines, but it is possible that "
         "wavefronts for this Event only appear on a subset of Shader Engines.<br><br>"
         "Note: Gaps between Event start and the start of the first shader stage (and between "
         "shader stages) can be due to parameter cache or vgpr/sgpr pressures",
         m_event_name_fn(param1).c_str());
    CASE(kNavigatorGfxEventNoPs,
         "Event %s has a VS shader stage but no PS shader stage. In this case, the end of the "
         "last shader stage represents when the ROP units are finished writing to the surface",
         m_event_name_fn(param1).c_str());
    CASE(kNavigatorVulkanBarrier,
         "This block represents the duration of Vulkan barrier %s<br>%s",
         m_barrier_name_fn(param1).c_str(),
         m_barrier_params_fn(param1).c_str());
    CASE(kNavigatorDriverBarrier,
         "This block represents the duration of driver-inserted barrier %s<br>%s",
         m_barrier_name_fn(param1).c_str(),
         m_barrier_params_fn(param1).c_str());
    CASE(kNavigatorDebugLabel,
         "This block represents the duration of debug marker <b>%s</b>. This label includes %s."
         "The duration of the debug marker includes all Events and barriers that are part of the "
         "debug marker.<br><br>The debug marker rectangle represents only 1 level of the "
         "hierarchy at a time. It can be collapsed/expanded from the Events view only",
         m_label_name_fn(param1).c_str(),
         m_label_includes_fn(param1).c_str());
    CASE(kEngineView,
         "This view shows all the command buffers submitted to each GPU \"engine\". The "
         "<b>universal engine</b> supports graphics and compute workloads, the "
         "<b>compute engine</b> supports async compute, and the <b>dma engine</b> utilizes the "
         "GPU's DMA hardware. The \"Engine Index\" specifies which instance of the hardware engine "
         "is being used. For example, there can be multiple queues in the <b>compute engine</b>, "
         "and the \"Engine Index\" can be used to distinguish between them");
    CASE(kSubmitView,
         "This view shows all the submits that were made in the captured frame, in the order they "
         "were submitted to the driver. Each submit is composed of 1 or more <b>Indirect "
         "Buffers</b> (IBs), and each Vulkan command buffer is usually represented by a single "
         "IB.<br><br> Note: The driver can insert up to 3 preamble IBs and 2 postamble IBs for "
         "driver purposes. This means that a vkQueueSubmit with 1 command buffer may result in a "
         "Submit with 6 IBs");
    CASE(kVulkanEventsView,
         "This view shows, for each submit, the list of Vulkan Events associated with that submit. "
         "A Vulkan Event is a Vulkan command that causes the GPU to do actual work (as opposed to, "
         "for example, setting a register or state). These Events include draw, dispatch, barrier, "
         "renderpass, clear, buffer/image, and query pool commands");
    CASE(kAllVulkanCallsView,
         "This view shows, for each submit, <b>all Vulkan commands</b> in that submit, even ones "
         "that do not generate any PM4 packets");
    CASE(kAllVulkanCallsGpuEventsView,
         "This view shows, for each submit, <b>all Vulkan commands</b> in that submit, even ones "
         "that do not generate any PM4 packets. In addition, where appropriate, underlying GPU "
         "Events for each Vulkan command are shown. For example, a Vulkan Barrier may be composed "
         "of 1 or more underlying ReleaseMem() or AcquireMem() GPU Events. A GPU Event is any "
         "sequence of PM4 commands that causes the GPU to do actual work or synchronization (as "
         "opposed to, for example, setting a register or state). These Events include draws, "
         "dispatches, and any synchronization or GPU cache operation");
    CASE(kChainNode,
         "CHAIN packets are created by the driver. The driver manages <b>Indirect Buffers</b> (ie. "
         "command buffers) in granularities of 64KB each. When more than 64KB (16384 dwords) is "
         "required for an IB, a CHAIN packet is inserted to point to the next 64KB chunk. CHAINS "
         "are also used for secondary command buffers (ie. vkCmdExecuteCommands), as well as used "
         "to CHAIN together multiple commands buffers from within single submits");
    CASE(kIbNode,
         "<b>Indirect Buffers</b>, or IBs, are raw PM4 format command buffers passed to the GPU. A "
         "Vulkan command buffer is translated by the driver into a PM4 command buffer for "
         "consumption by the GPU. An IB can be at most 64KB (16384 dwords). When more than 64KB is "
         "required for an IB, a CHAIN packet is inserted to point to the next 64KB chunk");
    CASE(kVulkanEmptyNode,
         "This Vulkan command does not generate any PM4 packets. Some Vulkan commands do not "
         "generate any PM4 packets. Rather, they update the internal state tracker, and the "
         "resulting state is set in the actual draw call");
    CASE(kBarrierNode,
         "This node represents a sub-barrier that groups together a sequence of GPU operations "
         "used to implement a barrier")
    CASE(kBarrierNoopNode,
         "This node represents a sub-barrier that evaluates to a NO-OP. No operation is needed on "
         "this GPU to implement this Vulkan call")
    CASE(kIbNodeNotFullyCaptured,
         "The memory for this command buffer was reset mid-way through the captured frame. As a "
         "result, this command buffer could not be captured. Please make sure not to re-use or "
         "unnecessarily reset a command buffer so that it can be captured");
    CASE_CUSTOM(kSyncNode, GetSyncNodeString);
    CASE(kBarrierMultiEvent,
         "This Vulkan barrier is composed of more than 1 GPU dispatches/draws and/or sub-barriers. "
         "The implementation of certain layout transitions may be done via draws/dispatches and/or "
         "sub-barriers. For example, a transition of an image to SHADER_READ may involve a "
         "full-screen fast-clear-elimination draw and some sub-barriers");
    CASE(kRenderPassMultiEvent,
         "This Vulkan renderpass command is composed of more than 1 GPU dispatches/draws and/or "
         "sub-barriers. The GPU may implement certain layout transitions or operations via "
         "draws/dispatches. For example, a color clear operation in a vkCmdBeginRenderPass may be "
         "implemented by a dispatch and some sub-barriers");
    CASE(kQueryPoolMultiEvent,
         "vkCmdResetQueryPool is composed of 3 Events: 1 GPU dispatch and a pre and post barrier");
    CASE(kEventsDuration,
         "Duration between start of Command Processor (CP) initiation of Event %s to the end of "
         "that Event. This duration includes actual occupancy durations as well as stall "
         "durations. Reasons for stalls can include: register pressure, parameter cache pressure, "
         "waiting for ROP units to finish writing, and waiting for earlier Events to retire "
         "(Events in the same queue must end in the order they were initiated)",
         m_event_name_fn(param1).c_str());
    CASE(kEventsOccupancyDuration,
         "Duration during which Event %s wavefronts are occupying the shader cores. Does not "
         "include any stall durations",
         m_event_name_fn(param1).c_str());
    CASE(kGFRCompleted, "This Vulkan API command has completed and did not crash.");
    CASE(kGFRIncomplete,
         "This Vulkan API command was being executed by the GPU at the time of the crash and could "
         "potentially be the reason for the crash.");
    CASE(kGFRPending,
         "This Vulkan API command did not start execution at the time of the crash and could not "
         "be the reason for the crash.");
    CASE(kWaveStateActive,
         "This event has shader wavefronts that are active in the system at the time of the crash. "
         "It could cause a hang, however it is unlikely to cause a crash.");
    CASE(kWaveStateActiveCausal,
         "This event has shader wavefronts that are active in the system at the time of the crash. "
         "It could cause a hang or crash.");
    CASE(kWaveStateFatal, "This event has shader wavefronts that have crashed.");
    CASE(kEngineCacheMatch,
         "This event was found in a list of recently processed events, and so we have marked it as "
         "suspicious.");
    CASE(kShaderViewWaveStateNote,
         "The current instruction pointers are highlighted. They could point to the instruction "
         "been executed or the next instruction to be executed.");
    END_SWITCH();

    // Concatenate any additional custom string
    cur_string += custom_string;

    m_prev_item = item;
    m_prev_param1 = param1;
    m_prev_param2 = param2;
    m_prev_param3 = param3;
    m_prev_custom_string = custom_string;
    emit CurrStringChanged(QString::fromUtf8(cur_string.c_str()));
}

//--------------------------------------------------------------------------------------------------
void HoverHelp::SetCommandHierarchyNodeItem(const Dive::CommandHierarchy& command_hierarchy,
                                            uint32_t                      node_index)
{
    bool hover_msg_sent = false;

    Dive::NodeType node_type = command_hierarchy.GetNodeType(node_index);
    if (node_type == Dive::NodeType::kIbNode)
    {
        bool fully_captured = command_hierarchy.GetIbNodeIsFullyCaptured(node_index);
        if (!fully_captured)
        {
            hover_msg_sent = true;
            SetCurItem(Item::kIbNodeNotFullyCaptured);
        }
        else
        {
            // CHAIN node
            Dive::IbType type = command_hierarchy.GetIbNodeType(node_index);
            if (type == Dive::IbType::kChain)
            {
                hover_msg_sent = true;
                SetCurItem(Item::kChainNode);
            }
            else if (type == Dive::IbType::kNormal)
            {
                hover_msg_sent = true;
                SetCurItem(Item::kIbNode);
            }
        }
    }
    else if (node_type == Dive::NodeType::kSyncNode)
    {
        hover_msg_sent = true;
        Dive::SyncType sync_type = command_hierarchy.GetSyncNodeSyncType(node_index);
        Dive::SyncInfo sync_info = command_hierarchy.GetSyncNodeSyncInfo(node_index);
        SetCurItem(Item::kSyncNode, (uint32_t)sync_type, sync_info.m_u32All);
    }

    if (!hover_msg_sent)
    {
        SetCurItem(Item::kNone);
    }
}

//--------------------------------------------------------------------------------------------------
std::string HoverHelp::GetSyncNodeString(uint32_t param1, uint32_t param2, uint32_t param3)
{
    Dive::SyncType sync_type = (Dive::SyncType)param1;
    Dive::SyncInfo sync_info;
    sync_info.m_u32All = param2;

    std::ostringstream os;
    if (sync_type == Dive::SyncType::kAcquireMem)
    {
        if (sync_info.acquiremem.m_coher_base_set)
            os
            << "This AcquireMem stalls the CP's micro-engine (ME) until all surfaces (eg. render "
               "targets) within the address range are inactive. ";
        else
            os
            << "This AcquireMem stalls the CP's micro-engine (ME) until all surfaces (eg. render "
               "targets) are inactive. ";

        // Determine if any cache ops
        Dive::SyncInfo sync_info_common;
        sync_info_common.m_u32All = sync_info.m_u32All;
        sync_info_common.common.m_releasemem_acquiremem_common = 0;
        if (sync_info_common.m_u32All != 0 || sync_info.acquiremem.m_wb_inv_db ||
            sync_info.acquiremem.m_inv_sq_k || sync_info.acquiremem.m_inv_sq_i ||
            sync_info.acquiremem.m_flush_sq_k)
        {
            os << "Once the stall phase completes, the following cache actions are performed:";
        }
        if (sync_info.acquiremem.m_wb_inv_db)
            os << std::endl << "  wbInvDb: Db caches flushed and invalidated";
        if (sync_info.acquiremem.m_inv_sq_k)
            os << std::endl
               << "  invSqK: Sq scalar constant cache (for scalar register reads) invalidated";
        if (sync_info.acquiremem.m_inv_sq_i)
            os << std::endl << "  invSqI: Sq instruction cache (for shader programs) invalidated";
        if (sync_info.acquiremem.m_flush_sq_k)
            os << std::endl
               << "  flushSqK: Sq scalar constant cache (for scalar register reads) flushed";
    }
    else if (sync_type == Dive::SyncType::kWaitRegMem)
    {
        os << "WaitRegMem stalls the CP until a location in register or memory space satisfies the "
              "specified comparison operation.";
    }
    else if (sync_type == Dive::SyncType::kReleaseMem)
    {
        os << "This ReleaseMem triggers the following actions when ";
        if (sync_info.releasemem.m_bottom_of_pipe)
            os << " the end-of-pipe event is reached: ";
        else if (sync_info.releasemem.m_cs_done)
            os << " when all CS wavefronts are completed: ";
        else if (sync_info.releasemem.m_ps_done)
            os << " when all PS wavefronts are completed: ";
        switch (sync_info.releasemem.m_write_data_sel)
        {
        case Dive::data_sel__me_release_mem__none: break;
        case Dive::data_sel__me_release_mem__send_32_bit_low:
            os << "Writes a 32-bit data value to the specified address. ";
            break;
        case Dive::data_sel__me_release_mem__send_64_bit_data:
            os << "Writes a 64-bit data value to the specified address. ";
            break;
        case Dive::data_sel__me_release_mem__send_gpu_clock_counter:
            os << "Writes the 64-bit GPU clock counter to the specified address. ";
            break;
        case Dive::data_sel__me_release_mem__send_cp_perfcounter_hi_lo:
            os << "Writes the 64-bit system clock counter to the specified address. ";
            break;
        case Dive::data_sel__me_release_mem__store_gds_data_to_memory:
            os << "Writes the GDS data to the specified address. ";
            break;
        };

        // Determine if any cache ops
        Dive::SyncInfo sync_info_common;
        sync_info_common.m_u32All = sync_info.m_u32All;
        sync_info_common.common.m_releasemem_acquiremem_common = 0;
        if (sync_info_common.m_u32All != 0 || sync_info.releasemem.m_wb_inv_cbdb)
            os << std::endl << "The following cache actions are also performed:";
        if (sync_info.releasemem.m_wb_inv_cbdb)
            os << std::endl << "  wbInvCbDb: All Cb/Db caches flushed and invalidated";
    }
    else if (sync_type == Dive::SyncType::kPfpSyncMe)
    {
        os << "This command forces a stall in the CP's PFP (prefetcher) until the CP's ME "
              "(micro-engine) is no longer busy. ME is no longer busy if it is not processing any "
              "work, and therefore no longer dispatching any tasks to the rest of the GPU. This "
              "packet is necessary, for example, if previous draws/dispatches are writing the "
              "indirect parameters used by later draws/dispatches";
    }
    else if (sync_type == Dive::SyncType::kFlushInvCbMeta)
    {
        os
        << "This EventWrite inserts a pipelined event which flushes and invalidates the Cb's "
           "metadata caches (eg. fmask/cmask). It noteably does not flush any DCC-related caches.";
    }
    else if (sync_type == Dive::SyncType::kFlushInvCbPixelData)
    {
        os
        << "This EventWrite inserts a pipelined event which flushes and invalidates the Cb's pixel "
           "and dcc caches. It noteably does not flush any of the Cb's metadata (eg. fmask/cmask) "
           "caches.";
    }
    else if (sync_type == Dive::SyncType::kCacheFlushInvEvent)
    {
        os << "This EventWrite inserts a pipelined event which flushes and invalidates all Cb and "
              "Db caches.";
    }
    else if (sync_type == Dive::SyncType::kVsPartialFlush)
    {
        os << "This EventWrite causes the CP's ME to stall until the SPI reports that all "
              "previously issued VS wavefronts are drained from the pipeline";
    }
    else if (sync_type == Dive::SyncType::kPsPartialFlush)
    {
        os << "This EventWrite causes the CP's ME to stall until the SPI reports that all "
              "previously issued PS wavefronts are drained from the pipeline";
    }
    else if (sync_type == Dive::SyncType::kCsPartialFlush)
    {
        os << "This EventWrite causes the CP's ME to stall until the SPI reports that all "
              "previously issued CS wavefronts are drained from the pipeline";
    }
    else if (sync_type == Dive::SyncType::kDbCacheFlushAndInv)
    {
        os
        << "This EventWrite inserts a pipelined event which flushes and invalidates all Db caches.";
    }
    else if (sync_type == Dive::SyncType::kVgtFlush)
    {
        // Note: I'm not sure exactly what this is used for, and if it'll ever appear
        os << "This EventWrite inserts a pipelined event which flushes and invalidates all Vgt "
              "caches.";
    }
    else
        DIVE_ASSERT(false);

    // Common fields
    if (sync_type == Dive::SyncType::kAcquireMem || sync_type == Dive::SyncType::kReleaseMem)
    {
        if (sync_info.common.m_wb_inv_l1l2)
            os << std::endl << "  wbInvL1L2: L1 and L2 flushed and invalidated";
        if (sync_info.common.m_wb_inv_l2)
            os << std::endl << "  wbInvL2: L2 flushed and invalidated";
        if (sync_info.common.m_wb_l2)
            os << std::endl << "  wbL2: L2 flushed";
        if (sync_info.common.m_inv_l2)
            os << std::endl << "  invL2: L2 invalidated";
        if (sync_info.common.m_inv_l2_md)
            os << std::endl
               << "  invL2Md: L2 metadata (for shader-read decompress-on-the-fly) invalidated";
        if (sync_info.common.m_inv_l1)
            os << std::endl << "  invL1: L1 is invalidated";
        if (sync_info.common.m_wb_inv_cbdata)
            os << std::endl << "  wbInvCbData: Cb pixel and dcc data flushed and invalidated";
    }
    return os.str();
}

//--------------------------------------------------------------------------------------------------
void HoverHelp::OnEnter(Item        item,
                        uint32_t    param1,
                        uint32_t    param2,
                        uint32_t    param3,
                        const char* custom_string)
{
    SetCurItem(item, param1, param2, param3, custom_string);
}

//--------------------------------------------------------------------------------------------------
void HoverHelp::OnExit(Item        item,
                       uint32_t    param1,
                       uint32_t    param2,
                       uint32_t    param3,
                       const char* custom_string_ptr)
{
    std::string custom_string;
    if (custom_string_ptr != nullptr)
        custom_string = custom_string_ptr;
    // Only dismiss hoverhelp on exit if it's still the same message.
    if (m_prev_item == item && m_prev_param1 == param1 && m_prev_param2 == param2 &&
        m_prev_param3 == param3 && m_prev_custom_string == custom_string)
    {
        SetCurItem(Item::kNone);
    }
}

//--------------------------------------------------------------------------------------------------
HoverHelp* HoverHelp::Get()
{
    if (m_instance == 0)
    {
        m_instance = new HoverHelp();
    }

    return m_instance;
}

/* Null, because instance will be initialized on demand. */
HoverHelp* HoverHelp::m_instance = 0;
