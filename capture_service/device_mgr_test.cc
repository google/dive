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
#include "absl/status/status_matchers.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace Dive
{
namespace
{

using ::absl_testing::IsOkAndHolds;
using ::absl_testing::StatusIs;

MATCHER_P(GfxrReplaySettingsEq, expected, "")
{
    EXPECT_EQ(arg.remote_capture_path, expected.remote_capture_path);
    EXPECT_EQ(arg.local_download_dir, expected.local_download_dir);
    EXPECT_EQ(arg.run_type, expected.run_type);
    EXPECT_EQ(arg.metrics, expected.metrics);
    EXPECT_EQ(arg.loop_single_frame_count, expected.loop_single_frame_count);
    return true;
}

TEST(ValidateGfxrReplaySettingsTest, NoLocalDownloadDirFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";

    absl::StatusOr<GfxrReplaySettings> ret = ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true);
    EXPECT_THAT(ret,
                StatusIs(absl::StatusCode::kInvalidArgument, "Must provide local_download_dir"))
    << ret.status();
}

TEST(ValidateGfxrReplaySettingsTest, NoRemoteCapturePathFail)
{
    GfxrReplaySettings rs = {};
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";

    absl::StatusOr<GfxrReplaySettings> ret = ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true);
    EXPECT_THAT(ret,
                StatusIs(absl::StatusCode::kInvalidArgument, "Must provide remote_capture_path"))
    << ret.status();
}

TEST(ValidateGfxrReplaySettingsTest, ReplayFlagsStrEqualsSignFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.replay_flags_str = "--loop-single-frame-count=200";

    absl::StatusOr<GfxrReplaySettings> ret = ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true);
    EXPECT_THAT(ret,
                StatusIs(absl::StatusCode::kInvalidArgument, "replay_flags_str cannot contain '='"))
    << ret.status();
}

TEST(ValidateGfxrReplaySettingsTest, DoubleLoopSingleFrameCountFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.loop_single_frame_count = 200;
    rs.replay_flags_str = "--loop-single-frame-count 200";

    absl::StatusOr<GfxrReplaySettings> ret = ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true);
    EXPECT_THAT(ret,
                StatusIs(absl::StatusCode::kInvalidArgument,
                         "Do not specify loop_single_frame_count in GfxrReplaySettings and also as "
                         "flag --loop-single-frame-count"))
    << ret.status();
}

TEST(ValidateGfxrReplaySettingsTest, NonAdrenoDevicePM4DumpFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kPm4Dump;

    absl::StatusOr<GfxrReplaySettings> ret = ValidateGfxrReplaySettings(rs,
                                                                        /*is_adreno_gpu=*/false);
    EXPECT_THAT(ret,
                StatusIs(absl::StatusCode::kUnimplemented,
                         "Dump PM4 is only implemented for Adreno GPU"))
    << ret.status();
}

TEST(ValidateGfxrReplaySettingsTest, NonAdrenoDevicePerfCountersFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kPerfCounters;

    absl::StatusOr<GfxrReplaySettings> ret = ValidateGfxrReplaySettings(rs,
                                                                        /*is_adreno_gpu=*/false);
    EXPECT_THAT(ret,
                StatusIs(absl::StatusCode::kUnimplemented,
                         "Perf counters feature is only implemented for Adreno GPU"))
    << ret.status();
}

TEST(ValidateGfxrReplaySettingsTest, NormalDefaultPass)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";

    GfxrReplaySettings expected_rs = {};
    expected_rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    expected_rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";

    absl::StatusOr<GfxrReplaySettings> ret = ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true);
    EXPECT_THAT(ret, IsOkAndHolds(GfxrReplaySettingsEq(expected_rs))) << ret.status();
}

TEST(ValidateGfxrReplaySettingsTest, Pm4DumpDefaultPass)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kPm4Dump;

    GfxrReplaySettings expected_rs = {};
    expected_rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    expected_rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    expected_rs.run_type = GfxrReplayOptions::kPm4Dump;
    expected_rs.loop_single_frame_count = 2;
    expected_rs.replay_flags_str = "--loop-single-frame-count 2";

    absl::StatusOr<GfxrReplaySettings> ret = ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true);
    EXPECT_THAT(ret, IsOkAndHolds(GfxrReplaySettingsEq(expected_rs))) << ret.status();
}

TEST(ValidateGfxrReplaySettingsTest, PerfCountersDefaultPass)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kPerfCounters;
    rs.metrics = { "PLACEHOLDER_METRICS" };

    GfxrReplaySettings expected_rs = {};
    expected_rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    expected_rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    expected_rs.run_type = GfxrReplayOptions::kPerfCounters;
    expected_rs.metrics = { "PLACEHOLDER_METRICS" };
    expected_rs.loop_single_frame_count = 0;
    expected_rs.replay_flags_str = "--loop-single-frame-count 0";

    absl::StatusOr<GfxrReplaySettings> ret = ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true);
    EXPECT_THAT(ret, IsOkAndHolds(GfxrReplaySettingsEq(expected_rs))) << ret.status();
}

TEST(ValidateGfxrReplaySettingsTest, PerfCountersNoMetricsFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kPerfCounters;

    absl::StatusOr<GfxrReplaySettings> ret = ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true);
    EXPECT_THAT(ret,
                StatusIs(absl::StatusCode::kInvalidArgument,
                         "Must provide metrics for kPerfCounters type run"))
    << ret.status();
}

