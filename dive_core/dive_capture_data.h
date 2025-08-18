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
#include "dive_core/gfxr_capture_data.h"
#include "dive_core/pm4_capture_data.h"

namespace Dive
{

class CommandHierarchy;

//--------------------------------------------------------------------------------------------------
class DiveCaptureData
{
public:
    DiveCaptureData();
    DiveCaptureData(Pm4CaptureData m_pm4_capture_data, GfxrCaptureData m_gfxr_capture_data);
    virtual ~DiveCaptureData();
    DiveCaptureData &operator=(DiveCaptureData &&) = default;

    CaptureData::LoadResult LoadFile(const std::string &file_name);
    CaptureData::LoadResult LoadFiles(const std::string &pm4_file_name,
                                      const std::string &gfxr_file_name);
    const Pm4CaptureData   &GetPm4CaptureData() const;
    const GfxrCaptureData  &GetGfxrCaptureData() const;

private:
    CaptureData::LoadResult LoadCaptureFileStream(std::istream &capture_file);
    CaptureData::LoadResult LoadDiveFile(const std::string &file_name);
    ProgressTracker        *m_progress_tracker;
    Pm4CaptureData          m_pm4_capture_data;
    GfxrCaptureData         m_gfxr_capture_data;
};

}  // namespace Dive
