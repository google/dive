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

#include "window_scissors_stats_model.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QDebug>

WindowScissorsStatsModel::WindowScissorsStatsModel(QObject *parent) :
    QAbstractItemModel(parent)
{
    QStringList headers;

    for (const auto &header_str : Dive::window_scissor_stats_desc)
    {
        headers.append(QString::fromStdString(header_str));
    }

    m_headers = headers;
    m_column_count = m_headers.size();
    m_window_scissors = {};
}

//--------------------------------------------------------------------------------------------------
void WindowScissorsStatsModel::LoadData(std::set<Dive::WindowScissor> window_scissors)
{
    beginResetModel();
    // Clear existing data
    m_window_scissors.clear();
    m_window_scissors.assign(window_scissors.begin(), window_scissors.end());
    endResetModel();
}

//--------------------------------------------------------------------------------------------------
QModelIndex WindowScissorsStatsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid())
    {
        return QModelIndex();
    }

    if (row < 0 || static_cast<size_t>(row) >= m_window_scissors.size() || column < 0 ||
        column >= m_column_count)
    {
        return QModelIndex();
    }
    return createIndex(row, column, (void *)0);
}

//--------------------------------------------------------------------------------------------------
QModelIndex WindowScissorsStatsModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

//--------------------------------------------------------------------------------------------------
int WindowScissorsStatsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return static_cast<int>(m_window_scissors.size());
}

//--------------------------------------------------------------------------------------------------
int WindowScissorsStatsModel::columnCount(const QModelIndex &parent) const
{
    return m_column_count;
}

QVariant WindowScissorsStatsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
    {
        return QVariant();
    }

    int row = index.row();
    int col = index.column();

    if (col >= m_headers.length())
    {
        return QVariant();
    }

    const auto &record = m_window_scissors[row];

    switch (col)
    {
    case Dive::kWindowScissors:
        return QString::number(row);
    case Dive::kWindowScissors_tl_x:
        return QString::number(record.m_tl_x);
    case Dive::kWindowScissors_br_x:
        return QString::number(record.m_br_x);
    case Dive::kWindowScissors_tl_y:
        return QString::number(record.m_tl_y);
    case Dive::kWindowScissors_br_y:
        return QString::number(record.m_br_y);
    case Dive::kWindowScissors_Width:
        return QString::number(record.m_br_x - record.m_tl_x + 1);
    case Dive::kWindowScissors_Height:
        return QString::number(record.m_br_y - record.m_tl_y + 1);
    default:
        return QVariant();
    }
}

//--------------------------------------------------------------------------------------------------
QVariant WindowScissorsStatsModel::headerData(int             section,
                                              Qt::Orientation orientation,
                                              int             role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
    {
        return QVariant();
    }
    if (section < m_column_count)
    {
        return m_headers.at(section);
    }
    return QVariant();
}
