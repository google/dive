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

#include "idynamic_library_loader.h"
#include <dlfcn.h>
#include <string>
#include <memory>

namespace Dive
{
class LinuxLibraryLoader : public IDynamicLibraryLoader
{
public:
    NativeLibraryHandle Load(const std::string& path) override
    {
        return dlopen(path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
    }

    void* GetSymbol(NativeLibraryHandle handle, const std::string& symbolName) override
    {
        dlerror();
        return dlsym(handle, symbolName.c_str());
    }

    bool Free(NativeLibraryHandle handle) override { return dlclose(handle) == 0; }

    std::string GetLastErrorString() override
    {
        const char* error = dlerror();
        return error ? std::string(error) : "";
    }

    std::string GetPluginFileExtension() const override { return ".so"; }
};

std::unique_ptr<IDynamicLibraryLoader> CreateDynamicLibraryLoader()
{
    return std::make_unique<LinuxLibraryLoader>();
}

}  // namespace Dive