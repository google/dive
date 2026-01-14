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

#include <iomanip>

#include "absl/status/status.h"
#include "absl/status/status_matchers.h"
#include "absl/strings/str_join.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace Dive
{

std::ostream& operator<<(std::ostream& os, const GfxrReplayOptions& run_type)
{
    switch (run_type)
    {
        case GfxrReplayOptions::kNormal:
            os << "GfxrReplayOptions::kNormal";
            break;
        case GfxrReplayOptions::kPm4Dump:
            os << "GfxrReplayOptions::kPm4Dump";
            break;
        case GfxrReplayOptions::kPerfCounters:
            os << "GfxrReplayOptions::kPerfCounters";
            break;
        case GfxrReplayOptions::kGpuTiming:
            os << "GfxrReplayOptions::kGpuTiming";
            break;
        case GfxrReplayOptions::kRenderDoc:
            os << "GfxrReplayOptions::kRenderDoc";
            break;
        default:
            os << "<Unkonwn GfxrReplayOptions>";
            break;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const std::optional<int>& optional_int)
{
    if (!optional_int)
    {
        os << "std::nullopt";
        return os;
    }

    os << absl::StrCat(*optional_int);
    return os;
}

void PrintTo(const GfxrReplaySettings& settings, std::ostream* os)
{
    *os << "GfxrReplaySettings {\n";
    *os << "  remote_capture_path: " << std::quoted(settings.remote_capture_path) << ",\n";
    *os << "  local_download_dir: " << std::quoted(settings.local_download_dir) << ",\n";
    *os << "  run_type: " << settings.run_type << ",\n";
    *os << "  replay_flags_str: " << std::quoted(settings.replay_flags_str) << ",\n";
    *os << "  wait_for_debugger: " << (settings.wait_for_debugger ? "true" : "false") << ",\n";
    *os << "  metrics: [" << absl::StrJoin(settings.metrics, ", ") << "],\n";
    *os << "  loop_single_frame_count: " << settings.loop_single_frame_count << ",\n";
    *os << "  use_validation_layer: " << (settings.use_validation_layer ? "true" : "false")
        << ",\n";
    *os << "}";
}

// Called by IsOkAndHolds if !ok(). Removes the print of "200-byte object <CC, ...>" which clutters
// the output without adding diagnostic value.
void PrintTo(const absl::StatusOr<GfxrReplaySettings>& settings, std::ostream* os)
{
    if (!settings.ok())
    {
        *os << "is NOT OK";
        return;
    }
    PrintTo(*settings, os);
}

namespace
{

using ::absl_testing::IsOkAndHolds;
using ::absl_testing::StatusIs;
using ::testing::Field;
using ::testing::HasSubstr;
using ::testing::Values;

MATCHER_P(GfxrReplaySettingsEq, expected, "")
{
    EXPECT_EQ(arg.remote_capture_path, expected.remote_capture_path);
    EXPECT_EQ(arg.local_download_dir, expected.local_download_dir);
    EXPECT_EQ(arg.run_type, expected.run_type);
    EXPECT_EQ(arg.replay_flags_str, expected.replay_flags_str);
    EXPECT_EQ(arg.metrics, expected.metrics);
    EXPECT_EQ(arg.loop_single_frame_count, expected.loop_single_frame_count);
    EXPECT_EQ(arg.use_validation_layer, expected.use_validation_layer);
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

    EXPECT_THAT(
        ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true).status(),
        StatusIs(absl::StatusCode::kInvalidArgument, "replay_flags_str cannot contain '='"));
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

    EXPECT_THAT(
        ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/false).status(),
        StatusIs(absl::StatusCode::kUnimplemented, "Dump PM4 is only implemented for Adreno GPU"));
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
    rs.metrics = {"PLACEHOLDER_METRICS"};

    GfxrReplaySettings expected_rs = {};
    expected_rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    expected_rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    expected_rs.run_type = GfxrReplayOptions::kPerfCounters;
    expected_rs.metrics = {"PLACEHOLDER_METRICS"};
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
    expected_rs.replay_flags_str =
        "PLACEHOLDER_FLAG_0 PLACEHOLDER_FLAG_1 --loop-single-frame-count 3 "
        "--enable-gpu-time";

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true),
                IsOkAndHolds(GfxrReplaySettingsEq(expected_rs)));
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
    expected_rs.replay_flags_str = "--loop-single-frame-count 1 --remove-unsupported";

    EXPECT_THAT(ValidateGfxrReplaySettings(rs, /*is_adreno_gpu=*/true),
                IsOkAndHolds(GfxrReplaySettingsEq(expected_rs)));
}

// Post-condition: ValidateGfxrReplaySettings(MakeValidGfxrReplaySettings(x)).ok() == true
GfxrReplaySettings MakeValidGfxrReplaySettings(GfxrReplayOptions run_type)
{
    GfxrReplaySettings settings = {
        .remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH",
        .local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR",
        .run_type = run_type,
    };

    switch (run_type)
    {
        case GfxrReplayOptions::kPerfCounters:
            settings.metrics = {"PLACEHOLDER_METRICS"};
            break;
        case GfxrReplayOptions::kNormal:
        case GfxrReplayOptions::kPm4Dump:
        case GfxrReplayOptions::kGpuTiming:
        case GfxrReplayOptions::kRenderDoc:
            // Valid without further additions
            break;
    }

    return settings;
}

