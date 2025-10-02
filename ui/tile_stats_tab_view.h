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
class SearchBar;
class ViewportStatsModel;
class WindowScissorsStatsModel;

namespace Dive
{
struct CaptureStats;
};  // namespace Dive

class TileStatsTabView : public QWidget
{
    Q_OBJECT
public:
    explicit TileStatsTabView(const Dive::CaptureStats &stats, QWidget *parent = nullptr);
    void LoadStatistics();
public slots:
signals:

private:
    void ResizeColumns(QAbstractItemModel *model, QTableView *view);

    const Dive::CaptureStats &m_stats;
    ViewportStatsModel       *m_viewport_stats_model;
    WindowScissorsStatsModel *m_window_scissors_stats_model;
    QTableView               *m_viewport_stats_view;
    QTableView               *m_window_scissors_stats_view;
};
