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

#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>

#include "absl/base/no_destructor.h"
#include "dive_core/command_hierarchy.h"
#include "dive_core/available_metrics.h"
#include "utils/string_utils.h"

namespace Dive
{
namespace
{

bool IsMetricsRecordDrawOrDispatch(const PerfMetricsRecord& record)
{
    return record.m_draw_type == 1 || record.m_draw_type == 3;
}

// A wrapper type for uint64_t / size_t to reduce the chance of using the wrong index.
template<typename ValueT, typename TagT = void> class IndexWrapper
{
public:
    using ValueType = ValueT;
    using TagType = TagT;

    struct Hash
    {
        std::hash<ValueType> m_impl;

        auto operator()(const IndexWrapper& w) const { return m_impl(w.m_value); }
    };

    static constexpr ValueType kInvalid = std::numeric_limits<ValueType>::max();

    IndexWrapper() = default;
    explicit IndexWrapper(ValueType value) :
        m_value(value)
    {
    }

    std::optional<ValueT> AsOptional() const
    {
        if (has_value())
        {
            return value();
        }
        return std::nullopt;
    }

    // std::optional
    bool      has_value() const { return m_value != kInvalid; }
    ValueType value() const
    {
        assert(has_value());
        return m_value;
    }

    ValueType operator*() const { return value(); }
    explicit  operator bool() const { return has_value(); }

    template<typename T, typename Tag>
    auto Into(const std::vector<IndexWrapper<T, Tag>>& v) -> IndexWrapper<T, Tag>
    {
        if (has_value() && value() < v.size())
        {
            return v[value()];
        }
        return IndexWrapper<T, Tag>();
    }

    template<typename T, typename Tag>
    auto Into(const std::unordered_map<IndexWrapper, IndexWrapper<T, Tag>, Hash>& m)
    -> IndexWrapper<T, Tag>
    {
        if (!has_value())
        {
            return IndexWrapper<T, Tag>();
        }
        if (auto iter = m.find(*this); iter != m.end())
        {
            return iter->second;
        }
        return IndexWrapper<T, Tag>();
    }

    bool operator==(const IndexWrapper& other) const { return m_value == other.m_value; }

private:
    ValueType m_value = kInvalid;
};

}  // namespace

namespace
{

struct ParseHeadersResult
{
    std::vector<std::string>       metric_names;
    std::vector<const MetricInfo*> metric_infos;
};

std::optional<ParseHeadersResult> ParseHeaders(const std::string&      line,
                                               const AvailableMetrics& available_metrics)
{
    std::vector<const MetricInfo*> metric_infos;
    std::vector<std::string>       metric_names;

    std::stringstream header_ss(line);
    std::string       header_field;
    int               column_index = 0;
    while (StringUtils::GetTrimmedField(header_ss, header_field, ','))
    {
        if (column_index < kFixedPerfMetricsDataHeaderCount)
        {
            if (header_field != kFixedHeaders[column_index])
                return std::nullopt;  // Fixed header mismatch
        }
        else
        {
            metric_names.push_back(header_field);
            metric_infos.push_back(available_metrics.GetMetricInfo(header_field));
        }
        column_index++;
    }
    if (column_index < kFixedPerfMetricsDataHeaderCount)
    {
        return std::nullopt;  // Not enough columns
    }

    return ParseHeadersResult{ std::move(metric_names), std::move(metric_infos) };
}

std::optional<PerfMetricsRecord> ParseRecordFixedFields(const std::vector<std::string>& fields)
{
    if (fields.size() < kFixedPerfMetricsDataHeaderCount)
    {
        return std::nullopt;
    }

    PerfMetricsRecord record{};
    if (!StringUtils::SafeConvertFromString(fields[0], record.m_context_id) ||
        !StringUtils::SafeConvertFromString(fields[1], record.m_process_id) ||
        !StringUtils::SafeConvertFromString(fields[2], record.m_frame_id) ||
        !StringUtils::SafeConvertFromString(fields[3], record.m_cmd_buffer_id) ||
        !StringUtils::SafeConvertFromString(fields[4], record.m_draw_id) ||
        !StringUtils::SafeConvertFromString(fields[5], record.m_draw_type) ||
        !StringUtils::SafeConvertFromString(fields[6], record.m_draw_label) ||
        !StringUtils::SafeConvertFromString(fields[7], record.m_program_id) ||
        !StringUtils::SafeConvertFromString(fields[8], record.m_lrz_state))
    {
        return std::nullopt;  // Parsing failed
    }
    return record;
}

bool ParseMetrics(const std::vector<std::string>&       fields,
                  const std::vector<const MetricInfo*>& metric_infos,
                  PerfMetricsRecord&                    record)
{
    for (size_t i = 0; i < metric_infos.size(); ++i)
    {
        const std::string& value_str = fields[kFixedPerfMetricsDataHeaderCount + i];
        const MetricInfo*  info = metric_infos[i];
        if (info == nullptr)
        {
            // Unknown metric, this is an error. The number of metric values
            // will not match the number of metric names.
            return false;
        }

        double value;
        if (!StringUtils::SafeConvertFromString(value_str, value))
        {
            return false;
        }
        record.m_metric_values.emplace_back(value);
    }
    return true;
}

}  // namespace

std::unique_ptr<PerfMetricsData> PerfMetricsData::LoadFromCsv(
const std::filesystem::path& file_path,
const AvailableMetrics&      available_metrics)
{
    std::vector<PerfMetricsRecord> records;

    std::ifstream file(file_path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        return nullptr;
    }

    std::string line;
    // Read header line
    if (!StringUtils::GetTrimmedLine(file, line) || line.empty())
    {
        return nullptr;
    }
    auto headers_opt = ParseHeaders(line, available_metrics);
    if (!headers_opt.has_value())
    {
        return nullptr;
    }

    auto& metric_names = headers_opt->metric_names;
    auto& metric_infos = headers_opt->metric_infos;

    for (size_t i = 0; i < metric_infos.size(); ++i)
    {
        const MetricInfo* info = metric_infos[i];
        if (info == nullptr)
        {
            std::cerr << "Found unknown metric." << std::endl;
            return nullptr;
        }
        switch (info->m_metric_type)
        {
        case MetricType::kCount:
        case MetricType::kPercent:
            break;
        default:
            std::cerr << "Unknown metric type: " << static_cast<int>(info->m_metric_type)
                      << std::endl;
            // kUnknown or other types are not supported.
            return nullptr;
        }
    }
    // Read data lines
    while (StringUtils::GetTrimmedLine(file, line))
    {
        std::stringstream        ss(line);
        std::string              field;
        std::vector<std::string> fields;
        while (StringUtils::GetTrimmedField(ss, field, ','))
        {
            fields.push_back(field);
        }

        if (fields.size() != kFixedPerfMetricsDataHeaderCount + metric_names.size())
        {
            continue;  // Skip malformed lines
        }

        auto record = ParseRecordFixedFields(fields);
        if (!record.has_value())
        {
            continue;  // Skip malformed lines
        }

        if (ParseMetrics(fields, metric_infos, *record))
        {
            records.push_back(*std::move(record));
        }
    }

    return std::unique_ptr<PerfMetricsData>(
    new PerfMetricsData(std::move(metric_names), std::move(metric_infos), std::move(records)));
}

PerfMetricsData::PerfMetricsData(std::vector<std::string>       metric_names,
                                 std::vector<const MetricInfo*> metric_infos,
                                 std::vector<PerfMetricsRecord> records) :
    m_metric_names(std::move(metric_names)),
    m_metric_infos(std::move(metric_infos)),
    m_records(std::move(records))
{
}

class PerfMetricsDataProvider::Correlator
{
    struct NodeTag;
    struct DrawTag;
    struct MetricTag;
    struct RecordTag;

public:
    // Mapping: NodeIndex <-> DrawIndex <-> MetricIndex
    using NodeIndex = IndexWrapper<uint64_t, NodeTag>;
    using DrawIndex = IndexWrapper<uint64_t, DrawTag>;
    using MetricIndex = IndexWrapper<size_t, MetricTag>;

