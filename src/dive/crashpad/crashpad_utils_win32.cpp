/*
 Copyright 2026 Google LLC

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

#include <cstdlib>
#include <filesystem>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "crashpad_utils.h"

namespace Dive
{

absl::StatusOr<std::filesystem::path> GetWritableRoot()
{
    if (const char* local_app_data = std::getenv("LOCALAPPDATA"))
    {
        return std::filesystem::path(local_app_data) / kProductName;
    }
    else
    {
        return absl::NotFoundError("LOCALAPPDATA environment variable not set.");
    }
}

std::filesystem::path GetHandlerBinaryName()
{
    return std::filesystem::path(kHandlerBinary).replace_extension(".exe");
}

}  // namespace Dive
