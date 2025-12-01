/*
 Copyright 2025 Google LLC

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

// Forward declarations for UI.

class QAbstractItemModel;
class QAbstractProxyModel;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QPushButton;
class QScrollArea;
class QSortFilterProxyModel;
class QTabWidget;

namespace Dive
{

class AvailableMetrics;
class DataCore;
class TraceStats;

struct CaptureStats;
struct ComponentFilePaths;

}  // namespace Dive

class AnalyzeDialog;
class ApplicationController;
class BufferView;
class CaptureFileManager;
class CommandModel;
class CommandTabView;
class DiveFilterModel;
class DiveTreeView;
class ErrorDialog;
class EventSelection;
class EventStateView;
class EventTimingView;
class FrameTabView;
class GfxrVulkanCommandArgumentsFilterProxyModel;
class GfxrVulkanCommandArgumentsTabView;
class GfxrVulkanCommandFilter;
class GfxrVulkanCommandFilterProxyModel;
class GfxrVulkanCommandModel;
class GpuTimingModel;
class GpuTimingTabView;
class HoverHelp;
class OverlayHelper;
class OverviewTabView;
class PerfCounterModel;
class PerfCounterTabView;
class PropertyPanel;
class SearchBar;
class ShaderView;
class SqttView;
class TextFileView;
class TraceDialog;
class TreeViewComboBox;

struct LoadFileResult;
