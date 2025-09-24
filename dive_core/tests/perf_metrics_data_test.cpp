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
#include "gmock/gmock.h"

namespace Dive
{

void PrintTo(const PerfMetricsRecord& record, std::ostream* os)
{
    *os << "PerfMetricsRecord {" << std::endl;
    *os << "  m_context_id: " << record.m_context_id << "," << std::endl;
    *os << "  m_process_id: " << record.m_process_id << "," << std::endl;
    *os << "  m_frame_id: " << record.m_frame_id << "," << std::endl;
    *os << "  m_cmd_buffer_id: " << record.m_cmd_buffer_id << "," << std::endl;
    *os << "  m_draw_id: " << record.m_draw_id << "," << std::endl;
    *os << "  m_draw_label: " << record.m_draw_label << "," << std::endl;
    *os << "  m_program_id: " << record.m_program_id << "," << std::endl;
    *os << "  m_draw_type: " << static_cast<int>(record.m_draw_type) << "," << std::endl;
    *os << "  m_lrz_state: " << static_cast<int>(record.m_lrz_state) << "," << std::endl;
    *os << "  m_metric_values: [";
    for (size_t i = 0; i < record.m_metric_values.size(); ++i)
    {
        *os << record.m_metric_values[i];
        if (i < record.m_metric_values.size() - 1)
        {
            *os << ", ";
        }
    }
    *os << "]" << std::endl;
    *os << "}";
}

namespace
{

using ::testing::AllOf;
using ::testing::DoubleEq;
using ::testing::ElementsAre;
using ::testing::Field;
using ::testing::FloatEq;
using ::testing::IsEmpty;
using ::testing::SizeIs;
using ::testing::VariantWith;

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

TEST(PerfMetricsData, LoadFromCsv)
{
    auto available_metrics = AvailableMetrics::LoadFromCsv(TEST_DATA_DIR
                                                           "/mock_available_metrics.csv");
    ASSERT_NE(available_metrics, nullptr);

    auto perf_metrics_data = PerfMetricsData::LoadFromCsv(TEST_DATA_DIR
                                                          "/mock_perf_metrics_data.csv",
                                                          *available_metrics);
    ASSERT_NE(perf_metrics_data, nullptr);

    const auto& records = perf_metrics_data->GetRecords();
    EXPECT_THAT(records,
                ElementsAre(
                // First incomplete frame
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 999, 10001, 1, 1, 1, 7, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(DoubleEq(2109), DoubleEq(2.109)))),
                // First complete frame:
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1000, 10000, 1, 1, 1, 4, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(DoubleEq(1230), DoubleEq(1.230)))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1000, 10000, 2, 1, 1, 5, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(DoubleEq(1100), DoubleEq(1.100)))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1000, 10000, 3, 1, 1, 6, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(DoubleEq(1350), DoubleEq(1.350)))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1000, 10000, 1, 1, 1, 4, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(DoubleEq(1500), DoubleEq(1.500)))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1000, 10000, 2, 1, 1, 5, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(DoubleEq(1200), DoubleEq(1.300)))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1000, 10000, 3, 1, 1, 6, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(DoubleEq(1450), DoubleEq(1.450)))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1000, 10001, 1, 1, 1, 7, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(DoubleEq(2100), DoubleEq(2.100)))),
                // Second complete frame:
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1001, 10000, 1, 1, 1, 4, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(DoubleEq(1232), DoubleEq(1.232)))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1001, 10000, 2, 1, 1, 5, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(DoubleEq(1102), DoubleEq(1.102)))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1001, 10000, 3, 1, 1, 6, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(DoubleEq(1352), DoubleEq(1.352)))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1001, 10000, 1, 1, 1, 4, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(DoubleEq(1502), DoubleEq(1.502)))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1001, 10000, 2, 1, 1, 5, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(DoubleEq(1202), DoubleEq(1.302)))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1001, 10000, 3, 1, 1, 6, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(DoubleEq(1452), DoubleEq(1.452)))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1001, 10001, 1, 1, 1, 7, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(DoubleEq(2102), DoubleEq(2.102)))),
                // Incomplete frame at the end:
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1002, 10000, 1, 1, 1, 4, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(DoubleEq(1234), DoubleEq(1.234)))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1002, 10000, 2, 1, 1, 5, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(DoubleEq(1104), DoubleEq(1.104))))));

    ASSERT_THAT(perf_metrics_data->GetMetricNames(), ElementsAre("COUNTER_A", "COUNTER_B"));

    const auto& metric_infos = perf_metrics_data->GetMetricInfos();
    ASSERT_THAT(metric_infos, SizeIs(2));

    ASSERT_NE(metric_infos[0], nullptr);
    EXPECT_EQ(metric_infos[0]->m_metric_id, 1);
    ASSERT_NE(metric_infos[1], nullptr);
    EXPECT_EQ(metric_infos[1]->m_metric_id, 2);
}

