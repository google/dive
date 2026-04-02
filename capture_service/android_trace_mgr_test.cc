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

#include "android_trace_mgr.h"

#include "absl/time/time.h"
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
    AndroidTraceManager::TraceByFrameConfig config = {};
    config.total_frames = 1;
    AndroidTraceManager android_trace_manager(config);

    EXPECT_EQ(android_trace_manager.GetState(), AndroidTraceManager::TraceState::Idle);

    // Simulating frame loop 0
    android_trace_manager.OnNewFrame();

    // This request to trace will be deferred until the next frame boundary.
    android_trace_manager.TriggerTrace();
    EXPECT_EQ(android_trace_manager.GetState(), AndroidTraceManager::TraceState::Triggered);
    // Can detect that a frame-based trace was chosen based on the trace file path. Duration-based
    // is trace-XXXX.rd instead.
    EXPECT_EQ(android_trace_manager.GetTraceFilePath(), "/sdcard/Download/trace-frame-0001.rd");

    // Simulating frame loop 1
    // We should start tracing on this frame boundary
    android_trace_manager.OnNewFrame();
    EXPECT_EQ(android_trace_manager.GetState(), AndroidTraceManager::TraceState::Tracing);

    // Simulating frame loop 2
    // We should stop tracing on this frame boundary
    android_trace_manager.OnNewFrame();
    EXPECT_EQ(android_trace_manager.GetState(), AndroidTraceManager::TraceState::Finished);
}

TEST(AndroidTraceManagerTest, TraceByDurationTransistionsToFinishAndUsesByDurationFileName)
{
    // TODO: b/462154186 - Never sleep in tests.
    AndroidTraceManager::TraceByDurationConfig config = {};
    config.trace_duration = absl::Milliseconds(1);
    AndroidTraceManager android_trace_manager(config);
    EXPECT_EQ(android_trace_manager.GetState(), AndroidTraceManager::TraceState::Idle);

    android_trace_manager.TriggerTrace();

    // Unlike trace by frame, all states transitions occur during TriggerTrace.
    EXPECT_EQ(android_trace_manager.GetState(), AndroidTraceManager::TraceState::Finished);
    // Can detect trace by duration based on the trace file path.
    EXPECT_EQ(android_trace_manager.GetTraceFilePath(), "/sdcard/Download/trace-0000.rd");
}

}  // namespace
}  // namespace Dive