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

#include "dive_capture_data.h"

namespace Dive
{

// =================================================================================================
// DiveCaptureData
// =================================================================================================
DiveCaptureData::DiveCaptureData() :
    m_pm4_capture_data(),
    m_gfxr_capture_data()
{
}

//--------------------------------------------------------------------------------------------------
DiveCaptureData::~DiveCaptureData() {}

//--------------------------------------------------------------------------------------------------
CaptureData::LoadResult DiveCaptureData::LoadFile(const std::string& file_name)
{
    CaptureData::LoadResult result;

    m_gfxr_capture_data = GfxrCaptureData();
    m_pm4_capture_data = Pm4CaptureData(m_progress_tracker);
    result = m_pm4_capture_data.LoadCaptureFile(file_name);
    result = m_gfxr_capture_data.LoadCaptureFile(file_name);
    return result;
}

//--------------------------------------------------------------------------------------------------
CaptureData::LoadResult DiveCaptureData::LoadFiles(const std::string& pm4_file_name,
                                                   const std::string& gfxr_file_name)
{
    // Initialize capture data objects
    m_gfxr_capture_data = GfxrCaptureData();
    m_pm4_capture_data = Pm4CaptureData(m_progress_tracker);

    // 1. Load the PM4 capture file
    CaptureData::LoadResult pm4_result = m_pm4_capture_data.LoadCaptureFile(pm4_file_name);
    if (pm4_result != CaptureData::LoadResult::kSuccess)
    {
        // If the first file fails, stop and return its error code
        return pm4_result;
    }

    // 2. Load the GFXR capture file
    CaptureData::LoadResult gfxr_result = m_gfxr_capture_data.LoadCaptureFile(gfxr_file_name);
    if (gfxr_result != CaptureData::LoadResult::kSuccess)
    {
        // If the second file fails, you might want to clean up the first one if necessary
        // before returning the error.
        return gfxr_result;
    }

    // Both files loaded successfully
    return CaptureData::LoadResult::kSuccess;
}

//--------------------------------------------------------------------------------------------------
const Pm4CaptureData& DiveCaptureData::GetPm4CaptureData() const
{
    return m_pm4_capture_data;
}

//--------------------------------------------------------------------------------------------------
const GfxrCaptureData& DiveCaptureData::GetGfxrCaptureData() const
{
    return m_gfxr_capture_data;
}

}  // namespace Dive
