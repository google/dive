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

#include "misc_stats_model.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QDebug>

constexpr std::array<Dive::Stats::Type, 11> kMiscStats = { Dive::Stats::kNumBinningPasses,
                                                           Dive::Stats::kNumTilingPasses,
                                                           Dive::Stats::kWaitMemWrites,
                                                           Dive::Stats::kWaitForIdle,
                                                           Dive::Stats::kWaitForMe,
                                                           Dive::Stats::kTotalResolves,
                                                           Dive::Stats::kColorSysMemToGmemResolves,
                                                           Dive::Stats::kColorGmemToSysMemResolves,
                                                           Dive::Stats::kDepthSysMemToGmemResolves,
                                                           Dive::Stats::kDepthGmemToSysMemResolves,
                                                           Dive::Stats::kClearGmemResolves };

constexpr std::array<const char *, Dive::Stats::kNumStats> kStatDescriptions = [] {
    std::array<const char *, Dive::Stats::kNumStats> arr{};
    for (const auto &[stat, description] : Dive::kStatMap)
    {
        arr[stat] = description;
    }
    return arr;
}();

MiscStatsModel::MiscStatsModel(QObject *parent) :
    QAbstractItemModel(parent)
{
    QStringList headers;
    headers.append(QString::fromStdString("Statistic"));
    headers.append(QString::fromStdString("Value"));

    m_headers = headers;
    m_column_count = m_headers.size();
    m_stats_list = {};
}

//--------------------------------------------------------------------------------------------------
void MiscStatsModel::LoadData(const std::array<uint64_t, Dive::Stats::kNumStats> &stats_list)
{
    beginResetModel();
    // Clear existing data
    m_stats_list = stats_list;
    endResetModel();
}

//--------------------------------------------------------------------------------------------------
QModelIndex MiscStatsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid())
    {
        return QModelIndex();
    }

    if (row < 0 || static_cast<size_t>(row) >= m_stats_list.size() || column < 0 ||
        column >= m_column_count)
    {
        return QModelIndex();
    }

    return createIndex(row, column, (void *)0);
}

//--------------------------------------------------------------------------------------------------
QModelIndex MiscStatsModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

//--------------------------------------------------------------------------------------------------
int MiscStatsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return static_cast<int>(kMiscStats.size());
}

//--------------------------------------------------------------------------------------------------
int MiscStatsModel::columnCount(const QModelIndex &parent) const
{
    return m_headers.size();
}

//--------------------------------------------------------------------------------------------------
QVariant MiscStatsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole || m_stats_list.empty())
    {
        return QVariant();
    }

    int row = index.row();
    int col = index.column();

    if (static_cast<size_t>(row) >= kMiscStats.size())
    {
        return QVariant();
    }

    const Dive::Stats::Type stat_key = kMiscStats[row];

    if (col >= m_headers.length())
    {
        return QVariant();
    }

    if (col == 0)
    {
        if (row < 0 || static_cast<size_t>(row) >= m_stats_list.size())
        {
            return QVariant();
        }
        return QString::fromStdString(kStatDescriptions[stat_key]);
    }

    if (row < 0 || static_cast<size_t>(row) >= m_stats_list.size())
    {
        return QVariant();
    }

    return QString::number(m_stats_list[stat_key]);
}

//--------------------------------------------------------------------------------------------------
QVariant MiscStatsModel::headerData(int section, Qt::Orientation orientation, int role) const
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
