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

#include "perf_counter_tab_view.h"
#include "perf_counter_model.h"
#include "search_bar.h"
#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QIcon>
#include <QPoint>
#include "object_names.h"

PerfCounterTabView::PerfCounterTabView(PerfCounterModel &perf_counter_model, QWidget *parent) :
    QWidget(parent),
    m_perf_counter_model(perf_counter_model)
{
    m_perf_counter_view = new QTableView();
    m_perf_counter_view->setModel(&m_perf_counter_model);
    m_perf_counter_view->setContextMenuPolicy(Qt::CustomContextMenu);
    m_perf_counter_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    ResizeColumns();

    m_search_trigger_button = new QPushButton;
    m_search_trigger_button->setObjectName(kPerfCounterSearchButtonName);
    m_search_trigger_button->setIcon(QIcon(":/images/search.png"));

    QHBoxLayout *options_layout = new QHBoxLayout();
    options_layout->addWidget(m_search_trigger_button);
    options_layout->addStretch();

    m_search_bar = new SearchBar(this);
    m_search_bar->setObjectName(kPerfCounterSearchBarName);
    m_search_bar->hide();
    m_search_bar->setView(m_perf_counter_view);

    QVBoxLayout *main_layout = new QVBoxLayout(this);
    main_layout->addLayout(options_layout);
    main_layout->addWidget(m_search_bar);
    main_layout->addWidget(m_perf_counter_view);

    QObject::connect(m_search_trigger_button, SIGNAL(clicked()), this, SLOT(OnSearchCounters()));

    QObject::connect(m_search_bar,
                     SIGNAL(hide_search_bar(bool)),
                     this,
                     SLOT(OnSearchBarVisibilityChange(bool)));

    connect(m_perf_counter_view->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            &PerfCounterTabView::OnSelectionChanged);
}

//--------------------------------------------------------------------------------------------------
void PerfCounterTabView::ClearSelection()
{
    QItemSelectionModel *selection_model = m_perf_counter_view->selectionModel();
    if (selection_model)
    {
        selection_model->clear();
    }
}

//--------------------------------------------------------------------------------------------------
void PerfCounterTabView::OnSelectionChanged(const QModelIndex &index)
{
    if (!index.isValid())
    {
        return;
    }

    // Resize columns to fit the content
    uint32_t column_count = (uint32_t)m_perf_counter_model.columnCount(QModelIndex());
    for (uint32_t column = 0; column < column_count; ++column)
    {
        m_perf_counter_view->resizeColumnToContents(column);
    }
    auto draw_index = m_perf_counter_model.GetDrawIndexFromRow(index.row());
    if (draw_index)
    {
        emit CounterSelected(*draw_index);
    }
}

//--------------------------------------------------------------------------------------------------
void PerfCounterTabView::OnSearchCounters()
{
    if (m_search_bar->isVisible())
    {
        m_search_bar->clearSearch();
        m_search_bar->hide();
        m_search_trigger_button->show();

        DisconnectSearchBar();
    }
    else
    {
        ConnectSearchBar();

        m_search_trigger_button->hide();
        m_search_bar->positionCurser();
        m_search_bar->show();
        emit HideOtherSearchBars();
    }
}

//--------------------------------------------------------------------------------------------------
void PerfCounterTabView::OnSearchBarVisibilityChange(bool isHidden)
{
    if (isHidden)
    {
        m_search_bar->clearSearch();
        m_search_bar->hide();
        m_search_trigger_button->show();
    }
}

