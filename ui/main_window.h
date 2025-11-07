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

#include <array>

#include <QMainWindow>
#include <QString>

#include "impl_pointer.h"

class QAction;
class QMenu;
class QShortcut;

class ApplicationController;

struct MainWindowActions
{
    QMenu   *m_file_menu = nullptr;
    QMenu   *m_recent_captures_menu = nullptr;
    QAction *m_open_action = nullptr;
    QAction *m_save_action = nullptr;
    QAction *m_save_as_action = nullptr;
    QAction *m_exit_action = nullptr;
    QMenu   *m_capture_menu = nullptr;
    QAction *m_gfxr_capture_action = nullptr;
    QAction *m_pm4_capture_action = nullptr;
    QAction *m_capture_action = nullptr;
    QAction *m_capture_delay_action = nullptr;
    QAction *m_capture_setting_action = nullptr;
    QMenu   *m_analyze_menu = nullptr;
    QAction *m_analyze_action = nullptr;
    QMenu   *m_help_menu = nullptr;
    QAction *m_about_action = nullptr;
    QAction *m_shortcuts_action = nullptr;

    std::array<QAction *, 3> m_recent_file_actions = {};
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    struct Impl;

    MainWindow(ApplicationController &);
    ~MainWindow();

private slots:
    void SetCurrentFile(const QString &file_name, bool is_temp_file);
    void ShowTempStatus(const QString &status_message);
    void OnInteractiveStateUpdated(bool status);

protected:
    virtual void closeEvent(QCloseEvent *closeEvent) Q_DECL_OVERRIDE;

private:
    ImplPointer<Impl> m_impl;
};
