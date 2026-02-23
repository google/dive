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

#include "absl/log/check.h"
#include "dive_core/capture_data.h"
#include "dive_core/data_core.h"
#include "gfxr_ext/decode/dive_block_data.h"

namespace Dive::HostCli
{

DataCoreWrapper::DataCoreWrapper()
{
    // Initialize DataCore
    m_data_core = std::make_unique<Dive::DataCore>();
}

bool DataCoreWrapper::IsGfxrLoaded() const
{
    CHECK(m_data_core != nullptr) << "data core is null";
    return m_data_core->GetGfxrCaptureData().IsDiveBlockDataInitialized();
}

absl::Status DataCoreWrapper::LoadGfxrFile(const std::filesystem::path& original_gfxr_file_path)
{
    CHECK(m_data_core != nullptr) << "data core is null";

    if (CaptureData::LoadResult res = m_data_core->GetMutableGfxrCaptureData().LoadCaptureFile(
            original_gfxr_file_path.string());
        res != CaptureData::LoadResult::kSuccess)
    {
        return absl::UnknownError(
            absl::StrFormat("Could not load GFXR file: %s", original_gfxr_file_path));
    }
    return absl::OkStatus();
}

absl::Status DataCoreWrapper::WriteNewGfxrFile(const std::filesystem::path& new_gfxr_file_path)
{
    CHECK(m_data_core != nullptr) << "data core is null";
    if (!IsGfxrLoaded())
    {
        return absl::FailedPreconditionError("Must load original GFXR first");
    }

    if (bool write_result = m_data_core->GetMutableGfxrCaptureData().WriteModifiedGfxrFile(
            new_gfxr_file_path.string().c_str());
        !write_result)
    {
        return absl::InternalError("Could not write GFXR file");
    }

    return absl::OkStatus();
}

absl::Status DataCoreWrapper::RemoveGfxrBlocks(std::vector<int> block_ids)
{
    CHECK(m_data_core != nullptr) << "data core is null";
    if (!IsGfxrLoaded())
    {
        return absl::FailedPreconditionError("Must load original GFXR first");
    }
    if (block_ids.empty())
    {
        return absl::FailedPreconditionError("No block_ids to remove");
    }

    std::shared_ptr<gfxrecon::decode::DiveBlockData> dive_block_data =
        m_data_core->GetMutableGfxrCaptureData().GetMutableGfxrData();

    for (const auto& id : block_ids)
    {
        bool res = dive_block_data->AddModification(/*primary_id=*/id, /*secondary_id=*/0,
                                                    /*blob_ptr=*/nullptr);
        if (!res)
        {
            return absl::InternalError(absl::StrFormat("Could not delete block id: %d", id));
        }
    }
    return absl::OkStatus();
}

}  // namespace Dive::HostCli
