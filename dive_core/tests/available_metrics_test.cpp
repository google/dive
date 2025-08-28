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

#include "dive_core/available_metrics.h"
#include "gtest/gtest.h"

namespace Dive
{
namespace
{

TEST(AvailableMetrics, LoadFromCsv)
{
    auto metrics_opt = Dive::AvailableMetrics::LoadFromCsv(TEST_DATA_DIR
                                                           "/mock_available_metrics.csv");
    ASSERT_TRUE(metrics_opt.has_value());
    const auto& metrics = metrics_opt.value();

    const Dive::MetricInfo* info_a = metrics.GetMetricInfo("COUNTER_A");
    ASSERT_NE(info_a, nullptr);
    EXPECT_EQ(info_a->m_metric_id, 1);
    EXPECT_EQ(info_a->m_metric_type, Dive::MetricType::kCount);
    EXPECT_EQ(info_a->m_name, "Counter A");
    EXPECT_EQ(info_a->m_description, "Description A");

    const Dive::MetricInfo* info_b = metrics.GetMetricInfo("COUNTER_B");
    ASSERT_NE(info_b, nullptr);
    EXPECT_EQ(info_b->m_metric_id, 2);
    EXPECT_EQ(info_b->m_metric_type, Dive::MetricType::kPercent);
    EXPECT_EQ(info_b->m_name, "Counter B");
    EXPECT_EQ(info_b->m_description, "Description B");

    const Dive::MetricInfo* info_c = metrics.GetMetricInfo("COUNTER_C");
    EXPECT_EQ(info_c, nullptr);

    EXPECT_EQ(metrics.GetMetricType("COUNTER_A"), Dive::MetricType::kCount);
    EXPECT_EQ(metrics.GetMetricType("COUNTER_B"), Dive::MetricType::kPercent);
    EXPECT_EQ(metrics.GetMetricType("COUNTER_C"), Dive::MetricType::kUnknown);
}

TEST(AvailableMetrics, LoadFromCsv_Malformed)
{
    auto metrics_opt = Dive::AvailableMetrics::LoadFromCsv(TEST_DATA_DIR
                                                           "/mock_available_metrics_malformed.csv");
    ASSERT_TRUE(metrics_opt.has_value());
    const auto&             metrics = metrics_opt.value();
    const Dive::MetricInfo* info_a = metrics.GetMetricInfo("COUNTER_A");
    ASSERT_NE(info_a, nullptr);
    const Dive::MetricInfo* info_b = metrics.GetMetricInfo("COUNTER_B");
    ASSERT_EQ(info_b, nullptr);
}

TEST(AvailableMetrics, LoadFromCsv_NoHeader)
{
    auto metrics_opt = Dive::AvailableMetrics::LoadFromCsv(TEST_DATA_DIR
                                                           "/mock_available_metrics_no_header.csv");
    ASSERT_FALSE(metrics_opt.has_value());
}
}  // namespace
}  // namespace Dive