TEST(PerfMetricsData, LoadFromCsvMalformedRowSkipped)
{
    auto available_metrics = AvailableMetrics::LoadFromCsv(TEST_DATA_DIR
                                                           "/mock_available_metrics.csv");
    ASSERT_NE(available_metrics, nullptr);

    auto perf_metrics_data = PerfMetricsData::LoadFromCsv(TEST_DATA_DIR
                                                          "/mock_perf_metrics_data_malformed.csv",
                                                          *available_metrics);
    ASSERT_NE(perf_metrics_data, nullptr);
    ASSERT_THAT(perf_metrics_data->GetRecords(), SizeIs(1));
    EXPECT_THAT(perf_metrics_data->GetRecords(),
                ElementsAre(AllOf(PerfMetricsRecordEq(
                                  PerfMetricsRecord{ 2, 200, 2000, 20000, 2, 2, 2, 2, 2, {} }),
                                  Field(&PerfMetricsRecord::m_metric_values,
                                        ElementsAre(DoubleEq(456), DoubleEq(4.56))))));
}

TEST(PerfMetricsData, LoadFromCsvFailedWithNoHeader)
{
    auto available_metrics = AvailableMetrics::LoadFromCsv(
    TEST_DATA_DIR "/mock_available_metrics_unknown_type.csv");
    ASSERT_NE(available_metrics, nullptr);

    ASSERT_EQ(PerfMetricsData::LoadFromCsv(TEST_DATA_DIR "/mock_perf_metrics_data_no_header.csv",
                                           *available_metrics),
              nullptr);
}

TEST(PerfMetricsData, LoadFromCsvWrongColumnsSkipped)
{
    auto available_metrics = AvailableMetrics::LoadFromCsv(
    TEST_DATA_DIR "/mock_available_metrics_unknown_type.csv");
    ASSERT_NE(available_metrics, nullptr);

    auto
    perf_metrics_data = PerfMetricsData::LoadFromCsv(TEST_DATA_DIR
                                                     "/mock_perf_metrics_data_wrong_columns.csv",
                                                     *available_metrics);
    ASSERT_EQ(perf_metrics_data, nullptr);
}

TEST(PerfMetricsData, LoadFromCsvUnknownMeticTypeSkipped)
{
    auto available_metrics = AvailableMetrics::LoadFromCsv(
    TEST_DATA_DIR "/mock_available_metrics_unknown_type.csv");
    ASSERT_NE(available_metrics, nullptr);

    auto
    perf_metrics_data = PerfMetricsData::LoadFromCsv(TEST_DATA_DIR
                                                     "/mock_perf_metrics_data_unknown_type.csv",
                                                     *available_metrics);
    ASSERT_EQ(perf_metrics_data, nullptr);
}

std::unique_ptr<PerfMetricsDataProvider> CreateTestMetricProvider()
{
    auto available_metrics = AvailableMetrics::LoadFromCsv(TEST_DATA_DIR
                                                           "/mock_available_metrics.csv");
    if (!available_metrics)
    {
        return nullptr;
    }
    auto perf_metrics_data = PerfMetricsData::LoadFromCsv(TEST_DATA_DIR
                                                          "/mock_perf_metrics_data.csv",
                                                          *available_metrics);
    if (!perf_metrics_data)
    {
        return nullptr;
    }
    return PerfMetricsDataProvider::CreateForTest(std::move(perf_metrics_data),
                                                  std::move(available_metrics));
}

