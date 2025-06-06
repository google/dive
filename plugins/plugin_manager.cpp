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

#include "plugin_manager.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

PluginManager::PluginManager(MainWindow* main_window) :
    m_main_window(main_window)
{
}

PluginManager::~PluginManager()
{
    UnloadPlugins();
}

void PluginManager::LoadPlugins(const std::string& plugin_directory_path)
{
    fs::path plugins_dir_path(plugin_directory_path);
    if (!fs::exists(plugins_dir_path) || !fs::is_directory(plugins_dir_path))
    {
        return;
    }

    for (const auto& entry : fs::directory_iterator(plugins_dir_path))
    {
        if (entry.is_regular_file())
        {
            std::string file_name = entry.path().filename().string();
            std::string file_path = entry.path().string();

#ifdef WIN32
            if (file_name.length() < 4 || file_name.substr(file_name.length() - 4) != ".dll")
                continue;
            HMODULE h_library = LoadLibraryA(file_path.c_str());
            if (h_library == nullptr)
            {
                std::cout << "PluginManager: Failed to load library: " << file_path
                          << " Error: " << GetLastError() << std::endl;
                continue;
            }

            CreatePluginFunc create_func = (CreatePluginFunc)
            GetProcAddress(h_library, "CreateDivePluginInstance");
#else
            if (file_name.length() < 3 || file_name.substr(file_name.length() - 3) != ".so")
                continue;
            void* h_library = dlopen(file_path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
            if (h_library == nullptr)
            {
                std::cout << "PluginManager: Failed to load library: " << file_path
                          << " Error: " << dlerror() << std::endl;
                continue;
            }

            // Clear any old errors
            dlerror();
            CreatePluginFunc create_func = (CreatePluginFunc)dlsym(h_library,
                                                                   "CreateDivePluginInstance");
            const char*      dlsym_error = dlerror();
            if (dlsym_error != nullptr)
            {
                std::cout << "PluginManager: Failed to find 'CreateDivePluginInstance' in "
                          << file_path << " Error: " << dlsym_error << std::endl;
                dlclose(h_library);
                continue;
            }
#endif

            if (create_func)
            {
                IDivePlugin* plugin = create_func();
                if (plugin)
                {
                    std::cout << "PluginManager: Successfully instantiated plugin: "
                              << plugin->PluginName() << " Version: " << plugin->PluginVersion()
                              << std::endl;

                    if (plugin->Initialize(m_main_window))
                    {
                        m_loaded_plugins.push_back(plugin);
                        // Store the handle for unloading
                        m_library_handles.push_back(h_library);
                        std::cout << "PluginManager: Plugin " << plugin->PluginName()
                                  << " initialized successfully." << std::endl;
                    }
                    else
                    {
                        std::cout << "PluginManager: Failed to initialize plugin: "
                                  << plugin->PluginName() << std::endl;
                        delete plugin;
#ifdef WIN32
                        FreeLibrary(h_library);
#else
                        dlclose(h_library);
#endif
                    }
                }
                else
                {
                    std::cout << "PluginManager: 'CreateDivePluginInstance' returned null for: "
                              << file_path << std::endl;
#ifdef WIN32
                    FreeLibrary(h_library);
#else
                    dlclose(h_library);
#endif
                }
            }
            else
            {
                std::cout << "PluginManager: 'CreateDivePluginInstance' not found in " << file_path
                          << std::endl;
#ifdef WIN32
                FreeLibrary(h_library);
#else
                dlclose(h_library);
#endif
            }
        }
    }
}

void PluginManager::UnloadPlugins()
{
    std::cout << "PluginManager: Unloading all plugins..." << std::endl;
    // Iterate through loaded plugins in reverse to ensure proper cleanup if dependencies exist
    for (int i = m_loaded_plugins.size() - 1; i >= 0; --i)
    {
        IDivePlugin* plugin = m_loaded_plugins[i];
        std::cout << "PluginManager: Shutting down plugin: " << plugin->PluginName() << std::endl;
        plugin->Shutdown();
        delete plugin;
    }
    m_loaded_plugins.clear();

    for (auto handle : m_library_handles)
    {
#ifdef WIN32
        if (FreeLibrary(handle))
        {
            std::cout << "PluginManager: Library unloaded successfully." << std::endl;
        }
        else
        {
            std::cout << "PluginManager: Failed to unload library. Error: " << GetLastError()
                      << std::endl;
        }
#else
        if (dlclose(handle) == 0)
        {
            std::cout << "PluginManager: Library unloaded successfully." << std::endl;
        }
        else
        {
            std::cout << "PluginManager: Failed to unload library. Error: " << dlerror()
                      << std::endl;
        }
#endif
    }
    m_library_handles.clear();
    std::cout << "PluginManager: All plugins unloaded." << std::endl;
}