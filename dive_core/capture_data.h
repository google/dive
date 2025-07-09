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

#include <ostream>

namespace Dive
{

class CaptureData
{
public:
    enum class LoadResult
    {
        kSuccess,
        kFileIoError,
        kCorruptData,
        kVersionError
    };
    virtual ~CaptureData() = default;
    virtual LoadResult LoadCaptureFile(const char* file_name) = 0;

protected:
    std::string m_cur_capture_file;
};

// Inline function used to include CaptureData::LoadResult in a std::cerr log
inline std::ostream& operator<<(std::ostream& os, const CaptureData::LoadResult& r)
{
    switch (r)
    {
    case CaptureData::LoadResult::kSuccess:
        os << "Success";
        break;
    case CaptureData::LoadResult::kFileIoError:
        os << "File IO Error";
        break;
    case CaptureData::LoadResult::kCorruptData:
        os << "Corrupt Data";
        break;
    case CaptureData::LoadResult::kVersionError:
        os << "Version Error";
        break;
    default:
        os << "Unknown";
        break;
    }
    return os;
}

}  // namespace Dive
