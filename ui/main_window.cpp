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

#include <filesystem>

#include <QAction>
#include <QCloseEvent>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>
#include <QFileInfo>

#include "application_controller.h"
#include "main_view.h"
#include "settings.h"
#include "shortcuts.h"

namespace
{
QString StrippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}

}  // namespace

struct MainWindow::Impl : public MainWindowActions
{
    Impl(MainWindow &self, ApplicationController &controller) :
        m_self(self),
        m_controller(controller)
    {
    }

    MainWindow            &m_self;
    ApplicationController &m_controller;

    MainView *m_view = nullptr;

    QStatusBar *m_status_bar = nullptr;

    bool                  m_capture_saved = false;
    std::filesystem::path m_unsaved_capture_path = {};

    void CreateActions();
    void CreateMenus();
    void CreateToolBars();
    void CreateStatusBar();

    void SetCurrentFile(const QString &file_name, bool is_temp_file);
    void UpdateRecentFiles(QStringList recent_files);
};

MainWindow::MainWindow(ApplicationController &controller) :
    m_impl(*this, controller)
{
    controller.SetMainWindow(this);

    m_impl->m_view = new MainView(this);
    setCentralWidget(m_impl->m_view);
    controller.SetMainView(m_impl->m_view);

    m_impl->CreateActions();
    m_impl->CreateMenus();
    m_impl->CreateStatusBar();
    m_impl->m_view->InstallShortcut(this);
    m_impl->CreateToolBars();
    m_impl->UpdateRecentFiles(Settings::Get()->ReadRecentFiles());

    QObject::connect(m_impl->m_view, &MainView::SetCurrentFile, this, &MainWindow::SetCurrentFile);
    QObject::connect(m_impl->m_view, &MainView::ShowTempStatus, this, &MainWindow::ShowTempStatus);
    QObject::connect(m_impl->m_view,
                     &MainView::InteractiveStateUpdated,
                     this,
                     &MainWindow::OnInteractiveStateUpdated);
}

MainWindow::~MainWindow() {}

void MainWindow::closeEvent(QCloseEvent *closeEvent)
{
    m_impl->m_controller.OnMainWindowClosed();
    closeEvent->accept();
}

