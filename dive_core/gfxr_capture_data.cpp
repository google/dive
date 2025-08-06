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

#include "gfxr_capture_data.h"

#include <iostream>
#include "dive_core/common/common.h"
#include "generated/generated_vulkan_dive_consumer.h"
#include "gfxr_ext/decode/dive_file_processor.h"
#include "third_party/gfxreconstruct/framework/generated/generated_vulkan_decoder.h"

namespace Dive
{
// =================================================================================================
// GfxrCaptureData
// =================================================================================================

//--------------------------------------------------------------------------------------------------
CaptureData::LoadResult GfxrCaptureData::LoadCaptureFile(const char *file_name)
{
    if (m_gfxr_capture_block_data != nullptr)
    {
        std::cerr << "Error: cannot load another gfxr file with one currently stored: " << file_name
                  << std::endl;
        return LoadResult::kFileIoError;
    }

    m_gfxr_capture_block_data = std::make_shared<gfxrecon::decode::DiveBlockData>();

    gfxrecon::decode::DiveFileProcessor file_processor;

    if (!file_processor.Initialize(file_name))
    {
        return LoadResult::kFileIoError;
    }

    file_processor.SetLoopSingleFrameCount(1);
    file_processor.SetDiveBlockData(m_gfxr_capture_block_data);

    gfxrecon::decode::VulkanExportDiveConsumer dive_consumer;
    gfxrecon::decode::VulkanDecoder            decoder;
    decoder.AddConsumer(&dive_consumer);
    file_processor.AddDecoder(&decoder);

    DiveAnnotationProcessor dive_annotation_processor;
    file_processor.SetAnnotationProcessor(&dive_annotation_processor);
    dive_consumer.Initialize(&dive_annotation_processor);

    if (!file_processor.ProcessAllFrames())
    {
        std::cerr << "Error using gfxrecon DiveFileProcessor to load file: " << file_name
                  << std::endl;
        std::cerr << file_processor.GetErrorState() << std::endl;
        return LoadResult::kFileIoError;
    }

    m_gfxr_submits = dive_annotation_processor.getSubmits();

    if (!m_gfxr_capture_block_data->FinalizeOriginalBlocksMapSizes())
    {
        std::cerr << "Error: cannot lock gfxrecon DiveBlockData" << std::endl;
        return LoadResult::kFileIoError;
    }

    m_cur_capture_file = file_name;

    return LoadResult::kSuccess;
}

//--------------------------------------------------------------------------------------------------
bool GfxrCaptureData::WriteModifiedGfxrFile(const char *new_file_name)
{
    if (m_cur_capture_file.empty())
    {
        std::cerr << "Error: no loaded gfxr file" << std::endl;
        return false;
    }

    if (!m_gfxr_capture_block_data->WriteGFXRFile(m_cur_capture_file, new_file_name))
    {
        std::cerr << "Error writing modified GFXR file" << std::endl;
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
const std::vector<std::unique_ptr<DiveAnnotationProcessor::SubmitInfo>> &
GfxrCaptureData::GetGfxrSubmits() const
{
    return std::move(m_gfxr_submits);
}

}  // namespace Dive
