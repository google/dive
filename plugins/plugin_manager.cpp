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

#include <iostream>
#include <string>
#include "plugin_manager.h"

using CreatePluginFunc = IDivePlugin* (*)();

PluginManager::PluginManager(MainWindow& main_window) :
    m_main_window(main_window),
    m_library_loader(CreateDynamicLibraryLoader())
{
}

PluginManager::~PluginManager()
{
    UnloadPlugins();
}

void PluginManager::LoadPlugins(const std::filesystem::path& plugins_dir_path)
{
    std::string expected_suffix = m_library_loader->GetPluginFileExtension();

    for (const auto& entry : std::filesystem::directory_iterator(plugins_dir_path))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        std::string file_name = entry.path().filename().string();
        std::string file_path = entry.path().string();

        auto HasSuffix = [&](const std::string& file_name, const std::string& expected_suffix) {
            return file_name.length() < expected_suffix.length() ||
                   file_name.substr(file_name.length() - expected_suffix.length()) !=
                   expected_suffix;
        };

        if (HasSuffix(file_name, expected_suffix))
        {
            continue;
        }

        NativeLibraryHandle handle = m_library_loader->LoadDynamicLibrary(file_path);
        if (handle == nullptr)
        {
            std::cout << "PluginManager: Failed to load library: " << file_path
                      << " Error: " << m_library_loader->GetLastErrorString() << std::endl;
            continue;
        }

        LibraryHandleUniquePtr library_handle_ptr(handle,
                                                  NativeLibraryHandleDeleter(
                                                  m_library_loader.get()));

        CreatePluginFunc create_func = reinterpret_cast<CreatePluginFunc>(
        m_library_loader->GetSymbol(handle, "CreateDivePluginInstance"));

        if (!create_func)
        {
            std::cout << "PluginManager: 'CreateDivePluginInstance' returned null for: "
                      << file_path << std::endl;
            continue;
        }

        IDivePlugin* raw_plugin = create_func();

        if (!raw_plugin)
        {
            std::cout << "PluginManager: 'CreateDivePluginInstance' not found in " << file_path
                      << std::endl;
            continue;
        }

        PluginUniquePtr plugin(raw_plugin);

        std::cout << "PluginManager: Successfully instantiated plugin: " << plugin->PluginName()
                  << " Version: " << plugin->PluginVersion() << std::endl;

        if (plugin->Initialize(m_main_window))
        {
            m_loaded_plugins.push_back(std::move(plugin));
            m_library_handles.push_back(std::move(library_handle_ptr));
            std::cout << "PluginManager: Plugin " << m_loaded_plugins.back()->PluginName()
                      << " initialized successfully." << std::endl;
        }
        else
        {
            std::cout << "PluginManager: Failed to initialize plugin: " << raw_plugin->PluginName()
                      << std::endl;
        }
    }
}

void PluginManager::UnloadPlugins()
{
    std::cout << "PluginManager: Unloading all plugins..." << std::endl;

    m_loaded_plugins.clear();
    m_library_handles.clear();

    std::cout << "PluginManager: All plugins unloaded." << std::endl;
}

void PluginManager::NativeLibraryHandleDeleter::operator()(NativeLibraryHandle handle) const
{
    if (handle != nullptr && loader != nullptr)
    {
        if (loader->FreeLibrary(handle))
        {
            std::cout << "PluginManager: Library handle auto-freed via RAII." << std::endl;
        }
        else
        {
            std::cout << "PluginManager: Failed to auto-free library handle. Error: "
                      << loader->GetLastErrorString() << std::endl;
        }
    }
}

void PluginManager::PluginDeleter::operator()(IDivePlugin* plugin) const
{
    if (plugin)
    {
        plugin->Shutdown();
    }
    delete plugin;
}