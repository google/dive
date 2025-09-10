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
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace Dive
{
namespace
{

MATCHER_P(MetricInfoEq, expected, "")
{
    EXPECT_EQ(arg->m_metric_id, expected.m_metric_id);
    EXPECT_EQ(arg->m_metric_type, expected.m_metric_type);
    EXPECT_EQ(arg->m_key, expected.m_key);
    EXPECT_EQ(arg->m_name, expected.m_name);
    EXPECT_EQ(arg->m_description, expected.m_description);
    return true;
}

TEST(AvailableMetrics, LoadFromCsv)
{
    auto metrics = AvailableMetrics::LoadFromCsv(TEST_DATA_DIR "/mock_available_metrics.csv");
    ASSERT_NE(metrics, nullptr);

    EXPECT_THAT(metrics->GetMetricInfo("COUNTER_A"),
                AllOf(Not(testing::IsNull()),
                      MetricInfoEq(MetricInfo{
                      1, Dive::MetricType::kCount, "COUNTER_A", "Counter A", "Description A" })));

    EXPECT_THAT(metrics->GetMetricInfo("COUNTER_B"),
                AllOf(Not(testing::IsNull()),
                      MetricInfoEq(MetricInfo{
                      2, Dive::MetricType::kPercent, "COUNTER_B", "Counter B", "Description B" })));

    const Dive::MetricInfo* info_c = metrics->GetMetricInfo("COUNTER_C");
    EXPECT_EQ(info_c, nullptr);
    EXPECT_EQ(metrics->GetMetricType("COUNTER_C"), Dive::MetricType::kUnknown);
}

TEST(AvailableMetrics, LoadFromCsvIgnoresRowsThatAreMissingColumns)
{
    auto metrics = AvailableMetrics::LoadFromCsv(TEST_DATA_DIR
                                                 "/mock_available_metrics_malformed.csv");
    ASSERT_NE(metrics, nullptr);
    ASSERT_NE(metrics->GetMetricInfo("COUNTER_A"), nullptr);
    ASSERT_EQ(metrics->GetMetricInfo("COUNTER_B"), nullptr);
}

TEST(AvailableMetrics, LoadFromCsvFailsWhenNoHeader)
{
    auto metrics = AvailableMetrics::LoadFromCsv(TEST_DATA_DIR
                                                 "/mock_available_metrics_no_header.csv");
    ASSERT_EQ(metrics, nullptr);
}

}  // namespace
}  // namespace Dive
