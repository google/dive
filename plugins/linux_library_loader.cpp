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
    absl::StatusOr<NativeLibraryHandle> Load(const std::string& path) override
    {
        // Clear any existing error state from dlopen.
        dlerror();
        NativeLibraryHandle handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
        if (handle == nullptr)
        {
            const char* error_msg = dlerror();
            return absl::Status(absl::StatusCode::kUnknown,
                                absl::StrCat("Failed to load library '",
                                             path,
                                             "': ",
                                             error_msg ? error_msg : "Unknown error"));
        }
        return handle;
    }

    absl::StatusOr<void*> GetSymbol(NativeLibraryHandle handle,
                                    const std::string&  symbolName) override
    {
        // Clear any existing error state from dlsym.
        dlerror();
        void*       symbol = dlsym(handle, symbolName.c_str());
        const char* error_msg = dlerror();
        if (error_msg != nullptr)
        {
            return absl::Status(absl::StatusCode::kNotFound,
                                absl::StrCat("Failed to find symbol '",
                                             symbolName,
                                             "': ",
                                             error_msg));
        }
        return symbol;
    }

    absl::Status Free(NativeLibraryHandle handle) override
    {
        if (dlclose(handle) != 0)
        {
            const char* error_msg = dlerror();
            return absl::Status(absl::StatusCode::kInternal,
                                absl::StrCat("Failed to free library handle: ",
                                             error_msg ? error_msg : "Unknown error"));
        }
        return absl::OkStatus();
    }

    std::string GetPluginFileExtension() const override { return ".so"; }
};

std::unique_ptr<IDynamicLibraryLoader> CreateDynamicLibraryLoader()
{
    return std::make_unique<LinuxLibraryLoader>();
}

}  // namespace Dive