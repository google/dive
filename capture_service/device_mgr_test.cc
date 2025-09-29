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
    EXPECT_STREQ(std::string(ret.status().message()).c_str(), "Must provide local_download_dir");
}

TEST(ValidateGfxrReplaySettingsTest, NoRemoteCapturePathFail)
{
    GfxrReplaySettings rs = {};
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";

    auto ret = ValidateGfxrReplaySettings(rs, true);
    ASSERT_FALSE(ret.ok());
    EXPECT_EQ(ret.status().code(), absl::StatusCode::kInvalidArgument);
    EXPECT_STREQ(std::string(ret.status().message()).c_str(), "Must provide remote_capture_path");
}

TEST(ValidateGfxrReplaySettingsTest, ReplayFlagsStrEqualsSignFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.replay_flags_str = "--loop-single-frame-count=200 --loop-single-frame";

    auto ret = ValidateGfxrReplaySettings(rs, true);
    ASSERT_FALSE(ret.ok());
    EXPECT_EQ(ret.status().code(), absl::StatusCode::kInvalidArgument);
    EXPECT_STREQ(std::string(ret.status().message()).c_str(),
                 "replay_flags_str cannot contain '='");
}

TEST(ValidateGfxrReplaySettingsTest, DoubleLoopSingleFrameFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.loop_single_frame = true;
    rs.replay_flags_str = "--loop-single-frame";

    auto ret = ValidateGfxrReplaySettings(rs, true);
    ASSERT_FALSE(ret.ok());
    EXPECT_EQ(ret.status().code(), absl::StatusCode::kInvalidArgument);
    EXPECT_STREQ(std::string(ret.status().message()).c_str(),
                 "Do not specify loop_single_frame in GfxrReplaySettings and also as flag "
                 "--loop-single-frame");
}

TEST(ValidateGfxrReplaySettingsTest, MutuallyExclusiveOptionsFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.enable_perf_counters = true;
    rs.enable_gpu_time = true;

    auto ret = ValidateGfxrReplaySettings(rs, true);
    ASSERT_FALSE(ret.ok());
    EXPECT_EQ(ret.status().code(), absl::StatusCode::kInvalidArgument);
    EXPECT_STREQ(std::string(ret.status().message()).c_str(),
                 "Only one of the following settings allowed: enable_dump_pm4, "
                 "enable_perf_counters, enable_gpu_time");
}

TEST(ValidateGfxrReplaySettingsTest, NonAdrenoDevicePM4DumpFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.enable_dump_pm4 = true;

    auto ret = ValidateGfxrReplaySettings(rs, false);
    ASSERT_FALSE(ret.ok());
    EXPECT_EQ(ret.status().code(), absl::StatusCode::kUnimplemented);
    EXPECT_STREQ(std::string(ret.status().message()).c_str(),
                 "Dump PM4 is only implemented for Adreno GPU");
}

TEST(ValidateGfxrReplaySettingsTest, NonAdrenoDevicePerfCountersFail)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.enable_perf_counters = true;

    auto ret = ValidateGfxrReplaySettings(rs, false);
    ASSERT_FALSE(ret.ok());
    EXPECT_EQ(ret.status().code(), absl::StatusCode::kUnimplemented);
    EXPECT_STREQ(std::string(ret.status().message()).c_str(),
                 "Perf counters feature is only implemented for Adreno GPU");
}

TEST(ValidateGfxrReplaySettingsTest, BasicPass)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";

    auto ret = ValidateGfxrReplaySettings(rs, true);
    ASSERT_TRUE(ret.ok()) << ret.status();
    EXPECT_STREQ(ret->replay_flags_str.c_str(), "");
}

TEST(ValidateGfxrReplaySettingsTest, ReplayFlagsStrSpacesPass)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.replay_flags_str = "   ";

    auto ret = ValidateGfxrReplaySettings(rs, true);
    ASSERT_TRUE(ret.ok()) << ret.status();
    EXPECT_STREQ(ret->replay_flags_str.c_str(), "");
}

TEST(ValidateGfxrReplaySettingsTest, FlagToFlagPass)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.replay_flags_str = "--loop-single-frame --loop-single-frame-count 3 --enable-gpu-time";

    auto ret = ValidateGfxrReplaySettings(rs, true);
    ASSERT_TRUE(ret.ok()) << ret.status();
    EXPECT_EQ(ret->loop_single_frame, true);
    EXPECT_EQ(ret->loop_single_frame_count, 3);
    EXPECT_EQ(ret->enable_gpu_time, true);
    EXPECT_STREQ(ret->replay_flags_str.c_str(),
                 "--loop-single-frame --loop-single-frame-count 3 --enable-gpu-time");
}

TEST(ValidateGfxrReplaySettingsTest, SettingToFlagPass)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.enable_gpu_time = true;
    rs.loop_single_frame = true;
    rs.loop_single_frame_count = 3;
    rs.replay_flags_str = "";

    auto ret = ValidateGfxrReplaySettings(rs, true);
    ASSERT_TRUE(ret.ok()) << ret.status();
    EXPECT_EQ(ret->loop_single_frame, true);
    EXPECT_EQ(ret->loop_single_frame_count, 3);
    EXPECT_EQ(ret->enable_gpu_time, true);
    EXPECT_STREQ(ret->replay_flags_str.c_str(),
                 "--loop-single-frame --loop-single-frame-count 3 --enable-gpu-time");
}

TEST(ValidateGfxrReplaySettingsTest, MixFlagsSettingsPass)
{
    GfxrReplaySettings rs = {};
    rs.remote_capture_path = "PLACEHOLDER_REMOTE_CAPTURE_PATH";
    rs.local_download_dir = "PLACEHOLDER_LOCAL_DOWNLOAD_DIR";
    rs.loop_single_frame = true;
    rs.loop_single_frame_count = 3;
    rs.replay_flags_str = "--enable-gpu-time";

    auto ret = ValidateGfxrReplaySettings(rs, true);
    ASSERT_TRUE(ret.ok()) << ret.status();
    EXPECT_EQ(ret->loop_single_frame, true);
    EXPECT_EQ(ret->loop_single_frame_count, 3);
    EXPECT_EQ(ret->enable_gpu_time, true);
    EXPECT_STREQ(ret->replay_flags_str.c_str(),
                 "--loop-single-frame --loop-single-frame-count 3 --enable-gpu-time");
}

TEST(DeviceManagerTest, EmptySerialIsInvalidForSelectDevice)
{
    ASSERT_EQ(DeviceManager().SelectDevice("").status().code(), absl::StatusCode::kInvalidArgument);
}

}  // namespace
}  // namespace Dive