    // RecordIndex is index into raw metric data.
    using RecordIndex = IndexWrapper<size_t, RecordTag>;

    void Reset()
    {
        m_draw_to_node.clear();
        m_node_to_draw.clear();

        m_draw_to_metric.clear();
        m_metric_to_draw.clear();

        m_record_to_metric.clear();
    }

    void AnalyzeCommands(const CommandHierarchy&);

    void AnalyzeRecords(const std::vector<PerfMetricsRecord>&);

    size_t GetPatternSize() const { return m_metric_to_draw.size(); }

    MetricIndex MatchOf(RecordIndex index) const { return index.Into(m_record_to_metric); }

    NodeIndex GetNodeFromDraw(DrawIndex index) const { return index.Into(m_draw_to_node); }
    DrawIndex GetDrawFromNode(NodeIndex index) const { return index.Into(m_node_to_draw); }
    DrawIndex GetDrawFromMetric(MetricIndex index) const
    {
        return RequireCorrelationEnabled(index).Into(m_metric_to_draw);
    }
    MetricIndex GetMetricFromDraw(DrawIndex index) const
    {
        return RequireCorrelationEnabled(index).Into(m_draw_to_metric);
    }
    NodeIndex GetNodeFromMetric(MetricIndex index) const
    {
        return GetNodeFromDraw(GetDrawFromMetric(index));
    }
    MetricIndex GetMetricFromNode(NodeIndex index) const
    {
        return GetMetricFromDraw(GetDrawFromNode(index));
    }

private:
    template<typename, typename U> using ArrayMap = std::vector<U>;
    template<typename T, typename U> using HashMap = std::unordered_map<T, U, typename T::Hash>;

