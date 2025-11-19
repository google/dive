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

#include "trace_mgr.h"

#include <string>
#include <thread>

#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "common/log.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"

extern "C"
{
    void SetCaptureState(int state);
    void SetCaptureName(const char* name, const char* frame_num);
}

namespace
{
constexpr auto kTraceFilePath = "/sdcard/Download/";
}

namespace Dive
{

AndroidTraceManager::AndroidTraceManager(absl::Duration trace_duration) :
    m_trace_duration(trace_duration)
{
}

void AndroidTraceManager::TraceByFrame()
{
    std::string num = absl::StrCat(m_frame_num);
    std::string path = absl::StrCat(kTraceFilePath, "trace-frame");
    std::string full_path = absl::StrFormat("%s-%04u.rd", path, m_frame_num);

    SetTraceFilePath(full_path);
    LOGD("Set capture file path as %s", GetTraceFilePath().c_str());
    // We can't give libwrap `full_path` so we expect it to combine `path` and `num` as above.
    SetCaptureName(path.c_str(), num.c_str());
    {
        absl::MutexLock lock(&m_state_lock);
        m_state = TraceState::Triggered;
    }
}

void AndroidTraceManager::TraceByDuration()
{
    m_trace_num++;
    std::string num = absl::StrCat(m_trace_num);
    std::string path = absl::StrCat(kTraceFilePath, "trace");
    std::string full_path = absl::StrFormat("%s-%04u.rd", path, m_trace_num);
    // We can't give libwrap `full_path` so we expect it to combine `path` and `num` as above.
    SetCaptureName(path.c_str(), num.c_str());
    {
        absl::MutexLock lock(&m_state_lock);
        m_state = TraceState::Triggered;
    }
    SetTraceFilePath(std::string(full_path));

    {
        absl::MutexLock lock(&m_state_lock);
        SetCaptureState(1);
        m_state = TraceState::Tracing;
    }
    LOGD("Set capture file path as %s", GetTraceFilePath().c_str());

    absl::SleepFor(m_trace_duration);
    {
        absl::MutexLock lock(&m_state_lock);
        SetCaptureState(0);
        m_state = TraceState::Finished;
    }
}

void AndroidTraceManager::TriggerTrace()
{
    // There are two kinds of traces: by duration, and by frame. If OnNewFrame is called then trace
    // by frame for GetNumFrameToTrace() frames; this function immediately returns and the trace
    // will be collected the next time OnFewFrame is called. Otherwise, trace by duration for
    // `m_trace_duration` time; this function will block until the trace is done.
    if (m_frame_num > 0)
    {
        TraceByFrame();
    }
    else
    {
        TraceByDuration();
    }
}

void AndroidTraceManager::OnNewFrame()
{
    m_frame_num++;
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
#ifndef NDEBUG
    m_state_lock.AssertHeld();
#endif
    return (m_state == TraceState::Triggered);
}

bool AndroidTraceManager::ShouldStopTrace() const
{
#ifndef NDEBUG
    m_state_lock.AssertHeld();
#endif
    return (m_state == TraceState::Tracing &&
            m_frame_num - m_trace_start_frame >= GetNumFrameToTrace());
}

void AndroidTraceManager::OnTraceStart()
{
#ifndef NDEBUG
    m_state_lock.AssertHeld();
#endif
    SetCaptureState(1);
    m_state = TraceState::Tracing;
    LOGI("Triggered at frame %d", m_frame_num);
    m_trace_start_frame = m_frame_num;
}

void AndroidTraceManager::OnTraceStop()
{
#ifndef NDEBUG
    m_state_lock.AssertHeld();
#endif
    SetCaptureState(0);
    m_state = TraceState::Finished;
    LOGI("Finished at frame %d", m_frame_num);
}

}  // namespace Dive