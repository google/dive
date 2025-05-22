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
#include <QObject>
#include <functional>
#include <stdint.h>

// Forward Declaration
namespace Dive
{
class CommandHierarchy;
class DataCore;
enum class PerfCategory : uint32_t;
};  // namespace Dive

// The "model" class to the hover-help system
class HoverHelp : public QObject
{
    Q_OBJECT

public:
    enum class Item
    {
        kNone,
        kSqttSeView,               // Se view
        kSqttCuView,               // Cu view (Se# in param1)
        kSqttSimdView,             // Simd view (Params: [Se#,Cu#])
        kSqttSlotView,             // Slot view (Params: [Se#,Cu#,Simd#])
        kContextView,              // Hw Context view
        kBarChartView,             // Bar Chart view
        kSqttSeText,               // The "Se" label text (Param: [Se#])
        kSqttCuText,               // The "Cu" label text (Param: [Cu#])
        kSqttSimdText,             // The "Simd" label text (Param: [Simd#])
        kSqttSlotText,             // The "Slot" label text (Param: [Slot#])
        kSqttContextText,          // The "Context" label text (Param: [Context#])
        kSqttCuViewSummarySe,      // Cu view: The summary "Se" row (Param: [Se#])
        kSqttSimdViewSummaryCu,    // Simd view: The summary "Cu" row
        kSqttSlotViewSummarySimd,  // Slot view: The summary "Simd" row
        kSqttInstTraceCheckbox,    // The inst-trace checkbox and text (Param: [Cu#])
        kSqttZoomToFitPushButton,  // The zoom-to-fit button
        kSqttZoomFullPushButton,   // The zoom-full button
        kSqttRuler,                // The ruler (Params: [Profile freq,Min freq,Max freq])
        kSqttSeRect,               // Any rect in Se view (Params: [ShaderStage,Event#,Se#])
        kSqttCuRect,               // Any rect in Cu view (Params: [ShaderStage,Event#,Cu#])
        kSqttSimdRect,             // Any rect in Simd view (Params: [ShaderStage,Event#,Simd#])
        kSqttSlotRect,             // Any rect in Slot view (Params: [Event#-Waves,Event#,Slot#])
        kSqttGap,                  // Empty gap in time axis (Params: [Idle cycles#])
        kSqttDisabledCu,           // Disabled Cu (Param: [PerSE-ActiveCus])
        kContextStalledEvent,      // Stalled Event
        kNavigatorGfxText,         // The "Navigator" Gfx label text
        kNavigatorAceText,         // The "Navigator" Ace label text
        kNavigatorEvent,           // Any Navigator Event (Param: [Event#])
        kNavigatorGfxEventNoPs,    // Any Event that has a VS and no PS (Param: [Event#])
        kNavigatorVulkanBarrier,   // Any Navigator Vulkan Barrier (Param: [Barrier#])
        kNavigatorDriverBarrier,   // Any Navigator Driver Barrier (Param: [Barrier#])
        kNavigatorDebugLabel,      // Any Navigator Debug Label (Param: [Label#])

        // Command hierarchy views
        kEngineView,                   // The "Engine" command hierarchy view
        kSubmitView,                   // The "Submit" command hierarchy view
        kVulkanEventsView,             // The "Vulkan Events" command hierarchy view
        kAllVulkanCallsView,           // The "All Vulkan Calls" command hierarchy view
        kAllVulkanCallsGpuEventsView,  // The "All Vulkan Calls + Gpu Events" command hierarchy view

