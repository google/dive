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

#include <QWidget>
#include <QTableView>
#include <QPushButton>
#include <QMenu>
#pragma once

// Forward declaration
class PerfCounterModel;
class SearchBar;

class PerfCounterTabView : public QWidget
{
    Q_OBJECT
public:
    explicit PerfCounterTabView(PerfCounterModel &perf_counter_model, QWidget *parent = nullptr);

public slots:
    void OnSearchBarVisibilityChange(bool isHidden);
    void OnSelectionChanged(const QModelIndex &index);
    void OnSearchCounters();
    void OnSearch(const QString &text);
    void OnNextMatch();
    void OnPrevMatch();

signals:
    void UpdateSearchInfo(uint64_t curr_item_pos, uint64_t total_search_results);
    void HideOtherSearchBars();

private:
    void ConnectSearchBar();
    void DisconnectSearchBar();

    PerfCounterModel &m_perf_counter_model;
    QTableView       *m_perf_counter_view;
    QPushButton      *m_search_trigger_button;
    SearchBar        *m_search_bar;
};
