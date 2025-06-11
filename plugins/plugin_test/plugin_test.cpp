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

#include "plugin_test.h"
#include <QMenu>
#include <QMenuBar>
#include <iostream>
#include "ui/main_window.h"

namespace Dive
{
PluginTest::PluginTest(QObject* parent) :
    QObject(parent)
{
}

PluginTest::~PluginTest() {}

bool PluginTest::Initialize(MainWindow& main_window)
{
    QMenu*          help_menu = nullptr;
    QList<QAction*> menu_bar_actions = main_window.menuBar()->actions();
    for (QAction* action : menu_bar_actions)
    {
        if (action->menu())
        {
            if (action->text().trimmed().replace("&", "") == "Help")
            {
                help_menu = action->menu();
                break;
            }
        }
    }

    if (!help_menu)
    {
        return false;
    }

    QAction* action = new QAction(QString::fromStdString("Plugin Info"), this);
    connect(action, &QAction::triggered, this, &PluginTest::OnPluginTestActionTriggered);
    help_menu->addAction(action);
    return true;
}

void PluginTest::Shutdown() {}

void PluginTest::OnPluginTestActionTriggered()
{
    QMessageBox::information(nullptr,
                             QString::fromStdString("Plugin Info"),
                             QString::fromStdString(
                             "All plugin .dll(s)/.so(s) need to be put into 'plugins' subdirectory "
                             "alongside the dive_ui executable."));
}

// This function must be exported from the shared library.
extern "C" DIVE_PLUGIN_EXPORT IDivePlugin* CreateDivePluginInstance()
{
    return new PluginTest();
}
}  // namespace Dive