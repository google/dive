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

#include "data_core_wrapper.h"

#include "dive_core/log.h"
#include "dive_core/capture_data.h"
#include "dive_core/data_core.h"

namespace Dive::HostCli
{

DataCoreWrapper::DataCoreWrapper()
{
    // Initialize DataCore
    m_data_core = std::make_unique<Dive::DataCore>(&m_log);
}

bool DataCoreWrapper::IsGfxrLoaded() const
{
    assert(m_data_core != nullptr);
    return m_data_core->GetCaptureData().IsDiveBlockDataInitialized();
}

absl::Status DataCoreWrapper::LoadGfxrFile(const std::string& original_gfxr_file_path)
{
    assert(m_data_core != nullptr);

    CaptureData::LoadResult load_result = m_data_core->GetMutableCaptureData().LoadGfxrFile(
    original_gfxr_file_path.c_str());
    if (load_result != CaptureData::LoadResult::kSuccess)
    {
        return absl::UnknownError(
        absl::StrFormat("Could not load GFXR file: %s", original_gfxr_file_path));
    }
    return absl::OkStatus();
}

absl::Status DataCoreWrapper::WriteNewGfxrFile(const std::string& new_gfxr_file_path)
{
    assert(m_data_core != nullptr);
    if (!IsGfxrLoaded())
    {
        return absl::FailedPreconditionError("Must load original GFXR first");
    }

    bool write_result = m_data_core->GetMutableCaptureData().WriteModifiedGfxrFile(
    new_gfxr_file_path.c_str());
    if (!write_result)
    {
        return absl::InternalError("Could not write GFXR file");
    }

    return absl::OkStatus();
}

}  // namespace Dive::HostCli
