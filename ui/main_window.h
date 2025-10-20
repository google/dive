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
#include <memory>
#include <vector>
#include <future>
#include <functional>
#include <QMainWindow>
#include <optional>
#include <qabstractitemmodel.h>
#include <qshortcut.h>
#include "dive_core/cross_ref.h"
#include "dive_core/command_hierarchy.h"
#include "progress_tracker_callback.h"
#include "dive_core/log.h"

// Forward declarations
class BufferView;
class CommandModel;
class CommandTabView;
class DiveFilterModel;
class DiveTreeView;
class EventSelection;
class EventStateView;
#ifndef NDEBUG
class EventTimingView;
#endif
class PerfCounterTabView;
class PerfCounterModel;
class GfxrVulkanCommandArgumentsTabView;
class GfxrVulkanCommandArgumentsFilterProxyModel;
class GfxrVulkanCommandFilterProxyModel;
class GfxrVulkanCommandModel;
class GpuTimingModel;
class GpuTimingTabView;
class HoverHelp;
class Overlay;
class OverlayWidget;
class OverviewTabView;
class PropertyPanel;
class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QTabWidget;
class SearchBar;
class ShaderView;
class SqttView;
class TextFileView;
class TraceDialog;
class TreeViewComboBox;
class AnalyzeDialog;
class GfxrVulkanCommandFilter;
class QGroupBox;
class QSortFilterProxyModel;
class QAbstractProxyModel;
class FrameTabView;
class QScrollArea;

enum class EventMode;

namespace Dive
{
class DataCore;
class PluginLoader;
class AvailableMetrics;
class TraceStats;
struct CaptureStats;

enum DrawCallContextMenuOption : uint32_t
{
    kArguments,
    kBinningPassOnly,
    kFirstTilePassOnly,
    kPerfCounterData,
    kGpuTimeData,
    kDrawCallContextMenuOptionCount
};

inline static constexpr const char
*kDrawCallContextMenuOptionStrings[kDrawCallContextMenuOptionCount] = {
    "Arguments",
    "PM4 Events with BinningPassOnly Filter",
    "PM4 Events with FirstTilePassOnly Filter",
    "Perf Counter Data",
    "Gpu Time Data"
};
}  // namespace Dive

#define MESSAGE_TIMEOUT 2500

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();
    ~MainWindow();
    bool LoadFile(const std::string &file_name, bool is_temp_file = false, bool async = true);
    bool InitializePlugins();

protected:
    virtual void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    virtual void closeEvent(QCloseEvent *closeEvent) Q_DECL_OVERRIDE;

signals:
    void HideOverlay();
    void EventSelected(uint64_t node_index);
    void SetSaveMenuStatus(bool);
    void SetSaveAsMenuStatus(bool);
    void FileLoaded();
    void PendingPerfCounterResults(const QString &file_name);
    void PendingGpuTimingResults(const QString &file_name);
    void PendingScreenshot(const QString &file_name);

public slots:
    void OnCapture(bool is_capture_delayed = false, bool is_gfxr_capture = false);
    void OnAnalyze(bool is_gfxr_capture_loaded, const std::string &file_path);
    void OnOpenFileFromAnalyzeDialog(const QString &file_path);
    void OnSwitchToShaderTab();
    void OnOpenVulkanDrawCallMenu(const QPoint &pos);
    void OnOpenVulkanCallMenu(const QPoint &pos);
    void OnCorrelateVulkanDrawCall(const QModelIndex &);
    void OnCorrelatePm4DrawCall(const QModelIndex &);
    void OnCounterSelected(uint64_t);
    void OnGpuTimingDataSelected(uint64_t);
    void OnCorrelationFilterApplied(uint64_t, int, const QModelIndex &);
    void OnPendingPerfCounterResults(const QString &file_name);
    void OnPendingGpuTimingResults(const QString &file_name);
    void OnPendingScreenshot(const QString &file_name);

