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

#include "perf_counter_model.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QDebug>
#include <optional>

#include "dive_core/available_metrics.h"
#include "dive_core/perf_metrics_data.h"

struct FixedHeader
{
    enum Type : int
    {
        kDrawID,
        kLRZState,
        kFixedHeaderCount
    };
};

PerfCounterModel::PerfCounterModel(QObject *parent) :
    QAbstractItemModel(parent)
{
    m_search_iterator = m_search_results.cbegin();
}

//--------------------------------------------------------------------------------------------------
void PerfCounterModel::OnPerfCounterResultsGenerated(
const std::filesystem::path                                        &file_path,
std::optional<std::reference_wrapper<const Dive::AvailableMetrics>> available_metrics)
{
    emit beginResetModel();

    m_search_results.clear();
    m_search_iterator = nullptr;
    m_headers.clear();
    m_column_count = 0;

    if (file_path.empty() || !available_metrics.has_value())
    {
        emit endResetModel();
        return;
    }

    auto perf_metrics_data = Dive::PerfMetricsData::LoadFromCsv(file_path, *available_metrics);
    m_perf_metrics_data_provider = Dive::PerfMetricsDataProvider::Create(
    std::move(perf_metrics_data));
    m_perf_metrics_data_provider->Analyze();
    LoadData();
    emit endResetModel();
}

//--------------------------------------------------------------------------------------------------
void PerfCounterModel::LoadData()
{
    QStringList headers;

    headers.append(QString::fromStdString(Dive::kHeaderMap.at(Dive::kDrawID).second));
    headers.append(QString::fromStdString(Dive::kHeaderMap.at(Dive::kLRZState).second));

    for (const auto &header_str : m_perf_metrics_data_provider->GetMetricsNames())
    {
        headers.append(QString::fromStdString(header_str));
    }

    m_headers = headers;
    m_column_count = static_cast<int>(m_headers.size());
    m_perf_metrics_record = m_perf_metrics_data_provider->GetComputedRecords();
}

//--------------------------------------------------------------------------------------------------
QModelIndex PerfCounterModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || !m_perf_metrics_data_provider)
    {
        return QModelIndex();
    }

    const size_t num_rows = m_perf_metrics_record.size();

    if (row < 0 || static_cast<size_t>(row) >= num_rows || column < 0 || column >= columnCount())
    {
        return QModelIndex();
    }
    return createIndex(row, column, (void *)0);
}

//--------------------------------------------------------------------------------------------------
QModelIndex PerfCounterModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

//--------------------------------------------------------------------------------------------------
int PerfCounterModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !m_perf_metrics_data_provider)
    {
        return 0;
    }
    return static_cast<int>(m_perf_metrics_record.size());
}

//--------------------------------------------------------------------------------------------------
int PerfCounterModel::columnCount(const QModelIndex &parent) const
{
    return static_cast<int>(m_headers.size());
}

QVariant PerfCounterModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole || !m_perf_metrics_data_provider)
    {
        return QVariant();
    }

    int row = index.row();
    int col = index.column();

    if (static_cast<size_t>(row) >= m_perf_metrics_record.size())
    {
        return QVariant();
    }

    const auto &record = m_perf_metrics_record.at(row);

    if (col >= m_headers.length())
    {
        return QVariant();
    }

    if (col < FixedHeader::kFixedHeaderCount)
    {
        switch (col)
        {
        case FixedHeader::kDrawID:
            return record.m_draw_id;
        case FixedHeader::kLRZState:
            return record.m_lrz_state;
        default:
            return QVariant();
        }
    }

    int metric_col_index = col - FixedHeader::kFixedHeaderCount;
    if (static_cast<size_t>(metric_col_index) < record.m_metric_values.size())
    {
        const auto &metric_value = record.m_metric_values.at(metric_col_index);

        if (role == Qt::DisplayRole)
        {
            return metric_value;
        }
        return QVariant();
    }

    return QVariant();
}

//--------------------------------------------------------------------------------------------------
QVariant PerfCounterModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
    {
        return QVariant();
    }
    if (section < m_headers.size())
    {
        return m_headers.at(section);
    }
    return QVariant();
}

