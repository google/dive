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

#include <QMainWindow>
#include <qaction.h>
#include "dive_core/cross_ref.h"
#include "dive_core/data_core.h"
#include "event_selection_model.h"
#include "overlay.h"
#include "progress_tracker_callback.h"

#pragma once

// Forward declarations
class DiveTreeView;
class QCheckBox;
class QComboBox;
class QProgressBar;
class QLabel;
class QTabWidget;
class ShaderView;
class TextFileView;
class EventStateView;
class BufferView;
class OverviewTabView;
class PerfCounterView;
class SqttView;
#ifndef NDEBUG
class EventTimingView;
#endif
class CommandTabView;
class CommandModel;
class PropertyPanel;
class HoverHelp;
class QItemSelection;
class TreeViewComboBox;
class SearchDialog;
class CaptureSettingView;
class TraceDialog;
enum class EventMode;

#define MESSAGE_TIMEOUT 2500

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();
    bool LoadFile(const char *file_name, bool is_temp_file = false);

protected:
    virtual void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    virtual void closeEvent(QCloseEvent *closeEvent) Q_DECL_OVERRIDE;

signals:
    void EventSelected(uint64_t node_index);
    void SetSaveMenuStatus(bool);
    void SetSaveAsMenuStatus(bool);
    void FileLoaded();

public slots:
    void OnCapture(bool is_capture_delayed = false);
    void OnSwitchToShaderTab();

private slots:
    void OnCommandViewModeChange(const QString &string);
    void OnCommandViewModeComboBoxHover(const QString &);
    void OnSelectionChanged(const QModelIndex &index);
    void OnOpenFile();
    void OnNormalCapture();
    void OnCaptureTrigger();
    void OnExpandToLevel();
    void OnAbout();
    void OnShortcuts();
    void OnSaveCapture();
    void OnSearchTrigger();
    void OnCheckboxStateChanged(int state);
    void OpenRecentFile();
    void UpdateOverlay(const QString &);
    void OnCrossReference(Dive::CrossRef);
    void OnFileLoaded();
    void OnTraceAvailable(const QString &);

private:
    void    CreateActions();
    void    CreateMenus();
    void    CreateToolBars();
    void    CreateShortcuts();
    void    CreateStatusBar();
    void    ShowTempStatus(const QString &status_message);
    void    ExpandResizeHierarchyView();
    void    ShowEventView(const Dive::CommandHierarchy &command_hierarchy, EventMode event_mode);
    void    SetCurrentFile(const QString &fileName, bool is_temp_file = false);
    void    UpdateRecentFileActions(QStringList recent_files);
    QString StrippedName(const QString &fullFileName);
    void    HideOverlay();
    void    UpdateTabAvailability();

    QMenu       *m_file_menu;
    QMenu       *m_recent_captures_menu;
    QAction     *m_open_action;
    QAction     *m_save_action;
    QAction     *m_save_as_action;
    QAction     *m_exit_action;
    QMenu       *m_capture_menu;
    QAction     *m_capture_action;
    QAction     *m_capture_delay_action;
    QAction     *m_capture_setting_action;
    QMenu       *m_help_menu;
    QAction     *m_about_action;
    QAction     *m_shortcuts_action;
    QToolBar    *m_file_tool_bar;
    TraceDialog *m_trace_dig;

    enum
    {
        MaxRecentFiles = 3
    };
    QAction *m_recent_file_actions[MaxRecentFiles];

    ProgressTrackerCallback m_progress_tracker;
    Dive::DataCore         *m_data_core;
    QString                 m_capture_file;
    QString                 m_last_file_path;
    Dive::LogRecord         m_log_record;
    Dive::LogConsole        m_log_console;
    Dive::LogCompound       m_log_compound;

    QStatusBar *m_status_bar;

    // Left pane
    QString       m_prev_command_view_mode;
    DiveTreeView *m_command_hierarchy_view;
    CommandModel *m_command_hierarchy_model;
    QPushButton  *m_search_trigger_button;
    SearchDialog *m_search_dialog = nullptr;

    TreeViewComboBox    *m_view_mode_combo_box;
    QPushButton         *m_prev_event_button;
    QPushButton         *m_next_event_button;
    QList<QPushButton *> m_expand_to_lvl_buttons;

#ifndef NDEBUG
    QCheckBox *m_show_marker_checkbox;
#endif

    // Right pane
    QTabWidget      *m_tab_widget;
    CommandTabView  *m_command_tab_view;
    int              m_command_view_tab_index;
    OverviewTabView *m_overview_tab_view;
    int              m_overview_view_tab_index;
    ShaderView      *m_shader_view;
    int              m_shader_view_tab_index;
    EventStateView  *m_event_state_view;
    int              m_event_state_view_tab_index;
#if defined(ENABLE_CAPTURE_BUFFERS)
    BufferView *m_buffer_view;
#endif
#ifndef NDEBUG
    EventTimingView *m_event_timing_view;
    int              m_event_timing_view_tab_index;
#endif
    TextFileView *m_text_file_view;
    int           m_text_file_view_tab_index = -1;

    // Side pane
    PropertyPanel *m_property_panel;
    HoverHelp     *m_hover_help;

    std::string m_unsaved_capture_path;
    bool        m_capture_saved = false;
    int         m_capture_num = 0;

    EventSelection *m_event_selection;

    // Overlay to be displayed while capture
    Overlay *m_overlay;
};
