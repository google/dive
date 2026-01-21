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

#pragma once

#include <filesystem>
#include <memory>
#include <vector>

namespace Dive
{

class DiveArchive
{
 public:
    static std::unique_ptr<DiveArchive> Open(std::filesystem::path filename);
    static bool Create(std::filesystem::path dst, std::vector<std::filesystem::path> components);

    // Extract to `dst` directory, returns extracted filepaths.
    std::vector<std::filesystem::path> ExtractTo(std::filesystem::path dst);

    static bool IsSupportedInputFormat(std::filesystem::path filename);

 private:
    explicit DiveArchive(std::filesystem::path archive_path);

    std::filesystem::path m_path;
};

}  // namespace Dive
