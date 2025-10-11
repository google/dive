/*
Copyright 2025 Google Inc.

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

namespace Dive
{

// These functions are inline since it's difficult to share libraries between capture_service and
// gfxr_ext with the current CMake structure. The fucntions are short enough so inlining should work
// well anyway.

// Get the filename prefix that we want RenderDoc to use when creating a capture file. For the
// actual filename, use GetRenderDocCaptureFilePath. The returned path is designed to be used in
// portable generic format since that's the native format of the target device.
inline std::filesystem::path GetRenderDocCaptureFilePathTemplate(
const std::filesystem::path& gfxr_filepath)
{
    return std::filesystem::path(kDeviceCapturePath) / gfxr_filepath.stem();
}

// Get the expected filename of the completed RenderDoc capture. The returned path is designed to
// be used in portable generic format since that's the native format of the target device.
inline std::filesystem::path GetRenderDocCaptureFilePath(const std::filesystem::path& gfxr_filepath)
{
    // This is predicated on the current implementation of RenderDoc::CreateRDC() and the use of
    // StartFrameCapture. If either of these change then this won't work. Ideally, replay would
    // just tell us where the file is instead of us having to guess.
    return GetRenderDocCaptureFilePathTemplate(gfxr_filepath).concat(kRenderDocRdcSuffix);
}

}  // namespace Dive