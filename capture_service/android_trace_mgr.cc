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

AndroidTraceManager::AndroidTraceManager(const TraceByFrameConfig& frame_config)
    : m_frame_config(frame_config), m_trace_type(TraceType::Frame)
{
}

AndroidTraceManager::AndroidTraceManager(const TraceByDurationConfig& duration_config)
    : m_duration_config(duration_config), m_trace_type(TraceType::Duration)
{
}

void AndroidTraceManager::SetupTraceName(std::string_view trace_name, uint64_t trace_num)
{
    // This is an Android path so the path separator must be "/"
    std::string partial_path =
        absl::StrFormat("%s/%s", DeviceResourcesConstants::kDeviceDownloadPath, trace_name);
    std::string trace_num_str = absl::StrFormat("%04u", trace_num);
    std::string full_path = absl::StrFormat("%s-%s.rd", partial_path, trace_num_str);

    // Set up TraceManager
    SetTraceFilePath(full_path);
    LOG(INFO) << "Set capture file path in TraceManager: " << GetTraceFilePath();

    // Set up libwrap
    // We can't give libwrap `full_path` so we expect it to combine these parts as above.
    SetCaptureName(partial_path.c_str(), trace_num_str.c_str());
    LOG(INFO) << "Passed to libwrap: " << partial_path << ", " << trace_num_str;
}

void AndroidTraceManager::TraceByFrame()
{
    // Legacy naming prefix
    SetupTraceName("trace-frame", m_curr_frame);

    {
        absl::MutexLock lock(&m_state_lock);
        m_state = TraceState::Triggered;
    }
}

void AndroidTraceManager::TraceByDuration()
{
    // Legacy naming prefix
    SetupTraceName("trace", m_curr_frame);

    {
        absl::MutexLock lock(&m_state_lock);
        m_state = TraceState::Triggered;
    }

    {
        absl::MutexLock lock(&m_state_lock);
        SetCaptureState(1);
        m_state = TraceState::Tracing;
    }

    m_trace_started_frame = m_curr_frame;

    absl::SleepFor(m_duration_config.trace_duration);
    {
        absl::MutexLock lock(&m_state_lock);
        SetCaptureState(0);
        m_state = TraceState::Finished;
    }
}

void AndroidTraceManager::TriggerTrace()
{
    switch (m_trace_type)
    {
        case TraceType::Frame:
        {
            // Will trigger trace and immediately return, and the trace will be collected the next
            // time OnFewFrame() is called.
            TraceByFrame();
            return;
        }
        case TraceType::Duration:
        {
            // Will trigger trace and block until the trace is done.
            TraceByDuration();
            return;
        }
        default:
        {
            LOG(ERROR) << "Unrecognized TraceType: " << static_cast<int>(m_trace_type);
            return;
        }
    }
}

void AndroidTraceManager::OnNewFrame()
{
    if (m_trace_type != TraceType::Frame)
    {
        LOG(ERROR)
            << "AndroidTraceManager::OnNewFrame() should only be called for m_trace_type Frame";
        return;
    }

    m_curr_frame++;
    absl::MutexLock lock(&m_state_lock);
    if (ShouldStartTrace())
    {
        OnTraceStart();
    }
    else if (ShouldStopTrace())
    {
        OnTraceStop();
    }
}

void AndroidTraceManager::WaitForTraceDone()
{
    // TODO(renfeng): add timeout.
    m_state_lock.Lock();
    auto capture_done = [this] { return m_state == TraceState::Finished; };
    m_state_lock.Await(absl::Condition(&capture_done));
    m_state_lock.Unlock();
}

bool AndroidTraceManager::ShouldStartTrace() const
{
    if (m_trace_type != TraceType::Frame)
    {
        LOG(ERROR) << "AndroidTraceManager::ShouldStartTrace() should only be called for "
                      "m_trace_type Frame";
        return false;
    }

#ifndef NDEBUG
    m_state_lock.AssertHeld();
#endif
    return (m_state == TraceState::Triggered);
}

bool AndroidTraceManager::ShouldStopTrace() const
{
    if (m_trace_type != TraceType::Frame)
    {
        LOG(ERROR) << "AndroidTraceManager::ShouldStopTrace() should only be called for "
                      "m_trace_type Frame";
        return false;
    }

#ifndef NDEBUG
    m_state_lock.AssertHeld();
#endif
    return ((m_state == TraceState::Tracing) &&
            (m_curr_frame - m_trace_started_frame >= m_frame_config.total_frames));
}

void AndroidTraceManager::OnTraceStart()
{
    if (m_trace_type != TraceType::Frame)
    {
        LOG(ERROR)
            << "AndroidTraceManager::OnTraceStart() should only be called for m_trace_type Frame";
        return;
    }

#ifndef NDEBUG
    m_state_lock.AssertHeld();
#endif
    SetCaptureState(1);
    m_state = TraceState::Tracing;
    m_trace_started_frame = m_curr_frame;
    LOG(INFO) << "Trace triggered at frame: " << m_trace_started_frame;
}

void AndroidTraceManager::OnTraceStop()
{
    if (m_trace_type != TraceType::Frame)
    {
        LOG(ERROR)
            << "AndroidTraceManager::OnTraceStop() should only be called for m_trace_type Frame";
        return;
    }

#ifndef NDEBUG
    m_state_lock.AssertHeld();
#endif
    SetCaptureState(0);
    m_state = TraceState::Finished;
    LOG(INFO) << "Trace finished at frame: " << m_curr_frame;
    LOG(INFO) << "Trace elapsed n frames: " << m_curr_frame - m_trace_started_frame;
}

AndroidTraceManager& GetDefaultFrameConfigAndroidTraceManager()
{
    AndroidTraceManager::TraceByFrameConfig config = {};
    static AndroidTraceManager trace_mgr(config);
    return trace_mgr;
}

}  // namespace Dive