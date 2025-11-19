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

#include "application_controller.h"

#include <QAction>
#include <QCoreApplication>
#include <QMenu>
#include <QMessageBox>

#include "main_window.h"
#include "plugins/plugin_loader.h"

struct ApplicationController::Impl
{
    MainWindow* m_main_window = nullptr;
    QAction*    m_advanced_option = nullptr;

    Dive::PluginLoader m_plugin_manager;
};

ApplicationController::ApplicationController() {}

ApplicationController::~ApplicationController() {}

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

    QObject::connect(m_impl->m_advanced_option,
                     &QAction::toggled,
                     this,
                     &ApplicationController::AdvancedOptionToggled);
}

void ApplicationController::MainWindowClosed()
{
    m_impl->m_plugin_manager.UnloadPlugins();
}

bool ApplicationController::InitializePlugins()
{
    if (!m_impl->m_main_window)
    {
        return false;
    }
    // This assumes plugins are in a 'plugins' subdirectory relative to the executable's directory.
    std::string plugin_path = QCoreApplication::applicationDirPath().toStdString() + "/plugins";

    std::filesystem::path plugins_dir_path(plugin_path);

    if (absl::Status load_status = m_impl->m_plugin_manager.LoadPlugins(plugins_dir_path);
        !load_status.ok())
    {
        QMessageBox::warning(m_impl->m_main_window,
                             tr("Plugin Loading Failed"),
                             tr("Failed to load plugins from '%1'. \nError: %2")
                             .arg(QString::fromStdString(plugin_path))
                             .arg(QString::fromStdString(std::string(load_status.message()))));
        return false;
    }
    return true;
}
