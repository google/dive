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
#include "main_window.h"
#include <QAction>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtWidgets/QDesktopWidget>
#include <QtWidgets>
#include <chrono>
#include <cstdlib>
#include <filesystem>

#include "about_window.h"
#include "buffer_view.h"
#include "command_buffer_model.h"
#include "command_buffer_view.h"
#include "command_model.h"
#include "dive_core/log.h"
#include "dive_tree_view.h"
#include "object_names.h"
#include "settings.h"
#include "trace_window.h"
#ifndef NDEBUG
#    include "event_timing/event_timing_view.h"
#endif
#include "command_tab_view.h"
#include "event_state_view.h"
#include "hover_help_model.h"
#include "overview_tab_view.h"
#include "property_panel.h"
#include "search_bar.h"
#include "shader_view.h"
#include "shortcuts.h"
#include "shortcuts_window.h"
#include "text_file_view.h"
#include "tree_view_combo_box.h"

static const int   kViewModeStringCount = 2;
static const int   kEventViewModeStringCount = 1;
static const char *kViewModeStrings[kViewModeStringCount] = { "Submit", "Events" };
static const char *kEventViewModeStrings[kEventViewModeStringCount] = { "GPU Events" };

void SetTabAvailable(QTabWidget *widget, int index, bool available)
{
    if (index < 0)
        return;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    widget->setTabVisible(index, available);
#else
    widget->setTabEnabled(index, available);
#endif
}

enum class EventMode
{
    AllEvent = 0
};

