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
#include "main_window.h"
#include "adreno.h"
#include <QAction>
#include <QComboBox>
#include <QCoreApplication>
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
#include <future>
#include <optional>
#include <qabstractitemmodel.h>
#include <qvariant.h>

#include "about_window.h"
#include "buffer_view.h"
#include "capture_service/constants.h"
#include "command_buffer_model.h"
#include "command_buffer_view.h"
#include "command_model.h"
#include "dive_core/capture_data.h"
#include "dive_core/command_hierarchy.h"
#include "dive_core/log.h"
#include "dive_tree_view.h"
#include "object_names.h"
#include "settings.h"
#include "trace_window.h"
#include "analyze_window.h"
#ifndef NDEBUG
#    include "event_timing/event_timing_view.h"
#endif
#include "command_tab_view.h"
#include "dive_core/data_core.h"
#include "event_selection_model.h"
#include "event_state_view.h"
#include "gfxr_vulkan_command_model.h"
#include "gfxr_vulkan_command_filter_proxy_model.h"
#include "gfxr_vulkan_command_arguments_filter_proxy_model.h"
#include "gfxr_vulkan_command_arguments_tab_view.h"
#include "gpu_timing_model.h"
#include "gpu_timing_tab_view.h"
#include "hover_help_model.h"
#include "overlay.h"
#include "overview_tab_view.h"
#include "perf_counter_tab_view.h"
#include "perf_counter_model.h"
#include "plugins/plugin_loader.h"
#include "property_panel.h"
#include "search_bar.h"
#include "shader_view.h"
#include "shortcuts.h"
#include "shortcuts_window.h"
#include "text_file_view.h"
#include "ui/dive_tree_view.h"
#include "gfxr_vulkan_command_filter.h"
#include "viewport_stats_model.h"
#include "window_scissors_stats_model.h"
#include "trace_stats/trace_stats.h"
#include "frame_tab_view.h"

static constexpr int         kViewModeStringCount = 2;
static constexpr int         kEventViewModeStringCount = 1;
static constexpr int         kFrameTitleStringCount = 3;
static constexpr const char *kViewModeStrings[kViewModeStringCount] = { "Submit", "Events" };
static constexpr const char *kEventViewModeStrings[kEventViewModeStringCount] = { "GPU Events" };
static constexpr const char *kFrameTitleStrings[kFrameTitleStringCount] = { "No File Loaded",
                                                                            "Gfxr Capture",
                                                                            "Adreno Rd Capture" };
static constexpr const char *kFilterStrings[DiveFilterModel::kFilterModeCount] = {
    "None",
    "BinningPassOnly",
    "FirstTilePassOnly",
    "BinningAndFirstTilePass"
};
constexpr DiveFilterModel::FilterMode kDefaultFilterMode = DiveFilterModel::kFirstTilePassOnly;

static constexpr const char *kMetricsFilePath = ":/resources/available_metrics.csv";
static constexpr const char *kMetricsFileName = "available_metrics.csv";

namespace
{

std::optional<std::filesystem::path> ResolveAssetPath(const std::string &name)
{
    std::vector<std::filesystem::path> search_paths{
        std::filesystem::path{ "./install" },
        std::filesystem::path{ "../../build_android/Release/bin" },
        std::filesystem::path{ "../../install" },
        std::filesystem::path{ "./" },
    };

    for (const auto &p : search_paths)
    {
        auto result_path = p / name;
        if (std::filesystem::exists(result_path))
        {
            return std::filesystem::canonical(result_path);
        }
    }
    return std::nullopt;
}

}  // namespace

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

    m_data_core = std::make_unique<Dive::DataCore>(&m_progress_tracker);

    m_event_selection = new EventSelection(m_data_core->GetCommandHierarchy());

    // Left side panel
    m_left_group_box = new QGroupBox(kFrameTitleStrings[0]);
    m_left_group_box->setAlignment(Qt::AlignHCenter);
    m_view_mode_combo_box = new TreeViewComboBox();
    m_view_mode_combo_box->setMinimumWidth(150);

    {
        QVBoxLayout *left_vertical_layout = new QVBoxLayout();

        QFrame *text_combo_box_frame = new QFrame();

        QHBoxLayout *text_combo_box_layout = new QHBoxLayout();

        QLabel *combo_box_label = new QLabel(tr("Mode:"));

        // Set model for the view mode combo box
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

        QLabel *filter_combo_box_label = new QLabel(tr("Filter:"));
        m_filter_mode_combo_box = new TreeViewComboBox();
        m_filter_mode_combo_box->setMinimumWidth(150);

        // Set model for the filter combo box
        QStandardItemModel *filter_combo_box_model = new QStandardItemModel();
        for (uint32_t i = 0; i < DiveFilterModel::kFilterModeCount; i++)
        {
            QStandardItem *item = new QStandardItem(kFilterStrings[i]);
            filter_combo_box_model->appendRow(item);
        }
        m_filter_mode_combo_box->setModel(filter_combo_box_model);
        m_filter_mode_combo_box->setCurrentIndex(kDefaultFilterMode);

        text_combo_box_layout->addWidget(filter_combo_box_label);
        text_combo_box_layout->addWidget(m_filter_mode_combo_box, 1);

        m_event_search_bar = new SearchBar(this);
        m_event_search_bar->setObjectName("Event Search Bar");

        QHBoxLayout *search_layout = new QHBoxLayout;
        search_layout->addWidget(m_event_search_bar);
        m_event_search_bar->hide();

        m_command_hierarchy_model = new CommandModel(m_data_core->GetCommandHierarchy());
        m_gfxr_vulkan_command_hierarchy_model = new GfxrVulkanCommandModel(
        m_data_core->GetCommandHierarchy());

        m_command_hierarchy_view = new DiveTreeView(m_data_core->GetCommandHierarchy());
        m_command_hierarchy_view->SetDataCore(m_data_core.get());
        m_event_search_bar->setView(m_command_hierarchy_view);

        m_gfxr_vulkan_commands_filter_proxy_model =
        new GfxrVulkanCommandFilterProxyModel(m_data_core->GetCommandHierarchy(),
                                              m_command_hierarchy_view);

        m_gfxr_vulkan_commands_arguments_filter_proxy_model =
        new GfxrVulkanCommandArgumentsFilterProxyModel(m_command_hierarchy_view,
                                                       &m_data_core->GetCommandHierarchy());

        m_filter_model = new DiveFilterModel(m_data_core->GetCommandHierarchy(), this);
        m_filter_model->setSourceModel(m_command_hierarchy_model);
        // Set the proxy model as the view's model
        m_command_hierarchy_view->setModel(m_filter_model);
        m_command_hierarchy_view->setContextMenuPolicy(Qt::CustomContextMenu);
        m_filter_model->SetMode(kDefaultFilterMode);

        m_filter_gfxr_commands_combo_box =
        new GfxrVulkanCommandFilter(*m_command_hierarchy_view,
                                    *m_gfxr_vulkan_commands_filter_proxy_model);
        text_combo_box_layout->addWidget(m_filter_gfxr_commands_combo_box, 1);
        m_filter_gfxr_commands_combo_box->hide();

        m_search_trigger_button = new QPushButton;
        m_search_trigger_button->setIcon(QIcon(":/images/search.png"));
        text_combo_box_layout->addWidget(m_search_trigger_button);

        text_combo_box_layout->addStretch();
        text_combo_box_frame->setLayout(text_combo_box_layout);

        m_perf_counter_model = new PerfCounterModel();

        m_gpu_timing_model = new GpuTimingModel(this);

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
        m_left_group_box->setLayout(left_vertical_layout);
    }

    // Middle panel
    m_middle_group_box = new QGroupBox(kFrameTitleStrings[0]);
    m_middle_group_box->setAlignment(Qt::AlignHCenter);
    m_pm4_view_mode_combo_box = new TreeViewComboBox();
    m_pm4_view_mode_combo_box->setMinimumWidth(150);
    {
        QVBoxLayout *middle_vertical_layout = new QVBoxLayout();

        QFrame *text_combo_box_frame = new QFrame();

        QHBoxLayout *text_combo_box_layout = new QHBoxLayout();

        QLabel *combo_box_label = new QLabel(tr("Mode:"));

        // Set model for the view mode combo box
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
        m_pm4_view_mode_combo_box->setModel(combo_box_model);

        QModelIndex vulkan_event_item_index = combo_box_model->index(0, 0, event_item_index);
        m_pm4_view_mode_combo_box->setRootModelIndex(vulkan_event_item_index.parent());
        m_pm4_view_mode_combo_box->setCurrentIndex(vulkan_event_item_index.row());

        text_combo_box_layout->addWidget(combo_box_label);
        text_combo_box_layout->addWidget(m_pm4_view_mode_combo_box, 1);

        QLabel *filter_combo_box_label = new QLabel(tr("Filter:"));
        m_pm4_filter_mode_combo_box = new TreeViewComboBox();
        m_pm4_filter_mode_combo_box->setMinimumWidth(150);

        // Set model for the filter combo box
        QStandardItemModel *filter_combo_box_model = new QStandardItemModel();
        for (uint32_t i = 0; i < DiveFilterModel::kFilterModeCount; i++)
        {
            QStandardItem *item = new QStandardItem(kFilterStrings[i]);
            filter_combo_box_model->appendRow(item);
        }
        m_pm4_filter_mode_combo_box->setModel(filter_combo_box_model);
        m_pm4_filter_mode_combo_box->setCurrentIndex(kDefaultFilterMode);

        text_combo_box_layout->addWidget(filter_combo_box_label);
        text_combo_box_layout->addWidget(m_pm4_filter_mode_combo_box, 1);

        m_pm4_search_trigger_button = new QPushButton;
        m_pm4_search_trigger_button->setIcon(QIcon(":/images/search.png"));
        text_combo_box_layout->addWidget(m_pm4_search_trigger_button);

        text_combo_box_layout->addStretch();
        text_combo_box_frame->setLayout(text_combo_box_layout);

        m_pm4_event_search_bar = new SearchBar(this);
        m_pm4_event_search_bar->setObjectName("Event Search Bar");
        m_pm4_event_search_bar->setView(m_pm4_command_hierarchy_view);

        QHBoxLayout *search_layout = new QHBoxLayout;
        search_layout->addWidget(m_pm4_event_search_bar);
        m_pm4_event_search_bar->hide();

        m_pm4_command_hierarchy_view = new DiveTreeView(m_data_core->GetCommandHierarchy());
        m_pm4_command_hierarchy_view->SetDataCore(m_data_core.get());
        m_pm4_event_search_bar->setView(m_pm4_command_hierarchy_view);

        // Set the proxy model as the view's model
        m_pm4_command_hierarchy_view->setModel(m_filter_model);
        m_pm4_command_hierarchy_view->setContextMenuPolicy(Qt::CustomContextMenu);
        m_filter_model->SetMode(kDefaultFilterMode);
        //
        QLabel *pm4_goto_draw_call_label = new QLabel(tr("Go To:"));
        m_pm4_prev_event_button = new QPushButton("Prev Event");
        m_pm4_next_event_button = new QPushButton("Next Event");
        QHBoxLayout *pm4_goto_draw_call_layout = new QHBoxLayout();
        pm4_goto_draw_call_layout->addWidget(pm4_goto_draw_call_label);
        pm4_goto_draw_call_layout->addWidget(m_pm4_prev_event_button);
        pm4_goto_draw_call_layout->addWidget(m_pm4_next_event_button);
        pm4_goto_draw_call_layout->addStretch();

        QLabel *expand_to_lvl_label = new QLabel(tr("Expand to level:"));
        for (int i = 1; i <= 3; i++)
        {
            auto button = new QPushButton{ QString::number(i) };
            m_pm4_expand_to_lvl_buttons << button;
        }

        QHBoxLayout *pm4_expand_to_lvl_layout = new QHBoxLayout();
        pm4_expand_to_lvl_layout->addWidget(expand_to_lvl_label);
        foreach (auto expand_to_lvl_button, m_pm4_expand_to_lvl_buttons)
            pm4_expand_to_lvl_layout->addWidget(expand_to_lvl_button);
        pm4_expand_to_lvl_layout->addStretch();

        middle_vertical_layout->addWidget(m_pm4_event_search_bar);
        middle_vertical_layout->addWidget(text_combo_box_frame);
        middle_vertical_layout->addWidget(m_pm4_command_hierarchy_view);
        middle_vertical_layout->addLayout(pm4_goto_draw_call_layout);
        middle_vertical_layout->addLayout(pm4_expand_to_lvl_layout);
        m_middle_group_box->setLayout(middle_vertical_layout);
    }

    // Tabbed View
    m_tab_widget = new QTabWidget();
    {
        m_command_tab_view = new CommandTabView(m_data_core->GetCommandHierarchy());
        m_shader_view = new ShaderView(*m_data_core);

        m_trace_stats = new Dive::TraceStats();
        m_capture_stats = new Dive::CaptureStats();
        m_overview_tab_view = new OverviewTabView(m_data_core->GetCaptureMetadata(),
                                                  *m_capture_stats);
        m_event_state_view = new EventStateView(*m_data_core);

        m_perf_counter_tab_view = new PerfCounterTabView(*m_perf_counter_model, this);
        m_gfxr_vulkan_command_arguments_tab_view =
        new GfxrVulkanCommandArgumentsTabView(m_data_core->GetCommandHierarchy(),
                                              m_gfxr_vulkan_commands_arguments_filter_proxy_model,
                                              m_gfxr_vulkan_command_hierarchy_model);
        m_gpu_timing_tab_view = new GpuTimingTabView(*m_gpu_timing_model,
                                                     m_data_core->GetCommandHierarchy(),
                                                     this);

        m_frame_tab_view = new FrameTabView(this);

        m_overview_view_tab_index = m_tab_widget->addTab(m_overview_tab_view, "Overview");

        m_command_view_tab_index = m_tab_widget->addTab(m_command_tab_view, "PM4 Packets");
        m_shader_view_tab_index = m_tab_widget->addTab(m_shader_view, "Shaders");
        m_event_state_view_tab_index = m_tab_widget->addTab(m_event_state_view, "Event State");
        m_frame_view_tab_index = m_tab_widget->addTab(m_frame_tab_view, "Frame View");

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
    // TODO (b/445754645) Remove the PropertyPanel and replace with a tooltip or overlay.
    m_property_panel = new PropertyPanel();
    m_property_panel->setMinimumWidth(350);
    m_hover_help = HoverHelp::Get();
    m_shader_view->SetupHoverHelp(*m_hover_help);

    m_left_group_box->setMinimumSize(QSize(50, 0));
    m_middle_group_box->setMinimumSize(QSize(50, 0));
    m_tab_widget->setMinimumSize(QSize(50, 0));

    // The main horizontal splitter (Left, Middle, and Right panels, with a 1:1:1 size ratio)
    QSplitter *horizontal_splitter = new QSplitter(Qt::Horizontal);
    horizontal_splitter->addWidget(m_left_group_box);
    horizontal_splitter->addWidget(m_middle_group_box);
    horizontal_splitter->addWidget(m_tab_widget);
    horizontal_splitter->setStretchFactor(0, 1);
    horizontal_splitter->setStretchFactor(1, 1);
    horizontal_splitter->setStretchFactor(2, 1);

    QList<int> equal_sizes;
    equal_sizes << 1 << 1 << 1;
    horizontal_splitter->setSizes(equal_sizes);

    // Retrieve the available metrics
    LoadAvailableMetrics();

    m_trace_dig = new TraceDialog(this);
    m_analyze_dig = new AnalyzeDialog(m_available_metrics.get() ?
                                      std::optional<
                                      std::reference_wrapper<const Dive::AvailableMetrics>>(
                                      std::ref(*m_available_metrics.get())) :
                                      std::nullopt,
                                      this);

    // Main Window requires a central widget.
    // Make the horizontal splitter that central widget so it takes up the whole area.
    setCentralWidget(horizontal_splitter);

    m_middle_group_box->hide();

    // Connections
    QObject::connect(m_hover_help,
                     SIGNAL(CurrStringChanged(const QString &)),
                     m_property_panel,
                     SLOT(OnHoverStringChange(const QString &)));
    // Event selection connections
    QObject::connect(m_event_selection,
                     &EventSelection::vulkanParams,
                     m_property_panel,
                     &PropertyPanel::OnVulkanParams);
    QObject::connect(m_trace_dig,
                     &TraceDialog::TraceAvailable,
                     this,
                     &MainWindow::OnTraceAvailable);

    QObject::connect(this, &MainWindow::FileLoaded, m_text_file_view, &TextFileView::OnFileLoaded);
    QObject::connect(this, &MainWindow::FileLoaded, this, &MainWindow::OnFileLoaded);
    QObject::connect(m_search_trigger_button, SIGNAL(clicked()), this, SLOT(OnSearchTrigger()));

    QObject::connect(m_event_search_bar,
                     SIGNAL(hide_search_bar(bool)),
                     this,
                     SLOT(OnTabViewSearchBarVisibilityChange(bool)));

    QObject::connect(m_tab_widget, &QTabWidget::currentChanged, this, &MainWindow::OnTabViewChange);

    QObject::connect(m_analyze_dig,
                     &AnalyzeDialog::OnNewFileOpened,
                     this,
                     &MainWindow::OnOpenFileFromAnalyzeDialog);
    QObject::connect(m_analyze_dig,
                     &AnalyzeDialog::ReloadCapture,
                     this,
                     &MainWindow::OnOpenFileFromAnalyzeDialog);

    QObject::connect(this,
                     &MainWindow::PendingGpuTimingResults,
                     this,
                     &MainWindow::OnPendingGpuTimingResults);
    QObject::connect(this,
                     &MainWindow::PendingPerfCounterResults,
                     this,
                     &MainWindow::OnPendingPerfCounterResults);
    QObject::connect(this, &MainWindow::PendingScreenshot, this, &MainWindow::OnPendingScreenshot);

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
    QObject::connect(this, &MainWindow::HideOverlay, this, &MainWindow::OnHideOverlay);

#ifndef NDEBUG
    showMaximized();
#endif
    // Set default view mode
    OnCommandViewModeChange(tr(kEventViewModeStrings[0]));
    m_hover_help->SetCurItem(HoverHelp::Item::kNone);
    m_hover_help->SetDataCore(m_data_core.get());
    setAccessibleName("DiveMainWindow");

    m_plugin_manager = std::unique_ptr<Dive::PluginLoader>(new Dive::PluginLoader(*this));
}