private slots:
    void OnCommandViewModeChange(const QString &string);
    void OnCommandViewModeComboBoxHover(const QString &);
    void OnSelectionChanged(const QModelIndex &index);
    void OnFilterModeChange(const QString &string);
    void OnGfxrFilterModeChange();
    void OnOpenFile();
    void OnGFXRCapture();
    void OnNormalCapture();
    void OnCaptureTrigger();
    void OnAnalyzeCapture();
    void OnExpandToLevel();
    void OnAbout();
    void OnShortcuts();
    void OnSaveCapture();
    void OnSearchTrigger();
    void OpenRecentFile();
    void UpdateOverlay(const QString &);
    void OnHideOverlay();
    void OnCrossReference(Dive::CrossRef);
    void OnFileLoaded();
    void OnTraceAvailable(const QString &);
    void OnTabViewSearchBarVisibilityChange(bool isHidden);
    void OnTabViewChange();
    void ConnectDiveFileTabs();
    void ConnectAdrenoRdFileTabs();
    void ConnectGfxrFileTabs();
    void ConnectSearchBar();
    void DisconnectSearchBar();
    void ConnectPm4SearchBar();
    void DisconnectPm4SearchBar();
    void DisconnectAllTabs();

private:
    enum class LoadedFileType
    {
        kUnknown,  // Load failure
        kDiveFile,
        kRdFile,
        kGfxrFile,
    };

    struct LoadFileResult
    {
        LoadedFileType file_type;
        std::string    file_name;
        bool           is_temp_file;
    };

    enum class CorrelationTarget
    {
        kGfxrDrawCall,
        kPm4DrawCall
    };

    LoadedFileType LoadFileImpl(const std::string &file_name, bool is_temp_file = false);

    void OnDiveFileLoaded();
    void OnAdrenoRdFileLoaded();
    void OnGfxrFileLoaded();

    void RunOnUIThread(std::function<void()> f);
    // Dialogs for async loading:
    void OnLoadFailure(Dive::CaptureData::LoadResult result, const std::string &file_name);
    void OnParseFailure(const std::string &file_name);
    void OnUnsupportedFile(const std::string &file_name);

    void    CreateActions();
    void    CreateMenus();
    void    CreateToolBars();
    void    CreateShortcuts();
    void    CreateStatusBar();
    void    LoadAvailableMetrics();
    void    ShowTempStatus(const QString &status_message);
    void    ExpandResizeHierarchyView(DiveTreeView &tree_view, const QSortFilterProxyModel &model);
    void    SetCurrentFile(const QString &fileName, bool is_temp_file = false);
    void    UpdateRecentFileActions(QStringList recent_files);
    QString StrippedName(const QString &fullFileName);
    void    UpdateTabAvailability();
    void    ResetTabWidget();
    QModelIndex             FindSourceIndexFromNode(QAbstractItemModel *model,
                                                    uint64_t            target_node_index,
                                                    const QModelIndex  &parent = QModelIndex());
    void                    ResetEventSearchBar();
    void                    ResetPm4EventSearchBar();
    void                    ResetHorizontalScroll(const DiveTreeView &tree_view);
    void                    ResetVerticalScroll(const DiveTreeView &tree_view);
    void                    ClearViewModelSelection(DiveTreeView &tree_view, bool should_clear_tab);
    void                    CorrelateCounter(const QModelIndex &index, bool called_from_gfxr_view);
    std::optional<uint64_t> GetDrawCallIndexFromProxyIndex(
    const QModelIndex           &proxy_index,
    const QAbstractProxyModel   &proxy_model,
    const std::vector<uint64_t> &draw_call_indices,
    CorrelationTarget            target);

    QMenu         *m_file_menu;
    QMenu         *m_recent_captures_menu;
    QAction       *m_open_action;
    QAction       *m_save_action;
    QAction       *m_save_as_action;
    QAction       *m_exit_action;
    QMenu         *m_capture_menu;
    QAction       *m_gfxr_capture_action;
    QAction       *m_pm4_capture_action;
    QAction       *m_capture_action;
    QAction       *m_capture_delay_action;
    QAction       *m_capture_setting_action;
    QMenu         *m_analyze_menu;
    QAction       *m_analyze_action;
    QMenu         *m_help_menu;
    QAction       *m_about_action;
    QAction       *m_shortcuts_action;
    QToolBar      *m_file_tool_bar;
    QScrollArea   *m_file_tool_bar_scroll_area;
    TraceDialog   *m_trace_dig;
    AnalyzeDialog *m_analyze_dig;

    enum
    {
        MaxRecentFiles = 3
    };
    QAction *m_recent_file_actions[MaxRecentFiles];

    ProgressTrackerCallback         m_progress_tracker;
    std::unique_ptr<Dive::DataCore> m_data_core;
    QString                         m_capture_file;
    QString                         m_last_file_path;
    Dive::LogRecord                 m_log_record;
    Dive::LogConsole                m_log_console;
    Dive::LogCompound               m_log_compound;

    QStatusBar *m_status_bar;

    // Left pane
    QGroupBox    *m_left_group_box;
    QString       m_prev_command_view_mode;
    DiveTreeView *m_command_hierarchy_view;
    CommandModel *m_command_hierarchy_model;
    QPushButton  *m_search_trigger_button;
    SearchBar    *m_event_search_bar = nullptr;

    TreeViewComboBox                  *m_view_mode_combo_box;
    TreeViewComboBox                  *m_filter_mode_combo_box;
    GfxrVulkanCommandFilter           *m_filter_gfxr_commands_combo_box;
    QPushButton                       *m_prev_event_button;
    QPushButton                       *m_next_event_button;
    QList<QPushButton *>               m_expand_to_lvl_buttons;
    GfxrVulkanCommandFilterProxyModel *m_gfxr_vulkan_commands_filter_proxy_model;
    GfxrVulkanCommandModel            *m_gfxr_vulkan_command_hierarchy_model;
    PerfCounterModel                  *m_perf_counter_model;
    GpuTimingModel                    *m_gpu_timing_model;

    // Middle pane
    QGroupBox           *m_middle_group_box;
    DiveTreeView        *m_pm4_command_hierarchy_view;
    QPushButton         *m_pm4_search_trigger_button;
    SearchBar           *m_pm4_event_search_bar = nullptr;
    QPushButton         *m_pm4_prev_event_button;
    QPushButton         *m_pm4_next_event_button;
    QList<QPushButton *> m_pm4_expand_to_lvl_buttons;

    TreeViewComboBox *m_pm4_view_mode_combo_box;
    TreeViewComboBox *m_pm4_filter_mode_combo_box;

    // Right pane
    QTabWidget                        *m_tab_widget;
    CommandTabView                    *m_command_tab_view;
    int                                m_command_view_tab_index;
    OverviewTabView                   *m_overview_tab_view;
    int                                m_overview_view_tab_index;
    ShaderView                        *m_shader_view;
    int                                m_shader_view_tab_index;
    EventStateView                    *m_event_state_view;
    int                                m_event_state_view_tab_index;
    GfxrVulkanCommandArgumentsTabView *m_gfxr_vulkan_command_arguments_tab_view;
    int                                m_gfxr_vulkan_command_arguments_view_tab_index;
    PerfCounterTabView                *m_perf_counter_tab_view;
    int                                m_perf_counter_view_tab_index;
    GpuTimingTabView                  *m_gpu_timing_tab_view;
    int                                m_gpu_timing_view_tab_index;
    FrameTabView                      *m_frame_tab_view;
    int                                m_frame_view_tab_index;