//--------------------------------------------------------------------------------------------------
void MainWindow::Impl::CreateActions()
{
    // Open file action
    m_open_action = new QAction(tr("&Open"), &m_self);
    m_open_action->setIcon(QIcon(":/images/open.png"));
    m_open_action->setShortcuts(QKeySequence::Open);
    m_open_action->setStatusTip(tr("Open an existing capture"));
    connect(m_open_action, &QAction::triggered, m_view, &MainView::OnOpenFile);

    // Exit application action
    m_exit_action = new QAction(tr("E&xit"), &m_self);
    m_exit_action->setIcon(QIcon(":/images/exit.png"));
    m_exit_action->setShortcut(tr("Ctrl+Q"));
    m_exit_action->setStatusTip(tr("Exit the application"));
    connect(m_exit_action, SIGNAL(triggered()), &m_self, SLOT(close()));

    // Save file action
    m_save_action = new QAction(tr("&Save"), &m_self);
    m_save_action->setStatusTip(tr("Save the current capture"));
    m_save_action->setIcon(QIcon(":/images/save.png"));
    m_save_action->setShortcut(QKeySequence::Save);
    m_save_action->setEnabled(false);
    connect(m_save_action, &QAction::triggered, m_view, &MainView::OnSaveCapture);
    connect(m_view, &MainView::SetSaveMenuStatus, m_save_action, &QAction::setEnabled);

    // Save as file action
    m_save_as_action = new QAction(tr("&Save As"), &m_self);
    m_save_as_action->setStatusTip(tr("Save the current capture"));
    m_save_as_action->setShortcut(QKeySequence::SaveAs);
    m_save_as_action->setEnabled(false);
    connect(m_save_as_action, &QAction::triggered, m_view, &MainView::OnSaveCapture);
    connect(m_view, &MainView::SetSaveMenuStatus, m_save_as_action, &QAction::setEnabled);

    // Recent file actions
    for (auto &action : m_recent_file_actions)
    {
        action = new QAction(&m_self);
        action->setVisible(false);
        connect(action, SIGNAL(triggered()), &m_controller, SLOT(OpenRecentFile()));
    }

    // Capture action
    m_capture_action = new QAction(tr("&Capture"), &m_self);
    m_capture_action->setStatusTip(tr("Capture a Dive trace"));
    m_capture_action->setShortcut(QKeySequence("f5"));
    connect(m_capture_action, &QAction::triggered, m_view, &MainView::OnNormalCapture);

    // PM4 Capture action
    m_pm4_capture_action = new QAction(tr("PM4 Capture"), &m_self);
    m_pm4_capture_action->setStatusTip(tr("Capture a Dive trace (PM4)"));
    m_pm4_capture_action->setShortcut(QKeySequence("f5"));
    connect(m_pm4_capture_action, &QAction::triggered, m_view, &MainView::OnNormalCapture);
    // GFXR Capture action
    m_gfxr_capture_action = new QAction(tr("GFXR Capture"), &m_self);
    m_gfxr_capture_action->setStatusTip(tr("Capture a Dive trace (GFXR)"));
    m_gfxr_capture_action->setShortcut(QKeySequence("f6"));
    connect(m_gfxr_capture_action, &QAction::triggered, m_view, &MainView::OnGFXRCapture);

    // Capture with delay action
    m_capture_delay_action = new QAction(tr("Capture with delay"), &m_self);
    m_capture_delay_action->setStatusTip(tr("Capture a Dive trace after a delay"));
    m_capture_delay_action->setShortcut(QKeySequence("Ctrl+f5"));
    connect(m_capture_delay_action, &QAction::triggered, m_view, &MainView::OnCaptureTrigger);

    // Analyze action
    m_analyze_action = new QAction(tr("Analyze Capture"), &m_self);
    m_analyze_action->setStatusTip(tr("Analyze a Capture"));
    m_analyze_action->setIcon(QIcon(":/images/analyze.png"));
    m_analyze_action->setShortcut(QKeySequence("f7"));
    connect(m_analyze_action, &QAction::triggered, m_view, &MainView::OnAnalyzeCapture);

    // Shortcuts action
    m_shortcuts_action = new QAction(tr("&Shortcuts"), &m_self);
    m_shortcuts_action->setStatusTip(tr("Display application keyboard shortcuts"));
    connect(m_shortcuts_action, &QAction::triggered, m_view, &MainView::OnShortcuts);

    // About action
    m_about_action = new QAction(tr("&About Dive"), &m_self);
    m_about_action->setStatusTip(tr("Display application version information"));
    connect(m_about_action, &QAction::triggered, m_view, &MainView::OnAbout);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::Impl::CreateMenus()
{
    // File Menu
    m_file_menu = m_self.menuBar()->addMenu(tr("&File"));
    m_file_menu->addAction(m_open_action);
    m_file_menu->addAction(m_save_action);

    // TODO (b/447422197) Show this action when the save/load system has been implemented.
    // m_file_menu->addAction(m_save_as_action);
    m_file_menu->addSeparator();
    m_recent_captures_menu = m_file_menu->addMenu(tr("Recent captures"));
    for (auto action : m_recent_file_actions)
        m_recent_captures_menu->addAction(action);
    m_file_menu->addSeparator();
    m_file_menu->addAction(m_exit_action);

    m_capture_menu = m_self.menuBar()->addMenu(tr("&Capture"));
    m_capture_menu->addAction(m_pm4_capture_action);
    m_capture_menu->addAction(m_gfxr_capture_action);

    m_analyze_menu = m_self.menuBar()->addMenu(tr("&Analyze"));
    m_analyze_menu->addAction(m_analyze_action);

    m_help_menu = m_self.menuBar()->addMenu(tr("&Help"));
    m_help_menu->addAction(m_shortcuts_action);
    m_help_menu->addAction(m_about_action);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::Impl::CreateToolBars()
{
    // Create the capture button for the toolbar
    QToolButton *capture_button = new QToolButton(&m_self);
    capture_button->setPopupMode(QToolButton::InstantPopup);
    capture_button->setMenu(m_capture_menu);
    capture_button->setIcon(QIcon(":/images/capture.png"));

    QToolButton *open_button = new QToolButton(&m_self);
    open_button->setPopupMode(QToolButton::MenuButtonPopup);
    open_button->setDefaultAction(m_open_action);
    open_button->setMenu(m_recent_captures_menu);

    auto file_tool_bar = m_self.addToolBar(tr("&File"));
    file_tool_bar->addWidget(open_button);
    file_tool_bar->addAction(m_save_action);
    file_tool_bar->addWidget(capture_button);
    file_tool_bar->addAction(m_analyze_action);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::Impl::CreateStatusBar()
{
    // Create status bar on the main window.
    m_status_bar = new QStatusBar(&m_self);
    m_status_bar->setStyleSheet("background:#D0D0D0; color:#282828");
    m_self.setStatusBar(m_status_bar);
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
        m_impl->UpdateRecentFiles(recent_files);
    }

    setWindowTitle(tr("%1[*] - %2").arg(shownName).arg(tr("Dive")));
}

//--------------------------------------------------------------------------------------------------
void MainWindow::Impl::UpdateRecentFiles(QStringList recent_files)
{
    int next_file_index = 0;
    for (auto action : m_recent_file_actions)
    {
        int file_index = next_file_index++;
        if (file_index < recent_files.count())
        {
            QString text = tr("%1").arg(StrippedName(recent_files[file_index]));
            action->setText(text);
            action->setData(recent_files[file_index]);
            action->setVisible(true);
        }
        else
        {
            action->setVisible(false);
        }
    }
}

//--------------------------------------------------------------------------------------------------
void MainWindow::ShowTempStatus(const QString &status_message)
{
    m_impl->m_status_bar->showMessage(status_message, MESSAGE_TIMEOUT);
}

//--------------------------------------------------------------------------------------------------
void MainWindow::OnInteractiveStateUpdated(bool status)
{
    setDisabled(status);
}
