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

PerfCounterModel::PerfCounterModel(const QString &filePath, QObject *parent) :
    QAbstractItemModel(parent)
{
    ParseCsv(filePath);
    m_search_iterator = m_search_results.cbegin();
}

//--------------------------------------------------------------------------------------------------
void PerfCounterModel::ParseCsv(const QString &filePath)
{
    QFile file(filePath);
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
            m_csv_data.append(fields);
        }
    }
    file.close();
}

//--------------------------------------------------------------------------------------------------
QModelIndex PerfCounterModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || row < 0 || row >= m_csv_data.size() || column < 0 ||
        column >= columnCount())
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
    return m_csv_data.size();
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

    if (index.row() < m_csv_data.size() && index.column() < m_csv_data.at(index.row()).size())
    {
        return m_csv_data.at(index.row()).at(index.column());
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
void PerfCounterModel::SetIteratorToNearest(const QModelIndex &currentIndex)
{
    if (m_search_results.isEmpty() || !currentIndex.isValid())
    {
        m_search_iterator = m_search_results.cbegin();
        return;
    }

    int current_row = currentIndex.row();
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
