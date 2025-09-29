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

#include "misc_stats_tab_view.h"
#include "misc_stats_model.h"
#include "search_bar.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QIcon>
#include <QPoint>
#include <iostream>
#include "object_names.h"
#include "trace_stats/trace_stats.h"

MiscStatsTabView::MiscStatsTabView(const Dive::CaptureStats &stats, QWidget *parent) :
    QWidget(parent),
    m_stats(stats)
{
    m_misc_stats_model = new MiscStatsModel();
    m_misc_stats_view = new QTableView();
    m_misc_stats_view->setModel(m_misc_stats_model);
    ResizeColumns(m_misc_stats_model, m_misc_stats_view);

    QVBoxLayout *main_layout = new QVBoxLayout(this);
    main_layout->addWidget(m_misc_stats_view);
}

void MiscStatsTabView::ResizeColumns(QAbstractItemModel *model, QTableView *view)
{
    // Resize columns to fit the content
    uint32_t column_count = (uint32_t)model->columnCount(QModelIndex());
    for (uint32_t column = 0; column < column_count; ++column)
    {
        view->resizeColumnToContents(column);
    }
}

// --------------------------------------------------------------------------------------------------
void MiscStatsTabView::LoadStatistics()
{
    if (m_stats.m_window_scissors.empty())
    {
        m_misc_stats_view->setVisible(false);
    }
    else
    {
        m_misc_stats_view->setVisible(true);
    }

    m_misc_stats_model->LoadData(m_stats.m_stats_list);
    ResizeColumns(m_misc_stats_model, m_misc_stats_view);
}