//--------------------------------------------------------------------------------------------------
void PerfCounterModel::SearchInColumn(const QString &text, int column)
{
    if (text.isEmpty() || column < 0 || column >= columnCount())
    {
        return;
    }

    Qt::CaseSensitivity cs = Qt::CaseInsensitive;

    for (int r = 0; r < rowCount(); ++r)
    {
        QModelIndex idx = index(r, column);
        if (idx.isValid())
        {
            QVariant v = data(idx, Qt::DisplayRole);
            if (v.toString().contains(text, cs))
            {
                m_search_results.append(idx);
            }
        }
    }
}

//--------------------------------------------------------------------------------------------------
QModelIndex PerfCounterModel::FirstMatch()
{
    if (m_search_results.isEmpty() || m_search_results.cbegin() == m_search_results.cend())
    {
        return QModelIndex();
    }

    return *m_search_iterator;
}

//--------------------------------------------------------------------------------------------------
QModelIndex PerfCounterModel::NextMatch()
{
    if (m_search_results.isEmpty() || m_search_results.cbegin() == m_search_results.cend())
    {
        return QModelIndex();
    }

    m_search_iterator++;

    if (m_search_iterator == m_search_results.cend())
    {
        m_search_iterator = m_search_results.cbegin();
    }

    return *m_search_iterator;
}

//--------------------------------------------------------------------------------------------------
QModelIndex PerfCounterModel::PreviousMatch()
{
    if (m_search_results.isEmpty())
    {
        return QModelIndex();
    }

    if (m_search_iterator == m_search_results.cbegin())
    {
        m_search_iterator = m_search_results.cend();
    }

    --m_search_iterator;

    return *m_search_iterator;
}

//--------------------------------------------------------------------------------------------------
int PerfCounterModel::GetCurrentMatchIndex() const
{
    if (m_search_results.isEmpty())
    {
        return -1;
    }
    return std::distance(m_search_results.cbegin(), m_search_iterator);
}

//--------------------------------------------------------------------------------------------------
int PerfCounterModel::GetTotalMatches() const
{
    return m_search_results.size();
}

//--------------------------------------------------------------------------------------------------
void PerfCounterModel::ClearSearchResults()
{
    m_search_results.clear();
    m_search_iterator = m_search_results.cbegin();
}

//--------------------------------------------------------------------------------------------------
void PerfCounterModel::ResetSearchIterator()
{
    m_search_iterator = m_search_results.cbegin();
}

//--------------------------------------------------------------------------------------------------
void PerfCounterModel::SetIteratorToNearest(const QModelIndex &current_index)
{
    if (m_search_results.isEmpty() || !current_index.isValid())
    {
        m_search_iterator = m_search_results.cbegin();
        return;
    }

    int current_row = current_index.row();
    int nearest_distance = std::numeric_limits<int>::max();
    int nearest_index_in_list = 0;

    for (int i = 0; i < m_search_results.size(); ++i)
    {
        int search_result_row = m_search_results.at(i).row();
        int distance = std::abs(search_result_row - current_row);

        if (distance < nearest_distance)
        {
            nearest_distance = distance;
            nearest_index_in_list = i;
        }

        if (search_result_row >= current_row)
        {
            break;
        }
    }

    m_search_iterator = m_search_results.cbegin() + nearest_index_in_list;
}

std::optional<uint64_t> PerfCounterModel::GetDrawIndexFromRow(int row) const
{
    if (!m_perf_metrics_data_provider)
    {
        return std::nullopt;
    }
    return m_perf_metrics_data_provider->GetDrawIndexFromComputedRecordIndex(
    static_cast<uint64_t>(row));
}

std::optional<int> PerfCounterModel::GetRowFromDrawIndex(uint64_t draw_index) const
{
    if (!m_perf_metrics_data_provider)
    {
        return std::nullopt;
    }
    auto row = m_perf_metrics_data_provider->GetComputedRecordIndexFromDrawIndex(draw_index);
    if (!row)
    {
        return std::nullopt;
    }
    return static_cast<int>(*row);
}