class WaitForDebuggerTest : public ::testing::TestWithParam<GfxrReplayOptions>
{};

TEST_P(WaitForDebuggerTest, WaitForDebuggerIsSupported)
{
    GfxrReplaySettings settings = MakeValidGfxrReplaySettings(GetParam());
    settings.wait_for_debugger = true;
    EXPECT_THAT(ValidateGfxrReplaySettings(settings, /*is_adreno_gpu=*/true),
                IsOkAndHolds(Field(&GfxrReplaySettings::wait_for_debugger, true)));
}

INSTANTIATE_TEST_SUITE_P(, WaitForDebuggerTest,
                         Values(GfxrReplayOptions::kNormal, GfxrReplayOptions::kPm4Dump,
                                GfxrReplayOptions::kPerfCounters, GfxrReplayOptions::kGpuTiming,
                                GfxrReplayOptions::kRenderDoc));

class UseValidationLayerIsSupportedTest : public ::testing::TestWithParam<GfxrReplayOptions>
{};

TEST_P(UseValidationLayerIsSupportedTest, UseValidationLayerIsSupported)
{
    GfxrReplaySettings settings = MakeValidGfxrReplaySettings(GetParam());
    settings.use_validation_layer = true;
    EXPECT_THAT(ValidateGfxrReplaySettings(settings, /*is_adreno_gpu=*/true),
                IsOkAndHolds(Field(&GfxrReplaySettings::use_validation_layer, true)));
}

INSTANTIATE_TEST_SUITE_P(, UseValidationLayerIsSupportedTest,
                         Values(GfxrReplayOptions::kNormal, GfxrReplayOptions::kPm4Dump,
                                GfxrReplayOptions::kGpuTiming));

// TODO: b/459459670 - Update tests once solved. While use_validation_layer is theoretically
// supported by all GfxrReplayOptions values, there's a small practical limitation about setting the
// system properties reliably that prevents this from working universally.
class UseValidationLayerIsNotSupportedTest : public ::testing::TestWithParam<GfxrReplayOptions>
{};

TEST_P(UseValidationLayerIsNotSupportedTest, UseValidationLayerIsNotSupported)
{
    GfxrReplaySettings settings = MakeValidGfxrReplaySettings(GetParam());
    settings.use_validation_layer = true;
    EXPECT_THAT(ValidateGfxrReplaySettings(settings, /*is_adreno_gpu=*/true).status(),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         HasSubstr("use_validation_layer is not allowed")));
}

INSTANTIATE_TEST_SUITE_P(, UseValidationLayerIsNotSupportedTest,
                         Values(GfxrReplayOptions::kPerfCounters, GfxrReplayOptions::kRenderDoc));

class MetricsAreNotSupportedTest : public ::testing::TestWithParam<GfxrReplayOptions>
{};

TEST_P(MetricsAreNotSupportedTest, MetricsAreNotSupported)
{
    GfxrReplaySettings settings = MakeValidGfxrReplaySettings(GetParam());
    settings.metrics = {"PLACEHOLDER_METRICS"};
    EXPECT_THAT(ValidateGfxrReplaySettings(settings, /*is_adreno_gpu=*/true).status(),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         "Cannot use metrics except for kPerfCounters type run"));
}

INSTANTIATE_TEST_SUITE_P(, MetricsAreNotSupportedTest,
                         Values(GfxrReplayOptions::kNormal, GfxrReplayOptions::kPm4Dump,
                                GfxrReplayOptions::kGpuTiming, GfxrReplayOptions::kRenderDoc));

class HardcodedLoopSingleFrameCountTest : public ::testing::TestWithParam<GfxrReplayOptions>
{};

TEST_P(HardcodedLoopSingleFrameCountTest, SettingLoopSingleFrameCountFieldIsNotSupported)
{
    GfxrReplaySettings settings = MakeValidGfxrReplaySettings(GetParam());
    settings.loop_single_frame_count = 10;  // arbitrary
    EXPECT_THAT(ValidateGfxrReplaySettings(settings, /*is_adreno_gpu=*/true).status(),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         HasSubstr("loop_single_frame_count is hardcoded")));
}

TEST_P(HardcodedLoopSingleFrameCountTest, SettingLoopSingleFrameCountFlagIsNotSupported)
{
    GfxrReplaySettings settings = MakeValidGfxrReplaySettings(GetParam());
    settings.replay_flags_str = "--loop-single-frame-count 10";  // arbitrary
    EXPECT_THAT(ValidateGfxrReplaySettings(settings, /*is_adreno_gpu=*/true).status(),
                StatusIs(absl::StatusCode::kInvalidArgument,
                         HasSubstr("loop_single_frame_count is hardcoded")));
}

INSTANTIATE_TEST_SUITE_P(, HardcodedLoopSingleFrameCountTest,
                         Values(GfxrReplayOptions::kPm4Dump, GfxrReplayOptions::kPerfCounters,
                                GfxrReplayOptions::kRenderDoc));

TEST(AndroidDeviceTest, EmptySerialIsInvalidForCreate)
{
    ASSERT_THAT(AndroidDevice::Create(""), StatusIs(absl::StatusCode::kInvalidArgument));
}

}  // namespace
}  // namespace Dive
