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
                ElementsAre(AllOf(PerfMetricsRecordEq(
                                  PerfMetricsRecord{ 1, 100, 1000, 10000, 1, 1, 1, 1, 1, {} }),
                                  Field(&PerfMetricsRecord::m_metric_values,
                                        ElementsAre(VariantWith<int64_t>(123),
                                                    VariantWith<float>(FloatEq(1.23f))))),
                            AllOf(PerfMetricsRecordEq(
                                  PerfMetricsRecord{ 2, 200, 2000, 20000, 2, 2, 2, 2, 2, {} }),
                                  Field(&PerfMetricsRecord::m_metric_values,
                                        ElementsAre(VariantWith<int64_t>(456),
                                                    VariantWith<float>(FloatEq(4.56f)))))));

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

class PerfMetricsDataProviderTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        auto available_metrics = AvailableMetrics::LoadFromCsv(TEST_DATA_DIR
                                                               "/mock_available_metrics.csv");
        ASSERT_NE(available_metrics, nullptr);

        auto perf_metrics_data = PerfMetricsData::LoadFromCsv(TEST_DATA_DIR
                                                              "/mock_perf_metrics_data.csv",
                                                              std::move(available_metrics));
        ASSERT_NE(perf_metrics_data, nullptr);

        m_provider = PerfMetricsDataProvider::Create(std::move(perf_metrics_data));
        ASSERT_NE(m_provider, nullptr);
    }

    std::unique_ptr<PerfMetricsDataProvider> m_provider;
};

TEST_F(PerfMetricsDataProviderTest, GetCommandBufferCount)
{
    EXPECT_EQ(m_provider->GetCommandBufferCount(), 2);
}

TEST_F(PerfMetricsDataProviderTest, GetDrawCountForCommandBuffer)
{
    EXPECT_EQ(m_provider->GetDrawCountForCommandBuffer(0), 1);
    EXPECT_EQ(m_provider->GetDrawCountForCommandBuffer(1), 1);
    EXPECT_EQ(m_provider->GetDrawCountForCommandBuffer(2), 0);
}

TEST_F(PerfMetricsDataProviderTest, GetComputedRecords)
{
    const auto& computed_records = m_provider->GetComputedRecords();
    ASSERT_THAT(computed_records, SizeIs(2));

    PerfMetricsRecord expected_record1{ 1, 100, 1000, 10000, 1, 1, 1, 1, 1 };
    EXPECT_THAT(computed_records[0], PerfMetricsRecordEq(expected_record1));
    EXPECT_THAT(computed_records[0].m_metric_values,
                ElementsAre(VariantWith<int64_t>(123), VariantWith<float>(FloatEq(1.23f))));

    PerfMetricsRecord expected_record2{ 2, 200, 2000, 20000, 2, 2, 2, 2, 2 };
    EXPECT_THAT(computed_records[1], PerfMetricsRecordEq(expected_record2));
    EXPECT_THAT(computed_records[1].m_metric_values,
                ElementsAre(VariantWith<int64_t>(456), VariantWith<float>(FloatEq(4.56f))));
}

TEST_F(PerfMetricsDataProviderTest, GetComputedRecord)
{
    auto record1_opt = m_provider->GetComputedRecord(0, 0);
    ASSERT_TRUE(record1_opt.has_value());
    const auto& record1 = record1_opt->get();
    EXPECT_EQ(record1.m_cmd_buffer_id, 10000);
    EXPECT_EQ(record1.m_draw_id, 1);
    EXPECT_THAT(record1.m_metric_values,
                ElementsAre(VariantWith<int64_t>(123), VariantWith<float>(FloatEq(1.23f))));

    EXPECT_FALSE(m_provider->GetComputedRecord(2, 0).has_value());
    EXPECT_FALSE(m_provider->GetComputedRecord(0, 1).has_value());
}

TEST_F(PerfMetricsDataProviderTest, GetMetricsNames)
{
    const auto& names = m_provider->GetMetricsNames();
    ASSERT_THAT(names, ElementsAre("COUNTER_A", "COUNTER_B"));
}

TEST_F(PerfMetricsDataProviderTest, GetMetricsDescription)
{
    EXPECT_EQ(m_provider->GetMetricsDescription(0), "Description A");
    EXPECT_EQ(m_provider->GetMetricsDescription(1), "Description B");
}

TEST(PerfMetricsDataProvider, CreateWithNullData)
{
    EXPECT_EQ(PerfMetricsDataProvider::Create(nullptr), nullptr);
}
}  // namespace

}  // namespace Dive