// =================================================================================================
// MainWindow
// =================================================================================================
MainWindow::MainWindow()
{
    // Output logs to both the "record" as well as console output
    m_log_compound.AddLog(&m_log_record);
    m_log_compound.AddLog(&m_log_console);

    m_data_core = new Dive::DataCore(&m_progress_tracker, &m_log_compound);

    m_event_selection = new EventSelection(m_data_core->GetCommandHierarchy());

    // Left side panel
    QFrame *left_frame = new QFrame();
    m_view_mode_combo_box = new TreeViewComboBox();
    m_view_mode_combo_box->setMinimumWidth(150);
    {
        QVBoxLayout *left_vertical_layout = new QVBoxLayout();

        QFrame *text_combo_box_frame = new QFrame();
        {
            QHBoxLayout *text_combo_box_layout = new QHBoxLayout();

            QLabel *combo_box_label = new QLabel(tr("Mode:"));

            // Set model for the combo box
            QStandardItemModel *combo_box_model = new QStandardItemModel();
            for (int i = 0; i < kViewModeStringCount; i++)
            {
                QStandardItem *item = new QStandardItem(kViewModeStrings[i]);
                combo_box_model->appendRow(item);
            }

            QModelIndex    event_item_index = combo_box_model->index(1, 0, QModelIndex());
            QStandardItem *event_item = combo_box_model->itemFromIndex(event_item_index);
            event_item->setSelectable(false);
            for (int i = 0; i < kEventViewModeStringCount; i++)
            {
                QStandardItem *item = new QStandardItem(kEventViewModeStrings[i]);
                event_item->appendRow(item);
            }
            m_view_mode_combo_box->setModel(combo_box_model);

            QModelIndex vulkan_event_item_index = combo_box_model->index(0, 0, event_item_index);
            m_view_mode_combo_box->setRootModelIndex(vulkan_event_item_index.parent());
            m_view_mode_combo_box->setCurrentIndex(vulkan_event_item_index.row());

            text_combo_box_layout->addWidget(combo_box_label);
            text_combo_box_layout->addWidget(m_view_mode_combo_box, 1);

            m_search_trigger_button = new QPushButton;
            m_search_trigger_button->setIcon(QIcon(":/images/search.png"));
            text_combo_box_layout->addWidget(m_search_trigger_button);

            text_combo_box_layout->addStretch();
            text_combo_box_frame->setLayout(text_combo_box_layout);
        }

        m_event_search_bar = new SearchBar(this);
        m_event_search_bar->setObjectName("Event Search Bar");

        QHBoxLayout *search_layout = new QHBoxLayout;
        search_layout->addWidget(m_event_search_bar);
        m_event_search_bar->hide();

        m_command_hierarchy_model = new CommandModel(m_data_core->GetCommandHierarchy());
        m_command_hierarchy_view = new DiveTreeView(m_data_core->GetCommandHierarchy());
        m_command_hierarchy_view->setModel(m_command_hierarchy_model);
        m_command_hierarchy_view->SetDataCore(m_data_core);
        m_event_search_bar->setTreeView(m_command_hierarchy_view);

        QLabel *goto_draw_call_label = new QLabel(tr("Go To:"));
        m_prev_event_button = new QPushButton("Prev Event");
        m_next_event_button = new QPushButton("Next Event");

        QHBoxLayout *goto_draw_call_layout = new QHBoxLayout();
        goto_draw_call_layout->addWidget(goto_draw_call_label);
        goto_draw_call_layout->addWidget(m_prev_event_button);
        goto_draw_call_layout->addWidget(m_next_event_button);
        goto_draw_call_layout->addStretch();

        QLabel *expand_to_lvl_label = new QLabel(tr("Expand to level:"));
        for (int i = 1; i <= 3; i++)
        {
            auto button = new QPushButton{ QString::number(i) };
            m_expand_to_lvl_buttons << button;
        }

        QHBoxLayout *expand_to_lvl_layout = new QHBoxLayout();
        expand_to_lvl_layout->addWidget(expand_to_lvl_label);
        foreach (auto expand_to_lvl_button, m_expand_to_lvl_buttons)
            expand_to_lvl_layout->addWidget(expand_to_lvl_button);
        expand_to_lvl_layout->addStretch();

        left_vertical_layout->addWidget(m_event_search_bar);
        left_vertical_layout->addWidget(text_combo_box_frame);
        left_vertical_layout->addWidget(m_command_hierarchy_view);
        left_vertical_layout->addLayout(goto_draw_call_layout);
        left_vertical_layout->addLayout(expand_to_lvl_layout);
        left_frame->setLayout(left_vertical_layout);
    }

    // Right side tab
    m_tab_widget = new QTabWidget();
    {
        m_command_tab_view = new CommandTabView(m_data_core->GetCommandHierarchy());
        m_shader_view = new ShaderView(*m_data_core);
        m_overview_tab_view = new OverviewTabView(m_data_core->GetCaptureMetadata(),
                                                  *m_event_selection);
        m_event_state_view = new EventStateView(*m_data_core);
        m_overview_view_tab_index = m_tab_widget->addTab(m_overview_tab_view, "Overview");

        m_command_view_tab_index = m_tab_widget->addTab(m_command_tab_view, "Commands");
        m_shader_view_tab_index = m_tab_widget->addTab(m_shader_view, "Shaders");
        m_event_state_view_tab_index = m_tab_widget->addTab(m_event_state_view, "Event State");
#if defined(ENABLE_CAPTURE_BUFFERS)
        m_buffer_view = new BufferView(*m_data_core);
        m_tab_widget->addTab(m_buffer_view, "Buffers");
#endif

        m_text_file_view = new TextFileView(*m_data_core);
        m_text_file_view->setParent(this);

        // Set to not visible by default.
        SetTabAvailable(m_tab_widget, m_text_file_view_tab_index, false);
    }
    // Side panel
    m_property_panel = new PropertyPanel();
    m_property_panel->setMinimumWidth(350);
    m_hover_help = HoverHelp::Get();
    m_shader_view->SetupHoverHelp(*m_hover_help);

    // The main horizontal splitter (Left<->Right panels, with a 1:2 size ratio)
    QSplitter *horizontal_splitter = new QSplitter(Qt::Horizontal);
    horizontal_splitter->addWidget(left_frame);
    horizontal_splitter->addWidget(m_tab_widget);
    horizontal_splitter->addWidget(m_property_panel);
    horizontal_splitter->setStretchFactor(0, 1);
    horizontal_splitter->setStretchFactor(1, 2);
    horizontal_splitter->setStretchFactor(2, 1);

    m_trace_dig = new TraceDialog(this);

    // Main Window requires a central widget.
    // Make the horizontal splitter that central widget so it takes up the whole area.
    setCentralWidget(horizontal_splitter);

    // Connections
    QObject::connect(m_view_mode_combo_box,
                     SIGNAL(currentTextChanged(const QString &)),
                     this,
                     SLOT(OnCommandViewModeChange(const QString &)));
    QObject::connect(m_view_mode_combo_box,
                     SIGNAL(textHighlighted(const QString &)),
                     this,
                     SLOT(OnCommandViewModeComboBoxHover(const QString &)));
    QObject::connect(m_command_hierarchy_view->selectionModel(),
                     SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
                     m_command_tab_view,
                     SLOT(OnSelectionChanged(const QModelIndex &)));
    QObject::connect(m_command_hierarchy_view->selectionModel(),
                     SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
                     this,
                     SLOT(OnSelectionChanged(const QModelIndex &)));
    QObject::connect(this,
                     SIGNAL(EventSelected(uint64_t)),
                     m_shader_view,
                     SLOT(OnEventSelected(uint64_t)));
    QObject::connect(this,
                     SIGNAL(EventSelected(uint64_t)),
                     m_event_state_view,
                     SLOT(OnEventSelected(uint64_t)));
#if defined(ENABLE_CAPTURE_BUFFERS)
    QObject::connect(this,
                     SIGNAL(EventSelected(uint64_t)),
                     m_buffer_view,
                     SLOT(OnEventSelected(uint64_t)));
#endif
    QObject::connect(m_hover_help,
                     SIGNAL(CurrStringChanged(const QString &)),
                     m_property_panel,
                     SLOT(OnHoverStringChange(const QString &)));
    QObject::connect(m_command_tab_view,
                     SIGNAL(SendNodeProperty(const QString &)),
                     m_property_panel,
                     SLOT(OnSelectionInfoChange(const QString &)));
    // Event selection connections
    QObject::connect(m_event_selection,
                     &EventSelection::vulkanParams,
                     m_property_panel,
                     &PropertyPanel::OnVulkanParams);
    QObject::connect(m_command_hierarchy_view,
                     &DiveTreeView::expanded,
                     m_command_hierarchy_view,
                     &DiveTreeView::expandNode);
    QObject::connect(m_command_hierarchy_view,
                     &DiveTreeView::collapsed,
                     m_command_hierarchy_view,
                     &DiveTreeView::collapseNode);
    QObject::connect(m_event_selection,
                     &EventSelection::currentNodeChanged,
                     m_command_hierarchy_view,
                     &DiveTreeView::setCurrentNode);
    QObject::connect(m_prev_event_button,
                     &QPushButton::clicked,
                     m_command_hierarchy_view,
                     &DiveTreeView::gotoPrevEvent);
    QObject::connect(m_next_event_button,
                     &QPushButton::clicked,
                     m_command_hierarchy_view,
                     &DiveTreeView::gotoNextEvent);
    QObject::connect(m_property_panel,
                     &PropertyPanel::crossReference,
                     this,
                     &MainWindow::OnCrossReference);
    QObject::connect(m_event_selection,
                     &EventSelection::crossReference,
                     this,
                     &MainWindow::OnCrossReference);
    QObject::connect(m_trace_dig,
                     &TraceDialog::TraceAvailable,
                     this,
                     &MainWindow::OnTraceAvailable);

    QObject::connect(this, &MainWindow::FileLoaded, m_text_file_view, &TextFileView::OnFileLoaded);
    QObject::connect(this, &MainWindow::FileLoaded, this, &MainWindow::OnFileLoaded);
    foreach (auto expand_to_lvl_button, m_expand_to_lvl_buttons)
    {
        QObject::connect(expand_to_lvl_button, SIGNAL(clicked()), this, SLOT(OnExpandToLevel()));
    }
    QObject::connect(m_search_trigger_button, SIGNAL(clicked()), this, SLOT(OnSearchTrigger()));

    QObject::connect(m_event_search_bar,
                     SIGNAL(hide_search_bar(bool)),
                     this,
                     SLOT(OnCommandBufferSearchBarVisibilityChange(bool)));

    QObject::connect(m_tab_widget, &QTabWidget::currentChanged, this, &MainWindow::OnTabViewChange);

    QObject::connect(m_command_tab_view,
                     SIGNAL(HideOtherSearchBars()),
                     this,
                     SLOT(OnTabViewChange()));

    CreateActions();
    CreateMenus();
    CreateStatusBar();
    CreateShortcuts();
    CreateToolBars();
    UpdateRecentFileActions(Settings::Get()->ReadRecentFiles());

    // Capture overlay widget
    m_overlay = new Overlay(this);
    QObject::connect(&m_progress_tracker,
                     SIGNAL(sendMessageSignal(const QString &)),
                     this,
                     SLOT(UpdateOverlay(const QString &)));
#ifndef NDEBUG
    showMaximized();
#endif
    // Set default view mode
    OnCommandViewModeChange(tr(kEventViewModeStrings[0]));
    m_hover_help->SetCurItem(HoverHelp::Item::kNone);
    m_hover_help->SetDataCore(m_data_core);
    setAccessibleName("DiveMainWindow");
}

