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
#include <windows.h>
#include <string>
#include <memory>

namespace Dive
{
class WindowsLibraryLoader : public IDynamicLibraryLoader
{
public:
    absl::StatusOr<NativeLibraryHandle> Load(const std::string& path) override
    {
        HMODULE handle = LoadLibraryA(path.c_str());
        if (handle == nullptr)
        {
            DWORD error_code = GetLastError();
            return absl::Status(absl::StatusCode::kNotFound,
                                absl::StrCat("Failed to load library '",
                                             path,
                                             "'. Error code: ",
                                             error_code));
        }
        return static_cast<NativeLibraryHandle>(handle);
    }

    absl::StatusOr<void*> GetSymbol(NativeLibraryHandle handle,
                                    const std::string&  symbolName) override
    {
        FARPROC symbol = GetProcAddress(static_cast<HMODULE>(handle), symbolName.c_str());
        if (symbol == nullptr)
        {
            DWORD error_code = GetLastError();
            if (error_code != 0)
            {
                return absl::Status(absl::StatusCode::kNotFound,
                                    absl::StrCat("Failed to find symbol '",
                                                 symbolName,
                                                 "'. Error code: ",
                                                 error_code));
            }

            return absl::Status(absl::StatusCode::kNotFound,
                                absl::StrCat("Symbol '",
                                             symbolName,
                                             "' not found or is a null export."));
        }
        return static_cast<void*>(symbol);
    }

    absl::Status Free(NativeLibraryHandle handle) override
    {
        if (::FreeLibrary(static_cast<HMODULE>(handle)) == 0)
        {
            DWORD error_code = GetLastError();
            return absl::Status(absl::StatusCode::kInternal,
                                absl::StrCat("Failed to free library handle. Error code: ",
                                             error_code));
        }
        return absl::OkStatus();
    }

    std::string GetPluginFileExtension() const override { return ".dll"; }
};

std::unique_ptr<IDynamicLibraryLoader> CreateDynamicLibraryLoader()
{
    return std::make_unique<WindowsLibraryLoader>();
}

}  // namespace Dive