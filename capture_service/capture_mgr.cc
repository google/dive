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

#include "capture_mgr.h"

#include "absl/base/log_severity.h"
#include "absl/base/thread_annotations.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"

namespace Dive
{

std::string CaptureManager::TraceState2Str(CaptureManager::TraceState state)
{
    switch (state)
    {
        case TraceState::Unknown:
            return "Unknown";
        case TraceState::Idle:
            return "Idle";
        case TraceState::Triggered:
            return "Triggered";
        case TraceState::Tracing:
            return "Tracing";
        case TraceState::Finished:
            return "Finished";
        default:
            return "INVALID";
    }
}

std::string CaptureManager::TraceMode2Str(CaptureManager::TraceMode mode)
{
    switch (mode)
    {
        case TraceMode::Unknown:
            return "Unknown";
        case TraceMode::Duration:
            return "Duration";
        case TraceMode::Frame:
            return "Frame";
        default:
            return "INVALID";
    }
}

void CaptureManager::OnNewFrame() ABSL_LOCKS_EXCLUDED(m_state_lock)
{
    absl::MutexLock lock(&m_state_lock);
    m_application_frames++;

    m_mode = TraceMode::Frame;

    if (absl::Status res = ProcessState(); !res.ok())
    {
        LOG(ERROR) << res.message();
    }
}

absl::Status CaptureManager::TriggerTrace() ABSL_LOCKS_EXCLUDED(m_state_lock)
{
    absl::MutexLock lock(&m_state_lock);
    if (m_state != TraceState::Idle)
    {
        return absl::FailedPreconditionError(absl::StrFormat(
            "Can only trigger trace when TraceManger is Idle, not TraceState: %s. Try ResetTrace()",
            TraceState2Str(m_state)));
    }

    m_state = TraceState::Triggered;

    LOG(INFO) << "Successfully triggered a trace";

    if (m_mode == TraceMode::Frame)
    {
        return absl::OkStatus();
    }

    if (absl::Status res = ProcessState(); !res.ok())
    {
        return res;
    }
    return absl::OkStatus();
}

absl::StatusOr<bool> CaptureManager::IsTraceFinished() ABSL_LOCKS_EXCLUDED(m_state_lock)
{
    absl::MutexLock lock(&m_state_lock);

    if ((m_state == TraceState::Idle) || (m_state == TraceState::Unknown))
    {
        return absl::FailedPreconditionError(absl::StrFormat(
            "Invalid TraceState, waiting won't make it more Finished, current TraceState: %s",
            TraceState2Str(m_state)));
    }

    if (!m_curr_trace_settings.has_value())
    {
        if ((m_state == TraceState::Tracing) || (m_state == TraceState::Finished))
        {
            return absl::FailedPreconditionError(
                "m_curr_trace_settings cannot be empty when the state is Tracing/Finished");
        }
        if (m_state == TraceState::Triggered)
        {
            // Edge case for a Frame mode trace, where IsTraceFinished() was called between
            // TriggerTrace() and the next OnNewFrame()
            return false;
        }
    }

    switch (m_curr_trace_settings->mode)
    {
        case TraceMode::Frame:
        {
            return m_state == TraceState::Finished;
        }
        case TraceMode::Duration:
        {
            if (m_state == TraceState::Finished)
            {
                return true;
            }
            if (absl::Status res = ProcessState(); !res.ok())
            {
                m_state = TraceState::Unknown;
                return res;
            }
            return false;
        }
        default:
        {
            return absl::FailedPreconditionError(absl::StrFormat(
                "Unrecognized current TraceMode: %s", TraceMode2Str(m_curr_trace_settings->mode)));
        }
    }
}

absl::StatusOr<std::string> CaptureManager::GetFinishedTracePath() ABSL_LOCKS_EXCLUDED(m_state_lock)
{
    absl::MutexLock lock(&m_state_lock);

    if (m_state != TraceState::Finished)
    {
        return absl::FailedPreconditionError(
            absl::StrFormat("Can only GetFinishedTracePath() when TraceManger is Finished, not "
                            "TraceState: %s. Try WaitTraceFinished() or ResetTrace()",
                            TraceState2Str(m_state)));
    }

    absl::StatusOr<TraceResult> trace_result = FromFinishedToIdle();
    if (!trace_result.ok())
    {
        m_state = TraceState::Unknown;
        return trace_result.status();
    }

    LOG(INFO) << "On-device trace path: " << trace_result->file_path;

    m_traces_this_session++;
    m_curr_trace_settings = std::nullopt;
    m_state = TraceState::Idle;

    return trace_result->file_path;
}

void CaptureManager::ResetTrace() ABSL_LOCKS_EXCLUDED(m_state_lock)
{
    absl::MutexLock lock(&m_state_lock);
    LOG(INFO) << "CaptureManager::ResetTrace()";
    m_curr_trace_settings = std::nullopt;
    m_state = TraceState::Idle;

    // We assume the same application is still running, so no reset for application-level members
}

void CaptureManager::ResetAppSession() ABSL_LOCKS_EXCLUDED(m_state_lock)
{
    absl::MutexLock lock(&m_state_lock);
    LOG(INFO) << "CaptureManager::ResetAppSession()";
    m_curr_trace_settings = std::nullopt;  // Otherwise "started" parameters here can be invalid
    m_state = TraceState::Idle;

    // Reset application
    m_traces_this_session = 0;
    m_application_frames = 0;
    m_mode = TraceMode::Duration;
}

void CaptureManager::SetFrameConfig(const CaptureManager::TraceByFrameConfig config)
{
    m_frame_config = config;
}

void CaptureManager::SetDurationConfig(const CaptureManager::TraceByDurationConfig config)
{
    m_duration_config = config;
}

bool CaptureManager::IsState(TraceState state) ABSL_LOCKS_EXCLUDED(m_state_lock)
{
    absl::MutexLock lock(&m_state_lock);
    if (state != m_state)
    {
        LOG(WARNING) << "Current state (" << TraceState2Str(m_state) << ") is not the expected ("
                     << TraceState2Str(state) << ")";
        return false;
    }
    return true;
}

absl::StatusOr<bool> CaptureManager::TraceShouldEnd() ABSL_EXCLUSIVE_LOCKS_REQUIRED(m_state_lock)
{
    if (m_state != TraceState::Tracing)
    {
        return absl::FailedPreconditionError(
            absl::StrFormat("Can only check TraceShouldEnd() when TraceManger is Tracing, not "
                            "TraceState: %s. Try ResetTrace()",
                            TraceState2Str(m_state)));
    }

    if (!m_curr_trace_settings.has_value())
    {
        return absl::FailedPreconditionError(
            "m_curr_trace_settings cannot be empty when the state is Tracing");
    }

    switch (m_curr_trace_settings->mode)
    {
        case TraceMode::Frame:
        {
            size_t desired_frames =
                std::get<TraceByFrameConfig>(m_curr_trace_settings->config).total_frames;
            if (m_application_frames - m_curr_trace_settings->trace_started_frame >= desired_frames)
            {
                return true;
            }
            return false;
        }
        case TraceMode::Duration:
        {
            absl::Duration desired_duration =
                std::get<TraceByDurationConfig>(m_curr_trace_settings->config).trace_duration;
            if (absl::Now() - m_curr_trace_settings->trace_started_time >= desired_duration)
            {
                return true;
            }
            return false;
        }
        default:
        {
            return absl::FailedPreconditionError(absl::StrFormat(
                "Unrecognized current TraceMode: %s", TraceMode2Str(m_curr_trace_settings->mode)));
        }
    }
}

absl::Status CaptureManager::ProcessState() ABSL_EXCLUSIVE_LOCKS_REQUIRED(m_state_lock)
{
    switch (m_state)
    {
        case TraceState::Triggered:
        {
            TraceSettings trace_settings;
            trace_settings.mode = m_mode;
            trace_settings.session_frame_index = m_traces_this_session;
            switch (trace_settings.mode)
            {
                case TraceMode::Duration:
                {
                    trace_settings.config = m_duration_config;
                    break;
                }
                case TraceMode::Frame:
                {
                    trace_settings.config = m_frame_config;
                    break;
                }
                default:
                {
                    m_state = TraceState::Unknown;
                    return absl::FailedPreconditionError(absl::StrFormat(
                        "Invalid TraceMode: %s", TraceMode2Str(trace_settings.mode)));
                }
            }
            trace_settings.trace_started_time = absl::Now();
            trace_settings.trace_started_frame = m_application_frames;

            if (absl::Status res = FromTriggeredToTracing(trace_settings); !res.ok())
            {
                m_state = TraceState::Unknown;
                return res;
            }
            m_curr_trace_settings = trace_settings;
            m_state = TraceState::Tracing;

            break;
        }
        case TraceState::Tracing:
        {
            absl::StatusOr<bool> should_end = TraceShouldEnd();
            if (!should_end.ok())
            {
                return should_end.status();
            }

            if (*should_end)
            {
                if (absl::Status res = FromTracingToFinished(); !res.ok())
                {
                    m_state = TraceState::Unknown;
                    return res;
                }
                m_state = TraceState::Finished;
            }

            break;
        }
        default:
        {
            break;
        }
    }
    return absl::OkStatus();
}

}  // namespace Dive
