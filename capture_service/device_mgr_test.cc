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
    EXPECT_EQ(arg.replay_flags_str, expected.replay_flags_str);
    EXPECT_EQ(arg.metrics, expected.metrics);
    EXPECT_EQ(arg.loop_single_frame_count, expected.loop_single_frame_count);
    return true;
}

TEST(ValidateGfxrReplaySettingsTest, NoLocalDownloadDirFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true).status(),
                StatusIs(absl::StatusCode::kInvalidArgument, "Must provide local_download_dir"));
}

TEST(ValidateGfxrReplaySettingsTest, NoRemoteCapturePathFail)
{
    GfxrReplaySettings rs = {};
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true).status(),
                StatusIs(absl::StatusCode::kInvalidArgument, "Must provide remote_capture_path"));
}

TEST(ValidateGfxrReplaySettingsTest, ReplayFlagsStrEqualsSignFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.replay_flags_str = "--loop-single-frame-count=200";

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true).status(),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         "replay_flags_str cannot contain '='"));
}

TEST(ValidateGfxrReplaySettingsTest, DoubleLoopSingleFrameCountFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.loop_single_frame_count = 200;
    rs.replay_flags_str = "--loop-single-frame-count 200";

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true).status(),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         "Do not specify loop_single_frame_count in GfxrReplaySettings and also as "
                         "flag --loop-single-frame-count"));
}

TEST(ValidateGfxrReplaySettingsTest, NonAdrenoDevicePM4DumpFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kPm4Dump;

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/false).status(),
                StatusIs(absl::StatusCode::kUnimplemented,
                         "Dump PM4 is only implemented for Adreno GPU"));
}

TEST(ValidateGfxrReplaySettingsTest, NonAdrenoDevicePerfCountersFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kPerfCounters;

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/false).status(),
                StatusIs(absl::StatusCode::kUnimplemented,
                         "Perf counters feature is only implemented for Adreno GPU"));
}

TEST(ValidateGfxrReplaySettingsTest, NormalDefaultPass)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";

    GfxrReplaySettings expected_rs = {};
    expected_rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    expected_rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true),
                IsOkAndHolds(GfxrReplaySettingsEq(expected_rs)));
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

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true),
                IsOkAndHolds(GfxrReplaySettingsEq(expected_rs)));
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

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true),
                IsOkAndHolds(GfxrReplaySettingsEq(expected_rs)));
}

TEST(ValidateGfxrReplaySettingsTest, PerfCountersNoMetricsFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kPerfCounters;

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true).status(),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         "Must provide metrics for kPerfCounters type run"));
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

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true),
                IsOkAndHolds(GfxrReplaySettingsEq(expected_rs)));
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

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true),
                IsOkAndHolds(GfxrReplaySettingsEq(expected_rs)));
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

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true),
                IsOkAndHolds(GfxrReplaySettingsEq(expected_rs)));
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

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true),
                IsOkAndHolds(GfxrReplaySettingsEq(expected_rs)));
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

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true),
                IsOkAndHolds(GfxrReplaySettingsEq(expected_rs)));
}

TEST(ValidateGfxrReplaySettingsTest, LoopSingleFrameCountSpecifiedPm4Fail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kPm4Dump;
    rs.loop_single_frame_count = 2;

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true).status(),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         "loop_single_frame_count is hardcoded for kPm4Dump, do not specify"));
}

TEST(ValidateGfxrReplaySettingsTest, LoopSingleFrameCountSpecifiedPerfCountersFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kPerfCounters;
    rs.loop_single_frame_count = 0;

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true).status(),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         "loop_single_frame_count is hardcoded for kPerfCounters, do not specify"));
}

TEST(ValidateGfxrReplaySettingsTest, LoopSingleFrameCountNegativeFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.loop_single_frame_count = -1;

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true).status(),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         "loop_single_frame_count must be >0 for kGpuTiming and kNormal runs"));
}

TEST(ValidateGfxrReplaySettingsTest, LoopSingleFrameCountStringFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.replay_flags_str = "--loop-single-frame-count PLACEHOLDER";

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true).status(),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         "Value specified for --loop-single-frame-count can't be parsed as "
                         "integer: PLACEHOLDER"));
}

TEST(ValidateGfxrReplaySettingsTest, RenderDocDefaultsLoopCountToOne)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kRenderDoc;

    GfxrReplaySettings expected_rs = {};
    expected_rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    expected_rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    expected_rs.run_type = GfxrReplayOptions::kRenderDoc;
    expected_rs.loop_single_frame_count = 1;
    expected_rs.replay_flags_str = "--loop-single-frame-count 1";

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true),
                IsOkAndHolds(GfxrReplaySettingsEq(expected_rs)));
}

TEST(ValidateGfxrReplaySettingsTest, RenderDocFailsValidationWithMetrics)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kRenderDoc;
    rs.metrics = { "PLACEHOLDER_METRICS" };

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true).status(),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         "Cannot use metrics except for kPerfCounters type run"));
}

TEST(ValidateGfxrReplaySettingsTest, RenderDocFailsValidationWithExplicitLoopCount)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.run_type = GfxrReplayOptions::kRenderDoc;
    rs.loop_single_frame_count = 0;

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true).status(),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         "loop_single_frame_count is hardcoded for kRenderDoc, do not specify"));
}

TEST(DeviceManagerTest, EmptySerialIsInvalidForSelectDevice)
{
    ASSERT_EQ(DeviceManager().SelectDevice("").status().code(), absl::StatusCode::kInvalidArgument);
}

}  // namespace
}  // namespace Dive
