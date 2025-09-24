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

#include "absl/strings/str_format.h"
#include "trace_mgr.h"

#include <string>
#include <thread>

#include "common/log.h"

// libwrap symbols, used to manipulate libwrap state.
extern "C"
{
    void SetCaptureState(int state);
    void SetCaptureName(const char* name, const char* frame_num);
    int  GetCaptureFilename(char* buffer, int size);
}

namespace Dive
{
namespace
{
// Convert C API to C++. Returns empty string on error.
std::string GetCaptureFilename()
{
    char name[256] = {};
    int  length = ::GetCaptureFilename(name, sizeof(name));
    if (length <= 0)
    {
        return "";
    }
    return std::string(name, length);
}

// Set trace path between libwrap and AndroidTraceManager to the same value.
// libwrap is written in C. When tracing is stopped, it uses the trace path to gather all PM4 trace data.
// AndroidTraceManager is written in C++. It communicates the trace path to the host for download.
// name and num are included in the file name; there are no prescribed semantics.
void SynchronizeTraceFilePath(TraceManager& trace_manager, const char* name, int num)
{
    SetCaptureName(name, std::to_string(num).c_str());

    std::string capture_filename = GetCaptureFilename();
    if (capture_filename.empty())
    {
        // Try the old behavior as fallback, although this likely won't end well.
        capture_filename = absl::StrFormat("/sdcard/Download/%s-%04u.rd", name, num);
    }

    trace_manager.SetTraceFilePath(capture_filename);
    LOGD("Set capture file path as %s", trace_manager.GetTraceFilePath().c_str());
}
}  // namespace

void AndroidTraceManager::TraceByFrame()
{
    SynchronizeTraceFilePath(*this, "trace-frame", m_frame_num);
    {
        absl::MutexLock lock(&m_state_lock);
        m_state = TraceState::Triggered;
    }
}

void AndroidTraceManager::TraceByDuration()
{
    m_trace_num++;
    SynchronizeTraceFilePath(*this, "trace", m_trace_num);
    {
        absl::MutexLock lock(&m_state_lock);
        m_state = TraceState::Triggered;
    }

    {
        absl::MutexLock lock(&m_state_lock);
        SetCaptureState(1);
        m_state = TraceState::Tracing;
    }

    // TODO: pass in this duration in stead of hard code a number.
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    {
        absl::MutexLock lock(&m_state_lock);
        SetCaptureState(0);
        m_state = TraceState::Finished;
    }
}

void AndroidTraceManager::TriggerTrace()
{
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