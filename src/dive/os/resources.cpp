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

#include "dive/os/resources.h"

#include <vector>

#include "absl/base/no_destructor.h"

namespace Dive
{

ResourceResolver &ResourceResolver::Get()
{
    static absl::NoDestructor<ResourceResolver> resolver;
    return *resolver;
}

void ResourceResolver::AddInstallPrefix(const std::filesystem::path &install_prefix)
{
    m_install_prefix.push_back(install_prefix);
}

std::optional<std::filesystem::path> ResourceResolver::ResolveAssetPath(const std::string &name)
{
    std::vector<std::filesystem::path> search_paths;

    for (const auto &install_prefix : m_install_prefix)
    {
        search_paths.push_back(install_prefix / "install");
        search_paths.push_back(install_prefix);
#if defined(__APPLE__)
        search_paths.push_back(install_prefix / "../Resources/");
#endif
    }

    search_paths.push_back(std::filesystem::path{ "./install" });
    search_paths.push_back(std::filesystem::path{ "../../build_android/Release/bin" });
    search_paths.push_back(std::filesystem::path{ "../../install" });
    search_paths.push_back(std::filesystem::path{ "./" });

    for (const auto &search_path : search_paths)
    {
        auto result_path = search_path / name;
        if (std::filesystem::exists(result_path))
        {
            return std::filesystem::canonical(result_path);
        }
    }
    return std::nullopt;
}

}  // namespace Dive
