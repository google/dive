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

#include "dive_core/perf_metrics_data.h"
#include "dive_core/available_metrics.h"
#include "gtest/gtest.h"
#include <filesystem>
#include "gmock/gmock.h"

namespace Dive
{

namespace
{

MATCHER_P(PerfMetricsRecordEq, expected, "has the correct perf metrics record fields")
{
    EXPECT_EQ(arg.m_context_id, expected.m_context_id);
    EXPECT_EQ(arg.m_process_id, expected.m_process_id);
    EXPECT_EQ(arg.m_frame_id, expected.m_frame_id);
    EXPECT_EQ(arg.m_cmd_buffer_id, expected.m_cmd_buffer_id);
    EXPECT_EQ(arg.m_draw_id, expected.m_draw_id);
    EXPECT_EQ(arg.m_draw_label, expected.m_draw_label);
    EXPECT_EQ(arg.m_program_id, expected.m_program_id);
    EXPECT_EQ(arg.m_draw_type, expected.m_draw_type);
    EXPECT_EQ(arg.m_lrz_state, expected.m_lrz_state);
    return true;
}

class PerfMetricsDataTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_available_metrics = Dive::AvailableMetrics::LoadFromCsv(TEST_DATA_DIR
                                                                  "/mock_available_metrics.csv");
        ASSERT_TRUE(m_available_metrics);
    }
    std::optional<Dive::AvailableMetrics> m_available_metrics;
};

TEST_F(PerfMetricsDataTest, LoadFromCsv)
{
    auto perf_metrics_data = Dive::PerfMetricsData::LoadFromCsv(TEST_DATA_DIR
                                                                "/mock_perf_metrics_data.csv",
                                                                *m_available_metrics);
    ASSERT_TRUE(perf_metrics_data);

    const auto& records = perf_metrics_data->GetRecords();
    ASSERT_EQ(records.size(), 2);

    // Check first record
    Dive::PerfMetricsRecord expected_record1;
    expected_record1.m_context_id = 1;
    expected_record1.m_process_id = 100;
    expected_record1.m_frame_id = 1000;
    expected_record1.m_cmd_buffer_id = 10000;
    expected_record1.m_draw_id = 1;
    expected_record1.m_draw_label = 1;
    expected_record1.m_program_id = 1;
    expected_record1.m_draw_type = 1;
    expected_record1.m_lrz_state = 1;
    const auto& record1 = records[0];
    EXPECT_THAT(record1, PerfMetricsRecordEq(expected_record1));
    ASSERT_EQ(record1.m_metric_values.size(), 2);
    EXPECT_EQ(std::get<int64_t>(record1.m_metric_values[0]), 123);
    EXPECT_FLOAT_EQ(std::get<float>(record1.m_metric_values[1]), 1.23f);

    // Check second record
    Dive::PerfMetricsRecord expected_record2;
    expected_record2.m_context_id = 2;
    expected_record2.m_process_id = 200;
    expected_record2.m_frame_id = 2000;
    expected_record2.m_cmd_buffer_id = 20000;
    expected_record2.m_draw_id = 2;
    expected_record2.m_draw_label = 2;
    expected_record2.m_program_id = 2;
    expected_record2.m_draw_type = 2;
    expected_record2.m_lrz_state = 2;
    const auto& record2 = records[1];
    EXPECT_THAT(record2, PerfMetricsRecordEq(expected_record2));
    ASSERT_EQ(record2.m_metric_values.size(), 2);
    EXPECT_EQ(std::get<int64_t>(record2.m_metric_values[0]), 456);
    EXPECT_FLOAT_EQ(std::get<float>(record2.m_metric_values[1]), 4.56f);

    const auto& metric_names = perf_metrics_data->GetMetricNames();
    ASSERT_EQ(metric_names.size(), 2);
    EXPECT_EQ(metric_names[0], "COUNTER_A");
    EXPECT_EQ(metric_names[1], "COUNTER_B");

    const auto& metric_infos = perf_metrics_data->GetMetricInfos();
    ASSERT_EQ(metric_infos.size(), 2);
    ASSERT_NE(metric_infos[0], nullptr);
    EXPECT_EQ(metric_infos[0]->m_metric_id, 1);
    ASSERT_NE(metric_infos[1], nullptr);
    EXPECT_EQ(metric_infos[1]->m_metric_id, 2);
}

TEST_F(PerfMetricsDataTest, LoadFromCsvMalformedRowSkipped)
{
    auto
    perf_metrics_data = Dive::PerfMetricsData::LoadFromCsv(TEST_DATA_DIR
                                                           "/mock_perf_metrics_data_malformed.csv",
                                                           *m_available_metrics);
    ASSERT_TRUE(perf_metrics_data);
    const auto& records = perf_metrics_data->GetRecords();
    ASSERT_EQ(records.size(), 1);
}

TEST_F(PerfMetricsDataTest, LoadFromCsvFailedWithNoHeader)
{
    auto
    perf_metrics_data = Dive::PerfMetricsData::LoadFromCsv(TEST_DATA_DIR
                                                           "/mock_perf_metrics_data_no_header.csv",
                                                           *m_available_metrics);
    ASSERT_FALSE(perf_metrics_data);
}

TEST_F(PerfMetricsDataTest, LoadFromCsvWrongColumnsSkipped)
{
    auto perf_metrics_data = Dive::PerfMetricsData::
    LoadFromCsv(TEST_DATA_DIR "/mock_perf_metrics_data_wrong_columns.csv", *m_available_metrics);
    ASSERT_TRUE(perf_metrics_data);
    const auto& records = perf_metrics_data->GetRecords();
    ASSERT_EQ(records.size(), 0);
}

TEST(PerfMetricsData, LoadFromCsvUnknownMeticTypeSkipped)
{
    auto available_metrics = Dive::AvailableMetrics::LoadFromCsv(
    TEST_DATA_DIR "/mock_available_metrics_unknown_type.csv");
    ASSERT_TRUE(available_metrics);

    auto perf_metrics_data = Dive::PerfMetricsData::
    LoadFromCsv(TEST_DATA_DIR "/mock_perf_metrics_data_unknown_type.csv", *available_metrics);
    ASSERT_TRUE(perf_metrics_data);

    const auto& records = perf_metrics_data->GetRecords();
    ASSERT_EQ(records.size(), 0);
}
}  // namespace

}  // namespace Dive
