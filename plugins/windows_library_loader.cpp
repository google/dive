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

#ifdef WIN32
#    include "idynamic_library_loader.h"
#    include <windows.h>
#    include <string>
#    include <memory>

class WindowsLibraryLoader : public IDynamicLibraryLoader
{
public:
    NativeLibraryHandle LoadDynamicLibrary(const std::string& path) override
    {
        return (NativeLibraryHandle)LoadLibraryA(path.c_str());
    }

    void* GetSymbol(NativeLibraryHandle handle, const std::string& symbolName) override
    {
        return (void*)GetProcAddress((HMODULE)handle, symbolName.c_str());
    }

    bool FreeLibrary(NativeLibraryHandle handle) override
    {
        return ::FreeLibrary((HMODULE)handle) != 0;
    }

    std::string GetLastErrorString() override { return std::to_string(GetLastError()); }

    std::string GetPluginFileExtension() const override { return ".dll"; }
};

std::unique_ptr<IDynamicLibraryLoader> CreateDynamicLibraryLoader()
{
    return std::make_unique<WindowsLibraryLoader>();
}

#endif  // WIN32