TEST(ValidateGfxrReplaySettingsTest, GpuTimingDefaultPass)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kGpuTiming;

    GfxrReplaySettings expected_rs = {};
    expected_rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    expected_rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    expected_rs.run_type = GfxrReplayOptions::kGpuTiming;
    expected_rs.replay_flags_str = "--enable-gpu-time";

    absl::StatusOr<GfxrReplaySettings> ret = ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true);
    EXPECT_THAT(ret, IsOkAndHolds(GfxrReplaySettingsEq(expected_rs))) << ret.status();
}

TEST(ValidateGfxrReplaySettingsTest, ReplayFlagsStrSpacesPass)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.replay_flags_str = "   ";

    GfxrReplaySettings expected_rs = {};
    expected_rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    expected_rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";

    absl::StatusOr<GfxrReplaySettings> ret = ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true);
    EXPECT_THAT(ret, IsOkAndHolds(GfxrReplaySettingsEq(expected_rs))) << ret.status();
}

TEST(ValidateGfxrReplaySettingsTest, FlagToFlagPass)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.replay_flags_str = "--enable-gpu-time --loop-single-frame-count 3";

    GfxrReplaySettings expected_rs = {};
    expected_rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    expected_rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    expected_rs.run_type = GfxrReplayOptions::kGpuTiming;
    expected_rs.loop_single_frame_count = 3;
    expected_rs.replay_flags_str = "--loop-single-frame-count 3 --enable-gpu-time";

    absl::StatusOr<GfxrReplaySettings> ret = ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true);
    EXPECT_THAT(ret, IsOkAndHolds(GfxrReplaySettingsEq(expected_rs))) << ret.status();
}

TEST(ValidateGfxrReplaySettingsTest, SettingToFlagPass)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kGpuTiming;
    rs.loop_single_frame_count = 3;
    rs.replay_flags_str = "";

    GfxrReplaySettings expected_rs = {};
    expected_rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    expected_rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    expected_rs.run_type = GfxrReplayOptions::kGpuTiming;
    expected_rs.loop_single_frame_count = 3;
    expected_rs.replay_flags_str = "--loop-single-frame-count 3 --enable-gpu-time";

    absl::StatusOr<GfxrReplaySettings> ret = ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true);
    EXPECT_THAT(ret, IsOkAndHolds(GfxrReplaySettingsEq(expected_rs))) << ret.status();
}

TEST(ValidateGfxrReplaySettingsTest, MixFlagsSettingsPass)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.loop_single_frame_count = 3;
    rs.replay_flags_str = "--enable-gpu-time PLACEHOLDER_FLAG_0 PLACEHOLDER_FLAG_1";

    GfxrReplaySettings expected_rs = {};
    expected_rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    expected_rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    expected_rs.run_type = GfxrReplayOptions::kGpuTiming;
    expected_rs.loop_single_frame_count = 3;
    expected_rs
    .replay_flags_str = "PLACEHOLDER_FLAG_0 PLACEHOLDER_FLAG_1 --loop-single-frame-count 3 "
                        "--enable-gpu-time";

    absl::StatusOr<GfxrReplaySettings> ret = ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true);
    EXPECT_THAT(ret, IsOkAndHolds(GfxrReplaySettingsEq(expected_rs))) << ret.status();
}

TEST(ValidateGfxrReplaySettingsTest, LoopSingleFrameCountSpecifiedPm4Fail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kPm4Dump;
    rs.loop_single_frame_count = 2;

    absl::StatusOr<GfxrReplaySettings> ret = ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true);
    EXPECT_THAT(ret,
                StatusIs(absl::StatusCode::kInvalidArgument,
                         "loop_single_frame_count is hardcoded for kPm4Dump, do not specify"))
    << ret.status();
}

TEST(ValidateGfxrReplaySettingsTest, LoopSingleFrameCountSpecifiedPerfCountersFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kPerfCounters;
    rs.loop_single_frame_count = 0;

    absl::StatusOr<GfxrReplaySettings> ret = ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true);
    EXPECT_THAT(ret,
                StatusIs(absl::StatusCode::kInvalidArgument,
                         "loop_single_frame_count is hardcoded for kPerfCounters, do not specify"))
    << ret.status();
}

TEST(ValidateGfxrReplaySettingsTest, LoopSingleFrameCountNegativeFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.loop_single_frame_count = -1;

    absl::StatusOr<GfxrReplaySettings> ret = ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true);
    EXPECT_THAT(ret,
                StatusIs(absl::StatusCode::kInvalidArgument,
                         "loop_single_frame_count must be >0 for kGpuTiming and kNormal runs"))
    << ret.status();
}

TEST(ValidateGfxrReplaySettingsTest, LoopSingleFrameCountStringFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.replay_flags_str = "--loop-single-frame-count PLACEHOLDER";

    absl::StatusOr<GfxrReplaySettings> ret = ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true);
    EXPECT_THAT(ret,
                StatusIs(absl::StatusCode::kInvalidArgument,
                         "Value specified for --loop-single-frame-count can't be parsed as "
                         "integer: invalid stoi argument"))
    << ret.status();
}

TEST(DeviceManagerTest, EmptySerialIsInvalidForSelectDevice)
{
    ASSERT_EQ(DeviceManager().SelectDevice("").status().code(), absl::StatusCode::kInvalidArgument);
}

}  // namespace
}  // namespace Dive