    static void ExtractDraws(const CommandHierarchy&         command_hierarchy,
                             ArrayMap<DrawIndex, NodeIndex>& out_draw_to_node,
                             HashMap<NodeIndex, DrawIndex>&  out_node_to_draw);

    struct DrawSignatures
    {
        const PerfMetricsRecord* m_begin;
        const PerfMetricsRecord* m_end;
    };
    static bool MatchDrawSignatures(const DrawSignatures&,
                                    const PerfMetricsRecord* begin,
                                    const PerfMetricsRecord* end);

    bool CorrelationEnabled() const
    {
        return m_draw_to_node.empty() || m_draw_to_metric.size() == m_draw_to_node.size();
    }
    template<typename IndexT> IndexT RequireCorrelationEnabled(IndexT index) const
    {
        return (CorrelationEnabled() ? index : IndexT());
    }

    ArrayMap<DrawIndex, NodeIndex> m_draw_to_node;
    HashMap<NodeIndex, DrawIndex>  m_node_to_draw;

    ArrayMap<DrawIndex, MetricIndex> m_draw_to_metric;
    ArrayMap<MetricIndex, DrawIndex> m_metric_to_draw;

    ArrayMap<RecordIndex, MetricIndex> m_record_to_metric;
};

void PerfMetricsDataProvider::Correlator::ExtractDraws(
const CommandHierarchy&         command_hierarchy,
ArrayMap<DrawIndex, NodeIndex>& out_draw_to_node,
HashMap<NodeIndex, DrawIndex>&  out_node_to_draw)
{
    // TODO: extract the command buffer pattern.
    HashMap<NodeIndex, DrawIndex> node_to_draw;

    // Draw calls from gfxr command stream.
    ArrayMap<DrawIndex, NodeIndex> gfxr_draws;

    // First instance of the draw call in the command stream (from binning pass).
    ArrayMap<DrawIndex, NodeIndex> actual_draws;
    // Draw call that already being accounted for (tile pass).
    ArrayMap<DrawIndex, NodeIndex> alias_draws;

    ArrayMap<DrawIndex, NodeIndex>* draws = &actual_draws;

    auto dedupe = [&]() {
        // We encountered either submit or render marker, unless it's
        // tile pass, we will be getting first instace of the draw call.
        draws = &actual_draws;

        if (alias_draws.size() <= actual_draws.size())
        {
            // Tile pass should corresponding to the last binning draws.
            size_t base = actual_draws.size() - alias_draws.size();
            for (size_t i = 0; i < alias_draws.size(); ++i)
            {
                node_to_draw[alias_draws[i]] = DrawIndex(base + i);
            }
        }
        alias_draws.clear();
    };

    auto& alias_marker = command_hierarchy.GetFilterExcludeIndices(
    Dive::CommandHierarchy::kBinningPassOnly);

    for (size_t i = 0; i < command_hierarchy.size(); ++i)
    {
        auto node_type = command_hierarchy.GetNodeType(i);
        if (node_type == Dive::NodeType::kSubmitNode)
        {
            dedupe();
        }
        if (node_type == Dive::NodeType::kRenderMarkerNode)
        {
            dedupe();
            if (alias_marker.find(i) != alias_marker.end())
            {
                draws = &alias_draws;
            }
        }
        if (node_type == Dive::NodeType::kGfxrVulkanDrawCommandNode)
        {
            gfxr_draws.push_back(NodeIndex(i));
            continue;
        }

        if (!Dive::IsDrawDispatchNode(node_type))
        {
            // Note: what if both pm4 node and vulkan draw command node exist?
            continue;
        }
        draws->push_back(NodeIndex(i));
    }
    dedupe();
    for (size_t i = 0; i < actual_draws.size(); ++i)
    {
        node_to_draw[actual_draws[i]] = DrawIndex(i);
    }
    for (size_t i = 0; i < gfxr_draws.size(); ++i)
    {
        node_to_draw[gfxr_draws[i]] = DrawIndex(i);
    }

    if (!actual_draws.empty())
    {
        out_draw_to_node = std::move(actual_draws);
    }
    else
    {
        out_draw_to_node = std::move(gfxr_draws);
    }
    out_node_to_draw = std::move(node_to_draw);
}

void PerfMetricsDataProvider::Correlator::AnalyzeCommands(const CommandHierarchy& command_hierarchy)
{
    ExtractDraws(command_hierarchy, m_draw_to_node, m_node_to_draw);
}

bool PerfMetricsDataProvider::Correlator::MatchDrawSignatures(const DrawSignatures&    signatures,
                                                              const PerfMetricsRecord* begin,
                                                              const PerfMetricsRecord* end)
{
    const ptrdiff_t size = (signatures.m_end - signatures.m_begin);
    if (size != end - begin)
    {
        return false;
    }
    const PerfMetricsRecord* at = begin;
    const PerfMetricsRecord* signatures_at = signatures.m_begin;
    for (; at != end; ++at, ++signatures_at)
    {

        if (at->m_cmd_buffer_id != signatures_at->m_cmd_buffer_id)
        {
            return false;
        }
        if (at->m_draw_id != signatures_at->m_draw_id)
        {
            return false;
        }
    }
    return true;
}

void PerfMetricsDataProvider::Correlator::AnalyzeRecords(
const std::vector<PerfMetricsRecord>& records)
{
    m_record_to_metric.clear();

    if (records.empty())
    {
        return;
    }
    size_t template_frame_start = 0;
    size_t template_frame_size = 0;

    // Find the frame with the max number of draw calls, and use that as template.
    {
        size_t frame_start = 0;
        auto   emit_frame = [&](size_t start, size_t end) {
            if (end - start > template_frame_size)
            {
                template_frame_start = start;
                template_frame_size = end - start;
            }
        };
        for (size_t i = 0; i < records.size(); ++i)
        {
            if (records[frame_start].m_frame_id != records[i].m_frame_id)
            {
                emit_frame(frame_start, i);
                frame_start = i;
            }
        }
        emit_frame(frame_start, records.size());
    }

    ArrayMap<DrawIndex, MetricIndex> draw_to_metric;
    ArrayMap<MetricIndex, DrawIndex> metric_to_draw;
    metric_to_draw.resize(template_frame_size);
    for (size_t i = 0; i < template_frame_size; ++i)
    {
        if (IsMetricsRecordDrawOrDispatch(records[template_frame_start + i]))
        {
            metric_to_draw[i] = DrawIndex(draw_to_metric.size());
            draw_to_metric.push_back(MetricIndex(i));
        }
    }

    if (!m_draw_to_node.empty() && m_draw_to_node.size() != draw_to_metric.size())
    {
        std::cerr << "Mismatch draw calls in performance counter data." << std::endl;
    }

    const DrawSignatures signature = {
        records.data() + template_frame_start,
        records.data() + template_frame_start + template_frame_size,
    };

    ArrayMap<RecordIndex, MetricIndex> record_to_metric(records.size());
    {
        size_t frame_start = 0;
        auto   emit_frame = [&](size_t start, size_t end) {
            if (!MatchDrawSignatures(signature, records.data() + start, records.data() + end))
            {
                // Bad data?
                return;
            }
            const size_t frame_size = end - start;
            for (int i = 0; i < frame_size; ++i)
            {
                record_to_metric[start + i] = MetricIndex(i);
            }
        };
        for (size_t i = 0; i < records.size(); ++i)
        {
            if (records[frame_start].m_frame_id != records[i].m_frame_id)
            {
                emit_frame(frame_start, i);
                frame_start = i;
            }
        }
        emit_frame(frame_start, records.size());
    }

    m_record_to_metric = std::move(record_to_metric);
    m_draw_to_metric = std::move(draw_to_metric);
    m_metric_to_draw = std::move(metric_to_draw);
}

std::unique_ptr<PerfMetricsDataProvider> PerfMetricsDataProvider::Create(
std::unique_ptr<PerfMetricsData> data)
{
    auto result = std::unique_ptr<PerfMetricsDataProvider>(new PerfMetricsDataProvider);
    result->Update(std::move(data));
    return result;
}

std::unique_ptr<PerfMetricsDataProvider> PerfMetricsDataProvider::CreateForTest(
std::unique_ptr<PerfMetricsData>  data,
std::unique_ptr<AvailableMetrics> desc)
{
    auto result = Create(std::move(data));
    result->m_owned_desc = std::move(desc);
    return result;
}

PerfMetricsDataProvider::~PerfMetricsDataProvider() = default;
PerfMetricsDataProvider::PerfMetricsDataProvider() :
    m_correlator(new Correlator)
{
}

void PerfMetricsDataProvider::Update(std::unique_ptr<PerfMetricsData> data)
{
    if (!data)
    {
        return;
    }

    m_raw_data = std::move(data);
    m_computed_records.clear();
    m_correlator->Reset();
}

void PerfMetricsDataProvider::Analyze(const CommandHierarchy* command_hierarchy)
{
    if (!m_raw_data)
    {
        return;
    }
    const size_t num_metrics = m_raw_data->GetMetricNames().size();
    const auto&  records = m_raw_data->GetRecords();
    m_correlator->Reset();
    if (command_hierarchy)
    {
        m_correlator->AnalyzeCommands(*command_hierarchy);
    }
    m_correlator->AnalyzeRecords(records);

    const size_t pattern_size = m_correlator->GetPatternSize();
    m_computed_records.resize(pattern_size);

    // This map will store intermediate sums and counts for averaging.
    // Key: {cmd_buffer_id, draw_id}
    // Value: {vector of metric sums, count of records}
    struct MetricSumWithRecordCount
    {
        std::vector<double> m_metric_sums{};
        uint32_t            m_record_count = 0;
    };

    std::vector<MetricSumWithRecordCount> sums_and_counts;
    sums_and_counts.resize(pattern_size);

    size_t skipped = 0;
    for (size_t record_index = 0; record_index < records.size(); ++record_index)
    {
        auto pattern_index = m_correlator->MatchOf(Correlator::RecordIndex(record_index));
        if (!pattern_index)
        {
            ++skipped;
            continue;
        }
        auto& record = records[record_index];
        auto& draw_call_data = sums_and_counts[*pattern_index];
        if (draw_call_data.m_record_count == 0)
        {
            draw_call_data.m_metric_sums.resize(num_metrics);

            m_computed_records[*pattern_index] = record;
        }

        if (record.m_metric_values.size() == num_metrics)
        {
            for (size_t i = 0; i < num_metrics; ++i)
            {
                draw_call_data.m_metric_sums[i] += record.m_metric_values[i];
            }
            draw_call_data.m_record_count++;
        }
    }
    if (skipped)
    {
        std::cerr << "Skipping " << skipped << " metrics." << std::endl;
    }

    // Now, build the final m_computed_records vector with averaged records in order.
    for (size_t draw_index = 0; draw_index < pattern_size; ++draw_index)
    {
        PerfMetricsRecord& averaged_record = m_computed_records[draw_index];
        averaged_record.m_metric_values.clear();
        // frame_id for aggregated data is meaningless.
        averaged_record.m_frame_id = 0;

        const auto count = sums_and_counts[draw_index].m_record_count;
        auto&      sums = sums_and_counts[draw_index].m_metric_sums;
        if (count == 0 || sums.empty())
        {
            continue;
        }

        averaged_record.m_metric_values.reserve(num_metrics);
        for (size_t i = 0; i < num_metrics; ++i)
        {
            averaged_record.m_metric_values.emplace_back(sums[i] / count);
        }
    }
}

const std::vector<std::string> PerfMetricsDataProvider::GetRecordHeader() const
{
    std::vector<std::string> full_header;
    full_header.reserve(kFixedPerfMetricsDataHeaderCount + GetMetricsNames().size());
    for (const auto& header : kFixedHeaders)
    {
        full_header.push_back(header);
    }
    for (const auto& metric_name : GetMetricsNames())
    {
        full_header.push_back(metric_name);
    }
    return full_header;
}

std::optional<uint64_t> PerfMetricsDataProvider::GetCorrelatedComputedRecordIndex(
uint64_t node_index) const
{
    return m_correlator->GetMetricFromNode(Correlator::NodeIndex(node_index)).AsOptional();
}

std::optional<uint64_t> PerfMetricsDataProvider::GetComputedRecordIndexFromDrawIndex(
uint64_t draw_index) const
{
    return m_correlator->GetMetricFromDraw(Correlator::DrawIndex(draw_index)).AsOptional();
}

std::optional<uint64_t> PerfMetricsDataProvider::GetDrawIndexFromComputedRecordIndex(
uint64_t index) const
{
    return m_correlator->GetDrawFromMetric(Correlator::MetricIndex(index)).AsOptional();
}

const std::vector<std::string>& PerfMetricsDataProvider::GetMetricsNames() const
{
    if (m_raw_data == nullptr)
    {
        static const absl::NoDestructor<std::vector<std::string>> empty;
        return *empty;
    }
    return m_raw_data->GetMetricNames();
}

std::string_view PerfMetricsDataProvider::GetMetricsDescription(size_t metric_index) const
{
    const auto& metrics_info = m_raw_data->GetMetricInfos();
    if (metric_index >= metrics_info.size() || metrics_info[metric_index] == nullptr)
    {
        return {};
    }
    return metrics_info[metric_index]->m_description;
}

}  // namespace Dive
