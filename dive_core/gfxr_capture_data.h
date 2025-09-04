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
#include "dive_core/capture_data.h"
#include "gfxr_ext/decode/dive_annotation_processor.h"
#include "gfxr_ext/decode/dive_block_data.h"

namespace Dive
{

class CommandHierarchy;

//--------------------------------------------------------------------------------------------------
class GfxrCaptureData : public CaptureData
{
public:
    GfxrCaptureData() = default;
    virtual ~GfxrCaptureData() = default;
    GfxrCaptureData &operator=(GfxrCaptureData &&) = default;

    const std::vector<std::unique_ptr<DiveAnnotationProcessor::SubmitInfo>> &GetGfxrSubmits() const;
    const std::vector<DiveAnnotationProcessor::VulkanCommandInfo>           &GetGfxrCommandBuffers(
              uint64_t cmd_handle) const;
    std::vector<uint64_t> GetCommandBufferDrawCallCounts(uint64_t cmd_handle) const;

    // Sets m_cur_capture_file and m_gfxr_capture_block_data with info from the original GFXR file
    LoadResult LoadCaptureFile(const std::string &file_name) override;

    // Get the gfxr data
    bool IsDiveBlockDataInitialized() const { return m_gfxr_capture_block_data != nullptr; }
    std::shared_ptr<gfxrecon::decode::DiveBlockData> GetMutableGfxrData()
    {
        return m_gfxr_capture_block_data;
    }

    // Writes a new GFXR file based on the original file m_cur_capture_file and modifications
    // recorded in m_gfxr_capture_block_data
    bool WriteModifiedGfxrFile(const char *new_file_name);

private:
    // Metadata for the original GFXR file m_cur_capture_file, as well as modifications
    std::shared_ptr<gfxrecon::decode::DiveBlockData> m_gfxr_capture_block_data = nullptr;

    // Vector of SubmitInfo objects used to add the GFXR vulkan commands to the UI.
    std::vector<std::unique_ptr<DiveAnnotationProcessor::SubmitInfo>> m_gfxr_submits;
    std::unordered_map<uint64_t, std::vector<DiveAnnotationProcessor::VulkanCommandInfo>>
                                                        m_gfxr_command_buffers;
    std::unordered_map<uint64_t, std::vector<uint64_t>> m_gfxr_command_buffer_draw_counts;
};

}  // namespace Dive
