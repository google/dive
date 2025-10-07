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

#include <QAbstractItemModel>
#include <QVector>
#include <QStringList>

#include "trace_stats/trace_stats.h"

class ViewportStatsModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit ViewportStatsModel(QObject *parent = nullptr);

    void LoadData(const std::set<Dive::Viewport> &viewports);

    // QAbstractItemModel interface
    QModelIndex index(int                row,
                      int                column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int         rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int         columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant    data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant    headerData(int             section,
                           Qt::Orientation orientation,
                           int             role = Qt::DisplayRole) const override;
public slots:

private:
    QStringList                 m_headers;
    int                         m_column_count = 0;
    std::vector<Dive::Viewport> m_viewports;
};
