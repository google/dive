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
#include <optional>
#include <string>
#include <variant>

#include "absl/base/thread_annotations.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"

namespace Dive
{

// TraceManager is an abstract class, defining a Finite State Machine (FSM) that will switch between
// different TraceStates. The TraceMode of the current trace may affect the transitioning behaviour.
class TraceManager
{
 public:
    enum class TraceState
    {
        Unknown = 0,
        Idle = 1,       // TraceManager is ready to start a new trace
        Triggered = 2,  // A trace has been requested via TriggerTrace()
        Tracing = 3,    // The trace is ongoing
        Finished = 4,   // The trace has finished and the result must be retrieved with
                        // GetFinishedTrace()
    };

    static std::string TraceState2Str(TraceState state);

    enum class TraceMode
    {
        Unknown = 0,
        Duration = 1,  // Captures for a specified duration after the trigger
        Frame = 2,     // Captures for a specified number of frames after the trigger
    };

    static std::string TraceMode2Str(TraceMode mode);

    struct TraceByFrameConfig
    {
        uint32_t total_frames = 1;
    };

    struct TraceByDurationConfig
    {
        absl::Duration trace_duration = absl::Seconds(3);
    };

    struct TraceSettings
    {
        TraceMode mode = TraceMode::Unknown;
        std::variant<std::monostate, TraceByFrameConfig, TraceByDurationConfig> config;
        absl::Time trace_started_time;
        size_t trace_started_frame = 0;
        size_t session_frame_index = 0;
    };

    struct TraceResult
    {
        // Not using std::filepath since the path delimiter of the device could differ from the host
        std::string file_path = "";
    };

    // ------------------------------------------------------------------------
    // Called once-per-frame by applications that are using the Dive layer to communicate with
    // TraceManager. Calling this will set the TraceManager's mode to Frame until the next
    // ResetAppSession()
    void OnNewFrame() ABSL_LOCKS_EXCLUDED(m_state_lock);

    // ------------------------------------------------------------------------
    // Non-blocking, so it should be polled until the return value is error or true, then the
    // finished trace can be checked with GetFinishedTracePath()
    absl::StatusOr<bool> IsTraceFinished() ABSL_LOCKS_EXCLUDED(m_state_lock);

    // ------------------------------------------------------------------------
    // Communication that can be independent of the application's frame loop

    // Indicate to TraceManager to start a trace, either immediately or at the next OnNewFrame()
    absl::Status TriggerTrace() ABSL_LOCKS_EXCLUDED(m_state_lock);

    // Return a Finished trace's on-device path and set the state to Idle
    absl::StatusOr<std::string> GetFinishedTracePath() ABSL_LOCKS_EXCLUDED(m_state_lock);

    void ResetTrace() ABSL_LOCKS_EXCLUDED(m_state_lock);
    void ResetAppSession() ABSL_LOCKS_EXCLUDED(m_state_lock);
    void SetFrameConfig(const TraceByFrameConfig config);
    void SetDurationConfig(const TraceByDurationConfig config);

    // Exposed for testing
    bool IsState(TraceState state) ABSL_LOCKS_EXCLUDED(m_state_lock);
    size_t GetApplicationFrames() const { return m_application_frames; };

 protected:
    // These need to be defined by the derived class to communicate with the device to perform the
    // trace
    virtual absl::Status FromTriggeredToTracing(const TraceSettings& trace_settings) = 0;
    virtual absl::Status FromTracingToFinished() = 0;
    virtual absl::StatusOr<TraceResult> FromFinishedToIdle() = 0;

 private:
    absl::StatusOr<bool> TraceShouldEnd() ABSL_EXCLUSIVE_LOCKS_REQUIRED(m_state_lock);
    absl::Status ProcessState() ABSL_EXCLUSIVE_LOCKS_REQUIRED(m_state_lock);

    mutable absl::Mutex m_state_lock;
    TraceState m_state ABSL_GUARDED_BY(m_state_lock) = TraceState::Idle;

    // Empty when state is Idle
    std::optional<TraceSettings> m_curr_trace_settings ABSL_GUARDED_BY(m_state_lock) = std::nullopt;

    // TraceManager holds multiple mode configs, but only one of these will be snapshotted and used
    // to populate m_curr_trace_settings
    TraceByFrameConfig m_frame_config = {};
    TraceByDurationConfig m_duration_config = {};

    // The TraceManager's mode cannot be set directly but it is intuited from the use of
    // OnNewFrame()
    TraceMode m_mode = TraceMode::Duration;

    // Tracking the current application session
    size_t m_application_frames = 0;
    size_t m_traces_this_session = 0;
};

}  // namespace Dive