TEST(PerfMetricsDataProviderTest, GetComputedRecords)
{
    auto provider = CreateTestMetricProvider();
    ASSERT_NE(provider, nullptr);
    provider->Analyze(nullptr);
    const auto& computed_records = provider->GetComputedRecords();
    ASSERT_THAT(computed_records, SizeIs(7));

    PerfMetricsRecord expected_record1{ 1, 100, 0, 10000, 1, 1, 1, 4, 1 };
    EXPECT_THAT(computed_records[0], PerfMetricsRecordEq(expected_record1));
    EXPECT_THAT(computed_records[0].m_metric_values, ElementsAre(DoubleEq(1231), DoubleEq(1.231)));

    PerfMetricsRecord expected_record2{ 1, 100, 0, 10000, 2, 1, 1, 5, 1 };
    EXPECT_THAT(computed_records[1], PerfMetricsRecordEq(expected_record2));
    EXPECT_THAT(computed_records[1].m_metric_values, ElementsAre(DoubleEq(1101), DoubleEq(1.101)));

    PerfMetricsRecord expected_record3{ 1, 100, 0, 10000, 3, 1, 1, 6, 1 };
    EXPECT_THAT(computed_records[2], PerfMetricsRecordEq(expected_record3));
    EXPECT_THAT(computed_records[2].m_metric_values, ElementsAre(DoubleEq(1351), DoubleEq(1.351)));

    PerfMetricsRecord expected_record4{ 1, 100, 0, 10000, 1, 1, 1, 4, 1 };
    EXPECT_THAT(computed_records[3], PerfMetricsRecordEq(expected_record4));
    EXPECT_THAT(computed_records[3].m_metric_values, ElementsAre(DoubleEq(1501), DoubleEq(1.501)));

    PerfMetricsRecord expected_record5{ 1, 100, 0, 10000, 2, 1, 1, 5, 1 };
    EXPECT_THAT(computed_records[4], PerfMetricsRecordEq(expected_record5));
    EXPECT_THAT(computed_records[4].m_metric_values, ElementsAre(DoubleEq(1201), DoubleEq(1.301)));

    PerfMetricsRecord expected_record6{ 1, 100, 0, 10000, 3, 1, 1, 6, 1 };
    EXPECT_THAT(computed_records[5], PerfMetricsRecordEq(expected_record6));
    EXPECT_THAT(computed_records[5].m_metric_values, ElementsAre(DoubleEq(1451), DoubleEq(1.451)));

    PerfMetricsRecord expected_record7{ 1, 100, 0, 10001, 1, 1, 1, 7, 1 };
    EXPECT_THAT(computed_records[6], PerfMetricsRecordEq(expected_record7));
    EXPECT_THAT(computed_records[6].m_metric_values, ElementsAre(DoubleEq(2101), DoubleEq(2.101)));
}

TEST(PerfMetricsDataProviderTest, GetRecordHeader)
{
    auto provider = CreateTestMetricProvider();
    ASSERT_NE(provider, nullptr);
    ASSERT_THAT(provider->GetRecordHeader(),
                ElementsAre("ContextID",
                            "ProcessID",
                            "FrameID",
                            "CmdBufferID",
                            "DrawID",
                            "DrawType",
                            "DrawLabel",
                            "ProgramID",
                            "LRZState",
                            "COUNTER_A",
                            "COUNTER_B"));
}

TEST(PerfMetricsDataProviderTest, GetMetricsNames)
{
    auto provider = CreateTestMetricProvider();
    ASSERT_NE(provider, nullptr);
    ASSERT_THAT(provider->GetMetricsNames(), ElementsAre("COUNTER_A", "COUNTER_B"));
}

TEST(PerfMetricsDataProviderTest, GetMetricsDescription)
{
    auto provider = CreateTestMetricProvider();
    ASSERT_NE(provider, nullptr);
    EXPECT_EQ(provider->GetMetricsDescription(0), "Description A");
    EXPECT_EQ(provider->GetMetricsDescription(1), "Description B");
}

}  // namespace

}  // namespace Dive
