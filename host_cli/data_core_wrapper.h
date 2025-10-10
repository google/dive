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

#pragma once

#include <memory>

#include "dive_core/capture_data.h"
#include "dive_core/data_core.h"

#include "absl/status/status.h"

namespace Dive::HostCli
{

// Initializes DataCore and provides access to it, also stores relevant info for operations
class DataCoreWrapper
{
public:
    DataCoreWrapper();
    bool         IsGfxrLoaded() const;
    bool         IsDataCoreInitialized() const { return m_data_core != nullptr; }
    absl::Status LoadGfxrFile(const std::string& original_gfxr_file_path);
    absl::Status WriteNewGfxrFile(const std::string& new_gfxr_file_path);
    absl::Status RemoveGfxrBlocks(std::vector<int> block_ids);

private:
    std::unique_ptr<Dive::DataCore> m_data_core = nullptr;
};

}  // namespace Dive::HostCli
