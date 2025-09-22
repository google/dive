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
        std::visit([os](auto&& arg) { *os << arg; }, record.m_metric_values[i]);
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
                                                          std::move(available_metrics));
    ASSERT_NE(perf_metrics_data, nullptr);

    const auto& records = perf_metrics_data->GetRecords();
    EXPECT_THAT(records,
                ElementsAre(
                // First incomplete frame
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 999, 10001, 1, 1, 1, 7, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(VariantWith<int64_t>(2109),
                                        VariantWith<float>(FloatEq(2.109f))))),
                // First complete frame:
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1000, 10000, 1, 1, 1, 4, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(VariantWith<int64_t>(1230),
                                        VariantWith<float>(FloatEq(1.230f))))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1000, 10000, 2, 1, 1, 5, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(VariantWith<int64_t>(1100),
                                        VariantWith<float>(FloatEq(1.100f))))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1000, 10000, 3, 1, 1, 6, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(VariantWith<int64_t>(1350),
                                        VariantWith<float>(FloatEq(1.350f))))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1000, 10000, 1, 1, 1, 4, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(VariantWith<int64_t>(1500),
                                        VariantWith<float>(FloatEq(1.500f))))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1000, 10000, 2, 1, 1, 5, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(VariantWith<int64_t>(1200),
                                        VariantWith<float>(FloatEq(1.300f))))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1000, 10000, 3, 1, 1, 6, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(VariantWith<int64_t>(1450),
                                        VariantWith<float>(FloatEq(1.450f))))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1000, 10001, 1, 1, 1, 7, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(VariantWith<int64_t>(2100),
                                        VariantWith<float>(FloatEq(2.100f))))),
                // Second complete frame:
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1001, 10000, 1, 1, 1, 4, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(VariantWith<int64_t>(1232),
                                        VariantWith<float>(FloatEq(1.232f))))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1001, 10000, 2, 1, 1, 5, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(VariantWith<int64_t>(1102),
                                        VariantWith<float>(FloatEq(1.102f))))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1001, 10000, 3, 1, 1, 6, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(VariantWith<int64_t>(1352),
                                        VariantWith<float>(FloatEq(1.352f))))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1001, 10000, 1, 1, 1, 4, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(VariantWith<int64_t>(1502),
                                        VariantWith<float>(FloatEq(1.502f))))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1001, 10000, 2, 1, 1, 5, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(VariantWith<int64_t>(1202),
                                        VariantWith<float>(FloatEq(1.302f))))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1001, 10000, 3, 1, 1, 6, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(VariantWith<int64_t>(1452),
                                        VariantWith<float>(FloatEq(1.452f))))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1001, 10001, 1, 1, 1, 7, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(VariantWith<int64_t>(2102),
                                        VariantWith<float>(FloatEq(2.102f))))),
                // Incomplete frame at the end:
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1002, 10000, 1, 1, 1, 4, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(VariantWith<int64_t>(1234),
                                        VariantWith<float>(FloatEq(1.234f))))),
                AllOf(PerfMetricsRecordEq(
                      PerfMetricsRecord{ 1, 100, 1002, 10000, 2, 1, 1, 5, 1, {} }),
                      Field(&PerfMetricsRecord::m_metric_values,
                            ElementsAre(VariantWith<int64_t>(1104),
                                        VariantWith<float>(FloatEq(1.104f)))))));

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
                                                          std::move(available_metrics));
    ASSERT_NE(perf_metrics_data, nullptr);
    ASSERT_THAT(perf_metrics_data->GetRecords(), SizeIs(1));
    EXPECT_THAT(perf_metrics_data->GetRecords(),
                ElementsAre(AllOf(PerfMetricsRecordEq(
                                  PerfMetricsRecord{ 2, 200, 2000, 20000, 2, 2, 2, 2, 2, {} }),
                                  Field(&PerfMetricsRecord::m_metric_values,
                                        ElementsAre(VariantWith<int64_t>(456),
                                                    VariantWith<float>(FloatEq(4.56f)))))));
}

