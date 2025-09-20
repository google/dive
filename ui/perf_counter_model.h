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

#include <optional>

namespace Dive
{
class CommandHierarchy;
class PerfMetricsDataProvider;
}  // namespace Dive

class PerfCounterModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit PerfCounterModel(const Dive::CommandHierarchy &command_hierarchy,
                              QObject                      *parent = nullptr);
    ~PerfCounterModel();

    PerfCounterModel(const PerfCounterModel &) = delete;
    PerfCounterModel(PerfCounterModel &&) = delete;
    PerfCounterModel &operator=(const PerfCounterModel &) = delete;
    PerfCounterModel &operator=(PerfCounterModel &&) = delete;
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

    std::optional<int> GetRowForNode(uint64_t node_index);

public slots:
    void OnPerfCounterResultsGenerated(const QString &file_path);

private:
    void ParseCsv(const QString &file_path);

    const Dive::CommandHierarchy &m_command_hierarchy;

    std::unique_ptr<Dive::PerfMetricsDataProvider> m_perf_metrics_data_provider;

    QList<QModelIndex>                 m_search_results;
    QList<QModelIndex>::const_iterator m_search_iterator;
    QStringList                        m_headers;
    int                                m_row_count = 0;
    int                                m_column_count = 0;
};