        // Command hierarchy nodes
        kChainNode,               // Any Ib-node that is a CHAIN node
        kIbNode,                  // Any Ib-node that is not a CHAIN or CALL node
        kVulkanEmptyNode,         // Vulkan marker node that has just a NOP child
        kBarrierNode,             // A barrier operation/node consisting of 1 or more GPU events
        kBarrierNoopNode,         // A barrier operation/node that evaluates to a NO-OP
        kIbNodeNotFullyCaptured,  // An IB node that is not fully captured
        kSyncNode,                // Sync node (Param: [Type, Info])
        kBarrierMultiEvent,       // A Vulkan barrier node with > 1 associated events
        kRenderPassMultiEvent,    // A Vulkan render pass node with > 1 associated events
        kQueryPoolMultiEvent,     // vkCmdResetQueryPool node with > 1 associated events

        // Overview Tab
        kEventsDuration,           // Duration column of the "Most Expensive Events" table
        kEventsOccupancyDuration,  // Occupancy duration column of the "Most Expensive Events" table

        // Command hierarchy crash dump highlighting
        kGFRCompleted,
        kGFRIncomplete,
        kGFRPending,
        kWaveStateActive,
        kWaveStateActiveCausal,  // If no crashing wave is found, and GFR is not available
        kWaveStateFatal,
        kEngineCacheMatch,

        // Shader view for crashdump
        kShaderViewWaveStateNote
    };

    // Set current item. Allows additional parameter(s) that can be used for certain items
    void SetCurItem(Item        item,
                    uint32_t    param1 = UINT32_MAX,
                    uint32_t    param2 = UINT32_MAX,
                    uint32_t    param3 = UINT32_MAX,
                    const char* custom_string = nullptr);

    void SetCommandHierarchyNodeItem(const Dive::CommandHierarchy& command_hierarchy,
                                     uint32_t                      param1);

    // Sync nodes strings are a bit more complicated and are thus done in their own function
    std::string GetSyncNodeString(uint32_t param1 = UINT32_MAX,
                                  uint32_t param2 = UINT32_MAX,
                                  uint32_t param3 = UINT32_MAX);

    void SetEventNameFn(std::function<std::string(uint32_t)> event_name_fn)
    {
        m_event_name_fn = event_name_fn;
    }

    void SetBarrierNameFn(std::function<std::string(uint32_t)> barrier_name_fn)
    {
        m_barrier_name_fn = barrier_name_fn;
    }

    void SetBarrierParamsFn(std::function<std::string(uint32_t)> barrier_params_fn)
    {
        m_barrier_params_fn = barrier_params_fn;
    }

    void SetLabelNameFn(std::function<std::string(uint32_t)> label_name_fn)
    {
        m_label_name_fn = label_name_fn;
    }

    void SetLabelIncludesFn(std::function<std::string(uint32_t)> label_includes_fn)
    {
        m_label_includes_fn = label_includes_fn;
    }

    void SetDataCore(Dive::DataCore* data_core) { m_data_core = data_core; }

    /* Static access method. */
    static HoverHelp* Get();

private:
    HoverHelp();

    static HoverHelp* m_instance;

    Item        m_prev_item = Item::kNone;
    uint32_t    m_prev_param1 = UINT32_MAX, m_prev_param2 = UINT32_MAX, m_prev_param3 = UINT32_MAX;
    std::string m_prev_custom_string;

    std::function<std::string(uint32_t)> m_event_name_fn;
    std::function<std::string(uint32_t)> m_barrier_name_fn;
    std::function<std::string(uint32_t)> m_barrier_params_fn;
    std::function<std::string(uint32_t)> m_label_name_fn;
    std::function<std::string(uint32_t)> m_label_includes_fn;

    Dive::DataCore* m_data_core = nullptr;

signals:
    void CurrStringChanged(const QString&);
public slots:
    void OnEnter(Item        item,
                 uint32_t    param1 = UINT32_MAX,
                 uint32_t    param2 = UINT32_MAX,
                 uint32_t    param3 = UINT32_MAX,
                 const char* custom_string = nullptr);
    void OnExit(Item        item,
                uint32_t    param1 = UINT32_MAX,
                uint32_t    param2 = UINT32_MAX,
                uint32_t    param3 = UINT32_MAX,
                const char* custom_string = nullptr);
};