//--------------------------------------------------------------------------------------------------
MainWindow::~MainWindow() {}

//--------------------------------------------------------------------------------------------------
bool MainWindow::InitializePlugins()
{
    // This assumes plugins are in a 'plugins' subdirectory relative to the executable's directory.
    std::string plugin_path = QCoreApplication::applicationDirPath().toStdString() + "/plugins";

    std::filesystem::path plugins_dir_path(plugin_path);

    if (absl::Status load_status = m_plugin_manager->LoadPlugins(plugins_dir_path);
        !load_status.ok())
    {
        QMessageBox::warning(this,
                             tr("Plugin Loading Failed"),
                             tr("Failed to load plugins from '%1'. \nError: %2")
                             .arg(QString::fromStdString(plugin_path))
                             .arg(QString::fromStdString(std::string(load_status.message()))));
        return false;
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnTraceAvailable(const QString &path)
{
    qDebug() << "Trace is at " << path;
    // Figure out what do we do if we get repeated trigger of LoadFile before async call is done.
    LoadFile(path.toStdString().c_str(), /*is_temp_file*/ true, /*async*/ false);
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
    QObject *sender_object = sender();
    if (sender_object == m_pm4_view_mode_combo_box)
    {
        m_pm4_command_hierarchy_view->header()->reset();
    }
    else
    {
        m_command_hierarchy_view->header()->reset();
    }

    const Dive::CommandHierarchy &command_hierarchy = m_data_core->GetCommandHierarchy();
    if (view_mode == tr(kViewModeStrings[0]))  // Submit
    {
        const Dive::SharedNodeTopology &topology = command_hierarchy.GetSubmitHierarchyTopology();
        m_command_hierarchy_model->SetTopologyToView(&topology);
        m_command_tab_view->SetTopologyToView(&topology);
    }
    else  // All Vulkan Calls + GPU Events
    {
        const Dive::SharedNodeTopology &topology = command_hierarchy.GetAllEventHierarchyTopology();
        if (m_gfxr_capture_loaded)
        {
            m_gfxr_vulkan_command_hierarchy_model->SetTopologyToView(&topology);
        }
        else
        {
            m_gfxr_vulkan_command_hierarchy_model->SetTopologyToView(&topology);
            m_command_hierarchy_model->SetTopologyToView(&topology);
            m_command_tab_view->SetTopologyToView(&topology);
        }

        // Put EventID column to the left of the tree. This forces the expand/collapse icon to be
        // part of the 2nd column (originally 1st)
        if (m_prev_command_view_mode.isEmpty() ||
            m_prev_command_view_mode == tr(kViewModeStrings[0]) ||
            m_prev_command_view_mode == tr(kViewModeStrings[1]))
        {
            if (sender_object == m_pm4_view_mode_combo_box)
            {
                m_pm4_command_hierarchy_view->header()->moveSection(1, 0);
            }
            else
            {
                m_command_hierarchy_view->header()->moveSection(1, 0);
            }
        }
    }

    m_prev_command_view_mode = view_mode;
    if (sender_object == m_pm4_view_mode_combo_box)
    {
        ExpandResizeHierarchyView(*m_pm4_command_hierarchy_view, *m_filter_model);
    }
    else if (m_gfxr_capture_loaded)
    {
        ExpandResizeHierarchyView(*m_command_hierarchy_view,
                                  *m_gfxr_vulkan_commands_filter_proxy_model);
    }
    else
    {
        ExpandResizeHierarchyView(*m_command_hierarchy_view, *m_filter_model);
    }
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
    uint64_t                      selected_item_node_index;

    if (m_gfxr_capture_loaded)
    {
        QModelIndex source_index = m_gfxr_vulkan_commands_filter_proxy_model->mapToSource(index);
        selected_item_node_index = (uint64_t)(source_index.internalPointer());
    }
    else
    {
        QModelIndex source_model_index = m_filter_model->mapToSource(index);
        selected_item_node_index = (uint64_t)(source_model_index.internalPointer());
    }

    Dive::NodeType node_type = command_hierarchy.GetNodeType(selected_item_node_index);
    if (Dive::IsDrawDispatchBlitNode(node_type) || node_type == Dive::NodeType::kMarkerNode)
    {
        emit EventSelected(selected_item_node_index);
    }
    else
    {
        emit EventSelected(UINT64_MAX);
    }
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnFilterModeChange(const QString &filter_mode)
{
    DiveFilterModel::FilterMode new_mode;

    if (filter_mode == kFilterStrings[DiveFilterModel::kNone])
    {
        new_mode = DiveFilterModel::kNone;
    }
    else if (filter_mode == kFilterStrings[DiveFilterModel::kBinningPassOnly])
    {
        new_mode = DiveFilterModel::kBinningPassOnly;
    }
    else if (filter_mode == kFilterStrings[DiveFilterModel::kFirstTilePassOnly])
    {
        new_mode = DiveFilterModel::kFirstTilePassOnly;
    }
    else if (filter_mode == kFilterStrings[DiveFilterModel::kBinningAndFirstTilePass])
    {
        new_mode = DiveFilterModel::kBinningAndFirstTilePass;
    }
    else
    {
        new_mode = DiveFilterModel::kNone;
    }

    if (m_filter_model)
    {
        m_filter_model->SetMode(new_mode);
    }

    if (m_command_hierarchy_view)
    {
        ResetVerticalScroll(*m_command_hierarchy_view);
        m_command_hierarchy_view->scrollToTop();
    }

    m_perf_counter_tab_view->ClearSelection();
    m_gpu_timing_tab_view->ClearSelection();

    if (m_correlated_capture_loaded)
    {
        ClearViewModelSelection(*m_command_hierarchy_view, true);
        ClearViewModelSelection(*m_pm4_command_hierarchy_view, false);
        ExpandResizeHierarchyView(*m_pm4_command_hierarchy_view, *m_filter_model);
    }
    else
    {
        ClearViewModelSelection(*m_command_hierarchy_view, true);
        ExpandResizeHierarchyView(*m_command_hierarchy_view, *m_filter_model);
    }
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnGfxrFilterModeChange()
{
    ClearViewModelSelection(*m_command_hierarchy_view, false);

    m_perf_counter_tab_view->ClearSelection();
    m_gpu_timing_tab_view->ClearSelection();

    ResetVerticalScroll(*m_command_hierarchy_view);
    m_command_hierarchy_view->scrollToTop();
    if (m_correlated_capture_loaded)
    {
        m_pm4_command_hierarchy_view->scrollToTop();
        ClearViewModelSelection(*m_pm4_command_hierarchy_view, false);
        ExpandResizeHierarchyView(*m_pm4_command_hierarchy_view, *m_filter_model);
    }
}

//--------------------------------------------------------------------------------------------------
void MainWindow::ResetTabWidget()
{
    // Disconnect OnTabViewChange so that it is not triggered during the reset of the tab widget.
    QObject::disconnect(m_tab_widget,
                        &QTabWidget::currentChanged,
                        this,
                        &MainWindow::OnTabViewChange);

    // Remove all the tabs.
    while (m_tab_widget->count() > 0)
    {
        m_tab_widget->removeTab(0);
    }

    // Reset all of the tab indices.
    m_gfxr_vulkan_command_arguments_view_tab_index = -1;
    m_overview_view_tab_index = -1;
    m_command_view_tab_index = -1;
    m_shader_view_tab_index = -1;
    m_event_state_view_tab_index = -1;
    m_perf_counter_view_tab_index = -1;
    m_gpu_timing_view_tab_index = -1;
    m_frame_view_tab_index = -1;

    // Reconnect OnTabViewChange.
    QObject::connect(m_tab_widget, &QTabWidget::currentChanged, this, &MainWindow::OnTabViewChange);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnDiveFileLoaded()
{
    // Reset models and views that display data from the capture
    m_left_group_box->setTitle(kFrameTitleStrings[1]);
    m_middle_group_box->show();
    m_middle_group_box->setTitle(kFrameTitleStrings[2]);
    m_gfxr_vulkan_command_arguments_tab_view->ResetModel();
    m_gfxr_vulkan_command_hierarchy_model->Reset();
    m_command_tab_view->ResetModel();
    m_command_hierarchy_model->Reset();
    m_event_selection->Reset();
    m_shader_view->Reset();
    m_text_file_view->Reset();
    m_prev_command_view_mode = QString();
    m_filter_gfxr_commands_combo_box->Reset();

    m_gfxr_vulkan_commands_filter_proxy_model->setSourceModel(
    m_gfxr_vulkan_command_hierarchy_model);

    m_command_hierarchy_model->BeginResetModel();

    // Reset the tab widget.
    ResetTabWidget();

    // Add the tabs required for an Dive file or .rd and .gfxr file.
    m_gfxr_vulkan_command_arguments_view_tab_index =
    m_tab_widget->addTab(m_gfxr_vulkan_command_arguments_tab_view, "Command Arguments");
    m_command_view_tab_index = m_tab_widget->addTab(m_command_tab_view, "PM4 Packets");
    m_event_state_view_tab_index = m_tab_widget->addTab(m_event_state_view, "Event State");
    m_overview_view_tab_index = m_tab_widget->addTab(m_overview_tab_view, "Overview");
    m_perf_counter_view_tab_index = m_tab_widget->addTab(m_perf_counter_tab_view, "Perf Counters");
    m_shader_view_tab_index = m_tab_widget->addTab(m_shader_view, "Shaders");
    m_gpu_timing_view_tab_index = m_tab_widget->addTab(m_gpu_timing_tab_view, "Gpu Timing");
    m_frame_view_tab_index = m_tab_widget->addTab(m_frame_tab_view, "Frame View");
#if defined(ENABLE_CAPTURE_BUFFERS)
    // If m_buffer_view is dynamically created/deleted, handle it here.
    // If it's a fixed member, ensure it's reset.
    if (!m_buffer_view)
    {  // Only create if null, otherwise just reset
        m_buffer_view = new BufferView(*m_data_core);
    }
    else
    {
        // m_buffer_view->Reset(); // Assuming it has a reset method
    }
    m_tab_widget->addTab(m_buffer_view, "Buffers");
#endif

    // Left Panel contains gfxr display
    m_command_hierarchy_view->setModel(m_gfxr_vulkan_commands_filter_proxy_model);
    // Reset and enable the gfxr command filter
    m_filter_gfxr_commands_combo_box->Reset();
    m_filter_gfxr_commands_combo_box->setEnabled(true);
    m_filter_gfxr_commands_combo_box->show();
    // Disable the view mode combo box and filter
    m_filter_model->SetMode(kDefaultFilterMode);
    m_filter_mode_combo_box->setCurrentIndex(kDefaultFilterMode);
    m_view_mode_combo_box->setEnabled(false);
    m_filter_mode_combo_box->hide();

    // Middle Panel contains pm4 display
    m_pm4_command_hierarchy_view->setModel(m_filter_model);
    m_pm4_filter_mode_combo_box->setEnabled(true);
    m_pm4_filter_mode_combo_box->show();
    m_pm4_filter_mode_combo_box->setCurrentIndex(kDefaultFilterMode);

    ConnectDiveFileTabs();

    {
        OnCommandViewModeChange(tr(kEventViewModeStrings[0]));
        // TODO (b/185579518): disable the dropdown list for vulkan events.
    }
    m_command_hierarchy_model->EndResetModel();
    m_gfxr_vulkan_command_hierarchy_model->EndResetModel();

    // Collect the gfxr draw call indices
    m_gfxr_vulkan_commands_filter_proxy_model->CollectGfxrDrawCallIndices();

    // Collect the PM4 draw call indices for the current filter
    m_filter_model->CollectPm4DrawCallIndices(QModelIndex());

    // Iterate m_gfxr_vulkan_command_hierarchy_model to collect the indices of the vulkan events
    // where gpu timing data will be collected
    m_gpu_timing_tab_view->CollectIndicesFromModel(*m_gfxr_vulkan_command_hierarchy_model,
                                                   QModelIndex());

    // Ensure there is no previous tab index set
    m_previous_tab_index = -1;

    // Gather the trace stats and display in the overview tab
    m_trace_stats->GatherTraceStats(m_data_core->GetCaptureMetadata(), *m_capture_stats);
    m_overview_tab_view->LoadStatistics();
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnAdrenoRdFileLoaded()
{
    // Reset models and views that display data from the capture
    m_left_group_box->setTitle(kFrameTitleStrings[2]);
    m_middle_group_box->setTitle(kFrameTitleStrings[0]);
    m_middle_group_box->hide();
    m_command_tab_view->ResetModel();
    m_command_hierarchy_model->Reset();
    m_event_selection->Reset();
    m_shader_view->Reset();
    m_text_file_view->Reset();
    m_prev_command_view_mode = QString();

    m_command_hierarchy_model->BeginResetModel();

    // Reset the tab widget.
    ResetTabWidget();

    // Add the tabs required for an AdrenoRd file.
    m_overview_view_tab_index = m_tab_widget->addTab(m_overview_tab_view, "Overview");
    m_command_view_tab_index = m_tab_widget->addTab(m_command_tab_view, "PM4 Packets");
    m_shader_view_tab_index = m_tab_widget->addTab(m_shader_view, "Shaders");
    m_event_state_view_tab_index = m_tab_widget->addTab(m_event_state_view, "Event State");
#if defined(ENABLE_CAPTURE_BUFFERS)
    // If m_buffer_view is dynamically created/deleted, handle it here.
    // If it's a fixed member, ensure it's reset.
    if (!m_buffer_view)
    {  // Only create if null, otherwise just reset
        m_buffer_view = new BufferView(*m_data_core);
    }
    else
    {
        // m_buffer_view->Reset(); // Assuming it has a reset method
    }
    m_tab_widget->addTab(m_buffer_view, "Buffers");
#endif

    m_filter_model->SetMode(kDefaultFilterMode);
    m_command_hierarchy_view->setModel(m_filter_model);

    m_filter_mode_combo_box->setEnabled(true);
    m_filter_mode_combo_box->show();
    m_filter_mode_combo_box->setCurrentIndex(kDefaultFilterMode);
    m_filter_gfxr_commands_combo_box->Reset();
    m_filter_gfxr_commands_combo_box->setEnabled(false);
    m_filter_gfxr_commands_combo_box->hide();
    m_view_mode_combo_box->setEnabled(true);
    m_pm4_filter_mode_combo_box->setEnabled(false);

    ConnectAdrenoRdFileTabs();

    {
        OnCommandViewModeChange(tr(kEventViewModeStrings[0]));
        // TODO (b/185579518): disable the dropdown list for vulkan events.
    }
    m_command_hierarchy_model->EndResetModel();

    // Collect the PM4 draw call indices for the current filter
    m_filter_model->CollectPm4DrawCallIndices(QModelIndex());

    // Ensure there is no previous tab index set
    m_previous_tab_index = -1;

    // Gather the trace stats and display in the overview tab
    m_trace_stats->GatherTraceStats(m_data_core->GetCaptureMetadata(), *m_capture_stats);
    m_overview_tab_view->LoadStatistics();
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnGfxrFileLoaded()
{
    // Reset models and views that display data from the capture
    m_left_group_box->setTitle(kFrameTitleStrings[1]);
    m_middle_group_box->setTitle(kFrameTitleStrings[0]);
    m_middle_group_box->hide();
    m_gfxr_vulkan_command_hierarchy_model->Reset();
    m_prev_command_view_mode = QString();
    m_filter_gfxr_commands_combo_box->Reset();

    m_gfxr_vulkan_command_hierarchy_model->BeginResetModel();

    // Reset the tab widget.
    ResetTabWidget();

    m_gfxr_vulkan_commands_filter_proxy_model->setSourceModel(
    m_gfxr_vulkan_command_hierarchy_model);
    m_command_hierarchy_view->setModel(m_gfxr_vulkan_commands_filter_proxy_model);

    ConnectGfxrFileTabs();

    m_gfxr_vulkan_command_arguments_view_tab_index =
    m_tab_widget->addTab(m_gfxr_vulkan_command_arguments_tab_view, "Command Arguments");
    m_perf_counter_view_tab_index = m_tab_widget->addTab(m_perf_counter_tab_view, "Perf Counters");
    m_gpu_timing_view_tab_index = m_tab_widget->addTab(m_gpu_timing_tab_view, "Gpu Timing");
    m_frame_view_tab_index = m_tab_widget->addTab(m_frame_tab_view, "Frame View");

    // Ensure the All Event topology is displayed.
    OnCommandViewModeChange(tr(kEventViewModeStrings[0]));
    // Disable the Mode and Filter combo boxes.
    m_view_mode_combo_box->setEnabled(false);
    m_filter_mode_combo_box->setEnabled(false);
    m_filter_mode_combo_box->hide();
    m_filter_gfxr_commands_combo_box->setEnabled(true);
    m_filter_gfxr_commands_combo_box->show();
    m_pm4_filter_mode_combo_box->setEnabled(false);

    m_gfxr_vulkan_command_hierarchy_model->EndResetModel();

    // Iterate m_gfxr_vulkan_command_hierarchy_model to collect the indices of the vulkan events
    // where gpu timing data will be collected
    m_gpu_timing_tab_view->CollectIndicesFromModel(*m_gfxr_vulkan_command_hierarchy_model,
                                                   QModelIndex());

    // Ensure there is no previous tab index set
    m_previous_tab_index = -1;
}

//--------------------------------------------------------------------------------------------------
void MainWindow::RunOnUIThread(std::function<void()> f)
{

    QMetaObject::invokeMethod(this, [=]() { f(); });
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnLoadFailure(Dive::CaptureData::LoadResult result, const std::string &file_name)
{
    if (result == Dive::CaptureData::LoadResult::kSuccess)
    {
        return;
    }

    // Do dialog on main thread.
    QMetaObject::invokeMethod(this, [=]() {
        HideOverlay();
        QString error_msg;
        if (result == Dive::CaptureData::LoadResult::kFileIoError)
            error_msg = QString("File I/O error!");
        else if (result == Dive::CaptureData::LoadResult::kCorruptData)
            error_msg = QString("File corrupt!");
        else if (result == Dive::CaptureData::LoadResult::kVersionError)
            error_msg = QString("Incompatible version!");
        QMessageBox::critical(this,
                              (QString("Unable to open file: ") + file_name.c_str()),
                              error_msg);
    });
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnParseFailure(const std::string &file_name)
{

    // Do dialog on main thread.
    QMetaObject::invokeMethod(this, [=]() {
        HideOverlay();
        QMessageBox::critical(this,
                              QString("Error parsing file"),
                              (QString("Unable to parse file: ") + file_name.c_str()));
    });
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnUnsupportedFile(const std::string &file_name)
{
    QMetaObject::invokeMethod(this, [=]() {
        QString error_msg = QString("File type not supported!");
        QMessageBox::critical(this,
                              (QString("Unable to open file: ") + file_name.c_str()),
                              error_msg);
    });
}

//--------------------------------------------------------------------------------------------------
bool MainWindow::LoadFile(const std::string &file_name, bool is_temp_file, bool async)
{
    if (m_loading_result.valid())
    {
        // We are still loading something else.
        return false;
    }

    // We don't want other UI interaction as they cause race conditions.
    setDisabled(true);

    m_progress_tracker.sendMessage("Loading " + file_name);

    m_log_record.Reset();

    m_command_hierarchy_view->setCurrentIndex(QModelIndex());

    // Disconnect the signals for all of the possible tabs.
    DisconnectAllTabs();

    // Clear vectors of draw call indices as they are only used for a correlated view.
    m_filter_model->ClearDrawCallIndices();

    // Discard associated timing results.
    m_perf_counter_model->OnPerfCounterResultsGenerated("", std::nullopt);
    m_gpu_timing_model->OnGpuTimingResultsGenerated("");
    if (async)
    {
        // Start async file loading, at the end of loading FileLoaded will be triggered.
        m_loading_result = std::async([this, file_name = file_name, is_temp_file = is_temp_file]() {
            std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

            auto file_type = LoadFileImpl(file_name, is_temp_file);
            [[maybe_unused]] int64_t
            time_used_to_load_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now() - begin)
                                   .count();

            DIVE_DEBUG_LOG("Time used to load the capture is %f seconds.\n",
                           (time_used_to_load_ms / 1000.0));
            // Now that the file is loaded, we can send a signal to UI thread.
            FileLoaded();
            return LoadFileResult{ file_type, file_name, is_temp_file };
        });
    }
    else
    {
        // This code path is for UI element that can't handle async operations.
        // e.g. AnalyzeWindow
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

        auto file_type = LoadFileImpl(file_name, is_temp_file);
        [[maybe_unused]] int64_t
        time_used_to_load_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - begin)
                               .count();
        DIVE_DEBUG_LOG("Time used to load the capture is %f seconds.\n",
                       (time_used_to_load_ms / 1000.0));

        std::promise<LoadFileResult> p;
        p.set_value(LoadFileResult{ file_type, file_name, is_temp_file });
        m_loading_result = p.get_future();
        FileLoaded();
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnPendingPerfCounterResults(const QString &file_name)
{
    if (!(m_perf_counter_model && m_available_metrics))
    {
        return;
    }

    auto file_path = std::filesystem::path(file_name.toStdString());
    auto task = [=]() {
        m_perf_counter_model->OnPerfCounterResultsGenerated(file_path, *m_available_metrics);
        if (!file_path.empty())
        {
            qDebug() << "Loaded: " << file_path.string().c_str();
        }
    };
    if (m_loading_result.valid())
    {
        m_loading_pending_task.push_back(task);
        return;
    }
    task();
}

void MainWindow::OnPendingGpuTimingResults(const QString &file_name)
{
    if (!m_gpu_timing_model)
    {
        return;
    }
    auto task = [=]() {
        m_gpu_timing_model->OnGpuTimingResultsGenerated(file_name);
        if (!file_name.isEmpty())
        {
            qDebug() << "Loaded: " << file_name;
        }
    };
    if (m_loading_result.valid())
    {
        m_loading_pending_task.push_back(task);
        return;
    }
    task();
}

void MainWindow::OnPendingScreenshot(const QString &file_name)
{
    if (!m_frame_tab_view)
    {
        return;
    }
    auto task = [=]() {
        m_frame_tab_view->OnCaptureScreenshotLoaded(file_name);
        if (!file_name.isEmpty())
        {
            qDebug() << "Loaded: " << file_name;
        }
    };
    if (m_loading_result.valid())
    {
        m_loading_pending_task.push_back(task);
        return;
    }
    task();
}

//--------------------------------------------------------------------------------------------------
MainWindow::LoadedFileType MainWindow::LoadFileImpl(const std::string &file_name, bool is_temp_file)
{
    // Note: this function might not run on UI thread, thus can't do any UI modification.

    // Check the file type to determine what is loaded.
    std::string file_extension = std::filesystem::path(file_name).extension().generic_string();

    // Check if the file loaded is a .gfxr file.
    m_gfxr_capture_loaded = (file_extension.compare(Dive::kGfxrSuffix) == 0);

    // Reset the correlated capture variable
    m_correlated_capture_loaded = false;

    if (m_gfxr_capture_loaded)
    {
        // Convert the filename to a string to perform a replacement.
        std::string potential_asset_name(file_name);

        const std::string trim_str = "_trim_trigger";
        const std::string asset_str = "_asset_file";

        // Find and replace the "trim_trigger" part of the filename.
        size_t pos = potential_asset_name.find(trim_str);
        if (pos != std::string::npos)
        {
            potential_asset_name.replace(pos, trim_str.length(), asset_str);
        }

        // Create a path object to the asset file.
        std::filesystem::path asset_file_path(potential_asset_name);
        asset_file_path.replace_extension(".gfxa");

        // Check if the required asset file exists.
        bool asset_file_exists = std::filesystem::exists(asset_file_path);

        if (!asset_file_exists)
        {
            RunOnUIThread([=]() {
                HideOverlay();
                QString title = QString("Unable to open file: %1").arg(file_name.c_str());
                QString description = QString("Required .gfxa file: %1 not found!")
                                      .arg(QString::fromStdString(asset_file_path.string()));
                QMessageBox::critical(this, title, description);
                m_gfxr_capture_loaded = false;
            });
            return LoadedFileType::kUnknown;
        }

        // Paths of associated files produced by Dive's GFXR replay
        std::filesystem::path capture_file_path = file_name;
        std::filesystem::path rd_file_path = capture_file_path.replace_extension(".rd");
        std::filesystem::path perf_counter_file_path = capture_file_path.parent_path() /
                                                       (capture_file_path.stem().string() +
                                                        Dive::kProfilingMetricsCsvSuffix);
        std::filesystem::path gpu_time_file_path = capture_file_path.parent_path() /
                                                   (capture_file_path.stem().string() +
                                                    Dive::kGpuTimingCsvSuffix);
        std::filesystem::path screenshot_file_path = capture_file_path.parent_path() /
                                                     (capture_file_path.stem().string() +
                                                      Dive::kPngSuffix);

        // Check if there is a corresponding .rd file
        if (std::filesystem::exists(rd_file_path))
        {
            m_gfxr_capture_loaded = false;
            m_correlated_capture_loaded = true;
        }

        // Check if there is existing perf counter data
        qDebug() << "Attempting to load perf counter data from: "
                 << perf_counter_file_path.string().c_str();
        if (std::filesystem::exists(perf_counter_file_path))
        {
            PendingPerfCounterResults(QString::fromStdString(perf_counter_file_path.string()));
        }
        else
        {
            PendingPerfCounterResults("");
            qDebug() << "Failed to find perf counter data";
        }

        // Check if there is existing gpu timing data
        qDebug() << "Attempting to load gpu timing data from: "
                 << gpu_time_file_path.string().c_str();
        if (std::filesystem::exists(gpu_time_file_path))
        {
            PendingGpuTimingResults(QString::fromStdWString(gpu_time_file_path.wstring()));
        }
        else
        {
            PendingGpuTimingResults("");
            qDebug() << "Failed to find gpu timing data";
        }

        // Check if there is an existing screenshot
        qDebug() << "Attempting to load screenshot from: " << screenshot_file_path.string().c_str();
        if (std::filesystem::exists(screenshot_file_path))
        {
            PendingScreenshot(QString::fromStdWString(screenshot_file_path.wstring()));
            qDebug() << "Loaded: " << screenshot_file_path.string().c_str();
        }
        else
        {
            PendingScreenshot("");
            qDebug() << "Failed to find gfxr capture screenshot";
        }
    }

    LoadedFileType file_type = LoadedFileType::kUnknown;
    if (m_gfxr_capture_loaded)
    {
        file_type = LoadedFileType::kGfxrFile;
    }
    else if (m_correlated_capture_loaded)
    {
        file_type = LoadedFileType::kDiveFile;
    }
    else if (file_extension.compare(".dive") == 0)
    {
        file_type = LoadedFileType::kRdFile;
    }
    else if (file_extension.compare(".rd") == 0)
    {
        file_type = LoadedFileType::kRdFile;
    }
    else
    {
        file_type = LoadedFileType::kUnknown;
    }

    switch (file_type)
    {
    case LoadedFileType::kUnknown:
        OnUnsupportedFile(file_name);
        break;
    case LoadedFileType::kDiveFile:
    {
        if (Dive::CaptureData::LoadResult load_res = m_data_core->LoadDiveCaptureData(file_name);
            load_res != Dive::CaptureData::LoadResult::kSuccess)
        {
            OnLoadFailure(load_res, file_name);
            return LoadedFileType::kUnknown;
        }

        if (!m_data_core->ParseDiveCaptureData())
        {
            OnParseFailure(file_name);
            return LoadedFileType::kUnknown;
        }
    }
    break;
    case LoadedFileType::kRdFile:
    {
        if (Dive::CaptureData::LoadResult load_res = m_data_core->LoadPm4CaptureData(file_name);
            load_res != Dive::CaptureData::LoadResult::kSuccess)
        {
            OnLoadFailure(load_res, file_name);
            return LoadedFileType::kUnknown;
        }

        if (!m_data_core->ParsePm4CaptureData())
        {
            OnParseFailure(file_name);
            return LoadedFileType::kUnknown;
        }
    }
    break;
    case LoadedFileType::kGfxrFile:
    {
        if (Dive::CaptureData::LoadResult load_res = m_data_core->LoadGfxrCaptureData(file_name);
            load_res != Dive::CaptureData::LoadResult::kSuccess)
        {
            OnLoadFailure(load_res, file_name);
            return LoadedFileType::kUnknown;
        }

        if (!m_data_core->ParseGfxrCaptureData())
        {
            OnParseFailure(file_name);
            return LoadedFileType::kUnknown;
        }
    }
    break;
    }
    return file_type;
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnFileLoaded()
{
    DIVE_ASSERT(m_loading_result.valid());

    // It should return almost immediately, the signal is sent just before async call return.
    auto result = m_loading_result.get();

    std::vector<std::function<void()>> tasks;
    std::swap(tasks, m_loading_pending_task);
    for (auto &task : tasks)
    {
        task();
    }

    // Re-enable UI interaction now we are done async loading.
    setDisabled(false);
    HideOverlay();

    switch (result.file_type)
    {
    case LoadedFileType::kUnknown:
        return;
    case LoadedFileType::kDiveFile:
        OnDiveFileLoaded();
        ExpandResizeHierarchyView(*m_command_hierarchy_view,
                                  *m_gfxr_vulkan_commands_filter_proxy_model);
        ExpandResizeHierarchyView(*m_pm4_command_hierarchy_view, *m_filter_model);
        break;
    case LoadedFileType::kRdFile:
        OnAdrenoRdFileLoaded();
        ExpandResizeHierarchyView(*m_command_hierarchy_view, *m_filter_model);
        break;
    case LoadedFileType::kGfxrFile:
        OnGfxrFileLoaded();
        ExpandResizeHierarchyView(*m_command_hierarchy_view,
                                  *m_gfxr_vulkan_commands_filter_proxy_model);
        break;
    }

    m_hover_help->SetCurItem(HoverHelp::Item::kNone);
    m_capture_file = QString(result.file_name.c_str());
    QFileInfo file_info(m_capture_file);
    SetCurrentFile(m_capture_file, result.is_temp_file);
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

    UpdateTabAvailability();
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnOpenFile()
{
    QString supported_files = QStringLiteral(
    "Dive files (*.rd);;GFXR files (*.gfxr);;All files (*.*)");
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
                                  (QString("Unable to open file: ") + file_name));
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
void MainWindow::OnAnalyzeCapture()
{
    if (!m_gfxr_capture_loaded && !m_correlated_capture_loaded)
    {
        OnOpenFile();
    }
    // If the a .gfxr file is still unsuccessfully loaded, do not open the analyze dialog. A .gfxr
    // file is loaded when m_correlated_capture_loaded or m_gfxr_capture_loaded are true.
    bool gfxr_capture_loaded = (m_gfxr_capture_loaded || m_correlated_capture_loaded);
    if (gfxr_capture_loaded)
    {
        OnAnalyze(gfxr_capture_loaded, m_capture_file.toStdString());
    }
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
    m_trace_dig->open();
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnAnalyze(bool is_gfxr_capture_loaded, const std::string &file_path)
{
    if (!is_gfxr_capture_loaded)
    {
        QMessageBox::
        critical(this,
                 tr("Analyzation Load Failed"),
                 tr("Unable to analyze without gfxr capture data loaded. Ensure a capture "
                    "containing gfxr data is loaded before attempting to analyze."));
        return;
    }
    QString file_path_q_string = QString::fromStdString(file_path);
    m_analyze_dig->SetSelectedCaptureFile(file_path_q_string);
    m_analyze_dig->open();
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
            int           level = button->text().toInt();
            DiveTreeView *target_view = nullptr;

            if (m_expand_to_lvl_buttons.contains(button))
            {
                target_view = m_command_hierarchy_view;
            }
            else
            {
                target_view = m_pm4_command_hierarchy_view;
            }

            if (target_view)
            {
                target_view->ExpandToLevel(level);
                target_view->RetainCurrentNode();
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnAbout()
{
    AboutDialog *about = new AboutDialog();
    QObject::connect(about, &AboutDialog::finished, about, &AboutDialog::deleteLater);
    about->open();
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnShortcuts()
{
    ShortcutsDialog *shortcuts = new ShortcutsDialog();
    QObject::connect(shortcuts,
                     &ShortcutsDialog::finished,
                     shortcuts,
                     &ShortcutsDialog::deleteLater);
    shortcuts->open();
}

//--------------------------------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent *closeEvent)
{
    DIVE_ASSERT(m_plugin_manager != nullptr);
    m_plugin_manager->UnloadPlugins();

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
        QMessageBox::information(this, tr("Save capture succeed"), tr("Save capture succeed."));
    }
    else
    {
        QMessageBox::critical(this,
                              tr("Save capture file failed"),
                              tr("Save capture file failed."));
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
void MainWindow::ResetEventSearchBar()
{
    m_event_search_bar->clearSearch();
    m_event_search_bar->hide();
    m_search_trigger_button->show();
    DisconnectSearchBar();
}

//--------------------------------------------------------------------------------------------------
void MainWindow::ResetPm4EventSearchBar()
{
    m_pm4_event_search_bar->clearSearch();
    m_pm4_event_search_bar->hide();
    m_pm4_search_trigger_button->show();
    DisconnectPm4SearchBar();
}

//--------------------------------------------------------------------------------------------------
void MainWindow::ResetHorizontalScroll(const DiveTreeView &tree_view)
{
    QScrollBar *h_scroll_bar = tree_view.horizontalScrollBar();
    if (h_scroll_bar)
    {
        h_scroll_bar->triggerAction(QAbstractSlider::SliderToMinimum);
        QApplication::processEvents();
    }
}

void MainWindow::ResetVerticalScroll(const DiveTreeView &tree_view)
{
    QScrollBar *v_scroll_bar = tree_view.verticalScrollBar();
    if (v_scroll_bar)
    {
        v_scroll_bar->triggerAction(QAbstractSlider::SliderToMinimum);
        QApplication::processEvents();
    }
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnSearchTrigger()
{
    QObject *sender_object = sender();

    if (sender_object == m_search_trigger_button)
    {
        if (m_event_search_bar->isVisible())
        {
            ResetEventSearchBar();
        }
        else
        {
            ConnectSearchBar();
            m_search_trigger_button->hide();
            m_event_search_bar->positionCurser();
            m_event_search_bar->show();

            if (m_pm4_event_search_bar->isVisible())
            {
                ResetPm4EventSearchBar();
            }
        }
    }
    else if (sender_object == m_pm4_search_trigger_button)
    {
        if (m_pm4_event_search_bar->isVisible())
        {
            ResetPm4EventSearchBar();
        }
        else
        {
            ConnectPm4SearchBar();
            m_pm4_search_trigger_button->hide();
            m_pm4_event_search_bar->positionCurser();
            m_pm4_event_search_bar->show();

            if (m_event_search_bar->isVisible())
            {
                ResetEventSearchBar();
            }
        }
    }

    int          current_index = m_tab_widget->currentIndex();
    QWidget     *current_tab = m_tab_widget->widget(current_index);
    SearchBar   *tab_wiget_search_bar;
    QPushButton *tab_wiget_search_button;

    if (current_index == m_command_view_tab_index)
    {
        tab_wiget_search_bar = current_tab->findChild<SearchBar *>(kCommandBufferSearchBarName);
        tab_wiget_search_button = current_tab->findChild<QPushButton *>(
        kCommandBufferSearchButtonName);
        if (!tab_wiget_search_bar->isHidden())
        {
            tab_wiget_search_bar->clearSearch();
            tab_wiget_search_bar->hide();
        }
        tab_wiget_search_button->show();
    }
    else if (current_index == m_gfxr_vulkan_command_arguments_view_tab_index)
    {
        tab_wiget_search_bar = current_tab->findChild<SearchBar *>(
        kGfxrVulkanCommandArgumentsSearchBarName);
        tab_wiget_search_button = current_tab->findChild<QPushButton *>(
        kGfxrVulkanCommandArgumentsSearchButtonName);

        if (!tab_wiget_search_bar->isHidden())
        {
            tab_wiget_search_bar->clearSearch();
            tab_wiget_search_bar->hide();
        }
        tab_wiget_search_button->show();
    }
    else if (current_index == m_perf_counter_view_tab_index)
    {
        tab_wiget_search_bar = current_tab->findChild<SearchBar *>(kPerfCounterSearchBarName);
        tab_wiget_search_button = current_tab->findChild<QPushButton *>(
        kPerfCounterSearchButtonName);

        if (!tab_wiget_search_bar->isHidden())
        {
            tab_wiget_search_bar->clearSearch();
            tab_wiget_search_bar->hide();
        }
        tab_wiget_search_button->show();
    }
}

//--------------------------------------------------------------------------------------------------
void MainWindow::LoadAvailableMetrics()
{
    std::optional<std::filesystem::path> metrics_description_file_path = std::nullopt;
    if (auto profile_plugin_folder = ResolveAssetPath(Dive::kProfilingPluginFolderName))
    {
        auto file_path = *profile_plugin_folder / kMetricsFileName;
        if (std::filesystem::exists(file_path))
        {
            metrics_description_file_path = file_path;
        }
    }

    std::optional<QTemporaryDir> temp_dir;
    if (!metrics_description_file_path)
    {
        QFile input_file(QString::fromStdString(kMetricsFilePath));
        if (!input_file.open(QIODevice::ReadOnly))
        {
            std::cerr << "Failed to open resource file: " << kMetricsFilePath << std::endl;
            return;
        }

        QByteArray file_contents = input_file.readAll();
        input_file.close();

        temp_dir.emplace();
        if (!temp_dir->isValid())
        {
            std::cerr << "Failed to create temporary directory." << std::endl;
            return;
        }

        // Get the temporary file path as a QString
        QString temp_file_path = QDir(temp_dir->path()).filePath(kMetricsFileName);

        QFile temp_file(temp_file_path);
        if (!temp_file.open(QIODevice::WriteOnly))
        {
            std::cerr << "Failed to create temporary file: " << temp_file_path.toStdString()
                      << std::endl;
            return;
        }

        temp_file.write(file_contents);
        temp_file.close();

        metrics_description_file_path = std::filesystem::path(temp_file_path.toStdString());
    }

    if (!metrics_description_file_path)
    {
        return;
    }

    m_available_metrics = Dive::AvailableMetrics::LoadFromCsv(*metrics_description_file_path);
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

    // Analyze action
    m_analyze_action = new QAction(tr("Analyze Capture"), this);
    m_analyze_action->setStatusTip(tr("Analyze a Capture"));
    m_analyze_action->setIcon(QIcon(":/images/analyze.png"));
    m_analyze_action->setShortcut(QKeySequence("f7"));
    connect(m_analyze_action, &QAction::triggered, this, &MainWindow::OnAnalyzeCapture);

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

    m_analyze_menu = menuBar()->addMenu(tr("&Analyze"));
    m_analyze_menu->addAction(m_analyze_action);

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
    m_capture_button->setPopupMode(QToolButton::InstantPopup);
    m_capture_button->setMenu(m_capture_menu);
    m_capture_button->setIcon(QIcon(":/images/capture.png"));

    m_file_tool_bar->addWidget(m_capture_button);

    m_file_tool_bar->addSeparator();

    QToolButton *m_analyze_button = new QToolButton();
    m_analyze_button->setIcon(QIcon(":/images/analyze.png"));
    m_analyze_button->setPopupMode(QToolButton::DelayedPopup);
    connect(m_analyze_button, &QToolButton::clicked, m_analyze_action, &QAction::trigger);

    m_file_tool_bar->addWidget(m_analyze_button);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::CreateShortcuts()
{
    // Search Shortcut
    m_search_shortcut = new QShortcut(QKeySequence(SHORTCUT_EVENTS_SEARCH), this);
    connect(m_search_shortcut, &QShortcut::activated, this, &MainWindow::OnSearchTrigger);

    // TabView Search Shortcut
    m_search_tab_view_shortcut = new QShortcut(QKeySequence(SHORTCUT_TAB_VIEW_SEARCH), this);
    connect(m_search_tab_view_shortcut, &QShortcut::activated, [this]() {
        int current_tab_index = m_tab_widget->currentIndex();
        if (current_tab_index == m_command_view_tab_index)
        {
            m_command_tab_view->OnSearchCommandBuffer();
        }
        else if (current_tab_index == m_gfxr_vulkan_command_arguments_view_tab_index)
        {
            m_gfxr_vulkan_command_arguments_tab_view->OnSearchCommandArgs();
        }
        else if (current_tab_index == m_perf_counter_view_tab_index)
        {
            m_perf_counter_tab_view->OnSearchCounters();
        }
        else
        {
            m_tab_widget->setCurrentIndex(m_command_view_tab_index);
            m_command_tab_view->OnSearchCommandBuffer();
        }
        ResetEventSearchBar();
        if (m_correlated_capture_loaded)
        {
            ResetPm4EventSearchBar();
        }
    });

    // Overview Shortcut
    m_overview_tab_shortcut = new QShortcut(QKeySequence(SHORTCUT_OVERVIEW_TAB), this);
    connect(m_overview_tab_shortcut, &QShortcut::activated, [this]() {
        m_tab_widget->setCurrentIndex(m_overview_view_tab_index);
    });
    // Commands Shortcut
    m_command_tab_shortcut = new QShortcut(QKeySequence(SHORTCUT_COMMANDS_TAB), this);
    connect(m_command_tab_shortcut, &QShortcut::activated, [this]() {
        m_tab_widget->setCurrentIndex(m_command_view_tab_index);
    });
    // Shaders Shortcut
    m_shader_tab_shortcut = new QShortcut(QKeySequence(SHORTCUT_SHADERS_TAB), this);
    connect(m_shader_tab_shortcut, &QShortcut::activated, [this]() {
        m_tab_widget->setCurrentIndex(m_shader_view_tab_index);
    });
    // Event State Shortcut
    m_event_state_tab_shortcut = new QShortcut(QKeySequence(SHORTCUT_EVENT_STATE_TAB), this);
    connect(m_event_state_tab_shortcut, &QShortcut::activated, [this]() {
        m_tab_widget->setCurrentIndex(m_event_state_view_tab_index);
    });
    // Gfxr Vulkan Command Arguments Shortcut
    m_gfxr_vulkan_command_arguments_tab_shortcut =
    new QShortcut(QKeySequence(SHORTCUT_GFXR_VULKAN_COMMAND_ARGUMENTS_TAB), this);
    connect(m_gfxr_vulkan_command_arguments_tab_shortcut, &QShortcut::activated, [this]() {
        if (m_gfxr_vulkan_command_arguments_view_tab_index != -1)
        {
            m_tab_widget->setCurrentIndex(m_gfxr_vulkan_command_arguments_view_tab_index);
        }
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
void MainWindow::OnHideOverlay()
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
void MainWindow::ExpandResizeHierarchyView(DiveTreeView                &tree_view,
                                           const QSortFilterProxyModel &model)
{
    tree_view.expandAll();
    // Set to -1 so that resizeColumnToContents() will consider *all* rows to determine amount
    // to resize. This can potentially be slow!
    // Then resize each column. This will also auto-adjust horizontal scroll bar size.
    tree_view.header()->setResizeContentsPrecision(-1);
    uint32_t column_count = (uint32_t)model.columnCount(QModelIndex());
    for (uint32_t column = 0; column < column_count; ++column)
        tree_view.resizeColumnToContents(column);
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
    bool has_text = m_data_core->GetPm4CaptureData().GetNumText() > 0;
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
void MainWindow::OnSwitchToShaderTab()
{
    DIVE_ASSERT(m_shader_view_tab_index >= 0);
    m_tab_widget->setCurrentIndex(m_shader_view_tab_index);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnTabViewSearchBarVisibilityChange(bool isHidden)
{
    if (isHidden)
    {
        ResetEventSearchBar();
        if (m_correlated_capture_loaded)
        {
            ResetPm4EventSearchBar();
        }
    }
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnTabViewChange()
{
    int current_index = m_tab_widget->currentIndex();

    // If no current index is selected, return.
    if (current_index == -1)
    {
        return;
    }

    // Check if there was a previous tab search that needs to be disabled.
    if (m_previous_tab_index != -1 && m_previous_tab_index != current_index)
    {
        QWidget *previous_tab = m_tab_widget->widget(m_previous_tab_index);
        if (previous_tab)
        {
            if (m_previous_tab_index == m_command_view_tab_index)
            {
                m_command_tab_view->OnSearchBarVisibilityChange(true);
            }
            else if (m_previous_tab_index == m_gfxr_vulkan_command_arguments_view_tab_index)
            {
                m_gfxr_vulkan_command_arguments_tab_view->OnSearchBarVisibilityChange(true);
            }
            else if (m_previous_tab_index == m_perf_counter_view_tab_index)
            {
                m_perf_counter_tab_view->OnSearchBarVisibilityChange(true);
            }
        }
    }

    QWidget *current_tab = m_tab_widget->widget(current_index);
    if (current_index == m_command_view_tab_index &&
        !current_tab->findChild<SearchBar *>(kCommandBufferSearchBarName)->isHidden())
    {
        ResetEventSearchBar();
        if (m_correlated_capture_loaded)
        {
            ResetPm4EventSearchBar();
        }
    }
    else if (current_index == m_gfxr_vulkan_command_arguments_view_tab_index &&
             current_tab->findChild<SearchBar *>(kGfxrVulkanCommandArgumentsSearchBarName))
    {
        if (!current_tab->findChild<SearchBar *>(kGfxrVulkanCommandArgumentsSearchBarName)
             ->isHidden())
        {
            ResetEventSearchBar();
            if (m_correlated_capture_loaded)
            {
                ResetPm4EventSearchBar();
            }
        }
    }
    else if (current_index == m_perf_counter_view_tab_index &&
             current_tab->findChild<SearchBar *>(kPerfCounterSearchBarName))
    {
        if (!current_tab->findChild<SearchBar *>(kPerfCounterSearchBarName)->isHidden())
        {
            ResetEventSearchBar();
            if (m_correlated_capture_loaded)
            {
                ResetPm4EventSearchBar();
            }
        }
    }
    else
    {
        m_command_tab_view->OnSearchBarVisibilityChange(true);
    }

    m_previous_tab_index = current_index;
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

//--------------------------------------------------------------------------------------------------
void MainWindow::ConnectPm4SearchBar()
{
    QObject::connect(m_pm4_event_search_bar,
                     SIGNAL(new_search(const QString &)),
                     m_pm4_command_hierarchy_view,
                     SLOT(searchNodeByText(const QString &)));
    QObject::connect(m_pm4_event_search_bar,
                     &SearchBar::next_search,
                     m_pm4_command_hierarchy_view,
                     &DiveTreeView::nextNodeInSearch);
    QObject::connect(m_pm4_event_search_bar,
                     &SearchBar::prev_search,
                     m_pm4_command_hierarchy_view,
                     &DiveTreeView::prevNodeInSearch);
    QObject::connect(m_pm4_command_hierarchy_view,
                     &DiveTreeView::updateSearch,
                     m_pm4_event_search_bar,
                     &SearchBar::updateSearchResults);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::DisconnectPm4SearchBar()
{
    QObject::disconnect(m_pm4_event_search_bar,
                        SIGNAL(new_search(const QString &)),
                        m_pm4_command_hierarchy_view,
                        SLOT(searchNodeByText(const QString &)));
    QObject::disconnect(m_pm4_event_search_bar,
                        &SearchBar::next_search,
                        m_pm4_command_hierarchy_view,
                        &DiveTreeView::nextNodeInSearch);
    QObject::disconnect(m_pm4_event_search_bar,
                        &SearchBar::prev_search,
                        m_pm4_command_hierarchy_view,
                        &DiveTreeView::prevNodeInSearch);
    QObject::disconnect(m_pm4_command_hierarchy_view,
                        &DiveTreeView::updateSearch,
                        m_pm4_event_search_bar,
                        &SearchBar::updateSearchResults);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::DisconnectAllTabs()
{
    // Get the current selection models before they potentially change.
    QItemSelectionModel *current_selection_model = m_command_hierarchy_view->selectionModel();
    QItemSelectionModel *current_pm4_selection_model = m_pm4_command_hierarchy_view
                                                       ->selectionModel();

    // Disconnect ALL signals from the selection models to all slots.
    if (current_selection_model)
    {
        QObject::disconnect(current_selection_model, nullptr, nullptr, nullptr);
    }

    if (current_pm4_selection_model)
    {
        QObject::disconnect(current_pm4_selection_model, nullptr, nullptr, nullptr);
    }

    foreach (auto expand_to_lvl_button, m_expand_to_lvl_buttons)
    {
        QObject::disconnect(expand_to_lvl_button, SIGNAL(clicked()), this, SLOT(OnExpandToLevel()));
    }

    QObject::disconnect(m_view_mode_combo_box,
                        SIGNAL(currentTextChanged(const QString &)),
                        this,
                        SLOT(OnCommandViewModeChange(const QString &)));
    QObject::disconnect(m_view_mode_combo_box,
                        SIGNAL(textHighlighted(const QString &)),
                        this,
                        SLOT(OnCommandViewModeComboBoxHover(const QString &)));

    QObject::disconnect(m_command_hierarchy_view,
                        &DiveTreeView::expanded,
                        m_command_hierarchy_view,
                        &DiveTreeView::expandNode);
    QObject::disconnect(m_command_hierarchy_view,
                        &DiveTreeView::collapsed,
                        m_command_hierarchy_view,
                        &DiveTreeView::collapseNode);
    QObject::disconnect(m_prev_event_button,
                        &QPushButton::clicked,
                        m_command_hierarchy_view,
                        &DiveTreeView::gotoPrevEvent);
    QObject::disconnect(m_next_event_button,
                        &QPushButton::clicked,
                        m_command_hierarchy_view,
                        &DiveTreeView::gotoNextEvent);

    QObject::disconnect(m_property_panel,
                        &PropertyPanel::crossReference,
                        this,
                        &MainWindow::OnCrossReference);
    QObject::disconnect(m_event_selection,
                        &EventSelection::crossReference,
                        this,
                        &MainWindow::OnCrossReference);
    QObject::disconnect(m_event_selection,
                        &EventSelection::currentNodeChanged,
                        m_command_hierarchy_view,
                        &DiveTreeView::setCurrentNode);

    QObject::disconnect(m_command_hierarchy_view,
                        SIGNAL(sourceCurrentChanged(const QModelIndex &, const QModelIndex &)),
                        m_command_tab_view,
                        SLOT(OnSelectionChanged(const QModelIndex &)));

    QObject::disconnect(m_command_hierarchy_view->selectionModel(),
                        SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
                        this,
                        SLOT(OnSelectionChanged(const QModelIndex &)));

    QObject::disconnect(m_command_hierarchy_view,
                        &QTreeView::customContextMenuRequested,
                        this,
                        &MainWindow::OnOpenVulkanDrawCallMenu);

    foreach (auto expand_to_lvl_button, m_pm4_expand_to_lvl_buttons)
    {
        QObject::disconnect(expand_to_lvl_button, SIGNAL(clicked()), this, SLOT(OnExpandToLevel()));
    }

    QObject::disconnect(m_pm4_view_mode_combo_box,
                        SIGNAL(currentTextChanged(const QString &)),
                        this,
                        SLOT(OnCommandViewModeChange(const QString &)));
    QObject::disconnect(m_pm4_view_mode_combo_box,
                        SIGNAL(textHighlighted(const QString &)),
                        this,
                        SLOT(OnCommandViewModeComboBoxHover(const QString &)));

    QObject::disconnect(m_pm4_filter_mode_combo_box,
                        SIGNAL(currentTextChanged(const QString &)),
                        this,
                        SLOT(OnFilterModeChange(const QString &)));

    QObject::disconnect(m_pm4_search_trigger_button,
                        SIGNAL(clicked()),
                        this,
                        SLOT(OnSearchTrigger()));

    QObject::disconnect(m_pm4_command_hierarchy_view,
                        &DiveTreeView::expanded,
                        m_pm4_command_hierarchy_view,
                        &DiveTreeView::expandNode);
    QObject::disconnect(m_pm4_command_hierarchy_view,
                        &DiveTreeView::collapsed,
                        m_pm4_command_hierarchy_view,
                        &DiveTreeView::collapseNode);
    QObject::disconnect(m_event_selection,
                        &EventSelection::currentNodeChanged,
                        m_pm4_command_hierarchy_view,
                        &DiveTreeView::setCurrentNode);
    QObject::disconnect(m_pm4_prev_event_button,
                        &QPushButton::clicked,
                        m_pm4_command_hierarchy_view,
                        &DiveTreeView::gotoPrevEvent);
    QObject::disconnect(m_pm4_next_event_button,
                        &QPushButton::clicked,
                        m_pm4_command_hierarchy_view,
                        &DiveTreeView::gotoNextEvent);

    QObject::disconnect(m_pm4_command_hierarchy_view,
                        SIGNAL(sourceCurrentChanged(const QModelIndex &, const QModelIndex &)),
                        m_command_tab_view,
                        SLOT(OnSelectionChanged(const QModelIndex &)));

    QObject::disconnect(m_pm4_command_hierarchy_view->selectionModel(),
                        SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
                        m_command_tab_view,
                        SLOT(OnSelectionChanged(const QModelIndex &)));

    QObject::disconnect(m_pm4_command_hierarchy_view->selectionModel(),
                        SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
                        this,
                        SLOT(OnSelectionChanged(const QModelIndex &)));

    QObject::disconnect(m_pm4_command_hierarchy_view->selectionModel(),
                        SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
                        this,
                        SLOT(OnCorrelatePm4DrawCall(const QModelIndex &)));

    QObject::disconnect(m_command_tab_view,
                        SIGNAL(HideOtherSearchBars()),
                        this,
                        SLOT(OnTabViewChange()));

    QObject::disconnect(m_command_tab_view,
                        SIGNAL(SendNodeProperty(const QString &)),
                        m_property_panel,
                        SLOT(OnSelectionInfoChange(const QString &)));

    QObject::disconnect(this,
                        SIGNAL(EventSelected(uint64_t)),
                        m_shader_view,
                        SLOT(OnEventSelected(uint64_t)));

    QObject::disconnect(this,
                        SIGNAL(EventSelected(uint64_t)),
                        m_event_state_view,
                        SLOT(OnEventSelected(uint64_t)));
#if defined(ENABLE_CAPTURE_BUFFERS)
    QObject::disconnect(this,
                        SIGNAL(EventSelected(uint64_t)),
                        m_buffer_view,
                        SLOT(OnEventSelected(uint64_t)));
#endif

    QObject::disconnect(m_command_hierarchy_view->selectionModel(),
                        SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
                        m_gfxr_vulkan_command_arguments_tab_view,
                        SLOT(OnSelectionChanged(const QModelIndex &)));

    QObject::disconnect(m_command_hierarchy_view->selectionModel(),
                        SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
                        this,
                        SLOT(OnCorrelateVulkanDrawCall(const QModelIndex &)));

    QObject::disconnect(m_gfxr_vulkan_command_arguments_tab_view,
                        SIGNAL(HideOtherSearchBars()),
                        this,
                        SLOT(OnTabViewChange()));

    QObject::disconnect(m_perf_counter_tab_view,
                        SIGNAL(HideOtherSearchBars()),
                        this,
                        SLOT(OnTabViewChange()));

    QObject::disconnect(m_perf_counter_tab_view,
                        &PerfCounterTabView::CounterSelected,
                        this,
                        &MainWindow::OnCounterSelected);

    QObject::disconnect(m_gpu_timing_tab_view,
                        &GpuTimingTabView::GpuTimingDataSelected,
                        this,
                        &MainWindow::OnGpuTimingDataSelected);

    QObject::disconnect(m_filter_mode_combo_box,
                        SIGNAL(currentTextChanged(const QString &)),
                        this,
                        SLOT(OnFilterModeChange(const QString &)));

    QObject::disconnect(m_filter_gfxr_commands_combo_box,
                        SIGNAL(FilterChanged()),
                        this,
                        SLOT(OnGfxrFilterModeChange()));

    QObject::disconnect(m_analyze_dig,
                        &AnalyzeDialog::OnDisplayPerfCounterResults,
                        this,
                        &MainWindow::OnPendingPerfCounterResults);

    QObject::disconnect(m_analyze_dig,
                        &AnalyzeDialog::OnDisplayGpuTimingResults,
                        this,
                        &MainWindow::OnPendingGpuTimingResults);

    // Temporarily set the model to nullptr and clear selection/current index
    // before loading new data. This forces a clean break.
    m_command_hierarchy_view->setModel(nullptr);
    m_pm4_command_hierarchy_view->setModel(nullptr);

    if (current_selection_model)
    {
        current_selection_model->clearSelection();
    }

    if (current_pm4_selection_model)
    {
        current_pm4_selection_model->clearSelection();
    }
}

//--------------------------------------------------------------------------------------------------
void MainWindow::ConnectDiveFileTabs()
{
    // Left Panel
    foreach (auto expand_to_lvl_button, m_expand_to_lvl_buttons)
    {
        QObject::connect(expand_to_lvl_button, SIGNAL(clicked()), this, SLOT(OnExpandToLevel()));
    }

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

    // Buttons
    QObject::connect(m_prev_event_button,
                     &QPushButton::clicked,
                     m_command_hierarchy_view,
                     &DiveTreeView::gotoPrevEvent);
    QObject::connect(m_next_event_button,
                     &QPushButton::clicked,
                     m_command_hierarchy_view,
                     &DiveTreeView::gotoNextEvent);

    // Tabs
    QObject::connect(m_command_hierarchy_view,
                     SIGNAL(sourceCurrentChanged(const QModelIndex &, const QModelIndex &)),
                     m_gfxr_vulkan_command_arguments_tab_view,
                     SLOT(OnSelectionChanged(const QModelIndex &)));

    QObject::connect(m_command_hierarchy_view->selectionModel(),
                     SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
                     m_gfxr_vulkan_command_arguments_tab_view,
                     SLOT(OnSelectionChanged(const QModelIndex &)));

    // Correlate between two calls
    QObject::connect(m_command_hierarchy_view->selectionModel(),
                     SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
                     this,
                     SLOT(OnCorrelateVulkanDrawCall(const QModelIndex &)));

    QObject::connect(m_command_hierarchy_view,
                     &QTreeView::customContextMenuRequested,
                     this,
                     &MainWindow::OnOpenVulkanDrawCallMenu);

    QObject::connect(m_property_panel,
                     &PropertyPanel::crossReference,
                     this,
                     &MainWindow::OnCrossReference);
    QObject::connect(m_event_selection,
                     &EventSelection::crossReference,
                     this,
                     &MainWindow::OnCrossReference);

    // Middle Panel
    foreach (auto expand_to_lvl_button, m_pm4_expand_to_lvl_buttons)
    {
        QObject::connect(expand_to_lvl_button, SIGNAL(clicked()), this, SLOT(OnExpandToLevel()));
    }

    QObject::connect(m_pm4_command_hierarchy_view->selectionModel(),
                     SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
                     this,
                     SLOT(OnSelectionChanged(const QModelIndex &)));

    QObject::connect(m_pm4_command_hierarchy_view,
                     &DiveTreeView::expanded,
                     m_pm4_command_hierarchy_view,
                     &DiveTreeView::expandNode);
    QObject::connect(m_pm4_command_hierarchy_view,
                     &DiveTreeView::collapsed,
                     m_pm4_command_hierarchy_view,
                     &DiveTreeView::collapseNode);
    QObject::connect(m_event_selection,
                     &EventSelection::currentNodeChanged,
                     m_pm4_command_hierarchy_view,
                     &DiveTreeView::setCurrentNode);

    // Combo Boxes
    QObject::connect(m_pm4_view_mode_combo_box,
                     SIGNAL(currentTextChanged(const QString &)),
                     this,
                     SLOT(OnCommandViewModeChange(const QString &)));

    QObject::connect(m_pm4_view_mode_combo_box,
                     SIGNAL(textHighlighted(const QString &)),
                     this,
                     SLOT(OnCommandViewModeComboBoxHover(const QString &)));

    QObject::connect(m_pm4_filter_mode_combo_box,
                     SIGNAL(currentTextChanged(const QString &)),
                     this,
                     SLOT(OnFilterModeChange(const QString &)));

    QObject::connect(m_filter_gfxr_commands_combo_box,
                     SIGNAL(FilterChanged()),
                     this,
                     SLOT(OnGfxrFilterModeChange()));

    // Buttons
    QObject::connect(m_pm4_search_trigger_button, SIGNAL(clicked()), this, SLOT(OnSearchTrigger()));

    QObject::connect(m_pm4_prev_event_button,
                     &QPushButton::clicked,
                     m_pm4_command_hierarchy_view,
                     &DiveTreeView::gotoPrevEvent);

    QObject::connect(m_pm4_next_event_button,
                     &QPushButton::clicked,
                     m_pm4_command_hierarchy_view,
                     &DiveTreeView::gotoNextEvent);

    // Tabs
    QObject::connect(m_pm4_command_hierarchy_view,
                     SIGNAL(sourceCurrentChanged(const QModelIndex &, const QModelIndex &)),
                     m_command_tab_view,
                     SLOT(OnSelectionChanged(const QModelIndex &)));

    QObject::connect(m_pm4_command_hierarchy_view->selectionModel(),
                     SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
                     m_command_tab_view,
                     SLOT(OnSelectionChanged(const QModelIndex &)));

    QObject::connect(m_command_tab_view,
                     SIGNAL(HideOtherSearchBars()),
                     this,
                     SLOT(OnTabViewChange()));

    QObject::connect(m_perf_counter_tab_view,
                     SIGNAL(HideOtherSearchBars()),
                     this,
                     SLOT(OnTabViewChange()));

    QObject::connect(m_command_tab_view,
                     SIGNAL(SendNodeProperty(const QString &)),
                     m_property_panel,
                     SLOT(OnSelectionInfoChange(const QString &)));

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

    QObject::connect(m_gfxr_vulkan_command_arguments_tab_view,
                     SIGNAL(HideOtherSearchBars()),
                     this,
                     SLOT(OnTabViewChange()));

    QObject::connect(m_perf_counter_tab_view,
                     &PerfCounterTabView::CounterSelected,
                     this,
                     &MainWindow::OnCounterSelected);

    QObject::connect(m_gpu_timing_tab_view,
                     &GpuTimingTabView::GpuTimingDataSelected,
                     this,
                     &MainWindow::OnGpuTimingDataSelected);

    // Correlate between two calls
    QObject::connect(m_pm4_command_hierarchy_view->selectionModel(),
                     SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
                     this,
                     SLOT(OnCorrelatePm4DrawCall(const QModelIndex &)));

    // Dialogs
    QObject::connect(m_analyze_dig,
                     &AnalyzeDialog::OnDisplayPerfCounterResults,
                     this,
                     &MainWindow::OnPendingPerfCounterResults);

    QObject::connect(m_analyze_dig,
                     &AnalyzeDialog::OnDisplayGpuTimingResults,
                     this,
                     &MainWindow::OnPendingGpuTimingResults);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::ConnectAdrenoRdFileTabs()
{
    foreach (auto expand_to_lvl_button, m_expand_to_lvl_buttons)
    {
        QObject::connect(expand_to_lvl_button, SIGNAL(clicked()), this, SLOT(OnExpandToLevel()));
    }

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

    // Combo Boxes
    QObject::connect(m_view_mode_combo_box,
                     SIGNAL(currentTextChanged(const QString &)),
                     this,
                     SLOT(OnCommandViewModeChange(const QString &)));
    QObject::connect(m_view_mode_combo_box,
                     SIGNAL(textHighlighted(const QString &)),
                     this,
                     SLOT(OnCommandViewModeComboBoxHover(const QString &)));
    QObject::connect(m_filter_mode_combo_box,
                     SIGNAL(currentTextChanged(const QString &)),
                     this,
                     SLOT(OnFilterModeChange(const QString &)));

    // Tabs
    QObject::connect(m_command_hierarchy_view,
                     SIGNAL(sourceCurrentChanged(const QModelIndex &, const QModelIndex &)),
                     m_command_tab_view,
                     SLOT(OnSelectionChanged(const QModelIndex &)));

    QObject::connect(m_command_hierarchy_view->selectionModel(),
                     SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
                     this,
                     SLOT(OnSelectionChanged(const QModelIndex &)));

    QObject::connect(m_command_tab_view,
                     SIGNAL(HideOtherSearchBars()),
                     this,
                     SLOT(OnTabViewChange()));

    QObject::connect(m_command_tab_view,
                     SIGNAL(SendNodeProperty(const QString &)),
                     m_property_panel,
                     SLOT(OnSelectionInfoChange(const QString &)));

    QObject::connect(this,
                     SIGNAL(EventSelected(uint64_t)),
                     m_shader_view,
                     SLOT(OnEventSelected(uint64_t)));

    QObject::connect(this,
                     SIGNAL(EventSelected(uint64_t)),
                     m_event_state_view,
                     SLOT(OnEventSelected(uint64_t)));

    // Buttons
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

#if defined(ENABLE_CAPTURE_BUFFERS)
    QObject::connect(this,
                     SIGNAL(EventSelected(uint64_t)),
                     m_buffer_view,
                     SLOT(OnEventSelected(uint64_t)));
#endif

    // Dialogs
    QObject::connect(m_analyze_dig,
                     &AnalyzeDialog::OnDisplayPerfCounterResults,
                     this,
                     &MainWindow::OnPendingPerfCounterResults);

    QObject::connect(m_analyze_dig,
                     &AnalyzeDialog::OnDisplayGpuTimingResults,
                     this,
                     &MainWindow::OnPendingGpuTimingResults);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::ConnectGfxrFileTabs()
{
    QObject::connect(m_command_hierarchy_view,
                     SIGNAL(sourceCurrentChanged(const QModelIndex &, const QModelIndex &)),
                     m_gfxr_vulkan_command_arguments_tab_view,
                     SLOT(OnSelectionChanged(const QModelIndex &)));

    QObject::connect(m_command_hierarchy_view->selectionModel(),
                     SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
                     m_gfxr_vulkan_command_arguments_tab_view,
                     SLOT(OnSelectionChanged(const QModelIndex &)));

    QObject::connect(m_command_hierarchy_view->selectionModel(),
                     SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
                     this,
                     SLOT(OnSelectionChanged(const QModelIndex &)));

    // Tabs
    QObject::connect(m_gfxr_vulkan_command_arguments_tab_view,
                     SIGNAL(HideOtherSearchBars()),
                     this,
                     SLOT(OnTabViewChange()));

    QObject::connect(m_perf_counter_tab_view,
                     SIGNAL(HideOtherSearchBars()),
                     this,
                     SLOT(OnTabViewChange()));

    QObject::connect(m_perf_counter_tab_view,
                     &PerfCounterTabView::CounterSelected,
                     this,
                     &MainWindow::OnCounterSelected);

    QObject::connect(m_gpu_timing_tab_view,
                     &GpuTimingTabView::GpuTimingDataSelected,
                     this,
                     &MainWindow::OnGpuTimingDataSelected);

    // Combo Boxes
    QObject::connect(m_filter_gfxr_commands_combo_box,
                     SIGNAL(FilterChanged()),
                     this,
                     SLOT(OnGfxrFilterModeChange()));
    // Buttons
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

    // Dialogs
    QObject::connect(m_analyze_dig,
                     &AnalyzeDialog::OnDisplayPerfCounterResults,
                     this,
                     &MainWindow::OnPendingPerfCounterResults);

    QObject::connect(m_analyze_dig,
                     &AnalyzeDialog::OnDisplayGpuTimingResults,
                     this,
                     &MainWindow::OnPendingGpuTimingResults);

    // Correlate calls
    QObject::connect(m_command_hierarchy_view->selectionModel(),
                     SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
                     this,
                     SLOT(OnCorrelateVulkanDrawCall(const QModelIndex &)));

    // Menus
    QObject::connect(m_command_hierarchy_view,
                     &QTreeView::customContextMenuRequested,
                     this,
                     &MainWindow::OnOpenVulkanDrawCallMenu);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnOpenFileFromAnalyzeDialog(const QString &file_path)
{
    const std::string file_path_std_str = file_path.toStdString();
    const char       *file_path_str = file_path_std_str.c_str();
    if (!LoadFile(file_path_str, /*is_temp_file*/ false, /*async*/ true))
    {
        return;
    }
}

//--------------------------------------------------------------------------------------------------
QModelIndex MainWindow::FindSourceIndexFromNode(QAbstractItemModel *model,
                                                uint64_t            target_node_index,
                                                const QModelIndex  &parent)
{
    if (!model)
        return QModelIndex();

    for (int r = 0; r < model->rowCount(parent); ++r)
    {
        QModelIndex index = model->index(r, 0, parent);
        if (index.isValid() && (uint64_t)index.internalPointer() == target_node_index)
        {
            return index;
        }
        if (model->hasChildren(index))
        {
            QModelIndex result = FindSourceIndexFromNode(model, target_node_index, index);
            if (result.isValid())
            {
                return result;
            }
        }
    }
    return QModelIndex();
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnOpenVulkanDrawCallMenu(const QPoint &pos)
{
    m_gfxr_vulkan_commands_filter_proxy_model->CollectGfxrDrawCallIndices();
    QModelIndex vulkan_draw_call_index = m_command_hierarchy_view->indexAt(pos);
    QModelIndex source_index = m_gfxr_vulkan_commands_filter_proxy_model->mapToSource(
    vulkan_draw_call_index);
    uint64_t node_index = (uint64_t)(source_index.internalPointer());

    if ((!source_index.isValid()) || (m_data_core->GetCommandHierarchy().GetNodeType(node_index) !=
                                      Dive::NodeType::kGfxrVulkanDrawCommandNode))
    {
        // Check if requesting a menu for a BeginCommandBuffer or BeginRenderPass node.
        OnOpenVulkanCallMenu(pos);
        return;
    }

    std::vector<uint64_t>
    gfxr_draw_call_indices = qobject_cast<GfxrVulkanCommandFilterProxyModel *>(
                             m_command_hierarchy_view->model())
                             ->GetGfxrDrawCallIndices();
    auto it = std::find(gfxr_draw_call_indices.begin(), gfxr_draw_call_indices.end(), node_index);

    if (it == gfxr_draw_call_indices.end())
    {
        QMessageBox::critical(this, "Correlation Failed", "Corresponding perf counter not found.");
        return;
    }

    uint64_t found_gfxr_draw_call_index = std::distance(gfxr_draw_call_indices.begin(), it);

    QMenu context_menu;
    for (size_t i = 0; i < std::size(Dive::kDrawCallContextMenuOptionStrings) - 1; ++i)
    {
        if (!m_correlated_capture_loaded &&
            (i == Dive::DrawCallContextMenuOption::kBinningPassOnly ||
             i == Dive::DrawCallContextMenuOption::kFirstTilePassOnly))
        {
            continue;
        }

        QAction *action = context_menu.addAction(Dive::kDrawCallContextMenuOptionStrings[i]);
        action->setData(static_cast<int>(i));
    }

    QAction *selected_action = context_menu.exec(
    m_command_hierarchy_view->viewport()->mapToGlobal(pos));

    // Ensures that if the user clicks outside of the context menu, a seg fault does not occur since
    // it is interpreted as a selection.
    if (selected_action == nullptr)
    {
        return;
    }

    QVariant selected_action_data = selected_action->data();
    if (selected_action_data == Dive::kPerfCounterData)
    {
        m_tab_widget->setCurrentIndex(m_perf_counter_view_tab_index);
    }
    else if (selected_action_data == Dive::kArguments)
    {
        m_tab_widget->setCurrentIndex(m_gfxr_vulkan_command_arguments_view_tab_index);
    }
    else
    {
        m_pm4_filter_mode_combo_box->setCurrentIndex(selected_action->data().toInt());

        OnCorrelationFilterApplied(found_gfxr_draw_call_index,
                                   selected_action->data().toInt(),
                                   vulkan_draw_call_index);
    }
}

void MainWindow::OnCorrelationFilterApplied(uint64_t           gfxr_draw_call_index,
                                            int                filter_index,
                                            const QModelIndex &vulkan_draw_call_model_index)
{
    std::vector<uint64_t> pm4_draw_call_indices = qobject_cast<DiveFilterModel *>(
                                                  m_pm4_command_hierarchy_view->model())
                                                  ->GetPm4DrawCallIndices();

    uint64_t corresponding_pm4_draw_call_index = pm4_draw_call_indices.at(gfxr_draw_call_index);

    QAbstractItemModel *source_model = m_filter_model->sourceModel();

    QModelIndex source_index = FindSourceIndexFromNode(source_model,
                                                       corresponding_pm4_draw_call_index);

    if (source_index.isValid())
    {
        QModelIndex proxy_index = m_filter_model->mapFromSource(source_index);

        if (proxy_index.isValid())
        {
            QItemSelectionModel *selection_model = m_pm4_command_hierarchy_view->selectionModel();
            QSignalBlocker       pm4_view_blocker(selection_model);

            QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::ClearAndSelect |
                                                        QItemSelectionModel::Rows;

            selection_model->setCurrentIndex(proxy_index, flags);
            m_command_tab_view->OnSelectionChanged(proxy_index);

            m_pm4_command_hierarchy_view->scrollTo(proxy_index,
                                                   QAbstractItemView::PositionAtCenter);
            m_pm4_command_hierarchy_view->expand(proxy_index);

            m_pm4_command_hierarchy_view->viewport()->update();
            QApplication::processEvents();
        }
    }

    QItemSelectionModel *gfxr_selection_model = m_command_hierarchy_view->selectionModel();
    QSignalBlocker       gfxr_view_blocker(gfxr_selection_model);

    QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::ClearAndSelect |
                                                QItemSelectionModel::Rows;

    gfxr_selection_model->setCurrentIndex(vulkan_draw_call_model_index, flags);

    ResetVerticalScroll(*m_command_hierarchy_view);
    m_command_hierarchy_view->scrollTo(vulkan_draw_call_model_index,
                                       QAbstractItemView::PositionAtCenter);
    m_command_hierarchy_view->expand(vulkan_draw_call_model_index);

    m_command_hierarchy_view->viewport()->update();
    QApplication::processEvents();

    CorrelateCounter(vulkan_draw_call_model_index, true);

    ResetHorizontalScroll(*m_pm4_command_hierarchy_view);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnOpenVulkanCallMenu(const QPoint &pos)
{
    QModelIndex source_index = m_gfxr_vulkan_commands_filter_proxy_model->mapToSource(
    m_command_hierarchy_view->indexAt(pos));
    uint64_t node_index = (uint64_t)(source_index.internalPointer());

    std::string    node_desc = m_data_core->GetCommandHierarchy().GetNodeDesc(node_index);
    Dive::NodeType node_type = m_data_core->GetCommandHierarchy().GetNodeType(node_index);

    // Only the BeginCommandBuffer and BeginRenderPass calls are used for correlation
    if ((!source_index.isValid()) ||
        ((node_type != Dive::NodeType::kGfxrVulkanBeginCommandBufferNode) &&
         (node_type != Dive::NodeType::kGfxrVulkanBeginRenderPassCommandNode) &&
         (node_type != Dive::NodeType::kGfxrRootFrameNode)))
    {
        return;
    }

    QMenu    context_menu;
    QAction *arguments_action = context_menu.addAction(
    Dive::kDrawCallContextMenuOptionStrings[Dive::kArguments]);
    arguments_action->setData(Dive::kArguments);
    QAction *gpu_time_action = context_menu.addAction(
    Dive::kDrawCallContextMenuOptionStrings[Dive::kGpuTimeData]);
    gpu_time_action->setData(Dive::kGpuTimeData);

    QAction *selected_action = context_menu.exec(
    m_command_hierarchy_view->viewport()->mapToGlobal(pos));

    // Ensures that if the user clicks outside of the context menu, a seg fault does not occur since
    // it is interpreted as a selection.
    if (selected_action == nullptr)
    {
        return;
    }

    QVariant selected_action_data = selected_action->data();
    if (selected_action_data == Dive::kGpuTimeData)
    {
        m_tab_widget->setCurrentIndex(m_gpu_timing_view_tab_index);
    }
    else if (selected_action_data == Dive::kArguments)
    {
        m_tab_widget->setCurrentIndex(m_gfxr_vulkan_command_arguments_view_tab_index);
    }
}

//--------------------------------------------------------------------------------------------------
void MainWindow::ClearViewModelSelection(DiveTreeView &tree_view, bool should_clear_tab)
{
    QItemSelectionModel *selection_model = tree_view.selectionModel();
    if (selection_model)
    {
        QSignalBlocker blocker(selection_model);
        selection_model->clear();
        QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::ClearAndSelect |
                                                    QItemSelectionModel::Rows;
        selection_model->setCurrentIndex(QModelIndex(), flags);
    }

    if (should_clear_tab && (m_correlated_capture_loaded || m_gfxr_capture_loaded))
    {
        m_perf_counter_tab_view->ClearSelection();
    }

    tree_view.viewport()->update();
    QApplication::processEvents();
}

//--------------------------------------------------------------------------------------------------
std::optional<uint64_t> MainWindow::GetDrawCallIndexFromProxyIndex(
const QModelIndex           &proxy_index,
const QAbstractProxyModel   &proxy_model,
const std::vector<uint64_t> &draw_call_indices,
CorrelationTarget            target)
{
    QModelIndex source_index = proxy_model.mapToSource(proxy_index);
    if (!source_index.isValid())
    {
        return std::nullopt;
    }

    uint64_t       node_index = (uint64_t)(source_index.internalPointer());
    Dive::NodeType node_type = m_data_core->GetCommandHierarchy().GetNodeType(node_index);
    bool           type_is_valid = false;

    if (target == CorrelationTarget::kGfxrDrawCall)
    {
        type_is_valid = (node_type == Dive::NodeType::kGfxrVulkanDrawCommandNode);
    }
    else if (target == CorrelationTarget::kPm4DrawCall)
    {
        type_is_valid = Dive::IsDrawDispatchNode(node_type);
    }

    if (!type_is_valid)
    {
        return std::nullopt;
    }

    auto it = std::find(draw_call_indices.begin(), draw_call_indices.end(), node_index);

    if (it == draw_call_indices.end())
    {
        return std::nullopt;
    }

    return std::distance(draw_call_indices.begin(), it);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnCorrelateVulkanDrawCall(const QModelIndex &index)
{
    m_gpu_timing_tab_view->ClearSelection();
    m_perf_counter_tab_view->ClearSelection();
    QModelIndex source_index = m_gfxr_vulkan_commands_filter_proxy_model->mapToSource(index);

    // Check if the selected node is a GPU timing node. If so, do not correlate with the PM4 view
    // and performance counters. Only correlate with GPU timing view.
    uint64_t       source_node_index = (uint64_t)source_index.internalPointer();
    Dive::NodeType node_type = m_data_core->GetCommandHierarchy().GetNodeType(source_node_index);
    bool           is_gpu_timing_node = (node_type == Dive::NodeType::kGfxrRootFrameNode) ||
                              (node_type ==
                               Dive::NodeType::kGfxrVulkanBeginRenderPassCommandNode) ||
                              (node_type == Dive::NodeType::kGfxrVulkanBeginCommandBufferNode);

    if (is_gpu_timing_node)
    {
        ClearViewModelSelection(*m_pm4_command_hierarchy_view, false);
        m_gpu_timing_tab_view->OnEventSelectionChanged(source_index);
        return;
    }

    if (m_correlated_capture_loaded &&
        (m_pm4_filter_mode_combo_box->currentIndex() != Dive::kBinningPassOnly &&
         m_pm4_filter_mode_combo_box->currentIndex() != Dive::kFirstTilePassOnly))
    {
        CorrelateCounter(index, true);
        ClearViewModelSelection(*m_pm4_command_hierarchy_view, false);
        return;
    }
    else if (m_gfxr_capture_loaded)
    {
        CorrelateCounter(index, true);
        return;
    }

    m_gfxr_vulkan_commands_filter_proxy_model->CollectGfxrDrawCallIndices();
    std::vector<uint64_t>
    gfxr_draw_call_indices = qobject_cast<GfxrVulkanCommandFilterProxyModel *>(
                             m_command_hierarchy_view->model())
                             ->GetGfxrDrawCallIndices();

    std::optional<uint64_t> found_gfxr_draw_call_index =
    GetDrawCallIndexFromProxyIndex(index,
                                   *m_gfxr_vulkan_commands_filter_proxy_model,
                                   gfxr_draw_call_indices,
                                   CorrelationTarget::kGfxrDrawCall);

    if (!found_gfxr_draw_call_index.has_value())
    {
        ClearViewModelSelection(*m_pm4_command_hierarchy_view, true);
        m_command_tab_view->ResetModel();
        return;
    }

    std::vector<uint64_t> pm4_draw_call_indices = qobject_cast<DiveFilterModel *>(
                                                  m_pm4_command_hierarchy_view->model())
                                                  ->GetPm4DrawCallIndices();

    uint64_t corresponding_pm4_draw_call_index = pm4_draw_call_indices.at(
    found_gfxr_draw_call_index.value());

    QAbstractItemModel *source_model = m_filter_model->sourceModel();
    QModelIndex
    pm4_draw_call_index_from_source = FindSourceIndexFromNode(source_model,
                                                              corresponding_pm4_draw_call_index);

    if (pm4_draw_call_index_from_source.isValid())
    {
        QModelIndex proxy_index = m_filter_model->mapFromSource(pm4_draw_call_index_from_source);

        if (proxy_index.isValid())
        {
            QItemSelectionModel *selection_model = m_pm4_command_hierarchy_view->selectionModel();
            QSignalBlocker       blocker(selection_model);
            QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::ClearAndSelect |
                                                        QItemSelectionModel::Rows;

            selection_model->setCurrentIndex(proxy_index, flags);
            m_command_tab_view->OnSelectionChanged(proxy_index);

            m_pm4_command_hierarchy_view->scrollTo(proxy_index,
                                                   QAbstractItemView::PositionAtCenter);
            m_pm4_command_hierarchy_view->expand(proxy_index);

            m_pm4_command_hierarchy_view->viewport()->update();
            QApplication::processEvents();

            CorrelateCounter(index, true);
            emit EventSelected(corresponding_pm4_draw_call_index);
        }
    }

    ResetHorizontalScroll(*m_pm4_command_hierarchy_view);
    m_command_tab_view->ResetHorizontalScroll();
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnCorrelatePm4DrawCall(const QModelIndex &index)
{
    m_gpu_timing_tab_view->ClearSelection();
    m_perf_counter_tab_view->ClearSelection();

    QItemSelectionModel *gfxr_selection_model = m_command_hierarchy_view->selectionModel();
    QSignalBlocker       blocker(gfxr_selection_model);

    if (m_pm4_filter_mode_combo_box->currentIndex() != Dive::kBinningPassOnly &&
        m_pm4_filter_mode_combo_box->currentIndex() != Dive::kFirstTilePassOnly)
    {
        ResetHorizontalScroll(*m_pm4_command_hierarchy_view);
        ClearViewModelSelection(*m_command_hierarchy_view, false);
        return;
    }

    m_gfxr_vulkan_commands_filter_proxy_model->CollectGfxrDrawCallIndices();

    std::vector<uint64_t> pm4_draw_call_indices = qobject_cast<DiveFilterModel *>(
                                                  m_pm4_command_hierarchy_view->model())
                                                  ->GetPm4DrawCallIndices();

    std::optional<uint64_t>
    found_pm4_draw_call_index = GetDrawCallIndexFromProxyIndex(index,
                                                               *m_filter_model,
                                                               pm4_draw_call_indices,
                                                               CorrelationTarget::kPm4DrawCall);

    if (!found_pm4_draw_call_index.has_value())
    {
        ClearViewModelSelection(*m_command_hierarchy_view, false);
        return;
    }

    std::vector<uint64_t>
    gfxr_draw_call_indices = qobject_cast<GfxrVulkanCommandFilterProxyModel *>(
                             m_command_hierarchy_view->model())
                             ->GetGfxrDrawCallIndices();

    uint64_t corresponding_gfxr_draw_call_index = gfxr_draw_call_indices.at(
    found_pm4_draw_call_index.value());

    QAbstractItemModel *source_model = m_gfxr_vulkan_commands_filter_proxy_model->sourceModel();
    QModelIndex
    gfxr_draw_call_index_from_source = FindSourceIndexFromNode(source_model,
                                                               corresponding_gfxr_draw_call_index);

    if (gfxr_draw_call_index_from_source.isValid())
    {
        QModelIndex proxy_index = m_gfxr_vulkan_commands_filter_proxy_model->mapFromSource(
        gfxr_draw_call_index_from_source);
        if (proxy_index.isValid())
        {
            QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::ClearAndSelect |
                                                        QItemSelectionModel::Rows;

            gfxr_selection_model->setCurrentIndex(proxy_index, flags);

            ResetVerticalScroll(*m_command_hierarchy_view);
            m_command_hierarchy_view->scrollTo(proxy_index, QAbstractItemView::PositionAtCenter);
            m_command_hierarchy_view->expand(proxy_index);

            m_command_hierarchy_view->viewport()->update();
            QApplication::processEvents();

            CorrelateCounter(index, false);
        }
    }

    ResetHorizontalScroll(*m_pm4_command_hierarchy_view);
    m_command_tab_view->ResetHorizontalScroll();
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnCounterSelected(uint64_t row_index)
{
    m_gpu_timing_tab_view->ClearSelection();
    m_gfxr_vulkan_commands_filter_proxy_model->CollectGfxrDrawCallIndices();
    std::vector<uint64_t>
    gfxr_draw_call_indices = qobject_cast<GfxrVulkanCommandFilterProxyModel *>(
                             m_command_hierarchy_view->model())
                             ->GetGfxrDrawCallIndices();

    if (row_index >= gfxr_draw_call_indices.size())
    {
        QMessageBox::critical(this,
                              "Correlation Failed",
                              "Selected row does not correlate with current loaded capture.");
        ClearViewModelSelection(*m_command_hierarchy_view, false);
        ClearViewModelSelection(*m_pm4_command_hierarchy_view, false);
        return;
    }

    QAbstractItemModel *gfxr_source_model = m_gfxr_vulkan_commands_filter_proxy_model
                                            ->sourceModel();
    QModelIndex gfxr_draw_call_index_from_source = FindSourceIndexFromNode(gfxr_source_model,
                                                                           gfxr_draw_call_indices
                                                                           .at(row_index));
    QModelIndex proxy_index;
    QItemSelectionModel                *selection_model;
    QItemSelectionModel::SelectionFlags flags;
    if (gfxr_draw_call_index_from_source.isValid())
    {
        proxy_index = m_gfxr_vulkan_commands_filter_proxy_model->mapFromSource(
        gfxr_draw_call_index_from_source);
        if (proxy_index.isValid())
        {
            selection_model = m_command_hierarchy_view->selectionModel();
            QSignalBlocker main_view_blocker(selection_model);
            flags = QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows;

            selection_model->setCurrentIndex(proxy_index, flags);

            ResetVerticalScroll(*m_command_hierarchy_view);
            m_command_hierarchy_view->scrollTo(proxy_index, QAbstractItemView::PositionAtCenter);
            m_command_hierarchy_view->expand(proxy_index);

            m_command_hierarchy_view->viewport()->update();
            QApplication::processEvents();
        }
    }

    if (m_pm4_filter_mode_combo_box->isEnabled() &&
        (m_pm4_filter_mode_combo_box->currentIndex() == Dive::kBinningPassOnly ||
         m_pm4_filter_mode_combo_box->currentIndex() == Dive::kFirstTilePassOnly))
    {
        std::vector<uint64_t> pm4_draw_call_indices = qobject_cast<DiveFilterModel *>(
                                                      m_pm4_command_hierarchy_view->model())
                                                      ->GetPm4DrawCallIndices();
        QAbstractItemModel *pm4_source_model = m_filter_model->sourceModel();
        QModelIndex pm4_draw_call_index_from_source = FindSourceIndexFromNode(pm4_source_model,
                                                                              pm4_draw_call_indices
                                                                              .at(row_index));

        if (pm4_draw_call_index_from_source.isValid())
        {
            proxy_index = m_filter_model->mapFromSource(pm4_draw_call_index_from_source);
            if (proxy_index.isValid())
            {
                selection_model = m_pm4_command_hierarchy_view->selectionModel();
                QSignalBlocker main_view_blocker(selection_model);
                flags = QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows;

                selection_model->setCurrentIndex(proxy_index, flags);

                m_pm4_command_hierarchy_view->scrollTo(proxy_index,
                                                       QAbstractItemView::PositionAtCenter);
                m_pm4_command_hierarchy_view->expand(proxy_index);

                m_pm4_command_hierarchy_view->viewport()->update();
                QApplication::processEvents();
            }
        }
    }
    else
    {
        ClearViewModelSelection(*m_pm4_command_hierarchy_view, false);
    }

    ResetHorizontalScroll(*m_pm4_command_hierarchy_view);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::CorrelateCounter(const QModelIndex &index, bool called_from_gfxr_view)
{
    m_perf_counter_tab_view->ClearSelection();

    std::optional<uint64_t> found_draw_call_index = 0;
    bool                    found = false;

    if (!called_from_gfxr_view)
    {
        m_gpu_timing_tab_view->ClearSelection();

        if (m_pm4_filter_mode_combo_box->currentIndex() == Dive::kBinningPassOnly ||
            m_pm4_filter_mode_combo_box->currentIndex() == Dive::kFirstTilePassOnly)
        {
            std::vector<uint64_t> pm4_draw_call_indices = qobject_cast<DiveFilterModel *>(
                                                          m_pm4_command_hierarchy_view->model())
                                                          ->GetPm4DrawCallIndices();

            found_draw_call_index = GetDrawCallIndexFromProxyIndex(index,
                                                                   *m_filter_model,
                                                                   pm4_draw_call_indices,
                                                                   CorrelationTarget::kPm4DrawCall);

            if (found_draw_call_index.has_value())
            {
                found = true;
            }
        }
    }
    else
    {
        m_gfxr_vulkan_commands_filter_proxy_model->CollectGfxrDrawCallIndices();
        std::vector<uint64_t>
        gfxr_draw_call_indices = qobject_cast<GfxrVulkanCommandFilterProxyModel *>(
                                 m_command_hierarchy_view->model())
                                 ->GetGfxrDrawCallIndices();

        // Use helper for GFXR sender
        found_draw_call_index =
        GetDrawCallIndexFromProxyIndex(index,
                                       *m_gfxr_vulkan_commands_filter_proxy_model,
                                       gfxr_draw_call_indices,
                                       CorrelationTarget::kGfxrDrawCall);

        if (found_draw_call_index.has_value())
        {
            found = true;
        }
        else
        {
            uint64_t source_node_index = (uint64_t)m_gfxr_vulkan_commands_filter_proxy_model
                                         ->mapToSource(index)
                                         .internalPointer();
            Dive::NodeType node_type = m_data_core->GetCommandHierarchy().GetNodeType(
            source_node_index);
            bool is_gpu_timing_node = (node_type == Dive::NodeType::kGfxrRootFrameNode) ||
                                      (node_type ==
                                       Dive::NodeType::kGfxrVulkanBeginRenderPassCommandNode) ||
                                      (node_type ==
                                       Dive::NodeType::kGfxrVulkanBeginCommandBufferNode);

            if (is_gpu_timing_node)
            {
                return;
            }

            m_gpu_timing_tab_view->ClearSelection();
        }
    }

    if (found)
    {
        m_perf_counter_tab_view->CorrelateCounter(found_draw_call_index.value());
    }
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnGpuTimingDataSelected(uint64_t node_index)
{
    if (m_correlated_capture_loaded)
    {
        QItemSelectionModel *pm4_selection_model = m_pm4_command_hierarchy_view->selectionModel();
        QSignalBlocker       pm4_blocker(pm4_selection_model);
        ClearViewModelSelection(*m_pm4_command_hierarchy_view, true);
    }

    QItemSelectionModel *gfxr_selection_model = m_command_hierarchy_view->selectionModel();
    QSignalBlocker       gfxr_blocker(gfxr_selection_model);
    ClearViewModelSelection(*m_command_hierarchy_view, true);

    QAbstractItemModel *gfxr_source_model = m_gfxr_vulkan_commands_filter_proxy_model
                                            ->sourceModel();
    QModelIndex gfxr_draw_call_index_from_source = FindSourceIndexFromNode(gfxr_source_model,
                                                                           node_index);
    QItemSelectionModel::SelectionFlags flags;
    QModelIndex proxy_index = m_gfxr_vulkan_commands_filter_proxy_model->mapFromSource(
    gfxr_draw_call_index_from_source);
    if (proxy_index.isValid())
    {
        QItemSelectionModel *selection_model = m_command_hierarchy_view->selectionModel();

        flags = QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows;

        selection_model->setCurrentIndex(proxy_index, flags);

        ResetVerticalScroll(*m_command_hierarchy_view);
        m_command_hierarchy_view->scrollTo(proxy_index, QAbstractItemView::PositionAtCenter);
        m_command_hierarchy_view->expand(proxy_index);

        m_command_hierarchy_view->viewport()->update();
        QApplication::processEvents();
    }
}
