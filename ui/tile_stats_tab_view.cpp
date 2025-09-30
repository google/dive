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

#include "tile_stats_tab_view.h"
#include "viewport_stats_model.h"
#include "window_scissors_stats_model.h"
#include "search_bar.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QIcon>
#include <QPoint>
#include <iostream>
#include "object_names.h"
#include "trace_stats/trace_stats.h"

TileStatsTabView::TileStatsTabView(const Dive::CaptureStats &stats, QWidget *parent) :
    QWidget(parent),
    m_stats(stats)
{
    m_viewport_stats_model = new ViewportStatsModel();
    m_viewport_stats_view = new QTableView();
    m_viewport_stats_view->setModel(m_viewport_stats_model);
    ResizeColumns(m_viewport_stats_model, m_viewport_stats_view);

    m_window_scissors_stats_model = new WindowScissorsStatsModel();
    m_window_scissors_stats_view = new QTableView();
    m_window_scissors_stats_view->setModel(m_window_scissors_stats_model);
    ResizeColumns(m_viewport_stats_model, m_window_scissors_stats_view);

    QVBoxLayout *main_layout = new QVBoxLayout(this);
    main_layout->addWidget(m_viewport_stats_view);
    main_layout->addWidget(m_window_scissors_stats_view);
}

void TileStatsTabView::ResizeColumns(QAbstractItemModel *model, QTableView *view)
{
    // Resize columns to fit the content
    uint32_t column_count = (uint32_t)model->columnCount(QModelIndex());
    for (uint32_t column = 0; column < column_count; ++column)
    {
        view->resizeColumnToContents(column);
    }
}

// --------------------------------------------------------------------------------------------------
void TileStatsTabView::LoadStatistics()
{
    if (m_stats.m_window_scissors.empty())
    {
        m_window_scissors_stats_view->setVisible(false);
    }
    else
    {
        m_window_scissors_stats_view->setVisible(true);
    }

    m_window_scissors_stats_model->LoadData(m_stats.m_window_scissors);
    ResizeColumns(m_window_scissors_stats_model, m_window_scissors_stats_view);

    if (m_stats.m_viewports.empty())
    {
        m_viewport_stats_view->setVisible(false);
    }
    else
    {
        m_viewport_stats_view->setVisible(true);
    }

    m_viewport_stats_model->LoadData(m_stats.m_viewports);
    ResizeColumns(m_viewport_stats_model, m_viewport_stats_view);
}