#if defined(ENABLE_CAPTURE_BUFFERS)
    BufferView *m_buffer_view;
#endif
#ifndef NDEBUG
    EventTimingView *m_event_timing_view;
    int              m_event_timing_view_tab_index;
#endif
    TextFileView *m_text_file_view;
    int           m_text_file_view_tab_index = -1;

    DiveFilterModel *m_filter_model;

    // Side pane
    PropertyPanel *m_property_panel;
    HoverHelp     *m_hover_help;

    // Shortcuts
    QShortcut *m_search_shortcut = nullptr;
    QShortcut *m_search_tab_view_shortcut = nullptr;
    QShortcut *m_overview_tab_shortcut = nullptr;
    QShortcut *m_command_tab_shortcut = nullptr;
    QShortcut *m_shader_tab_shortcut = nullptr;
    QShortcut *m_event_state_tab_shortcut = nullptr;
    QShortcut *m_gfxr_vulkan_command_arguments_tab_shortcut = nullptr;

    std::string m_unsaved_capture_path;
    bool        m_capture_saved = false;
    int         m_capture_num = 0;
    int         m_previous_tab_index = -1;
    bool        m_gfxr_capture_loaded = false;
    bool        m_correlated_capture_loaded = false;

    EventSelection *m_event_selection;

    // Overlay to be displayed while capture
    Overlay *m_overlay;

    std::unique_ptr<Dive::PluginLoader>         m_plugin_manager;
    GfxrVulkanCommandArgumentsFilterProxyModel *m_gfxr_vulkan_commands_arguments_filter_proxy_model;
    std::unique_ptr<Dive::AvailableMetrics>     m_available_metrics;
    Dive::TraceStats                           *m_trace_stats;
    Dive::CaptureStats                         *m_capture_stats;

    std::future<LoadFileResult>        m_loading_result;
    std::vector<std::function<void()>> m_loading_pending_task;
};