TEST(PerfMetricsData, LoadFromCsvFailedWithNoHeader)
{
    auto available_metrics = AvailableMetrics::LoadFromCsv(
    TEST_DATA_DIR "/mock_available_metrics_unknown_type.csv");
    ASSERT_NE(available_metrics, nullptr);

    ASSERT_EQ(PerfMetricsData::LoadFromCsv(TEST_DATA_DIR "/mock_perf_metrics_data_no_header.csv",
                                           std::move(available_metrics)),
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
                                                     std::move(available_metrics));
    ASSERT_NE(perf_metrics_data, nullptr);
    ASSERT_THAT(perf_metrics_data->GetRecords(), IsEmpty());
}

TEST(PerfMetricsData, LoadFromCsvUnknownMeticTypeSkipped)
{
    auto available_metrics = AvailableMetrics::LoadFromCsv(
    TEST_DATA_DIR "/mock_available_metrics_unknown_type.csv");
    ASSERT_NE(available_metrics, nullptr);

    auto
    perf_metrics_data = PerfMetricsData::LoadFromCsv(TEST_DATA_DIR
                                                     "/mock_perf_metrics_data_unknown_type.csv",
                                                     std::move(available_metrics));
    ASSERT_NE(perf_metrics_data, nullptr);
    ASSERT_THAT(perf_metrics_data->GetRecords(), IsEmpty());
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
                                                          std::move(available_metrics));
    if (!perf_metrics_data)
    {
        return nullptr;
    }
    return PerfMetricsDataProvider::Create(std::move(perf_metrics_data));
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
    EXPECT_THAT(computed_records[0].m_metric_values,
                ElementsAre(VariantWith<int64_t>(1231), VariantWith<float>(FloatEq(1.231f))));

    PerfMetricsRecord expected_record2{ 1, 100, 0, 10000, 2, 1, 1, 5, 1 };
    EXPECT_THAT(computed_records[1], PerfMetricsRecordEq(expected_record2));
    EXPECT_THAT(computed_records[1].m_metric_values,
                ElementsAre(VariantWith<int64_t>(1101), VariantWith<float>(FloatEq(1.101f))));

    PerfMetricsRecord expected_record3{ 1, 100, 0, 10000, 3, 1, 1, 6, 1 };
    EXPECT_THAT(computed_records[2], PerfMetricsRecordEq(expected_record3));
    EXPECT_THAT(computed_records[2].m_metric_values,
                ElementsAre(VariantWith<int64_t>(1351), VariantWith<float>(FloatEq(1.351f))));

    PerfMetricsRecord expected_record4{ 1, 100, 0, 10000, 1, 1, 1, 4, 1 };
    EXPECT_THAT(computed_records[3], PerfMetricsRecordEq(expected_record4));
    EXPECT_THAT(computed_records[3].m_metric_values,
                ElementsAre(VariantWith<int64_t>(1501), VariantWith<float>(FloatEq(1.501f))));

    PerfMetricsRecord expected_record5{ 1, 100, 0, 10000, 2, 1, 1, 5, 1 };
    EXPECT_THAT(computed_records[4], PerfMetricsRecordEq(expected_record5));
    EXPECT_THAT(computed_records[4].m_metric_values,
                ElementsAre(VariantWith<int64_t>(1201), VariantWith<float>(FloatEq(1.301f))));

    PerfMetricsRecord expected_record6{ 1, 100, 0, 10000, 3, 1, 1, 6, 1 };
    EXPECT_THAT(computed_records[5], PerfMetricsRecordEq(expected_record6));
    EXPECT_THAT(computed_records[5].m_metric_values,
                ElementsAre(VariantWith<int64_t>(1451), VariantWith<float>(FloatEq(1.451f))));

    PerfMetricsRecord expected_record7{ 1, 100, 0, 10001, 1, 1, 1, 7, 1 };
    EXPECT_THAT(computed_records[6], PerfMetricsRecordEq(expected_record7));
    EXPECT_THAT(computed_records[6].m_metric_values,
                ElementsAre(VariantWith<int64_t>(2101), VariantWith<float>(FloatEq(2.101f))));
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