//--------------------------------------------------------------------------------------------------
void PerfCounterTabView::OnSearch(const QString &text)
{
    // Clear previous search results
    m_perf_counter_model.ClearSearchResults();

    if (text.isEmpty())
    {
        emit UpdateSearchInfo(0, 0);
        return;
    }

    // Loop through all columns and populate the list
    int columnCount = m_perf_counter_model.columnCount();
    for (int column = 0; column < columnCount; ++column)
    {
        m_perf_counter_model.SearchInColumn(text, column);
    }

    m_perf_counter_model.ResetSearchIterator();

    QModelIndex currentIndex = m_perf_counter_view->currentIndex();
    if (currentIndex.isValid())
    {
        m_perf_counter_model.SetIteratorToNearest(currentIndex);
    }

    QModelIndex firstMatch = m_perf_counter_model.NextMatch();

    if (firstMatch.isValid())
    {
        m_perf_counter_view->setCurrentIndex(firstMatch);
        m_perf_counter_view->scrollTo(firstMatch);
    }

    emit UpdateSearchInfo(m_perf_counter_model.GetCurrentMatchIndex(),
                          m_perf_counter_model.GetTotalMatches());
}

//--------------------------------------------------------------------------------------------------
void PerfCounterTabView::OnNextMatch()
{
    QModelIndex nextMatch = m_perf_counter_model.NextMatch();
    if (nextMatch.isValid())
    {
        m_perf_counter_view->setCurrentIndex(nextMatch);
        m_perf_counter_view->scrollTo(nextMatch);
    }

    emit UpdateSearchInfo(m_perf_counter_model.GetCurrentMatchIndex(),
                          m_perf_counter_model.GetTotalMatches());
}

//--------------------------------------------------------------------------------------------------
void PerfCounterTabView::OnPrevMatch()
{
    QModelIndex prevMatch = m_perf_counter_model.PreviousMatch();
    if (prevMatch.isValid())
    {
        m_perf_counter_view->setCurrentIndex(prevMatch);
        m_perf_counter_view->scrollTo(prevMatch);
    }

    emit UpdateSearchInfo(m_perf_counter_model.GetCurrentMatchIndex(),
                          m_perf_counter_model.GetTotalMatches());
}

//--------------------------------------------------------------------------------------------------
void PerfCounterTabView::ConnectSearchBar()
{
    QObject::connect(m_search_bar, &SearchBar::new_search, this, &PerfCounterTabView::OnSearch);

    QObject::connect(m_search_bar, &SearchBar::next_search, this, &PerfCounterTabView::OnNextMatch);

    QObject::connect(m_search_bar, &SearchBar::prev_search, this, &PerfCounterTabView::OnPrevMatch);

    QObject::connect(this,
                     &PerfCounterTabView::UpdateSearchInfo,
                     m_search_bar,
                     &SearchBar::updateSearchResults);
}

//--------------------------------------------------------------------------------------------------
void PerfCounterTabView::DisconnectSearchBar()
{
    QObject::disconnect(m_search_bar, &SearchBar::new_search, this, &PerfCounterTabView::OnSearch);

    QObject::disconnect(m_search_bar,
                        &SearchBar::next_search,
                        this,
                        &PerfCounterTabView::OnNextMatch);

    QObject::disconnect(m_search_bar,
                        &SearchBar::prev_search,
                        this,
                        &PerfCounterTabView::OnPrevMatch);

    QObject::disconnect(this,
                        &PerfCounterTabView::UpdateSearchInfo,
                        m_search_bar,
                        &SearchBar::updateSearchResults);
}

//--------------------------------------------------------------------------------------------------
void PerfCounterTabView::CorrelateCounter(uint64_t index)
{
    QItemSelectionModel *selection_model = m_perf_counter_view->selectionModel();
    QSignalBlocker       blocker(selection_model);
    if (auto row = m_perf_counter_model.GetRowFromDrawIndex(index))
    {
        m_perf_counter_view->selectRow(*row);
    }
    m_perf_counter_view->update();
    m_perf_counter_view->viewport()->update();
}

//--------------------------------------------------------------------------------------------------

void PerfCounterTabView::ResizeColumns()
{
    QHeaderView *header = m_perf_counter_view->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::ResizeToContents);

    uint32_t column_count = static_cast<uint32_t>(m_perf_counter_model.columnCount(QModelIndex()));
    for (uint32_t column = 0; column < column_count; ++column)
    {
        m_perf_counter_view->resizeColumnToContents(column);
    }
}
