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

#include <array>
#include <filesystem>
#include <optional>
#include <qvariant.h>
#include <variant>

#include "dive_core/perf_metrics_data.h"
#include "dive_core/available_metrics.h"
#include "dive_core/command_hierarchy.h"

namespace
{

std::array<const char *, 8> kFixedPerfMetricHeader = {
    "Context ID", "Process ID", "Command Buffer ID", "Draw ID",
    "Draw Label", "Program ID", "Draw Type",         "LRZ State",
};

}
PerfCounterModel::~PerfCounterModel() = default;

PerfCounterModel::PerfCounterModel(const Dive::CommandHierarchy &command_hierarchy,
                                   QObject                      *parent) :
    QAbstractItemModel(parent),
    m_command_hierarchy(command_hierarchy)
{
    m_search_iterator = m_search_results.cbegin();
    m_perf_metrics_data_provider = Dive::PerfMetricsDataProvider::Create();
}

//--------------------------------------------------------------------------------------------------
std::optional<int> PerfCounterModel::GetRowForNode(uint64_t node_index)
{
    if (!m_perf_metrics_data_provider)
    {
        return std::nullopt;
    }
    if (auto index = m_perf_metrics_data_provider->GetCorrelatedComputedRecordIndex(node_index))
    {
        return *index;
    }
    return std::nullopt;
}

//--------------------------------------------------------------------------------------------------
void PerfCounterModel::OnPerfCounterResultsGenerated(const QString &file_path)
{
    if (!m_perf_metrics_data_provider)
    {
        return;
    }
    emit beginResetModel();

    std::filesystem::path fs_path = file_path.toStdString();

    m_perf_metrics_data_provider->Update(m_command_hierarchy,
                                         Dive::PerfMetricsData::LoadFromCsv(fs_path, nullptr));
    if (m_perf_metrics_data_provider)
    {
        m_row_count = static_cast<int>(m_perf_metrics_data_provider->GetComputedRecords().size());
        if (m_row_count > 0)
        {
            m_column_count = static_cast<int>(
            kFixedPerfMetricHeader.size() +
            m_perf_metrics_data_provider->GetComputedRecords()[0].m_metric_values.size());
        }
        m_headers.clear();
        for (auto header : kFixedPerfMetricHeader)
        {
            m_headers.push_back(QString::fromStdString(std::string(header)));
        }
        for (auto header : m_perf_metrics_data_provider->GetMetricsNames())
        {
            m_headers.push_back(QString::fromStdString(header));
        }
    }
    else
    {
        m_row_count = 0;
        m_column_count = 0;
    }

    emit endResetModel();
}

//--------------------------------------------------------------------------------------------------
void PerfCounterModel::ParseCsv(const QString &file_path)
{
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Could not open file:" << file.errorString();
        return;
    }

    QTextStream in(&file);
    if (!in.atEnd())
    {
        QString headerLine = in.readLine();
        m_headers = headerLine.split(',');
        m_column_count = m_headers.size();
    }
    else
    {
        file.close();
        return;
    }

    while (!in.atEnd())
    {
        QString     line = in.readLine();
        QStringList fields = line.split(',');

        if (fields.size() == m_column_count)
        {
            // Remove quotes from all fields
            for (QString &field : fields)
            {
                field.replace('"', "");
            }
            // m_csv_data.append(fields);
        }
    }
    file.close();
}

//--------------------------------------------------------------------------------------------------
QModelIndex PerfCounterModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || row < 0 || row >= rowCount() || column < 0 || column >= columnCount())
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
    if (parent.isValid())
    {
        return 0;
    }
    return m_row_count;
}

//--------------------------------------------------------------------------------------------------
int PerfCounterModel::columnCount(const QModelIndex &parent) const
{
    return m_column_count;
}

//--------------------------------------------------------------------------------------------------
QVariant PerfCounterModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
    {
        return QVariant();
    }

    if (!m_perf_metrics_data_provider)
    {
        return QVariant();
    }
    auto &records = m_perf_metrics_data_provider->GetComputedRecords();
    if (index.row() >= static_cast<int>(records.size()))
    {
        return QVariant();
    }
    auto &record = records[index.row()];
    if (index.column() < static_cast<int>(kFixedPerfMetricHeader.size()))
    {
        switch (index.column())
        {
        case 0:
            return QVariant(static_cast<qulonglong>(record.m_context_id));
        case 1:
            return QVariant(static_cast<qulonglong>(record.m_process_id));
        case 2:
            return QVariant(static_cast<qulonglong>(record.m_cmd_buffer_id));
        case 3:
            return QVariant(static_cast<qulonglong>(record.m_draw_id));
        case 4:
            return QVariant(static_cast<qulonglong>(record.m_draw_label));
        case 5:
            return QVariant(static_cast<qulonglong>(record.m_program_id));
        case 6:
            return QVariant(static_cast<qulonglong>(record.m_draw_type));
        case 7:
            return QVariant(static_cast<qulonglong>(record.m_lrz_state));
        default:
            break;
        }
        return QVariant();
    }
    int metric_index = index.column() - static_cast<int>(kFixedPerfMetricHeader.size());
    if (metric_index >= static_cast<int>(record.m_metric_values.size()))
    {
        return QVariant();
    }

    auto &metric = record.m_metric_values[metric_index];
    if (const int64_t *value = std::get_if<int64_t>(&metric))
    {
        return QVariant(static_cast<qlonglong>(*value));
    }
    else if (const float *value = std::get_if<float>(&metric))
    {
        return QVariant(*value);
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
