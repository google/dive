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

#include "device_mgr.h"

#include "absl/status/status.h"
#include "gtest/gtest.h"

namespace Dive
{
namespace
{

TEST(ValidateGfxrReplaySettingsTest, NoLocalDownloadDirFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";

    auto ret = ValidateGfxrReplaySettings(rs, true);
    ASSERT_FALSE(ret.ok());
    EXPECT_EQ(ret.status().code(), absl::StatusCode::kInvalidArgument);
    EXPECT_EQ(ret.status().message(), "Must provide local_download_dir");
}

TEST(ValidateGfxrReplaySettingsTest, NoRemoteCapturePathFail)
{
    GfxrReplaySettings rs = {};
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";

    auto ret = ValidateGfxrReplaySettings(rs, true);
    ASSERT_FALSE(ret.ok());
    EXPECT_EQ(ret.status().code(), absl::StatusCode::kInvalidArgument);
    EXPECT_EQ(ret.status().message(), "Must provide remote_capture_path");
}

TEST(ValidateGfxrReplaySettingsTest, ReplayFlagsStrEqualsSignFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.replay_flags_str = "--loop-single-frame-count=200";

    auto ret = ValidateGfxrReplaySettings(rs, true);
    ASSERT_FALSE(ret.ok());
    EXPECT_EQ(ret.status().code(), absl::StatusCode::kInvalidArgument);
    EXPECT_EQ(ret.status().message(), "replay_flags_str cannot contain '='");
}

TEST(ValidateGfxrReplaySettingsTest, DoubleLoopSingleFrameCountFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.loop_single_frame_count = 200;
    rs.replay_flags_str = "--loop-single-frame-count 200";

    auto ret = ValidateGfxrReplaySettings(rs, true);
    ASSERT_FALSE(ret.ok());
    EXPECT_EQ(ret.status().code(), absl::StatusCode::kInvalidArgument);
    EXPECT_EQ(ret.status().message(),
              "Do not specify loop_single_frame_count in GfxrReplaySettings and also as flag "
              "--loop-single-frame-count");
}

TEST(ValidateGfxrReplaySettingsTest, NonAdrenoDevicePM4DumpFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kPm4Dump;

    auto ret = ValidateGfxrReplaySettings(rs, false);
    ASSERT_FALSE(ret.ok());
    EXPECT_EQ(ret.status().code(), absl::StatusCode::kUnimplemented);
    EXPECT_EQ(ret.status().message(), "Dump PM4 is only implemented for Adreno GPU");
}

TEST(ValidateGfxrReplaySettingsTest, NonAdrenoDevicePerfCountersFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kPerfCounters;

    auto ret = ValidateGfxrReplaySettings(rs, false);
    ASSERT_FALSE(ret.ok());
    EXPECT_EQ(ret.status().code(), absl::StatusCode::kUnimplemented);
    EXPECT_EQ(ret.status().message(), "Perf counters feature is only implemented for Adreno GPU");
}

TEST(ValidateGfxrReplaySettingsTest, NormalDefaultPass)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";

    auto ret = ValidateGfxrReplaySettings(rs, true);
    ASSERT_TRUE(ret.ok()) << ret.status();
    EXPECT_EQ(ret->loop_single_frame_count, 1);
    EXPECT_EQ(ret->replay_flags_str, "--loop-single-frame-count 1");
}

TEST(ValidateGfxrReplaySettingsTest, Pm4DumpDefaultPass)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kPm4Dump;

    auto ret = ValidateGfxrReplaySettings(rs, true);
    ASSERT_TRUE(ret.ok()) << ret.status();
    EXPECT_EQ(ret->loop_single_frame_count, 2);
    EXPECT_EQ(ret->replay_flags_str, "--loop-single-frame-count 2");
}

TEST(ValidateGfxrReplaySettingsTest, PerfCountersDefaultPass)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kPerfCounters;
    rs.metrics = { "PLACEHOLDER_METRICS" };

    auto ret = ValidateGfxrReplaySettings(rs, true);
    ASSERT_TRUE(ret.ok()) << ret.status();
    EXPECT_EQ(ret->loop_single_frame_count, 0);
    EXPECT_EQ(ret->replay_flags_str, "--loop-single-frame-count 0");
}

TEST(ValidateGfxrReplaySettingsTest, PerfCountersNoMetricsFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kPerfCounters;

    auto ret = ValidateGfxrReplaySettings(rs, true);
    ASSERT_FALSE(ret.ok());
    EXPECT_EQ(ret.status().code(), absl::StatusCode::kInvalidArgument);
    EXPECT_EQ(ret.status().message(), "Must provide metrics for kPerfCounters type run");
}

TEST(ValidateGfxrReplaySettingsTest, GpuTimingDefaultPass)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kGpuTiming;

    auto ret = ValidateGfxrReplaySettings(rs, true);
    ASSERT_TRUE(ret.ok()) << ret.status();
    EXPECT_EQ(ret->loop_single_frame_count, 1);
    EXPECT_EQ(ret->replay_flags_str, "--loop-single-frame-count 1 --enable-gpu-time");
}

TEST(ValidateGfxrReplaySettingsTest, ReplayFlagsStrSpacesPass)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.replay_flags_str = "   ";

    auto ret = ValidateGfxrReplaySettings(rs, true);
    ASSERT_TRUE(ret.ok()) << ret.status();
    EXPECT_EQ(ret->loop_single_frame_count, 1);
    EXPECT_EQ(ret->replay_flags_str, "--loop-single-frame-count 1");
}

TEST(ValidateGfxrReplaySettingsTest, FlagToFlagPass)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.replay_flags_str = "--loop-single-frame-count 3 --enable-gpu-time";

    auto ret = ValidateGfxrReplaySettings(rs, true);
    ASSERT_TRUE(ret.ok()) << ret.status();
    EXPECT_EQ(ret->loop_single_frame_count, 3);
    EXPECT_EQ(ret->run_type, GfxrReplayOptions::kGpuTiming);
    EXPECT_EQ(ret->replay_flags_str, "--loop-single-frame-count 3 --enable-gpu-time");
}

TEST(ValidateGfxrReplaySettingsTest, SettingToFlagPass)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kGpuTiming;
    rs.loop_single_frame_count = 3;
    rs.replay_flags_str = "";

    auto ret = ValidateGfxrReplaySettings(rs, true);
    ASSERT_TRUE(ret.ok()) << ret.status();
    EXPECT_EQ(ret->loop_single_frame_count, 3);
    EXPECT_EQ(ret->run_type, GfxrReplayOptions::kGpuTiming);
    EXPECT_EQ(ret->replay_flags_str, "--loop-single-frame-count 3 --enable-gpu-time");
}

TEST(ValidateGfxrReplaySettingsTest, MixFlagsSettingsPass)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.loop_single_frame_count = 3;
    rs.replay_flags_str = "--enable-gpu-time";

    auto ret = ValidateGfxrReplaySettings(rs, true);
    ASSERT_TRUE(ret.ok()) << ret.status();
    EXPECT_EQ(ret->loop_single_frame_count, 3);
    EXPECT_EQ(ret->run_type, GfxrReplayOptions::kGpuTiming);
    EXPECT_EQ(ret->replay_flags_str, "--loop-single-frame-count 3 --enable-gpu-time");
}

TEST(DeviceManagerTest, EmptySerialIsInvalidForSelectDevice)
{
    ASSERT_EQ(DeviceManager().SelectDevice("").status().code(), absl::StatusCode::kInvalidArgument);
}

}  // namespace
}  // namespace Dive
