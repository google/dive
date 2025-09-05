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

struct StatsTestCase
{
    AvailableGpuTiming::ObjectType type;
    uint32_t                       row;
    float                          mean;
    float                          median;
};

TEST(AvailableGpuTiming, LoadFromCsv_Pass)
{
    AvailableGpuTiming g;
    EXPECT_FALSE(g.IsValid());
    EXPECT_TRUE(g.LoadFromCsv(fp / "mock_gpu_time.csv"));
    EXPECT_TRUE(g.IsValid());
}

TEST(AvailableGpuTiming, LoadFromCsv_MalformedFrameFail)
{
    AvailableGpuTiming g;
    EXPECT_FALSE(g.IsValid());
    EXPECT_FALSE(g.LoadFromCsv(fp / "mock_gpu_time_malformed.csv"));
    EXPECT_FALSE(g.IsValid());
}

TEST(AvailableGpuTiming, LoadFromCsv_TwiceFail)
{
    AvailableGpuTiming g;
    EXPECT_FALSE(g.IsValid());
    EXPECT_TRUE(g.LoadFromCsv(fp / "mock_gpu_time.csv"));
    EXPECT_TRUE(g.IsValid());
    EXPECT_FALSE(g.LoadFromCsv(fp / "mock_gpu_time.csv"));
    EXPECT_TRUE(g.IsValid());
}

TEST(AvailableGpuTiming, LoadFromString_Pass)
{
    AvailableGpuTiming g;
    std::string
    s = "Type,Id,Mean [ms],Median [ms]\nFrame,10,0.345,0.341\nCommandBuffer,0,0.001,0.002\n";
    EXPECT_FALSE(g.IsValid());
    EXPECT_TRUE(g.LoadFromString(s));
    EXPECT_TRUE(g.IsValid());
}

TEST(AvailableGpuTiming, LoadFromString_MalformedHeaderFail)
{
    AvailableGpuTiming g;
    std::string s = "Type,Id,Median [ms]\nFrame,10,0.345,0.341\nCommandBuffer,0,0.001,0.002\n";
    EXPECT_FALSE(g.IsValid());
    EXPECT_FALSE(g.LoadFromString(s));
}

TEST(AvailableGpuTiming, LoadFromString_NoHeaderFail)
{
    AvailableGpuTiming g;
    std::string        s = "Frame,10,0.345,0.341\nCommandBuffer,0,0.001,0.002\n";
    EXPECT_FALSE(g.IsValid());
    EXPECT_FALSE(g.LoadFromString(s));
}

TEST(AvailableGpuTiming, LoadFromString_StringIdFail)
{
    AvailableGpuTiming g;
    std::string        s = "Type,Id,Mean [ms],Median "
                           "[ms]\nFrame,10,0.345,0.341\nCommandBuffer,placeholder,0.001,0.002\n";
    EXPECT_FALSE(g.IsValid());
    EXPECT_FALSE(g.LoadFromString(s));
}

TEST(AvailableGpuTiming, LoadFromString_StringMeanFail)
{
    AvailableGpuTiming g;
    std::string
    s = "Type,Id,Mean [ms],Median [ms]\nFrame,10,0.345,0.341\nCommandBuffer,0,placeholder,0.002\n";
    EXPECT_FALSE(g.IsValid());
    EXPECT_FALSE(g.LoadFromString(s));
}

TEST(AvailableGpuTiming, LoadFromString_FloatIdFail)
{
    AvailableGpuTiming g;
    std::string
    s = "Type,Id,Mean [ms],Median [ms]\nFrame,10,0.345,0.341\nCommandBuffer,0.5,0.001,0.002\n";
    EXPECT_FALSE(g.IsValid());
    EXPECT_FALSE(g.LoadFromString(s));
    EXPECT_FALSE(g.IsValid());
}

TEST(AvailableGpuTiming, LoadFromString_IntMeanFail)
{
    AvailableGpuTiming g;
    std::string
    s = "Type,Id,Mean [ms],Median [ms]\nFrame,10,0.345,0.341\nCommandBuffer,0,10,0.002\n";
    EXPECT_FALSE(g.IsValid());
    EXPECT_FALSE(g.LoadFromString(s));
    EXPECT_FALSE(g.IsValid());
}

TEST(AvailableGpuTiming, LoadFromString_IntMedianFail)
{
    AvailableGpuTiming g;
    std::string
    s = "Type,Id,Mean [ms],Median [ms]\nFrame,10,0.345,0.341\nCommandBuffer,0,0.001,20\n";
    EXPECT_FALSE(g.IsValid());
    EXPECT_FALSE(g.LoadFromString(s));
    EXPECT_FALSE(g.IsValid());
}

