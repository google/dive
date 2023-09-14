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

namespace Dive
{
namespace
{
constexpr uint32_t kNumFrameToTrace = 1;
}

TraceManager* GetTraceMgr()
{
#if defined(__ANDROID__)
    static AndroidTraceManager trace_mgr;
#else
    static TraceManager trace_mgr;
#endif
    return &trace_mgr;
}

TraceManager::TraceManager() :
    m_num_frame_to_trace(kNumFrameToTrace)
{
}

}  // namespace Dive