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

#include <filesystem>
#include <memory>
#include <vector>

#include "idive_plugin.h"
#include "idynamic_library_loader.h"
#include "absl/status/status.h"

class MainWindow;

namespace Dive
{
class PluginLoader
{
public:
    explicit PluginLoader(MainWindow &main_window);
    ~PluginLoader();

    PluginLoader(const PluginLoader &) = delete;
    PluginLoader &operator=(const PluginLoader &) = delete;

    absl::Status LoadPlugins(const std::filesystem::path &plugin_directory_path);
    void         UnloadPlugins();

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

    struct LoadedPluginEntry
    {
        // The plugin must be shut down BEFORE its library handle is freed.
        // This ensures reverse destruction order (library_handle then plugin).
        LibraryHandleUniquePtr library_handle;
        PluginUniquePtr        plugin;

        LoadedPluginEntry(LibraryHandleUniquePtr lh, PluginUniquePtr p) :
            library_handle(std::move(lh)),
            plugin(std::move(p))
        {
        }
    };

    // Members are destroyed in the reverse order of their declaration.
    // m_library_loader must be destroyed AFTER m_loaded_plugin_entries
    // because NativeLibraryHandleDeleter uses a raw pointer to m_library_loader.
    std::unique_ptr<IDynamicLibraryLoader> m_library_loader;
    std::vector<LoadedPluginEntry>         m_loaded_plugin_entries;
    MainWindow                            &m_main_window;
};

}  // namespace Dive