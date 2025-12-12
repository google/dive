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
#include <qabstractitemmodel.h>
#include <qpushbutton.h>
#include "object_names.h"

PerfCounterTabView::PerfCounterTabView(PerfCounterModel &perf_counter_model, QWidget *parent) :
    QWidget(parent),
    m_perf_counter_model(perf_counter_model)
{
    m_proxy_model = new QSortFilterProxyModel(this);
    m_proxy_model->setSourceModel(&m_perf_counter_model);

    m_perf_counter_view = new QTableView();
    m_perf_counter_view->verticalHeader()->hide();
    m_perf_counter_view->setSortingEnabled(true);

    m_perf_counter_view->setModel(m_proxy_model);
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

    m_reset_sorting_button = new QPushButton("Reset Sorting");
    options_layout->addWidget(m_reset_sorting_button);

    m_horizontal_header = m_perf_counter_view->horizontalHeader();
    m_horizontal_header->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_horizontal_header->setSortIndicator(-1, Qt::AscendingOrder);

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

    QObject::connect(m_horizontal_header,
                     &QHeaderView::sectionClicked,
                     this,
                     &PerfCounterTabView::OnSortApplied);

    QObject::connect(m_reset_sorting_button, SIGNAL(clicked()), this, SLOT(OnResetSorting()));
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

    m_perf_counter_view->scrollTo(index, QAbstractItemView::EnsureVisible);

    QModelIndex source_index = m_proxy_model->mapToSource(index);
    int         source_row = source_index.row();

    // Resize columns to fit the content
    uint32_t column_count = (uint32_t)m_perf_counter_model.columnCount(QModelIndex());
    for (uint32_t column = 0; column < column_count; ++column)
    {
        m_perf_counter_view->resizeColumnToContents(column);
    }
    auto draw_index = m_perf_counter_model.GetDrawIndexFromRow(source_row);
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
        ClearSelection();
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

    QModelIndex firstMatchSource = m_perf_counter_model.FirstMatch();

    if (firstMatchSource.isValid())
    {
        QModelIndex firstMatchProxy = m_proxy_model->mapFromSource(firstMatchSource);

        if (firstMatchProxy.isValid())
        {
            QItemSelectionModel *selection_model = m_perf_counter_view->selectionModel();
            if (selection_model)
            {
                selection_model->select(firstMatchProxy,
                                        QItemSelectionModel::ClearAndSelect |
                                        QItemSelectionModel::Rows);
            }

            m_perf_counter_view->setCurrentIndex(firstMatchProxy);
            m_perf_counter_view->scrollTo(firstMatchProxy, QAbstractItemView::EnsureVisible);
        }
    }

    emit UpdateSearchInfo(m_perf_counter_model.GetCurrentMatchIndex(),
                          m_perf_counter_model.GetTotalMatches());
}

//--------------------------------------------------------------------------------------------------
void PerfCounterTabView::OnNextMatch()
{
    QModelIndex nextMatchSource = m_perf_counter_model.NextMatch();
    if (nextMatchSource.isValid())
    {
        QModelIndex nextMatchProxy = m_proxy_model->mapFromSource(nextMatchSource);
        if (nextMatchProxy.isValid())
        {
            QItemSelectionModel *selection_model = m_perf_counter_view->selectionModel();
            if (selection_model)
            {
                selection_model->select(nextMatchProxy,
                                        QItemSelectionModel::ClearAndSelect |
                                        QItemSelectionModel::Rows);
            }

            m_perf_counter_view->setCurrentIndex(nextMatchProxy);
            m_perf_counter_view->scrollTo(nextMatchProxy, QAbstractItemView::EnsureVisible);
        }
    }

    emit UpdateSearchInfo(m_perf_counter_model.GetCurrentMatchIndex(),
                          m_perf_counter_model.GetTotalMatches());
}

//--------------------------------------------------------------------------------------------------
void PerfCounterTabView::OnPrevMatch()
{
    QModelIndex prevMatchSource = m_perf_counter_model.PreviousMatch();
    if (prevMatchSource.isValid())
    {
        QModelIndex prevMatchProxy = m_proxy_model->mapFromSource(prevMatchSource);
        if (prevMatchProxy.isValid())
        {
            QItemSelectionModel *selection_model = m_perf_counter_view->selectionModel();

            if (selection_model)
            {
                selection_model->select(prevMatchProxy,
                                        QItemSelectionModel::ClearAndSelect |
                                        QItemSelectionModel::Rows);
            }

            m_perf_counter_view->setCurrentIndex(prevMatchProxy);
            m_perf_counter_view->scrollTo(prevMatchProxy, QAbstractItemView::EnsureVisible);
        }
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
        QModelIndex source_index = m_perf_counter_model.index(*row, 0);

        QModelIndex proxy_index = m_proxy_model->mapFromSource(source_index);

        selection_model->select(proxy_index,
                                QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

        m_perf_counter_view->scrollTo(proxy_index, QAbstractItemView::EnsureVisible);
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

//--------------------------------------------------------------------------------------------------
void PerfCounterTabView::OnSortApplied(int column_index)
{
    QItemSelectionModel *selection_model = m_perf_counter_view->selectionModel();
    if (!selection_model)
    {
        return;
    }

    QModelIndexList selected_indexes = selection_model->selectedIndexes();

    if (!selected_indexes.isEmpty())
    {
        QModelIndex proxy_index_to_preserve = selected_indexes.first();

        QModelIndex source_index = m_proxy_model->mapToSource(proxy_index_to_preserve);

        if (source_index.isValid())
        {
            QMetaObject::invokeMethod(this,
                                      "OnSortingCompletedAndScroll",
                                      Qt::QueuedConnection,
                                      Q_ARG(QModelIndex, source_index));
            return;
        }
    }
}

//--------------------------------------------------------------------------------------------------
void PerfCounterTabView::OnResetSorting()
{
    QItemSelectionModel *selection_model = m_perf_counter_view->selectionModel();
    if (!selection_model)
    {
        return;
    }

    QModelIndex source_index;

    QModelIndexList selected_indexes = selection_model->selectedIndexes();

    if (!selected_indexes.isEmpty())
    {
        QModelIndex proxy_index_to_preserve = selected_indexes.first();
        source_index = m_proxy_model->mapToSource(proxy_index_to_preserve);
    }

    m_proxy_model->sort(-1, Qt::AscendingOrder);
    m_horizontal_header->setSortIndicator(-1, Qt::AscendingOrder);

    if (source_index.isValid())
    {
        QMetaObject::invokeMethod(this,
                                  "OnSortingCompletedAndScroll",
                                  Qt::QueuedConnection,
                                  Q_ARG(QModelIndex, source_index));
    }

    if (m_search_bar->isVisible())
    {
        m_search_bar->clearSearch();
    }
}

//--------------------------------------------------------------------------------------------------
void PerfCounterTabView::OnSortingCompletedAndScroll(const QModelIndex &index_to_map)
{
    QItemSelectionModel   *selection_model = m_perf_counter_view->selectionModel();
    QSortFilterProxyModel *proxy_model = qobject_cast<QSortFilterProxyModel *>(
    m_perf_counter_view->model());

    if (!index_to_map.isValid() || !selection_model || !proxy_model)
    {
        return;
    }

    QModelIndex new_proxy_index = proxy_model->mapFromSource(index_to_map);

    if (new_proxy_index.isValid())
    {
        selection_model->select(new_proxy_index,
                                QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
        m_perf_counter_view->scrollTo(new_proxy_index, QAbstractItemView::EnsureVisible);
    }
}
