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

#include "absl/strings/str_cat.h"
#include "perfetto_trace.h"
#include "trace_mgr.h"

#include <string>
#include <thread>

#include "log.h"

extern "C"
{
    void SetCaptureState(int state);
    void SetCaptureName(const char* name, const char* frame_num);
}

namespace
{
const static std::string kTraceFilePath{ "/sdcard/Download/" };
}

namespace Dive
{

void AndroidTraceManager::TraceByFrame()
{
    std::string path = kTraceFilePath + "trace-frame";
    std::string num = std::to_string(m_frame_num);
    char        full_path[256];
    sprintf(full_path, "%s-%04u.rd", path.c_str(), m_frame_num);
    SetCaptureName(path.c_str(), num.c_str());
    {
        absl::MutexLock lock(&m_state_lock);
        m_state = TraceState::Triggered;
    }
    SetTraceFilePath(std::string(full_path));
    LOGD("Set capture file path as %s", GetTraceFilePath().c_str());
    std::string perfetto_path = absl::StrCat(full_path, ".pftrace");
    GetPerfettoMgr().StartNewSession(perfetto_path);
    LOGI("wait for 1 seconds");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

void AndroidTraceManager::TraceByDuration()
{
    m_trace_num++;
    std::string path = kTraceFilePath + "trace";
    std::string num = std::to_string(m_trace_num);
    char        full_path[256];
    sprintf(full_path, "%s-%04u.rd", path.c_str(), m_trace_num);
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
    GetPerfettoMgr().TraceFrame(m_frame_num);
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
            m_frame_num - m_trace_start_frame > GetNumFrameToTrace());
}

void AndroidTraceManager::OnTraceStart()
{
    GetPerfettoMgr().TraceStartFrame();
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
    GetPerfettoMgr().TraceEndFrame();
#ifndef NDEBUG
    m_state_lock.AssertHeld();
#endif
    SetCaptureState(0);
    m_state = TraceState::Finished;
    LOGI("Finished at frame %d", m_frame_num);
}

}  // namespace Dive