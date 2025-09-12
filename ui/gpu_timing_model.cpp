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

#include "gpu_timing_model.h"

#include <filesystem>
#include <QDebug>
#include <QString>
#include <string>

GpuTimingModel::GpuTimingModel(QObject *parent) :
    QAbstractItemModel(parent)
{
}

//--------------------------------------------------------------------------------------------------
void GpuTimingModel::OnGpuTimingResultsGenerated(const QString &file_path)
{
    emit beginResetModel();
    m_available_gpu_timing_data = {};  // Need to create a new AvailableGpuTiming object because it
                                       // can only be loaded once
    ParseCsv(file_path);
    emit endResetModel();
}

//--------------------------------------------------------------------------------------------------
void GpuTimingModel::ParseCsv(const QString &file_path)
{
    std::filesystem::path fp = file_path.toStdString();
    if (!m_available_gpu_timing_data.LoadFromCsv(fp))
    {
        qDebug() << "Could not load GPU timing info from CSV file: "
                 << file_path.toStdString().c_str();
    }
}

//--------------------------------------------------------------------------------------------------
QModelIndex GpuTimingModel::index(int row, int column, const QModelIndex &parent) const
{
    if ((parent.isValid()) || (row < 0) || (row >= m_available_gpu_timing_data.GetRows()) ||
        (column < 0) || column >= columnCount())
    {
        return QModelIndex();
    }
    return createIndex(row, column, (void *)0);
}

//--------------------------------------------------------------------------------------------------
QModelIndex GpuTimingModel::parent(const QModelIndex &index) const
{
    return QModelIndex();
}

//--------------------------------------------------------------------------------------------------
int GpuTimingModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
    {
        return 0;
    }
    return static_cast<int>(m_available_gpu_timing_data.GetRows());
}

//--------------------------------------------------------------------------------------------------
int GpuTimingModel::columnCount(const QModelIndex &parent) const
{
    return m_available_gpu_timing_data.GetColumns();
}

//--------------------------------------------------------------------------------------------------
QVariant GpuTimingModel::data(const QModelIndex &index, int role) const
{
    if ((!index.isValid()) || (role != Qt::DisplayRole))
    {
        return QVariant();
    }

    std::string ret = m_available_gpu_timing_data.GetCell(index.row(), index.column());
    if (ret.size() == 0)
    {
        qDebug() << "Could not get GPU timing stats for row: " << index;
        return QVariant();
    }

    return QString(ret.c_str());
}

//--------------------------------------------------------------------------------------------------
QVariant GpuTimingModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ((role != Qt::DisplayRole) || (orientation != Qt::Horizontal))
    {
        return QVariant();
    }
    if (section >= m_available_gpu_timing_data.GetColumns())
    {
        return QVariant();
    }
    return QString(m_available_gpu_timing_data.GetColumnHeader(section).c_str());
}
