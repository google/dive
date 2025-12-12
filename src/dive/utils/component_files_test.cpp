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

#include "dive/utils/component_files.h"

#include "absl/status/status_matchers.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace Dive
{
namespace
{

using ::absl_testing::IsOkAndHolds;
using ::absl_testing::StatusIs;

MATCHER_P(PathsEq, expected, "")
{
    EXPECT_EQ(arg.gfxr, expected.gfxr);
    EXPECT_EQ(arg.gfxa, expected.gfxa);
    EXPECT_EQ(arg.perf_counter_csv, expected.perf_counter_csv);
    EXPECT_EQ(arg.gpu_timing_csv, expected.gpu_timing_csv);
    EXPECT_EQ(arg.pm4_rd, expected.pm4_rd);
    EXPECT_EQ(arg.screenshot_png, expected.screenshot_png);
    EXPECT_EQ(arg.renderdoc_rdc, expected.renderdoc_rdc);
    return true;
}

TEST(ValidateGfxrReplaySettingsTest, EmptyGfxrStemFail)
{
    std::filesystem::path parent_dir = "PARENT";
    parent_dir /= "DIR";
    std::string gfxr_stem = "";

    EXPECT_THAT(GetComponentFilesHostPaths(parent_dir, gfxr_stem).status(),
                StatusIs(absl::StatusCode::kFailedPrecondition, "gfxr_stem cannot be empty"));
}

TEST(ValidateGfxrReplaySettingsTest, EmptyParentDirFail)
{
    std::filesystem::path parent_dir = "";
    std::string           gfxr_stem = "PLACEHOLDER_trim_trigger_ID";

    EXPECT_THAT(GetComponentFilesHostPaths(parent_dir, gfxr_stem).status(),
                StatusIs(absl::StatusCode::kFailedPrecondition, "parent_dir cannot be empty"));
}

TEST(ValidateGfxrReplaySettingsTest, BasicPass)
{
    std::filesystem::path parent_dir = "PARENT";
    parent_dir /= "DIR";
    std::string gfxr_stem = "PLACEHOLDER_trim_trigger_ID";

    ComponentFilePaths expected_res = {};
    expected_res.gfxr = parent_dir / "PLACEHOLDER_trim_trigger_ID.gfxr";
    expected_res.gfxa = parent_dir / "PLACEHOLDER_asset_file_ID.gfxa";
    expected_res.perf_counter_csv = parent_dir /
                                    "PLACEHOLDER_trim_trigger_ID_profiling_metrics.csv";
    expected_res.gpu_timing_csv = parent_dir / "PLACEHOLDER_trim_trigger_ID_gpu_time.csv";
    expected_res.pm4_rd = parent_dir / "PLACEHOLDER_trim_trigger_ID.rd";
    expected_res.screenshot_png = parent_dir / "PLACEHOLDER_trim_trigger_ID.png";
    expected_res.renderdoc_rdc = parent_dir / "PLACEHOLDER_trim_trigger_ID_capture.rdc";

    EXPECT_THAT(GetComponentFilesHostPaths(parent_dir, gfxr_stem),
                IsOkAndHolds(PathsEq(expected_res)));
}

TEST(ValidateGfxrReplaySettingsTest, GfxrSubstringInParentPathPass)
{
    std::filesystem::path parent_dir = "PARENT";
    parent_dir /= "DIR";
    parent_dir /= "_trim_trigger_";
    std::string gfxr_stem = "PLACEHOLDER_trim_trigger_ID";

    ComponentFilePaths expected_res = {};
    expected_res.gfxr = parent_dir / "PLACEHOLDER_trim_trigger_ID.gfxr";
    expected_res.gfxa = parent_dir / "PLACEHOLDER_asset_file_ID.gfxa";
    expected_res.perf_counter_csv = parent_dir /
                                    "PLACEHOLDER_trim_trigger_ID_profiling_metrics.csv";
    expected_res.gpu_timing_csv = parent_dir / "PLACEHOLDER_trim_trigger_ID_gpu_time.csv";
    expected_res.pm4_rd = parent_dir / "PLACEHOLDER_trim_trigger_ID.rd";
    expected_res.screenshot_png = parent_dir / "PLACEHOLDER_trim_trigger_ID.png";
    expected_res.renderdoc_rdc = parent_dir / "PLACEHOLDER_trim_trigger_ID_capture.rdc";

    EXPECT_THAT(GetComponentFilesHostPaths(parent_dir, gfxr_stem),
                IsOkAndHolds(PathsEq(expected_res)));
}

TEST(ValidateGfxrReplaySettingsTest, DotinGfxrStemPass)
{
    std::filesystem::path parent_dir = "PARENT";
    parent_dir /= "DIR";
    std::string gfxr_stem = "PLACEHOLDER._trim_trigger_ID.test";

    ComponentFilePaths expected_res = {};
    expected_res.gfxr = parent_dir / "PLACEHOLDER._trim_trigger_ID.test.gfxr";
    expected_res.gfxa = parent_dir / "PLACEHOLDER._asset_file_ID.test.gfxa";
    expected_res.perf_counter_csv = parent_dir /
                                    "PLACEHOLDER._trim_trigger_ID.test_profiling_metrics.csv";
    expected_res.gpu_timing_csv = parent_dir / "PLACEHOLDER._trim_trigger_ID.test_gpu_time.csv";
    expected_res.pm4_rd = parent_dir / "PLACEHOLDER._trim_trigger_ID.test.rd";
    expected_res.screenshot_png = parent_dir / "PLACEHOLDER._trim_trigger_ID.test.png";
    expected_res.renderdoc_rdc = parent_dir / "PLACEHOLDER._trim_trigger_ID.test_capture.rdc";

    EXPECT_THAT(GetComponentFilesHostPaths(parent_dir, gfxr_stem),
                IsOkAndHolds(PathsEq(expected_res)));
}

TEST(ValidateGfxrReplaySettingsTest, SlashinGfxrStemFail)
{
    std::filesystem::path parent_dir = "PARENT";
    parent_dir /= "DIR";
    std::string gfxr_stem = "PLACEHOLDER_trim_trigger_ID/oops";

    EXPECT_THAT(GetComponentFilesHostPaths(parent_dir, gfxr_stem).status(),
                StatusIs(absl::StatusCode::kFailedPrecondition,
                         "unexpected name for gfxr file: PLACEHOLDER_trim_trigger_ID/oops, not a "
                         "stem"));
}

TEST(ValidateGfxrReplaySettingsTest, BackSlashinGfxrStemFail)
{
    std::filesystem::path parent_dir = "PARENT";
    parent_dir /= "DIR";
    std::string gfxr_stem = "PLACEHOLDER_trim_trigger_ID\\oops";

    EXPECT_THAT(GetComponentFilesHostPaths(parent_dir, gfxr_stem).status(),
                StatusIs(absl::StatusCode::kFailedPrecondition,
                         "unexpected name for gfxr file: PLACEHOLDER_trim_trigger_ID\\oops, not a "
                         "stem"));
}

TEST(ValidateGfxrReplaySettingsTest, MissingSubstringGfxrStemFail)
{
    std::filesystem::path parent_dir = "PARENT";
    parent_dir /= "DIR";
    std::string gfxr_stem = "PLACEHOLDER_oops_ID";

    EXPECT_THAT(GetComponentFilesHostPaths(parent_dir, gfxr_stem).status(),
                StatusIs(absl::StatusCode::kFailedPrecondition,
                         "unexpected name for gfxr file: PLACEHOLDER_oops_ID, expecting name "
                         "containing: _trim_trigger_"));
}

}  // namespace
}  // namespace Dive
