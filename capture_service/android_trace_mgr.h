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

#pragma once

#include <chrono>
#include <cstdint>
#include <string>

#include "absl/base/thread_annotations.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"
#include "capture_service/trace_mgr.h"

namespace Dive
{

class AndroidTraceManager : public TraceManager
{
 public:
    enum class TraceState
    {
        Idle,
        Triggered,
        Tracing,
        Finished,
        Unknown,
    };

    enum class TraceType
    {
        Unknown,
        Frame,
        Duration,
    };

    struct TraceByFrameConfig
    {
        uint32_t total_frames = 1;
    };

    struct TraceByDurationConfig
    {
        absl::Duration trace_duration = absl::Seconds(3);
    };

    explicit AndroidTraceManager(const TraceByFrameConfig& frame_config);
    explicit AndroidTraceManager(const TraceByDurationConfig& duration_config);

    void TriggerTrace() override;
    void OnNewFrame() override;
    void WaitForTraceDone() override;

    TraceState GetState() ABSL_LOCKS_EXCLUDED(m_state_lock)
    {
        absl::MutexLock lock(&m_state_lock);
        return m_state;
    }

 private:
    void TraceByFrame();
    void TraceByDuration();
    bool ShouldStartTrace() const;
    bool ShouldStopTrace() const;
    void OnTraceStart();
    void OnTraceStop();

    // Sets up TraceManager and libwrap with the appropriate trace name and/or path in prepration
    // for triggering the PM4 capture
    void SetupTraceName(std::string_view trace_name, uint64_t trace_num);

    absl::Mutex m_state_lock;
    TraceState m_state ABSL_GUARDED_BY(m_state_lock) = TraceState::Idle;

    TraceType m_trace_type = TraceType::Unknown;

    // Counters
    uint32_t m_curr_frame = 0;
    uint32_t m_trace_started_frame = 0;

    // Only used when m_trace_type is Frame
    TraceByFrameConfig m_frame_config;

    // Only used when m_trace_type is Duration
    TraceByDurationConfig m_duration_config;
};

AndroidTraceManager& GetDefaultFrameConfigAndroidTraceManager();

}  // namespace Dive