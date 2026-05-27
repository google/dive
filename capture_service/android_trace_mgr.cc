/*
Copyright 2023 Google Inc.

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

#include "android_trace_mgr.h"

#include <string>
#include <string_view>

#include "absl/base/log_severity.h"
#include "absl/log/log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "dive/utils/device_resources_constants.h"

extern "C"
{
    void SetCaptureState(int state);
    void SetCaptureName(const char* name, const char* frame_num);
}

namespace Dive
{

namespace
{
static constexpr std::string_view kFrameConfigTracePrefix = "trace-frame";
static constexpr std::string_view kDurationConfigTracePrefix = "trace";
}  // namespace

absl::Status AndroidTraceManager::FromTriggeredToTracing(const TraceSettings& trace_settings)
{
    m_curr_trace_result = {};
    std::string prefix = "";
    size_t suffix_number = 0;

    switch (trace_settings.mode)
    {
        case TraceMode::Duration:
        {
            prefix = kDurationConfigTracePrefix;
            suffix_number = trace_settings.session_frame_index;
            break;
        }
        case TraceMode::Frame:
        {
            prefix = kFrameConfigTracePrefix;
            suffix_number = trace_settings.trace_started_frame;
            break;
        }
        default:
        {
            return absl::FailedPreconditionError(
                absl::StrFormat("Unrecognized TraceMode: %d", trace_settings.mode));
        }
    }

    // This is an Android path so the path separator must be "/" regardless of host platform
    std::string partial_path =
        absl::StrFormat("%s/%s", DeviceResourcesConstants::kDeviceDownloadPath, prefix);
    std::string trace_num_str = absl::StrFormat("%04u", suffix_number);
    std::string full_path = absl::StrFormat("%s-%s.rd", partial_path, trace_num_str);

    m_curr_trace_result.file_path = full_path;
    LOG(INFO) << "Set capture file path in AndroidTraceManager: " << m_curr_trace_result.file_path;

    // Set up libwrap
    // We can't give libwrap `full_path` so we expect it to combine these parts as above.
    SetCaptureName(partial_path.c_str(), trace_num_str.c_str());
    LOG(INFO) << "Passed to libwrap: " << partial_path << ", " << trace_num_str;

    // Tell libwrap to start capture
    SetCaptureState(1);

    return absl::OkStatus();
}

absl::Status AndroidTraceManager::FromTracingToFinished()
{
    // Tell libwrap to end capture
    SetCaptureState(0);
    return absl::OkStatus();
}

absl::StatusOr<TraceManager::TraceResult> AndroidTraceManager::FromFinishedToIdle()
{
    if (m_curr_trace_result.file_path.empty())
    {
        return absl::NotFoundError("Current finished trace is empty");
    }
    return m_curr_trace_result;
}

AndroidTraceManager& GetDefaultAndroidTraceManager()
{
    static AndroidTraceManager android_trace_mgr;
    return android_trace_mgr;
}

}  // namespace Dive
