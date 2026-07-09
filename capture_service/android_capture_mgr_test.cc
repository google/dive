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

#include "android_capture_mgr.h"

#include "absl/status/status_matchers.h"
#include "absl/strings/str_cat.h"
#include "absl/time/time.h"
#include "capture_service/capture_mgr.h"
#include "dive/utils/device_resources_constants.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

// AndroidCaptureManager uses these functions to talk with libwrap. They must be defined at link
// time. In the future, we might be able to use them to assert state.
extern "C"
{
    void SetCaptureState(int state) {}
    void SetCaptureName(const char* name, const char* frame_num) {}
}

namespace Dive
{
namespace
{

using ::absl_testing::IsOk;
using ::absl_testing::IsOkAndHolds;
using ::absl_testing::StatusIs;

TEST(AndroidCaptureManagerTest, TraceByFrameSuccessful)
{
    AndroidCaptureManager android_trace_manager;
    AndroidCaptureManager::TraceByFrameConfig frame_config;

    frame_config.total_frames = 1;
    android_trace_manager.SetFrameConfig(frame_config);

    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Idle));
    EXPECT_EQ(android_trace_manager.GetApplicationFrames(), 0);

    android_trace_manager.OnNewFrame();
    EXPECT_EQ(android_trace_manager.GetApplicationFrames(), 1);
    // This request to trace will be deferred until the next frame boundary, i.e. frame 2.
    EXPECT_THAT(android_trace_manager.TriggerTrace(), IsOk());
    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Triggered));

    // We should start tracing on this frame boundary
    android_trace_manager.OnNewFrame();
    EXPECT_EQ(android_trace_manager.GetApplicationFrames(), 2);
    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Tracing));

    // We should stop tracing on this frame boundary
    android_trace_manager.OnNewFrame();
    EXPECT_EQ(android_trace_manager.GetApplicationFrames(), 3);
    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Finished));

    android_trace_manager.OnNewFrame();
    EXPECT_EQ(android_trace_manager.GetApplicationFrames(), 4);
    // State stalls at Finished until the user fetches the result with GetFinishedTracePath()
    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Finished));
    // Can detect that a frame-based trace was chosen based on the trace file path prefix.
    std::string expected_trace_result =
        absl::StrCat(Dive::DeviceResourcesConstants::kDeviceDownloadPath, "/trace-frame-0002.rd");
    EXPECT_THAT(android_trace_manager.GetFinishedTracePath(), IsOkAndHolds(expected_trace_result));

    android_trace_manager.OnNewFrame();
    EXPECT_EQ(android_trace_manager.GetApplicationFrames(), 5);
    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Idle));
}

TEST(AndroidCaptureManagerTest, TraceByDurationSuccessful)
{
    AndroidCaptureManager android_trace_manager;
    AndroidCaptureManager::TraceByDurationConfig duration_config;

    duration_config.trace_duration = absl::Milliseconds(1);
    android_trace_manager.SetDurationConfig(duration_config);

    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Idle));

    // Unlike in Frame mode, TriggerTrace() for Duration mode will go through Triggered and end in
    // Tracing state.
    EXPECT_THAT(android_trace_manager.TriggerTrace(), IsOk());
    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Tracing));

    bool finished = false;
    while (!finished)
    {
        absl::StatusOr<bool> res = android_trace_manager.IsTraceFinished();
        EXPECT_THAT(res, IsOk());
        finished = *res;
    }

    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Finished));

    // Can detect trace by duration based on the trace file path prefix.
    std::string expected_trace_result =
        absl::StrCat(Dive::DeviceResourcesConstants::kDeviceDownloadPath, "/trace-0000.rd");
    EXPECT_THAT(android_trace_manager.GetFinishedTracePath(), IsOkAndHolds(expected_trace_result));

    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Idle));
}

TEST(AndroidCaptureManagerTest, TwoFrameTracesInSameSessionSuccessful)
{
    AndroidCaptureManager android_trace_manager;
    AndroidCaptureManager::TraceByFrameConfig frame_config;

    frame_config.total_frames = 1;
    android_trace_manager.SetFrameConfig(frame_config);

    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Idle));
    EXPECT_EQ(android_trace_manager.GetApplicationFrames(), 0);

    android_trace_manager.OnNewFrame();
    EXPECT_EQ(android_trace_manager.GetApplicationFrames(), 1);
    // Trace 1: This request to trace will be deferred until the next frame boundary, i.e. frame 2.
    EXPECT_THAT(android_trace_manager.TriggerTrace(), IsOk());
    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Triggered));

    android_trace_manager.OnNewFrame();
    EXPECT_EQ(android_trace_manager.GetApplicationFrames(), 2);
    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Tracing));

    android_trace_manager.OnNewFrame();
    EXPECT_EQ(android_trace_manager.GetApplicationFrames(), 3);
    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Finished));

    std::string expected_trace_result =
        absl::StrCat(Dive::DeviceResourcesConstants::kDeviceDownloadPath, "/trace-frame-0002.rd");
    EXPECT_THAT(android_trace_manager.GetFinishedTracePath(), IsOkAndHolds(expected_trace_result));

    android_trace_manager.OnNewFrame();
    EXPECT_EQ(android_trace_manager.GetApplicationFrames(), 4);
    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Idle));
    // Trace 2: This request to trace will be deferred until the next frame boundary, i.e. frame 5.
    EXPECT_THAT(android_trace_manager.TriggerTrace(), IsOk());
    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Triggered));

    android_trace_manager.OnNewFrame();
    EXPECT_EQ(android_trace_manager.GetApplicationFrames(), 5);
    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Tracing));

    android_trace_manager.OnNewFrame();
    EXPECT_EQ(android_trace_manager.GetApplicationFrames(), 6);
    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Finished));

    expected_trace_result =
        absl::StrCat(Dive::DeviceResourcesConstants::kDeviceDownloadPath, "/trace-frame-0005.rd");
    EXPECT_THAT(android_trace_manager.GetFinishedTracePath(), IsOkAndHolds(expected_trace_result));

    android_trace_manager.OnNewFrame();
    EXPECT_EQ(android_trace_manager.GetApplicationFrames(), 7);
    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Idle));
}

