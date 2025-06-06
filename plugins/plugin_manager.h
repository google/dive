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
#include <vector>

#ifdef WIN32
#    include <windows.h>
#else
#    include <dlfcn.h>
#endif

#include "idive_plugin.h"  // Include the pure C++ plugin interface

class MainWindow;

// The PluginManager class is responsible for discovering, loading, and managing IDivePlugin
// instances using platform-native dynamic library loading.
class PluginManager
{
public:
    explicit PluginManager(MainWindow *main_window);
    ~PluginManager();

    void LoadPlugins(const std::string &plugin_directory_path);
    void UnloadPlugins();

    const std::vector<IDivePlugin *> &GetLoadedPlugins() const { return m_loaded_plugins; }

private:
    std::vector<IDivePlugin *> m_loaded_plugins;

#ifdef WIN32
    std::vector<HMODULE> m_library_handles;
#else
    std::vector<void *> m_library_handles;
#endif
    MainWindow *m_main_window;
};