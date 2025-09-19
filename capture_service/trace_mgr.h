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

#include <cstdint>
#include <string>

#include "absl/base/thread_annotations.h"
#include "absl/synchronization/mutex.h"

namespace Dive
{

enum class TraceState
{
    Idle,
    Triggered,
    Tracing,
    Finished,
    Unknown,
};

class TraceManager
{
public:
    TraceManager();
    virtual void TriggerTrace() {}
    virtual void OnNewFrame() {}
    virtual void WaitForTraceDone() {}

    inline const std::string &GetTraceFilePath() const { return m_trace_file_path; }
    inline void               SetTraceFilePath(std::string trace_file_path)
    {
        m_trace_file_path = std::move(trace_file_path);
    }

    inline uint32_t GetNumFrameToTrace() const { return m_num_frame_to_trace; }
    inline void     SetNumFrameToTrace(uint32_t num_frame_to_trace)
    {
        m_num_frame_to_trace = num_frame_to_trace;
    }

private:
    std::string m_trace_file_path;
    uint32_t    m_num_frame_to_trace;
};

class AndroidTraceManager : public TraceManager
{
public:
    virtual void TriggerTrace() override;
    virtual void OnNewFrame() override;
    virtual void WaitForTraceDone() override;

    TraceState GetState() ABSL_LOCKS_EXCLUDED(m_state_lock)
    {
        absl::MutexLock lock(&m_state_lock);
        return m_state;
    }

private:
    void               TraceByFrame();
    void               TraceByDuration();
    bool               ShouldStartTrace() const;
    bool               ShouldStopTrace() const;
    void               OnTraceStart();
    void               OnTraceStop();
    absl::Mutex        m_state_lock;
    TraceState m_state ABSL_GUARDED_BY(m_state_lock) = TraceState::Idle;
    uint32_t           m_frame_num = 0;
    uint32_t           m_trace_start_frame = 0;
    uint32_t           m_trace_num = 0;
};

TraceManager &GetTraceMgr();

}  // namespace Dive