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
#include "plugin_loader.h"

namespace Dive
{
using CreatePluginFunc = IDivePlugin* (*)();

PluginLoader::PluginLoader(MainWindow& main_window) :
    m_main_window(main_window),
    m_library_loader(CreateDynamicLibraryLoader())
{
}

PluginLoader::~PluginLoader()
{
    UnloadPlugins();
}

void PluginLoader::LoadPlugins(const std::filesystem::path& plugins_dir_path)
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

        if (entry.path().extension() != m_library_loader->GetPluginFileExtension())
        {
            continue;
        }

        NativeLibraryHandle handle = m_library_loader->Load(file_path);
        if (handle == nullptr)
        {
            std::cout << "PluginLoader: Failed to load library: " << file_path
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
            std::cout << "PluginLoader: 'CreateDivePluginInstance' symbol not found in: "
                      << file_path << std::endl;
            continue;
        }

        IDivePlugin* raw_plugin = create_func();

        if (!raw_plugin)
        {
            std::cout << "PluginLoader: 'CreateDivePluginInstance' returned null for: " << file_path
                      << std::endl;
            continue;
        }

        PluginUniquePtr plugin(raw_plugin);

        std::cout << "PluginLoader: Successfully instantiated plugin: " << plugin->PluginName()
                  << " Version: " << plugin->PluginVersion() << std::endl;

        if (!plugin->Initialize(m_main_window))
        {
            std::cout << "PluginLoader: Failed to initialize plugin: " << raw_plugin->PluginName()
                      << std::endl;
        }

        m_loaded_plugin_entries.emplace_back(std::move(library_handle_ptr), std::move(plugin));
        std::cout << "PluginLoader: Plugin " << m_loaded_plugin_entries.back().plugin->PluginName()
                  << " initialized successfully." << std::endl;
    }
}

void PluginLoader::UnloadPlugins()
{
    std::cout << "PluginLoader: Unloading all plugins..." << std::endl;

    m_loaded_plugin_entries.clear();

    std::cout << "PluginLoader: All plugins unloaded." << std::endl;
}

void PluginLoader::NativeLibraryHandleDeleter::operator()(NativeLibraryHandle handle) const
{
    if (handle != nullptr && loader != nullptr)
    {
        if (loader->Free(handle))
        {
            std::cout << "PluginLoader: Library handle auto-freed via RAII." << std::endl;
        }
        else
        {
            std::cout << "PluginLoader: Failed to auto-free library handle. Error: "
                      << loader->GetLastErrorString() << std::endl;
        }
    }
}

void PluginLoader::PluginDeleter::operator()(IDivePlugin* plugin) const
{
    if (plugin)
    {
        plugin->Shutdown();
    }
    delete plugin;
}

}  // namespace Dive