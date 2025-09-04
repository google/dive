/*
 Copyright 2025 Google LLC

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

#include "dive_core/available_gpu_time.h"

#include <filesystem>

#include "gtest/gtest.h"

namespace Dive
{
namespace
{
std::filesystem::path fp = TEST_DATA_DIR;

TEST(AvailableGpuTiming, LoadFromCsv_Successful)
{
    AvailableGpuTiming g;
    EXPECT_FALSE(g.IsValid());
    EXPECT_TRUE(g.LoadFromCsv(fp / "mock_gpu_time.csv"));
    EXPECT_TRUE(g.IsValid());
}

TEST(AvailableMetrics, LoadFromCsv_MalformedFrameFail)
{
    AvailableGpuTiming g;
    EXPECT_FALSE(g.IsValid());
    EXPECT_FALSE(g.LoadFromCsv(fp / "mock_gpu_time_malformed.csv"));
    EXPECT_FALSE(g.IsValid());
}

TEST(AvailableMetrics, LoadFromString_Successful)
{
    AvailableGpuTiming g;
    std::string
    s = "Type,Id,Mean [ms],Median [ms]\nFrame,10,0.345,0.341\nCommandBuffer,0,0.001,0.002\n";
    EXPECT_FALSE(g.IsValid());
    EXPECT_TRUE(g.LoadFromString(s));
    EXPECT_TRUE(g.IsValid());
}

TEST(AvailableMetrics, LoadFromString_MalformedHeaderFail)
{
    AvailableGpuTiming g;
    std::string s = "Type,Id,Median [ms]\nFrame,10,0.345,0.341\nCommandBuffer,0,0.001,0.002\n";
    EXPECT_FALSE(g.IsValid());
    EXPECT_FALSE(g.LoadFromString(s));
}

TEST(AvailableMetrics, LoadFromString_NoHeaderFail)
{
    AvailableGpuTiming g;
    std::string        s = "Frame,10,0.345,0.341\nCommandBuffer,0,0.001,0.002\n";
    EXPECT_FALSE(g.IsValid());
    EXPECT_FALSE(g.LoadFromString(s));
}

TEST(AvailableMetrics, GetStats_TwoParam_Successful)
{
    AvailableGpuTiming g;
    EXPECT_FALSE(g.IsValid());
    EXPECT_TRUE(g.LoadFromCsv(fp / "mock_gpu_time.csv"));
    EXPECT_TRUE(g.IsValid());

    std::optional<AvailableGpuTiming::Stats>
    ret = g.GetStats(AvailableGpuTiming::ObjectType::kFrame, 0);
    ASSERT_NE(ret, std::nullopt);
    EXPECT_EQ(ret->mean_ms, 0.345f);
    EXPECT_EQ(ret->median_ms, 0.341f);

    ret = g.GetStats(AvailableGpuTiming::ObjectType::kCommandBuffer, 0);
    ASSERT_NE(ret, std::nullopt);
    EXPECT_EQ(ret->mean_ms, 0.001f);
    EXPECT_EQ(ret->median_ms, 0.002f);
    ret = g.GetStats(AvailableGpuTiming::ObjectType::kCommandBuffer, 1);
    ASSERT_NE(ret, std::nullopt);
    EXPECT_EQ(ret->mean_ms, 0.003f);
    EXPECT_EQ(ret->median_ms, 0.004f);
    ret = g.GetStats(AvailableGpuTiming::ObjectType::kCommandBuffer, 2);
    ASSERT_NE(ret, std::nullopt);
    EXPECT_EQ(ret->mean_ms, 0.005f);
    EXPECT_EQ(ret->median_ms, 0.006f);
    ret = g.GetStats(AvailableGpuTiming::ObjectType::kCommandBuffer, 3);
    ASSERT_NE(ret, std::nullopt);
    EXPECT_EQ(ret->mean_ms, 0.007f);
    EXPECT_EQ(ret->median_ms, 0.008f);
    ret = g.GetStats(AvailableGpuTiming::ObjectType::kCommandBuffer, 4);
    ASSERT_NE(ret, std::nullopt);
    EXPECT_EQ(ret->mean_ms, 0.009f);
    EXPECT_EQ(ret->median_ms, 0.010f);

    ret = g.GetStats(AvailableGpuTiming::ObjectType::kRenderPass, 0);
    ASSERT_NE(ret, std::nullopt);
    EXPECT_EQ(ret->mean_ms, 0.228f);
    EXPECT_EQ(ret->median_ms, 0.229f);
    ret = g.GetStats(AvailableGpuTiming::ObjectType::kRenderPass, 1);
    ASSERT_NE(ret, std::nullopt);
    EXPECT_EQ(ret->mean_ms, 0.109f);
    EXPECT_EQ(ret->median_ms, 0.107f);
}

TEST(AvailableMetrics, GetStats_TwoParam_InvalidFail)
{
    AvailableGpuTiming g;
    EXPECT_FALSE(g.IsValid());
    EXPECT_FALSE(g.LoadFromCsv(fp / "mock_gpu_time_malformed.csv"));
    EXPECT_FALSE(g.IsValid());

    std::optional<AvailableGpuTiming::Stats>
    ret = g.GetStats(AvailableGpuTiming::ObjectType::kFrame, 0);
    EXPECT_EQ(ret, std::nullopt);
    ret = g.GetStats(AvailableGpuTiming::ObjectType::kCommandBuffer, 0);
    EXPECT_EQ(ret, std::nullopt);
    ret = g.GetStats(AvailableGpuTiming::ObjectType::kRenderPass, 0);
    EXPECT_EQ(ret, std::nullopt);
}

TEST(AvailableMetrics, GetStats_TwoParam_OOBFail)
{
    AvailableGpuTiming g;
    EXPECT_FALSE(g.IsValid());
    EXPECT_TRUE(g.LoadFromCsv(fp / "mock_gpu_time.csv"));
    EXPECT_TRUE(g.IsValid());

    std::optional<AvailableGpuTiming::Stats>
    ret = g.GetStats(AvailableGpuTiming::ObjectType::kCommandBuffer, 10);
    EXPECT_EQ(ret, std::nullopt);
    ret = g.GetStats(AvailableGpuTiming::ObjectType::kRenderPass, 3);
    EXPECT_EQ(ret, std::nullopt);
}

TEST(AvailableMetrics, GetStats_OneParam_Successful)
{
    AvailableGpuTiming g;
    EXPECT_FALSE(g.IsValid());
    EXPECT_TRUE(g.LoadFromCsv(fp / "mock_gpu_time.csv"));
    EXPECT_TRUE(g.IsValid());

    std::optional<AvailableGpuTiming::Stats> ret = g.GetStats(1);
    ASSERT_NE(ret, std::nullopt);
    EXPECT_EQ(ret->mean_ms, 0.345f);
    EXPECT_EQ(ret->median_ms, 0.341f);
    ret = g.GetStats(2);
    ASSERT_NE(ret, std::nullopt);
    EXPECT_EQ(ret->mean_ms, 0.001f);
    EXPECT_EQ(ret->median_ms, 0.002f);
    ret = g.GetStats(3);
    ASSERT_NE(ret, std::nullopt);
    EXPECT_EQ(ret->mean_ms, 0.003f);
    EXPECT_EQ(ret->median_ms, 0.004f);
    ret = g.GetStats(4);
    ASSERT_NE(ret, std::nullopt);
    EXPECT_EQ(ret->mean_ms, 0.228f);
    EXPECT_EQ(ret->median_ms, 0.229f);
    ret = g.GetStats(5);
    ASSERT_NE(ret, std::nullopt);
    EXPECT_EQ(ret->mean_ms, 0.005f);
    EXPECT_EQ(ret->median_ms, 0.006f);
    ret = g.GetStats(6);
    ASSERT_NE(ret, std::nullopt);
    EXPECT_EQ(ret->mean_ms, 0.007f);
    EXPECT_EQ(ret->median_ms, 0.008f);
    ret = g.GetStats(7);
    ASSERT_NE(ret, std::nullopt);
    EXPECT_EQ(ret->mean_ms, 0.109f);
    EXPECT_EQ(ret->median_ms, 0.107f);
    ret = g.GetStats(8);
    ASSERT_NE(ret, std::nullopt);
    EXPECT_EQ(ret->mean_ms, 0.009f);
    EXPECT_EQ(ret->median_ms, 0.010f);
}

TEST(AvailableMetrics, GetStats_OneParam_InvalidFail)
{
    AvailableGpuTiming g;
    EXPECT_FALSE(g.IsValid());
    EXPECT_FALSE(g.LoadFromCsv(fp / "mock_gpu_time_malformed.csv"));
    EXPECT_FALSE(g.IsValid());

    std::optional<AvailableGpuTiming::Stats> ret = g.GetStats(1);
    EXPECT_EQ(ret, std::nullopt);
    ret = g.GetStats(2);
    EXPECT_EQ(ret, std::nullopt);
}

TEST(AvailableMetrics, GetStats_OneParam_OOBFail)
{
    AvailableGpuTiming g;
    EXPECT_FALSE(g.IsValid());
    EXPECT_TRUE(g.LoadFromCsv(fp / "mock_gpu_time.csv"));
    EXPECT_TRUE(g.IsValid());

    std::optional<AvailableGpuTiming::Stats> ret = g.GetStats(0);
    EXPECT_EQ(ret, std::nullopt);
    ret = g.GetStats(9);
    EXPECT_EQ(ret, std::nullopt);
    ret = g.GetStats(13);
    EXPECT_EQ(ret, std::nullopt);
}

}  // namespace
}  // namespace Dive