TEST(AvailableGpuTiming, GetStatsByType_Pass)
{
    AvailableGpuTiming g;
    EXPECT_FALSE(g.IsValid());
    EXPECT_TRUE(g.LoadFromCsv(fp / "mock_gpu_time.csv"));
    EXPECT_TRUE(g.IsValid());

    const std::vector<StatsTestCase> test_cases = {
        { AvailableGpuTiming::ObjectType::kFrame, 0, 0.345f, 0.341f },
        { AvailableGpuTiming::ObjectType::kCommandBuffer, 0, 0.001f, 0.002f },
        { AvailableGpuTiming::ObjectType::kCommandBuffer, 1, 0.003f, 0.004f },
        { AvailableGpuTiming::ObjectType::kCommandBuffer, 2, 0.005f, 0.006f },
        { AvailableGpuTiming::ObjectType::kCommandBuffer, 3, 0.007f, 0.008f },
        { AvailableGpuTiming::ObjectType::kCommandBuffer, 4, 0.009f, 0.010f },
        { AvailableGpuTiming::ObjectType::kRenderPass, 0, 0.228f, 0.229f },
        { AvailableGpuTiming::ObjectType::kRenderPass, 1, 0.109f, 0.107f },
    };

    for (const auto& tc : test_cases)
    {
        auto ret = g.GetStatsByType(tc.type, tc.row);
        ASSERT_NE(ret, std::nullopt);
        EXPECT_FLOAT_EQ(ret->mean_ms, tc.mean);
        EXPECT_FLOAT_EQ(ret->median_ms, tc.median);
    }
}

TEST(AvailableGpuTiming, GetStatsByType_InvalidFail)
{
    AvailableGpuTiming g;
    EXPECT_FALSE(g.IsValid());
    EXPECT_FALSE(g.LoadFromCsv(fp / "mock_gpu_time_malformed.csv"));
    EXPECT_FALSE(g.IsValid());

    std::optional<AvailableGpuTiming::Stats>
    ret = g.GetStatsByType(AvailableGpuTiming::ObjectType::kFrame, 0);
    EXPECT_EQ(ret, std::nullopt);
    ret = g.GetStatsByType(AvailableGpuTiming::ObjectType::kCommandBuffer, 0);
    EXPECT_EQ(ret, std::nullopt);
    ret = g.GetStatsByType(AvailableGpuTiming::ObjectType::kRenderPass, 0);
    EXPECT_EQ(ret, std::nullopt);
}

TEST(AvailableGpuTiming, GetStatsByType_OOBFail)
{
    AvailableGpuTiming g;
    EXPECT_FALSE(g.IsValid());
    EXPECT_TRUE(g.LoadFromCsv(fp / "mock_gpu_time.csv"));
    EXPECT_TRUE(g.IsValid());

    std::optional<AvailableGpuTiming::Stats>
    ret = g.GetStatsByType(AvailableGpuTiming::ObjectType::kCommandBuffer, 10);
    EXPECT_EQ(ret, std::nullopt);
    ret = g.GetStatsByType(AvailableGpuTiming::ObjectType::kRenderPass, 3);
    EXPECT_EQ(ret, std::nullopt);
}

TEST(AvailableGpuTiming, GetStatsByRow_Pass)
{
    AvailableGpuTiming g;
    EXPECT_FALSE(g.IsValid());
    EXPECT_TRUE(g.LoadFromCsv(fp / "mock_gpu_time.csv"));
    EXPECT_TRUE(g.IsValid());

    const std::vector<StatsTestCase> test_cases = {
        { AvailableGpuTiming::ObjectType::nObjectTypes, 1, 0.345f, 0.341f },
        { AvailableGpuTiming::ObjectType::nObjectTypes, 2, 0.001f, 0.002f },
        { AvailableGpuTiming::ObjectType::nObjectTypes, 3, 0.003f, 0.004f },
        { AvailableGpuTiming::ObjectType::nObjectTypes, 4, 0.228f, 0.229f },
        { AvailableGpuTiming::ObjectType::nObjectTypes, 5, 0.005f, 0.006f },
        { AvailableGpuTiming::ObjectType::nObjectTypes, 6, 0.007f, 0.008f },
        { AvailableGpuTiming::ObjectType::nObjectTypes, 7, 0.109f, 0.107f },
        { AvailableGpuTiming::ObjectType::nObjectTypes, 8, 0.009f, 0.010f },
    };

    for (const auto& tc : test_cases)
    {
        auto ret = g.GetStatsByRow(tc.row);
        ASSERT_NE(ret, std::nullopt);
        EXPECT_FLOAT_EQ(ret->mean_ms, tc.mean);
        EXPECT_FLOAT_EQ(ret->median_ms, tc.median);
    }
}

TEST(AvailableGpuTiming, GetStatsByRow_InvalidFail)
{
    AvailableGpuTiming g;
    EXPECT_FALSE(g.IsValid());
    EXPECT_FALSE(g.LoadFromCsv(fp / "mock_gpu_time_malformed.csv"));
    EXPECT_FALSE(g.IsValid());

    std::optional<AvailableGpuTiming::Stats> ret = g.GetStatsByRow(1);
    EXPECT_EQ(ret, std::nullopt);
    ret = g.GetStatsByRow(2);
    EXPECT_EQ(ret, std::nullopt);
}

TEST(AvailableGpuTiming, GetStatsByRow_OOBFail)
{
    AvailableGpuTiming g;
    EXPECT_FALSE(g.IsValid());
    EXPECT_TRUE(g.LoadFromCsv(fp / "mock_gpu_time.csv"));
    EXPECT_TRUE(g.IsValid());

    std::optional<AvailableGpuTiming::Stats> ret = g.GetStatsByRow(0);
    EXPECT_EQ(ret, std::nullopt);
    ret = g.GetStatsByRow(9);
    EXPECT_EQ(ret, std::nullopt);
    ret = g.GetStatsByRow(13);
    EXPECT_EQ(ret, std::nullopt);
}

}  // namespace
}  // namespace Dive
