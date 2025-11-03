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

#include <string>

class MainWindow;

namespace Dive
{
// The IDivePlugin class defines the interface for Dive plugins.
// Concrete plugin implementations (e.g., PluginSample) will still need to inherit from QObject if
// they interact with Qt UI elements or use Qt's signal/slot mechanism.
//
// IMPORTANT: This interface must NOT expose any types or functions related to proprietary
// libraries. The proprietary code should be entirely encapsulated within the plugin's concrete
// implementation.
class IDivePlugin
{
public:
    virtual ~IDivePlugin() = default;

    // Returns a human-readable name for the plugin
    virtual std::string PluginName() const = 0;

    // Return The version of the plugin
    virtual std::string PluginVersion() const = 0;

    // main_window: A pointer to the main application window. Plugins can use this
    // to interact with or modify the main UI using MainWindow's public API.
    // Return true if initialization was successful, false otherwise.
    virtual bool Initialize(MainWindow& main_window) = 0;

    // Shuts down the plugin and performs any necessary cleanup.
    virtual void Shutdown() = 0;
};

#ifdef WIN32
#    define DIVE_PLUGIN_EXPORT __declspec(dllexport)
#else
#    define DIVE_PLUGIN_EXPORT
#endif

extern "C" DIVE_PLUGIN_EXPORT IDivePlugin* CreateDivePluginInstance();
}  // namespace Dive