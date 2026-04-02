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

namespace Dive
{

class TraceManager
{
 public:
    // Useful when the TraceManager receives communication independent of the frame loop, useful for
    // testing.
    virtual void TriggerTrace() {}
    virtual void WaitForTraceDone() {}

    // Useful for when the TraceManager receives communication once per frame, the more common
    // method.
    virtual void OnNewFrame() {}

    inline const std::string& GetTraceFilePath() const { return m_trace_file_path; }
    inline void SetTraceFilePath(std::string trace_file_path)
    {
        m_trace_file_path = std::move(trace_file_path);
    }

 private:
    std::string m_trace_file_path;
};

}  // namespace Dive