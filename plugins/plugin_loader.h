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
#include <unordered_map>
#include <vector>

#include "idive_plugin.h"
#include "idynamic_library_loader.h"
#include "absl/status/status.h"

class MainWindow;

namespace Dive
{

class DivePluginBridge : public IDivePluginBridge
{
public:
    QObject *GetQObject(const char *name) const final;
    void     SetQObject(const char *name, QObject *object);

    void *GetMutable(const char *name) const final;
    void  SetMutable(const char *name, void *object);

    const void *GetConst(const char *name) const final;
    void        SetConst(const char *name, const void *object);

private:
    std::unordered_map<std::string, const void *> m_const_objects;
    std::unordered_map<std::string, void *>       m_mutable_objects;
    std::unordered_map<std::string, QObject *>    m_qt_objects;
};

class PluginLoader
{
public:
    PluginLoader();
    ~PluginLoader();

    PluginLoader(const PluginLoader &) = delete;
    PluginLoader &operator=(const PluginLoader &) = delete;

    absl::Status LoadPlugins(const std::filesystem::path &plugin_directory_path);
    void         UnloadPlugins();

    const DivePluginBridge &Bridge() const { return m_bridge; }
    DivePluginBridge       &Bridge() { return m_bridge; }

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
    DivePluginBridge                       m_bridge;
};

}  // namespace Dive