void MainWindow::OnTraceAvailable(const QString &path)
{
    qDebug() << "Trace is at " << path;
    LoadFile(path.toStdString().c_str(), true);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    m_overlay->UpdateSize(rect());
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnCommandViewModeChange(const QString &view_mode)
{
    m_command_hierarchy_view->header()->reset();
    const Dive::CommandHierarchy &command_hierarchy = m_data_core->GetCommandHierarchy();
    if (view_mode == tr(kViewModeStrings[0]))  // Submit
    {
        const Dive::Topology &topology = command_hierarchy.GetSubmitHierarchyTopology();
        m_command_hierarchy_model->SetTopologyToView(&topology);
        m_command_tab_view->SetTopologyToView(&topology);
    }
    else  // All Vulkan Calls + GPU Events
    {
        const Dive::Topology &topology = command_hierarchy.GetAllEventHierarchyTopology();
        m_command_hierarchy_model->SetTopologyToView(&topology);
        m_command_tab_view->SetTopologyToView(&topology);

        // Put EventID column to the left of the tree. This forces the expand/collapse icon to be
        // part of the 2nd column (originally 1st)
        if (m_prev_command_view_mode.isEmpty() ||
            m_prev_command_view_mode == tr(kViewModeStrings[0]) ||
            m_prev_command_view_mode == tr(kViewModeStrings[1]))
            m_command_hierarchy_view->header()->moveSection(1, 0);
    }

    m_prev_command_view_mode = view_mode;
    ExpandResizeHierarchyView();

    // Retain node selection
    m_command_hierarchy_view->RetainCurrentNode();
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnCommandViewModeComboBoxHover(const QString &view_mode)
{
    if (view_mode == tr(kViewModeStrings[0]))  // Engine
        m_hover_help->SetCurItem(HoverHelp::Item::kEngineView);
    else if (view_mode == tr(kViewModeStrings[1]))  // Submit
        m_hover_help->SetCurItem(HoverHelp::Item::kSubmitView);
    else if (view_mode == tr(kEventViewModeStrings[0]))  // GPU Events
        m_hover_help->SetCurItem(HoverHelp::Item::kAllVulkanCallsGpuEventsView);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnSelectionChanged(const QModelIndex &index)
{
    // Determine which node it is, and emit this signal
    const Dive::CommandHierarchy &command_hierarchy = m_data_core->GetCommandHierarchy();
    uint64_t                      selected_item_node_index = (uint64_t)(index.internalPointer());
    Dive::NodeType node_type = command_hierarchy.GetNodeType(selected_item_node_index);
    if (node_type == Dive::NodeType::kDrawDispatchBlitNode ||
        node_type == Dive::NodeType::kMarkerNode)
    {
        emit EventSelected(selected_item_node_index);
    }
    else
    {
        emit EventSelected(UINT64_MAX);
    }
}

//--------------------------------------------------------------------------------------------------
bool MainWindow::LoadFile(const char *file_name, bool is_temp_file)
{
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    m_log_record.Reset();

    // Reset before loading, since the overlay may cause the UI to update with outdated data
    m_command_tab_view->ResetModel();
    m_command_hierarchy_model->Reset();
    m_event_selection->Reset();
    m_shader_view->Reset();
    m_text_file_view->Reset();
    m_prev_command_view_mode = QString();
    Dive::CaptureData::LoadResult load_res = m_data_core->LoadCaptureData(file_name);
    if (load_res != Dive::CaptureData::LoadResult::kSuccess)
    {
        HideOverlay();
        QString error_msg;
        if (load_res == Dive::CaptureData::LoadResult::kFileIoError)
            error_msg = QString("File I/O error!");
        else if (load_res == Dive::CaptureData::LoadResult::kCorruptData)
            error_msg = QString("File corrupt!");
        else if (load_res == Dive::CaptureData::LoadResult::kVersionError)
            error_msg = QString("Incompatible version!");
        QMessageBox::critical(this,
                              (QString("Unable to open file: ") + file_name),
                              error_msg,
                              QMessageBox::Ok);
        return false;
    }

    m_command_hierarchy_model->BeginResetModel();
    if (!m_data_core->ParseCaptureData())
    {
        HideOverlay();
        QMessageBox::critical(this,
                              QString("Error parsing file"),
                              (QString("Unable to parse file: ") + file_name),
                              QMessageBox::Ok);
        return false;
    }

    {
        // Switch to GPU Events view
        QModelIndex event_item_index = m_view_mode_combo_box->model()->index(1, 0, QModelIndex());
        QModelIndex gpu_events_item_index = m_view_mode_combo_box->model()->index(0,
                                                                                  0,
                                                                                  event_item_index);
        m_view_mode_combo_box->setRootModelIndex(gpu_events_item_index.parent());
        m_view_mode_combo_box->setCurrentIndex(gpu_events_item_index.row());
        OnCommandViewModeChange(tr(kEventViewModeStrings[0]));
        // TODO (b/185579518): disable the dropdown list for vulkan events.
    }
    m_command_hierarchy_model->EndResetModel();

    ExpandResizeHierarchyView();

    m_hover_help->SetCurItem(HoverHelp::Item::kNone);

    m_capture_file = QString(file_name);
    QFileInfo file_info(m_capture_file);
    SetCurrentFile(m_capture_file, is_temp_file);

    emit SetSaveAsMenuStatus(true);

    if (m_unsaved_capture_path.empty())
    {
        emit SetSaveMenuStatus(false);
    }
    else
    {
        emit SetSaveMenuStatus(true);
    }

    HideOverlay();
    ShowTempStatus(tr("File loaded successfully"));
    [[maybe_unused]] int64_t
    time_used_to_load_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - begin)
                           .count();

    DIVE_DEBUG_LOG("Time used to load the capture is %f seconds.", (time_used_to_load_ms / 1000.0));

    FileLoaded();

    return true;
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnOpenFile()
{
    QString supported_files = QStringLiteral("Dive files (*.rd) ;; All files (*.*)");
    QString file_name = QFileDialog::getOpenFileName(this,
                                                     "Open Document",
                                                     Settings::Get()->ReadLastFilePath(),
                                                     supported_files);

    if (!file_name.isEmpty())
    {
        QString last_file_path = file_name.left(file_name.lastIndexOf('/'));
        Settings::Get()->WriteLastFilePath(last_file_path);
        if (!LoadFile(file_name.toStdString().c_str()))
        {
            QMessageBox::critical(this,
                                  QString("Error opening file"),
                                  (QString("Unable to open file: ") + file_name),
                                  QMessageBox::Ok);
        }
    }
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnGFXRCapture()
{
    emit OnCapture(false, true);
}

// =================================================================================================
// OnNormalCapture is triggered for captures without counters.
// =================================================================================================
void MainWindow::OnNormalCapture()
{

    emit OnCapture(false);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnCaptureTrigger()
{
    if (!m_capture_saved && !m_unsaved_capture_path.empty())
    {
        QMessageBox warning_box;
        warning_box.setText("The current capture is not saved.");
        warning_box.setInformativeText("Do you want to proceed with a new capture?");
        warning_box.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        warning_box.setDefaultButton(QMessageBox::Ok);
        warning_box.setModal(true);
        int ret = warning_box.exec();

        if (ret == QMessageBox::Cancel)
            return;
    }

    QInputDialog input_dialog;
    input_dialog.setWindowTitle("Capture with delay");
    input_dialog.setLabelText("Enter capture delay (in seconds)");
    input_dialog.setInputMode(QInputDialog::IntInput);
    input_dialog.setOkButtonText("Start Capture");
    input_dialog.setIntRange(0, INT_MAX);
    input_dialog.setIntValue(Settings::Get()->ReadCaptureDelay());

    bool ok = input_dialog.exec();
    if (ok)
    {
        uint32_t capture_delay = input_dialog.intValue();
        QTimer::singleShot(capture_delay * 1000, this, SLOT(OnCapture(true)));
        Settings::Get()->WriteCaptureDelay(capture_delay);
    }
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnCapture(bool is_capture_delayed, bool is_gfxr_capture)
{
    m_trace_dig->UseGfxrCapture(is_gfxr_capture);
    m_trace_dig->UpdateDeviceList(true);
    m_trace_dig->exec();
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnExpandToLevel()
{
    QObject *sender_obj = sender();
    if (sender_obj->isWidgetType())
    {
        QPushButton *button = qobject_cast<QPushButton *>(sender_obj);
        if (button)
        {
            int level = button->text().toInt();
            m_command_hierarchy_view->ExpandToLevel(level);
            m_command_hierarchy_view->RetainCurrentNode();
        }
    }
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnAbout()
{
    AboutDialog *about = new AboutDialog();
    about->exec();
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnShortcuts()
{
    ShortcutsDialog *shortcuts = new ShortcutsDialog();
    shortcuts->exec();
}

//--------------------------------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent *closeEvent)
{
    if (!m_capture_saved && !m_unsaved_capture_path.empty())
    {
        switch (QMessageBox::question(this,
                                      QString("Save current capture"),
                                      (QString("Do you want to save current capture")),
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::No))
        {
        case QMessageBox::Yes:
            OnSaveCapture();
            break;
        case QMessageBox::No:
        {
            // Remove unsaved capture files.
            if (!m_unsaved_capture_path.empty())
            {
                std::filesystem::remove(m_unsaved_capture_path);
                m_unsaved_capture_path.clear();
            }
            break;
        }
        default:
            DIVE_ASSERT(false);
        }
    }
    if (m_trace_dig)
        m_trace_dig->Cleanup();
    closeEvent->accept();
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OpenRecentFile()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
    {
        LoadFile(action->data().toString().toStdString().c_str());
    }
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnSaveCapture()
{
    if (m_capture_file.isEmpty())
    {
        return;
    }
    QString file_name = QFileDialog::getSaveFileName(this,
                                                     tr("Save the current capture"),
                                                     QDir::currentPath(),
                                                     tr("Dive files (*.dive)"),
                                                     nullptr,
                                                     QFileDialog::DontConfirmOverwrite);
    if (file_name.isNull() || file_name.isEmpty() || file_name == m_capture_file)
    {
        return;
    }

    if (!file_name.endsWith(".dive"))
    {
        file_name += ".dive";
    }
    QFile target_file(file_name);
    if (target_file.exists())
    {
        switch (QMessageBox::question(this,
                                      QString("File already exists"),
                                      (QString("Do you want to replace the existing capture?")),
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::No))
        {
        case QMessageBox::Yes:
            target_file.remove();
            break;
        case QMessageBox::No:
            return OnSaveCapture();
        default:
            DIVE_ASSERT(false);
        }
    }

    bool save_result = false;
    bool is_saving_new_capture = m_unsaved_capture_path == m_capture_file.toStdString();
    // Save the newly captured file by rename and existing capture by copy.
    if (is_saving_new_capture)
    {
        save_result = QFile::rename(m_capture_file, file_name);
    }
    else
    {
        save_result = QFile::copy(m_capture_file, file_name);
    }

    if (save_result)
    {
        QMessageBox::information(this,
                                 QString("Save capture succeed"),
                                 (QString("Save capture succeed.")),
                                 QMessageBox::Ok);
    }
    else
    {
        QMessageBox::critical(this,
                              QString("Save capture file failed"),
                              (QString("Save capture file failed.")),
                              QMessageBox::Ok);
        return;
    }
    if (is_saving_new_capture)
    {
        m_unsaved_capture_path.clear();
        // Disable the "Save" menu after the new capture saved.
        emit SetSaveMenuStatus(false);
    }
    m_capture_saved = true;
    m_capture_file = file_name;
    SetCurrentFile(m_capture_file);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnSearchTrigger()
{
    if (m_event_search_bar->isVisible())
    {
        m_event_search_bar->clearSearch();
        m_event_search_bar->hide();
        m_search_trigger_button->show();

        DisconnectSearchBar();
    }
    else
    {
        ConnectSearchBar();

        m_search_trigger_button->hide();

        m_event_search_bar->positionCurser();
        m_event_search_bar->show();
    }

    int        currentIndex = m_tab_widget->currentIndex();
    QWidget   *currentTab = m_tab_widget->widget(currentIndex);
    SearchBar *commandBufferSearchBar = currentTab->findChild<SearchBar *>(
    kCommandBufferSearchBarName);
    QPushButton *commandBufferSearchButton = currentTab->findChild<QPushButton *>(
    kCommandBufferSearchButtonName);

    if (currentIndex == m_command_view_tab_index)
    {
        if (!commandBufferSearchBar->isHidden())
        {
            commandBufferSearchBar->clearSearch();
            commandBufferSearchBar->hide();
        }
        commandBufferSearchButton->show();
    }
}

//--------------------------------------------------------------------------------------------------
void MainWindow::CreateActions()
{
    // Open file action
    m_open_action = new QAction(tr("&Open"), this);
    m_open_action->setIcon(QIcon(":/images/open.png"));
    m_open_action->setShortcuts(QKeySequence::Open);
    m_open_action->setStatusTip(tr("Open an existing capture"));
    connect(m_open_action, &QAction::triggered, this, &MainWindow::OnOpenFile);

    // Exit application action
    m_exit_action = new QAction(tr("E&xit"), this);
    m_exit_action->setIcon(QIcon(":/images/exit.png"));
    m_exit_action->setShortcut(tr("Ctrl+Q"));
    m_exit_action->setStatusTip(tr("Exit the application"));
    connect(m_exit_action, SIGNAL(triggered()), this, SLOT(close()));

    // Save file action
    m_save_action = new QAction(tr("&Save"), this);
    m_save_action->setStatusTip(tr("Save the current capture"));
    m_save_action->setIcon(QIcon(":/images/save.png"));
    m_save_action->setShortcut(QKeySequence::Save);
    m_save_action->setEnabled(false);
    connect(m_save_action, &QAction::triggered, this, &MainWindow::OnSaveCapture);
    connect(this, &MainWindow::SetSaveMenuStatus, m_save_action, &QAction::setEnabled);

    // Save as file action
    m_save_as_action = new QAction(tr("&Save As"), this);
    m_save_as_action->setStatusTip(tr("Save the current capture"));
    m_save_as_action->setShortcut(QKeySequence::SaveAs);
    m_save_as_action->setEnabled(false);
    connect(m_save_as_action, &QAction::triggered, this, &MainWindow::OnSaveCapture);
    connect(this, &MainWindow::SetSaveAsMenuStatus, m_save_as_action, &QAction::setEnabled);

    // Recent file actions
    for (int i = 0; i < MaxRecentFiles; ++i)
    {
        m_recent_file_actions[i] = new QAction(this);
        m_recent_file_actions[i]->setVisible(false);
        connect(m_recent_file_actions[i], SIGNAL(triggered()), this, SLOT(OpenRecentFile()));
    }

    // Capture action
    m_capture_action = new QAction(tr("&Capture"), this);
    m_capture_action->setStatusTip(tr("Capture a Dive trace"));
    m_capture_action->setShortcut(QKeySequence("f5"));
    connect(m_capture_action, &QAction::triggered, this, &MainWindow::OnNormalCapture);

    // PM4 Capture action
    m_pm4_capture_action = new QAction(tr("PM4 Capture"), this);
    m_pm4_capture_action->setStatusTip(tr("Capture a Dive trace (PM4)"));
    m_pm4_capture_action->setShortcut(QKeySequence("f5"));
    connect(m_pm4_capture_action, &QAction::triggered, this, &MainWindow::OnNormalCapture);
    // GFXR Capture action
    m_gfxr_capture_action = new QAction(tr("GFXR Capture"), this);
    m_gfxr_capture_action->setStatusTip(tr("Capture a Dive trace (GFXR)"));
    m_gfxr_capture_action->setShortcut(QKeySequence("f6"));
    connect(m_gfxr_capture_action, &QAction::triggered, this, &MainWindow::OnGFXRCapture);

    // Capture with delay action
    m_capture_delay_action = new QAction(tr("Capture with delay"), this);
    m_capture_delay_action->setStatusTip(tr("Capture a Dive trace after a delay"));
    m_capture_delay_action->setShortcut(QKeySequence("Ctrl+f5"));
    connect(m_capture_delay_action, &QAction::triggered, this, &MainWindow::OnCaptureTrigger);

    // Shortcuts action
    m_shortcuts_action = new QAction(tr("&Shortcuts"), this);
    m_shortcuts_action->setStatusTip(tr("Display application keyboard shortcuts"));
    connect(m_shortcuts_action, &QAction::triggered, this, &MainWindow::OnShortcuts);

    // About action
    m_about_action = new QAction(tr("&About Dive"), this);
    m_about_action->setStatusTip(tr("Display application version information"));
    connect(m_about_action, &QAction::triggered, this, &MainWindow::OnAbout);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::CreateMenus()
{
    // File Menu
    m_file_menu = menuBar()->addMenu(tr("&File"));
    m_file_menu->addAction(m_open_action);
    m_file_menu->addAction(m_save_action);
    m_file_menu->addAction(m_save_as_action);
    m_file_menu->addSeparator();
    m_recent_captures_menu = m_file_menu->addMenu(tr("Recent captures"));
    for (int i = 0; i < MaxRecentFiles; ++i)
        m_recent_captures_menu->addAction(m_recent_file_actions[i]);
    m_file_menu->addSeparator();
    m_file_menu->addAction(m_exit_action);

    m_capture_menu = menuBar()->addMenu(tr("&Capture"));
    m_capture_menu->addAction(m_pm4_capture_action);
    m_capture_menu->addAction(m_gfxr_capture_action);

    m_help_menu = menuBar()->addMenu(tr("&Help"));
    m_help_menu->addAction(m_shortcuts_action);
    m_help_menu->addAction(m_about_action);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::CreateToolBars()
{
    m_file_tool_bar = addToolBar(tr("&File"));
    m_file_tool_bar->addAction(m_open_action);
    m_file_tool_bar->addAction(m_save_action);
#ifndef NDEBUG
    m_file_tool_bar->addSeparator();
    for (int i = 0; i < MaxRecentFiles; ++i)
        m_file_tool_bar->addAction(m_recent_file_actions[i]);
#endif

    m_file_tool_bar->addSeparator();

    QToolButton *m_capture_button = new QToolButton();
    m_capture_button->setPopupMode(QToolButton::MenuButtonPopup);
    m_capture_button->setMenu(m_capture_menu);
    m_capture_button->setDefaultAction(m_pm4_capture_action);
    m_capture_button->setIcon(QIcon(":/images/capture.png"));

    m_file_tool_bar->addWidget(m_capture_button);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::CreateShortcuts()
{
    // Search Shortcut
    QShortcut *searchShortcut = new QShortcut(QKeySequence(SHORTCUT_EVENTS_SEARCH), this);
    connect(searchShortcut, &QShortcut::activated, this, &MainWindow::OnSearchTrigger);

    // Commands Search Shortcut
    QShortcut *searchCommandsShortcut = new QShortcut(QKeySequence(SHORTCUT_COMMANDS_SEARCH), this);
    connect(searchCommandsShortcut, &QShortcut::activated, [this]() {
        if (m_tab_widget->currentIndex() == m_command_view_tab_index)
        {
            m_command_tab_view->OnSearchCommandBuffer();
        }
        else
        {
            m_tab_widget->setCurrentIndex(m_command_view_tab_index);
            m_command_tab_view->OnSearchCommandBuffer();
        }
        m_event_search_bar->clearSearch();
        m_event_search_bar->hide();
        m_search_trigger_button->show();

        DisconnectSearchBar();
    });

    // Overview Shortcut
    QShortcut *overviewTabShortcut = new QShortcut(QKeySequence(SHORTCUT_OVERVIEW_TAB), this);
    connect(overviewTabShortcut, &QShortcut::activated, [this]() {
        m_tab_widget->setCurrentIndex(m_overview_view_tab_index);
    });
    // Commands Shortcut
    QShortcut *commandTabShortcut = new QShortcut(QKeySequence(SHORTCUT_COMMANDS_TAB), this);
    connect(commandTabShortcut, &QShortcut::activated, [this]() {
        m_tab_widget->setCurrentIndex(m_command_view_tab_index);
    });
    // Shaders Shortcut
    QShortcut *shaderTabShortcut = new QShortcut(QKeySequence(SHORTCUT_SHADERS_TAB), this);
    connect(shaderTabShortcut, &QShortcut::activated, [this]() {
        m_tab_widget->setCurrentIndex(m_shader_view_tab_index);
    });
    // Event State Shortcut
    QShortcut *eventStateTabShortcut = new QShortcut(QKeySequence(SHORTCUT_EVENT_STATE_TAB), this);
    connect(eventStateTabShortcut, &QShortcut::activated, [this]() {
        m_tab_widget->setCurrentIndex(m_event_state_view_tab_index);
    });
}

//--------------------------------------------------------------------------------------------------
void MainWindow::CreateStatusBar()
{
    // Create status bar on the main window.
    m_status_bar = new QStatusBar(this);
    m_status_bar->setStyleSheet("background:#D0D0D0; color:#282828");
    setStatusBar(m_status_bar);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::UpdateOverlay(const QString &message)
{
    m_overlay->SetMessage(message);
    if (m_overlay->isHidden())
        m_overlay->show();
    m_overlay->repaint();
}

//--------------------------------------------------------------------------------------------------
void MainWindow::HideOverlay()
{
    m_overlay->SetMessage(QString());
    m_overlay->hide();
}

//--------------------------------------------------------------------------------------------------
void MainWindow::ShowTempStatus(const QString &status_message)
{
    m_status_bar->showMessage(status_message, MESSAGE_TIMEOUT);
    m_status_bar->repaint();
}

//--------------------------------------------------------------------------------------------------
void MainWindow::ExpandResizeHierarchyView()
{
    m_command_hierarchy_view->expandAll();

    // Set to -1 so that resizeColumnToContents() will consider *all* rows to determine amount
    // to resize. This can potentially be slow!
    // Then resize each column. This will also auto-adjust horizontal scroll bar size.
    m_command_hierarchy_view->header()->setResizeContentsPrecision(-1);
    uint32_t column_count = (uint32_t)m_command_hierarchy_model->columnCount(QModelIndex());
    for (uint32_t column = 0; column < column_count; ++column)
        m_command_hierarchy_view->resizeColumnToContents(column);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::SetCurrentFile(const QString &file_name, bool is_temp_file)
{
    QString shownName = tr("Untitled");
    if (!file_name.isEmpty() && is_temp_file == false)
    {
        shownName = StrippedName(file_name);

        QStringList recent_files = Settings::Get()->ReadRecentFiles();
        recent_files.removeAll(file_name);
        recent_files.prepend(file_name);
        Settings::Get()->WriteRecentFiles(recent_files);
        UpdateRecentFileActions(recent_files);
    }

    setWindowTitle(tr("%1[*] - %2").arg(shownName).arg(tr("Dive")));
}

//--------------------------------------------------------------------------------------------------
void MainWindow::UpdateRecentFileActions(QStringList recent_files)
{
    for (int j = 0; j < MaxRecentFiles; ++j)
    {
        if (j < recent_files.count())
        {
            QString text = tr("%1").arg(StrippedName(recent_files[j]));
            m_recent_file_actions[j]->setText(text);
            m_recent_file_actions[j]->setData(recent_files[j]);
            m_recent_file_actions[j]->setVisible(true);
        }
        else
        {
            m_recent_file_actions[j]->setVisible(false);
        }
    }
}

//--------------------------------------------------------------------------------------------------
QString MainWindow::StrippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}

//--------------------------------------------------------------------------------------------------
void MainWindow::UpdateTabAvailability()
{
    m_overview_tab_view->UpdateTabAvailability();

    bool has_text = m_data_core->GetCaptureData().GetNumText() > 0;
    SetTabAvailable(m_tab_widget, m_text_file_view_tab_index, has_text);

    SetTabAvailable(m_tab_widget, m_event_state_view_tab_index, true);

#ifndef NDEBUG
    SetTabAvailable(m_tab_widget, m_event_timing_view_tab_index, true);
#endif
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnCrossReference(Dive::CrossRef ref)
{
    switch (ref.Type())
    {
    case Dive::CrossRefType::kShaderAddress:
        if (m_shader_view->OnCrossReference(ref))
            m_tab_widget->setCurrentIndex(m_shader_view_tab_index);
        break;
    case Dive::CrossRefType::kGFRIndex:
        m_command_hierarchy_view->setCurrentNode(ref.Id());
        break;
    default:
        // Ignore
        break;
    }
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnFileLoaded()
{
    UpdateTabAvailability();

    if (m_data_core->GetCaptureData().HasPm4Data())
        m_overview_tab_view->Update(&m_log_record);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnSwitchToShaderTab()
{
    DIVE_ASSERT(m_shader_view_tab_index >= 0);
    m_tab_widget->setCurrentIndex(m_shader_view_tab_index);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnCommandBufferSearchBarVisibilityChange(bool isHidden)
{
    if (isHidden)
    {
        m_event_search_bar->clearSearch();
        m_event_search_bar->hide();
        m_search_trigger_button->show();

        DisconnectSearchBar();
    }
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnTabViewChange()
{
    int      currentIndex = m_tab_widget->currentIndex();
    QWidget *currentTab = m_tab_widget->widget(currentIndex);
    if (currentIndex == m_command_view_tab_index &&
        !currentTab->findChild<SearchBar *>(kCommandBufferSearchBarName)->isHidden())
    {
        m_event_search_bar->clearSearch();
        m_event_search_bar->hide();
        m_search_trigger_button->show();
        DisconnectSearchBar();
    }
    else if (currentIndex != m_command_view_tab_index)
    {
        m_command_tab_view->OnSearchBarVisibilityChange(true);
    }
}

//--------------------------------------------------------------------------------------------------
void MainWindow::ConnectSearchBar()
{
    QObject::connect(m_event_search_bar,
                     SIGNAL(new_search(const QString &)),
                     m_command_hierarchy_view,
                     SLOT(searchNodeByText(const QString &)));
    QObject::connect(m_event_search_bar,
                     &SearchBar::next_search,
                     m_command_hierarchy_view,
                     &DiveTreeView::nextNodeInSearch);
    QObject::connect(m_event_search_bar,
                     &SearchBar::prev_search,
                     m_command_hierarchy_view,
                     &DiveTreeView::prevNodeInSearch);
    QObject::connect(m_command_hierarchy_view,
                     &DiveTreeView::updateSearch,
                     m_event_search_bar,
                     &SearchBar::updateSearchResults);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::DisconnectSearchBar()
{
    QObject::disconnect(m_event_search_bar,
                        SIGNAL(new_search(const QString &)),
                        m_command_hierarchy_view,
                        SLOT(searchNodeByText(const QString &)));
    QObject::disconnect(m_event_search_bar,
                        &SearchBar::next_search,
                        m_command_hierarchy_view,
                        &DiveTreeView::nextNodeInSearch);
    QObject::disconnect(m_event_search_bar,
                        &SearchBar::prev_search,
                        m_command_hierarchy_view,
                        &DiveTreeView::prevNodeInSearch);
    QObject::disconnect(m_command_hierarchy_view,
                        &DiveTreeView::updateSearch,
                        m_event_search_bar,
                        &SearchBar::updateSearchResults);
}