TEST(AndroidCaptureManagerTest, TwoDurationTracesInSameSessionSuccessful)
{
    AndroidCaptureManager android_trace_manager;
    AndroidCaptureManager::TraceByDurationConfig duration_config;

    duration_config.trace_duration = absl::Milliseconds(1);
    android_trace_manager.SetDurationConfig(duration_config);

    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Idle));

    // Trace 1
    EXPECT_THAT(android_trace_manager.TriggerTrace(), IsOk());
    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Tracing));

    bool finished = false;
    while (!finished)
    {
        absl::StatusOr<bool> res = android_trace_manager.IsTraceFinished();
        EXPECT_THAT(res, IsOk());
        finished = *res;
    }

    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Finished));

    std::string expected_trace_result =
        absl::StrCat(Dive::DeviceResourcesConstants::kDeviceDownloadPath, "/trace-0000.rd");
    EXPECT_THAT(android_trace_manager.GetFinishedTracePath(), IsOkAndHolds(expected_trace_result));

    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Idle));

    // Trace 2
    EXPECT_THAT(android_trace_manager.TriggerTrace(), IsOk());
    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Tracing));

    finished = false;
    while (!finished)
    {
        absl::StatusOr<bool> res = android_trace_manager.IsTraceFinished();
        EXPECT_THAT(res, IsOk());
        finished = *res;
    }

    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Finished));

    expected_trace_result =
        absl::StrCat(Dive::DeviceResourcesConstants::kDeviceDownloadPath, "/trace-0001.rd");
    EXPECT_THAT(android_trace_manager.GetFinishedTracePath(), IsOkAndHolds(expected_trace_result));

    EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Idle));
}

TEST(AndroidCaptureManagerTest, FrameAndDurationTraceInSeparateSessionsSuccessful)
{
    AndroidCaptureManager android_trace_manager;
    {
        AndroidCaptureManager::TraceByFrameConfig frame_config;
        frame_config.total_frames = 1;
        android_trace_manager.SetFrameConfig(frame_config);

        EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Idle));
        EXPECT_EQ(android_trace_manager.GetApplicationFrames(), 0);

        android_trace_manager.OnNewFrame();
        EXPECT_EQ(android_trace_manager.GetApplicationFrames(), 1);
        // Trace 1: Frame Trace
        // This request to trace will be deferred until the next frame boundary, i.e. frame 2.
        EXPECT_THAT(android_trace_manager.TriggerTrace(), IsOk());
        EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Triggered));

        android_trace_manager.OnNewFrame();
        EXPECT_EQ(android_trace_manager.GetApplicationFrames(), 2);
        EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Tracing));

        android_trace_manager.OnNewFrame();
        EXPECT_EQ(android_trace_manager.GetApplicationFrames(), 3);
        EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Finished));

        std::string expected_trace_result = absl::StrCat(
            Dive::DeviceResourcesConstants::kDeviceDownloadPath, "/trace-frame-0002.rd");
        EXPECT_THAT(android_trace_manager.GetFinishedTracePath(),
                    IsOkAndHolds(expected_trace_result));
    }

    android_trace_manager.ResetAppSession();
    EXPECT_EQ(android_trace_manager.GetApplicationFrames(), 0);

    {
        AndroidCaptureManager::TraceByDurationConfig duration_config;
        duration_config.trace_duration = absl::Milliseconds(1);
        android_trace_manager.SetDurationConfig(duration_config);

        EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Idle));

        // Trace 1
        EXPECT_THAT(android_trace_manager.TriggerTrace(), IsOk());
        EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Tracing));

        bool finished = false;
        while (!finished)
        {
            absl::StatusOr<bool> res = android_trace_manager.IsTraceFinished();
            EXPECT_THAT(res, IsOk());
            finished = *res;
        }

        EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Finished));

        std::string expected_trace_result =
            absl::StrCat(Dive::DeviceResourcesConstants::kDeviceDownloadPath, "/trace-0000.rd");
        EXPECT_THAT(android_trace_manager.GetFinishedTracePath(),
                    IsOkAndHolds(expected_trace_result));

        EXPECT_TRUE(android_trace_manager.IsState(AndroidCaptureManager::TraceState::Idle));
    }
}

// TODO: two app sessions, frame followed by duration trace

}  // namespace
}  // namespace Dive
