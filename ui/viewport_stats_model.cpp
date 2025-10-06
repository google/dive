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

#include "viewport_stats_model.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QDebug>

ViewportStatsModel::ViewportStatsModel(QObject *parent) :
    QAbstractItemModel(parent)
{
    QStringList headers;
    for (const auto &header_str : Dive::viewport_stats_desc)
    {
        headers.append(QString::fromStdString(header_str));
    }

    m_headers = headers;
    m_column_count = m_headers.size();
    m_viewports = {};
}

//--------------------------------------------------------------------------------------------------
void ViewportStatsModel::LoadData(const std::set<Dive::Viewport> &viewports)
{
    beginResetModel();
    // Clear existing data
    m_viewports.clear();
    m_viewports.assign(viewports.begin(), viewports.end());
    endResetModel();
}

//--------------------------------------------------------------------------------------------------
QModelIndex ViewportStatsModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid())
    {
        return QModelIndex();
    }

    if (row < 0 || static_cast<size_t>(row) >= m_viewports.size() || column < 0 ||
        column >= m_column_count)
    {
        return QModelIndex();
    }

    return createIndex(row, column, (void *)0);
}

//--------------------------------------------------------------------------------------------------
QModelIndex ViewportStatsModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

//--------------------------------------------------------------------------------------------------
int ViewportStatsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return static_cast<int>(m_viewports.size());
}

//--------------------------------------------------------------------------------------------------
int ViewportStatsModel::columnCount(const QModelIndex &parent) const
{
    return m_headers.size();
}

//--------------------------------------------------------------------------------------------------
QVariant ViewportStatsModel::data(const QModelIndex &index, int role) const
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

    if (col == Dive::kViewport)
    {
        return QString::number(row);
    }

    const auto &record = m_viewports[row];

    switch (col)
    {
    case Dive::kViewport_x:
        return QString::number(record.m_vk_viewport.x);
    case Dive::kViewport_y:
        return QString::number(record.m_vk_viewport.y);
    case Dive::kViewport_width:
        return QString::number(record.m_vk_viewport.width);
    case Dive::kViewport_height:
        return QString::number(record.m_vk_viewport.height);
    case Dive::kViewport_minDepth:
        return QString::number(record.m_vk_viewport.minDepth);
    case Dive::kViewport_maxDepth:
        return QString::number(record.m_vk_viewport.maxDepth);
    default:
        return QVariant();
    }
}

//--------------------------------------------------------------------------------------------------
QVariant ViewportStatsModel::headerData(int section, Qt::Orientation orientation, int role) const
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
