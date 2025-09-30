/*
 Copyright 2019 Google LLC

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

#include <QApplication>
#include <QSplitter>
#include <QVBoxLayout>
#include <qclipboard.h>
#include <qobject.h>

#include "dive_core/data_core.h"
#include "trace_stats/trace_stats.h"
#include "most_expensive_events_view.h"
#include "overview_tab_view.h"
#include "draw_dispatch_stats_tab_view.h"
#include "problems_view.h"
#include "tile_stats_tab_view.h"
#include "misc_stats_tab_view.h"

// =================================================================================================
// OverviewTabView
// =================================================================================================
OverviewTabView::OverviewTabView(const Dive::CaptureMetadata &capture_metadata,
                                 const Dive::CaptureStats    &stats) :
    m_stats(stats)
{

    m_clipboard_button = new QPushButton();
    m_clipboard_button->setIcon(QIcon(":/images/copy.png"));
    m_clipboard_button->setToolTip("Copy all trace stats to clipboard");

    m_draw_dispatch_statistics_view = new DrawDispatchStatsTabView(stats);
    m_tile_statistics_view = new TileStatsTabView(stats);
    m_misc_statistics_view = new MiscStatsTabView(stats);

    m_tab_widget = new QTabWidget();
    m_tab_widget->setCornerWidget(m_clipboard_button, Qt::TopRightCorner);
    m_draw_dispatch_statistics_view_index = m_tab_widget->addTab(m_draw_dispatch_statistics_view,
                                                                 "Draw/Dispatch Stats");
    m_tile_statistics_view_tab_index = m_tab_widget->addTab(m_tile_statistics_view, "Tile Stats");
    m_misc_statistics_view_tab_index = m_tab_widget->addTab(m_misc_statistics_view, "Misc Stats");

    QVBoxLayout *main_layout = new QVBoxLayout();
    main_layout->addWidget(m_tab_widget);
    setLayout(main_layout);

    QObject::connect(m_clipboard_button,
                     &QPushButton::clicked,
                     this,
                     &OverviewTabView::CopyStatistics);
}

// --------------------------------------------------------------------------------------------------
void OverviewTabView::LoadStatistics()
{
    m_tile_statistics_view->LoadStatistics();
    m_draw_dispatch_statistics_view->LoadStatistics();
    m_misc_statistics_view->LoadStatistics();
}

// --------------------------------------------------------------------------------------------------
void OverviewTabView::CopyStatistics()
{
    std::ostringstream oss;
    Dive::TraceStats   trace_stats;
    trace_stats.PrintTraceStats(m_stats, oss);

    QApplication::clipboard()->setText(QString::fromStdString(oss.str()));
}
