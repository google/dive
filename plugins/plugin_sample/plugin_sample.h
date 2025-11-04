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

#include <QObject>
#include <QDebug>
#include <QAction>
#include <QMessageBox>
#include <string>

#include "idive_plugin.h"

class MainWindow;

namespace Dive
{
// The PluginSample class is a sample implementation of IDivePlugin.
// It adds a new menu item to the "Help" menu of the MainWindow and displays a message box when that
// action is triggered.

// Still need Q_OBJECT for signals/slots/meta-object features for Qt UI
class PluginSample : public QObject, public IDivePlugin
{
    Q_OBJECT
public:
    explicit PluginSample(QObject* parent = nullptr);
    ~PluginSample() override;

    std::string PluginName() const override { return "Plugin Test"; }
    std::string PluginVersion() const override { return "1.0.0"; }

    bool Initialize(MainWindow& main_window) override;
    void Shutdown() override;

private slots:
    void OnPluginSampleActionTriggered();
};

}  // namespace Dive