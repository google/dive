/*
Copyright 2025 Google Inc.

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

#include "gtest/gtest.h"

// AndroidTraceManager uses these functions to talk with libwrap. They must be defined at link time.
// In the future, we might be able to use them to assert state.
extern "C"
{
    void SetCaptureState(int state) {}
    void SetCaptureName(const char* name, const char* frame_num) {}
}

namespace Dive
{
namespace
{

TEST(AndroidTraceManagerTest, OnFrameBoundaryDetectedTraceByFrameForOneFrame)
{
    AndroidTraceManager android_trace_manager;
    android_trace_manager.SetNumFrameToTrace(1);
    EXPECT_EQ(android_trace_manager.GetState(), TraceState::Idle);

    // Calling TriggerTrace now would cause a duration-based trace since no frame boundaries have
    // been detected. Provide a frame boundary so we use a frame-based trace.
    android_trace_manager.OnNewFrame();

    // This request to trace will be deferred until the next frame boundary.
    android_trace_manager.TriggerTrace();
    EXPECT_EQ(android_trace_manager.GetState(), TraceState::Triggered);
    // Can detect that a frame-based trace was chosen based on the trace file path. Duration-based
    // is trace-XXXX.rd instead.
    EXPECT_EQ(android_trace_manager.GetTraceFilePath(), "/sdcard/Download/trace-frame-0001.rd");

    // We should start tracing on this frame boundary
    android_trace_manager.OnNewFrame();
    EXPECT_EQ(android_trace_manager.GetState(), TraceState::Tracing);

    // We should stop tracing on this frame boundary
    android_trace_manager.OnNewFrame();
    EXPECT_EQ(android_trace_manager.GetState(), TraceState::Finished);
}

}  // namespace
}  // namespace Dive