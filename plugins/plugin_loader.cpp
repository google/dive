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
#include "absl/strings/str_cat.h"


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

absl::Status PluginLoader::LoadPlugins(const std::filesystem::path& plugins_dir_path)
{
    std::string error_message;

    auto append_error_message = [&](const std::string& msg) {
        absl::StrAppend(&error_message, "PluginLoader: ", msg, "\n");
    };

    if (!std::filesystem::exists(plugins_dir_path) ||
        !std::filesystem::is_directory(plugins_dir_path))
    {
        return absl::NotFoundError("Plugin directory not found: " + plugins_dir_path.string());
    }

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

        absl::StatusOr<NativeLibraryHandle> handle = m_library_loader->Load(file_path);
        if (!handle.ok())
        {
            append_error_message(std::string(handle.status().message()));
            continue;
        }

        LibraryHandleUniquePtr library_handle_ptr(handle.value(),
                                                  NativeLibraryHandleDeleter(
                                                  m_library_loader.get()));

        absl::StatusOr<void*> create_func_symbol = m_library_loader
                                                   ->GetSymbol(handle.value(),
                                                               "CreateDivePluginInstance");

        if (!create_func_symbol.ok())
        {
            append_error_message(std::string(create_func_symbol.status().message()));
            continue;
        }
        CreatePluginFunc create_func = reinterpret_cast<CreatePluginFunc>(
        create_func_symbol.value());

        if (!create_func)
        {
            append_error_message(
            "Function pointer for 'CreateDivePluginInstance' is null for plugin: " + file_path);
            continue;
        }

        IDivePlugin* raw_plugin = create_func();

        if (!raw_plugin)
        {
            append_error_message("'CreateDivePluginInstance' returned null for plugin: " +
                                 file_path);
            continue;
        }

        PluginUniquePtr plugin(raw_plugin);

        std::cout << "PluginLoader: Successfully instantiated plugin: " << plugin->PluginName()
                  << " Version: " << plugin->PluginVersion() << std::endl;

        if (!plugin->Initialize(m_main_window))
        {
            append_error_message("Failed to initialize plugin: " + plugin->PluginName());
        }

        m_loaded_plugin_entries.emplace_back(std::move(library_handle_ptr), std::move(plugin));
        std::cout << "PluginLoader: Plugin " << m_loaded_plugin_entries.back().plugin->PluginName()
                  << " initialized successfully." << std::endl;
    }

    if (!error_message.empty())
    {
        return absl::InternalError(
        "Some plugins encountered errors during loading/initialization: \n" + error_message);
    }

    return absl::OkStatus();
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
        if (absl::Status free = loader->Free(handle); free.ok())
        {
            std::cout << "PluginLoader: Library handle auto-freed via RAII." << std::endl;
        }
        else
        {
            std::cout << "PluginLoader: Failed to auto-free library handle. Error: "
                      << free.message() << std::endl;
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