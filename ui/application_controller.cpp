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

#include "ui/application_controller.h"

#include <QAction>
#include <QCoreApplication>
#include <QDebug>
#include <QMenu>
#include <QMessageBox>

#include "dive/plugin/loader/plugin_loader.h"
#include "ui/main_window.h"

struct ApplicationController::Impl
{
    MainWindow* m_main_window = nullptr;
    QAction* m_advanced_option = nullptr;
    bool m_interactive = true;

    Dive::PluginLoader m_plugin_manager;
};

ApplicationController::ApplicationController() {}

ApplicationController::~ApplicationController() {}

bool ApplicationController::IsInteractive() const { return m_impl->m_interactive; }
void ApplicationController::SetInteractive(bool enabled) { m_impl->m_interactive = enabled; }

void ApplicationController::Register(MainWindow& main_window)
{
    m_impl->m_plugin_manager.Bridge().SetQObject(Dive::DiveUIObjectNames::kMainWindow,
                                                 &main_window);
    m_impl->m_main_window = &main_window;
}

bool ApplicationController::AdvancedOptionEnabled() const
{
    return m_impl->m_advanced_option ? m_impl->m_advanced_option->isChecked() : false;
}

void ApplicationController::MainWindowInitialized()
{
    m_impl->m_advanced_option = new QAction(tr("Show Advanced Options"), this);
    m_impl->m_advanced_option->setCheckable(true);
    m_impl->m_advanced_option->setChecked(false);

    m_impl->m_main_window->m_file_menu->insertAction(m_impl->m_main_window->m_exit_action,
                                                     m_impl->m_advanced_option);

    QObject::connect(m_impl->m_advanced_option, &QAction::toggled, this,
                     &ApplicationController::AdvancedOptionToggled);
}

void ApplicationController::MainWindowClosed() { m_impl->m_plugin_manager.UnloadPlugins(); }

void ApplicationController::NotifyWarning(QWidget* parent, const QString& title,
                                          const QString& text)
{
    if (!IsInteractive())
    {
        qDebug() << "Warning dialog: " << title << ": " << text;
        return;
    }
    QMessageBox::warning(parent, title, text);
}

bool ApplicationController::InitializePlugins()
{
    if (!m_impl->m_main_window)
    {
        return false;
    }

    if (absl::Status load_status = m_impl->m_plugin_manager.LoadPlugins(); !load_status.ok())
    {
        NotifyWarning(
            m_impl->m_main_window, tr("Plugin Loading Failed"),
            tr("Error: '%1'").arg(QString::fromStdString(std::string(load_status.message()))));
        return false;
    }
    return true;
}
