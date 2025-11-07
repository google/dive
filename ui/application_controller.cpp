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

#include <QCoreApplication>
#include <QMessageBox>

#include "capture_service/device_mgr.h"
#include "dive_core/common/common.h"
#include "plugins/plugin_loader.h"

#include "main_view.h"
#include "main_window.h"

struct ApplicationController::Impl
{
    explicit Impl(ApplicationController& self) :
        m_self(self)
    {
    }

    ApplicationController& m_self;
    Dive::PluginLoader     m_plugin_manager = {};

    MainWindow* m_window = nullptr;
    MainView*   m_view = nullptr;

    bool                  m_capture_saved = false;
    std::filesystem::path m_unsaved_capture_path = {};

    void OnMainWindowClosed();
};

ApplicationController::ApplicationController(QObject* parent) :
    QObject(parent),
    m_impl(*this)
{
}

ApplicationController::~ApplicationController() {}

void ApplicationController::SetMainWindow(MainWindow* window)
{
    m_impl->m_window = window;
    m_impl->m_plugin_manager.Bridge().SetQObject(Dive::DiveUIObjectNames::kMainWindow, window);
}

void ApplicationController::SetMainView(MainView* view)
{
    m_impl->m_view = view;
}

void ApplicationController::LoadFile(const std::string& file_name, bool is_temp_file)
{
    DIVE_ASSERT(m_impl->m_view != nullptr);
    m_impl->m_view->LoadFile(file_name, is_temp_file);
}

bool ApplicationController::InitializePlugins()
{
    // This assumes plugins are in a 'plugins' subdirectory relative to the executable's directory.
    std::string plugin_path = QCoreApplication::applicationDirPath().toStdString() + "/plugins";

    std::filesystem::path plugins_dir_path(plugin_path);

    if (absl::Status load_status = m_impl->m_plugin_manager.LoadPlugins(plugins_dir_path);
        !load_status.ok())
    {
        QMessageBox::warning(m_impl->m_window,
                             tr("Plugin Loading Failed"),
                             tr("Failed to load plugins from '%1'. \nError: %2")
                             .arg(QString::fromStdString(plugin_path))
                             .arg(QString::fromStdString(std::string(load_status.message()))));
        return false;
    }
    return true;
}

void ApplicationController::Impl::OnMainWindowClosed()
{
    m_plugin_manager.UnloadPlugins();

    if (!m_capture_saved && !m_unsaved_capture_path.empty())
    {
        switch (QMessageBox::question(m_window,
                                      QString("Save current capture"),
                                      (QString("Do you want to save current capture")),
                                      QMessageBox::Yes | QMessageBox::No,
                                      QMessageBox::No))
        {
        case QMessageBox::Yes:
            DIVE_ASSERT(m_view != nullptr);
            m_view->OnSaveCapture();
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

    Dive::GetDeviceManager().RemoveDevice();
}

void ApplicationController::OnMainWindowClosed()
{
    m_impl->OnMainWindowClosed();
}
