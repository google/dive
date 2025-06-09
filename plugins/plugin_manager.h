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

#include <vector>
#include <memory>

#include "idive_plugin.h"
#include "idynamic_library_loader.h"
#include <filesystem>

class MainWindow;

class PluginManager
{
public:
    explicit PluginManager(MainWindow &main_window);
    ~PluginManager();

    PluginManager(const PluginManager &) = delete;
    PluginManager &operator=(const PluginManager &) = delete;

    void LoadPlugins(const std::filesystem::path &plugin_directory_path);
    void UnloadPlugins();

private:
    struct NativeLibraryHandleDeleter
    {
        IDynamicLibraryLoader *loader;

        explicit NativeLibraryHandleDeleter(IDynamicLibraryLoader *l = nullptr) :
            loader(l)
        {
        }

        using pointer = NativeLibraryHandle;
        void operator()(NativeLibraryHandle handle) const;
    };
    using LibraryHandleUniquePtr = std::unique_ptr<NativeLibraryHandle, NativeLibraryHandleDeleter>;

    struct PluginDeleter
    {
        void operator()(IDivePlugin *plugin) const;
    };
    using PluginUniquePtr = std::unique_ptr<IDivePlugin, PluginDeleter>;

    std::vector<PluginUniquePtr>           m_loaded_plugins;
    std::vector<LibraryHandleUniquePtr>    m_library_handles;
    std::unique_ptr<IDynamicLibraryLoader> m_library_loader;
    MainWindow                            &m_main_window;
};