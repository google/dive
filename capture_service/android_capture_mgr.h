/*
Copyright 2026 Google Inc.

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

#include <chrono>
#include <cstdint>
#include <string>

#include "absl/base/thread_annotations.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"
#include "capture_service/capture_mgr.h"

namespace Dive
{

class AndroidCaptureManager : public CaptureManager
{
 private:
    // Sets up CaptureManager and libwrap with the appropriate trace name and/or path in prepration
    // for triggering the PM4 capture
    // Name syntax examples:
    //   - trace-frame-0003.rd : Frame-based trace, initiated at application frame 3
    //   - trace-0005.rd: Duration based trace, 5th taken in this Dive session
    virtual absl::Status FromTriggeredToTracing(const TraceSettings& trace_settings) override;
    virtual absl::Status FromTracingToFinished() override;
    virtual absl::StatusOr<CaptureManager::TraceResult> FromFinishedToIdle() override;

    CaptureManager::TraceResult m_curr_trace_result = {};
};

AndroidCaptureManager& GetDefaultAndroidCaptureManager();

}  // namespace Dive
