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
#include <QAbstractItemModel>
#include <QVector>
#include <QStringList>
#include <optional>
#include <cstdint>

class PerfCounterModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit PerfCounterModel(QObject *parent = nullptr);

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
    void        SearchInColumn(const QString &value, int column = 0);
    QModelIndex FirstMatch();
    QModelIndex NextMatch();
    QModelIndex PreviousMatch();
    int         GetCurrentMatchIndex() const;
    int         GetTotalMatches() const;
    void        ClearSearchResults();
    void        ResetSearchIterator();
    void        SetIteratorToNearest(const QModelIndex &current_index);

    std::optional<uint64_t> GetDrawIndexFromRow(int row) const;
    std::optional<int>      GetRowFromDrawIndex(uint64_t draw_index) const;

public slots:
    void OnPerfCounterResultsGenerated(
    const std::filesystem::path                                        &file_path,
    std::optional<std::reference_wrapper<const Dive::AvailableMetrics>> available_metrics);

private:
    void ParseCsv(const QString &file_path);
    void LoadData();

    QList<QModelIndex>                             m_search_results;
    QList<QModelIndex>::const_iterator             m_search_iterator;
    QStringList                                    m_headers;
    int                                            m_column_count = 0;
    std::unique_ptr<Dive::PerfMetricsDataProvider> m_perf_metrics_data_provider;
    std::vector<Dive::PerfMetricsRecord>           m_perf_metrics_record;
};
