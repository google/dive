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

#include "dive_core/perf_counter_data.h"
#include "dive_core/available_metrics.h"
#include "gtest/gtest.h"

TEST(PerfCounterData, LoadFromCsv)
{
    Dive::AvailableMetrics available_metrics;
    ASSERT_TRUE(available_metrics.LoadFromCsv("dive_core/tests/mock_available_metrics.csv"));

    Dive::PerfCounterData perf_counter_data;
    ASSERT_TRUE(
    perf_counter_data.LoadFromCsv("dive_core/tests/mock_perf_counter_data.csv", available_metrics));

    const auto& records = perf_counter_data.GetRecords();
    ASSERT_EQ(records.size(), 2);

    // Check first record
    const auto& record1 = records[0];
    EXPECT_EQ(record1.m_context_id, 1);
    EXPECT_EQ(record1.m_process_id, 100);
    EXPECT_EQ(record1.m_frame_id, 1000);
    EXPECT_EQ(record1.m_cmd_buffer_id, 10000);
    EXPECT_EQ(record1.m_draw_id, 1);
    EXPECT_EQ(record1.m_draw_type, 1);
    EXPECT_EQ(record1.m_draw_label, 1);
    EXPECT_EQ(record1.m_program_id, 1);
    EXPECT_EQ(record1.m_lrz_state, 1);
    ASSERT_EQ(record1.m_counter_values.size(), 2);
    EXPECT_EQ(std::get<int64_t>(record1.m_counter_values[0]), 123);
    EXPECT_FLOAT_EQ(std::get<float>(record1.m_counter_values[1]), 1.23f);

    // Check second record
    const auto& record2 = records[1];
    EXPECT_EQ(record2.m_context_id, 2);
    EXPECT_EQ(record2.m_process_id, 200);
    EXPECT_EQ(record2.m_frame_id, 2000);
    EXPECT_EQ(record2.m_cmd_buffer_id, 20000);
    EXPECT_EQ(record2.m_draw_id, 2);
    EXPECT_EQ(record2.m_draw_type, 2);
    EXPECT_EQ(record2.m_draw_label, 2);
    EXPECT_EQ(record2.m_program_id, 2);
    EXPECT_EQ(record2.m_lrz_state, 2);
    ASSERT_EQ(record2.m_counter_values.size(), 2);
    EXPECT_EQ(std::get<int64_t>(record2.m_counter_values[0]), 456);
    EXPECT_FLOAT_EQ(std::get<float>(record2.m_counter_values[1]), 4.56f);

    const auto& counter_names = perf_counter_data.GetCounterNames();
    ASSERT_EQ(counter_names.size(), 2);
    EXPECT_EQ(counter_names[0], "COUNTER_A");
    EXPECT_EQ(counter_names[1], "COUNTER_B");

    const auto& counter_infos = perf_counter_data.GetCounterInfo();
    ASSERT_EQ(counter_infos.size(), 2);
    ASSERT_NE(counter_infos[0], nullptr);
    EXPECT_EQ(counter_infos[0]->m_metric_id, 1);
    ASSERT_NE(counter_infos[1], nullptr);
    EXPECT_EQ(counter_infos[1]->m_metric_id, 2);
}

TEST(PerfCounterData, LoadFromCsv_Malformed)
{
    Dive::AvailableMetrics available_metrics;
    ASSERT_TRUE(available_metrics.LoadFromCsv("dive_core/tests/mock_available_metrics.csv"));
    Dive::PerfCounterData perf_counter_data;
    ASSERT_TRUE(
    perf_counter_data.LoadFromCsv("dive_core/tests/mock_perf_counter_data_malformed.csv",
                                  available_metrics));
    const auto& records = perf_counter_data.GetRecords();
    ASSERT_EQ(records.size(), 1);
}

TEST(PerfCounterData, LoadFromCsv_NoHeader)
{
    Dive::AvailableMetrics available_metrics;
    ASSERT_TRUE(available_metrics.LoadFromCsv("dive_core/tests/mock_available_metrics.csv"));
    Dive::PerfCounterData perf_counter_data;
    ASSERT_TRUE(
    perf_counter_data.LoadFromCsv("dive_core/tests/mock_perf_counter_data_no_header.csv",
                                  available_metrics));
    const auto& records = perf_counter_data.GetRecords();
    ASSERT_EQ(records.size(), 1);
    EXPECT_EQ(records[0].m_context_id, 2);
}

TEST(PerfCounterData, LoadFromCsv_WrongColumns)
{
    Dive::AvailableMetrics available_metrics;
    ASSERT_TRUE(available_metrics.LoadFromCsv("dive_core/tests/mock_available_metrics.csv"));
    Dive::PerfCounterData perf_counter_data;
    ASSERT_TRUE(
    perf_counter_data.LoadFromCsv("dive_core/tests/mock_perf_counter_data_wrong_columns.csv",
                                  available_metrics));
    const auto& records = perf_counter_data.GetRecords();
    ASSERT_EQ(records.size(), 0);
}

TEST(PerfCounterData, LoadFromCsv_UnknownType)
{
    Dive::AvailableMetrics available_metrics;
    ASSERT_TRUE(
    available_metrics.LoadFromCsv("dive_core/tests/mock_available_metrics_unknown_type.csv"));
    Dive::PerfCounterData perf_counter_data;
    ASSERT_TRUE(
    perf_counter_data.LoadFromCsv("dive_core/tests/mock_perf_counter_data_unknown_type.csv",
                                  available_metrics));
    const auto& records = perf_counter_data.GetRecords();
    ASSERT_EQ(records.size(), 1);
    ASSERT_EQ(records[0].m_counter_values.size(), 0);
}