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

#include "command_tab_view.h"
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include "command_buffer_model.h"
#include "command_buffer_view.h"
#include "search_bar.h"
#include "shortcuts.h"

#include "dive_core/command_hierarchy.h"
#include "object_names.h"

// =================================================================================================
// CommandTabView
// =================================================================================================
CommandTabView::CommandTabView(const Dive::CommandHierarchy &command_hierarchy, QWidget *parent) :
    m_command_hierarchy(command_hierarchy)
{
    m_command_buffer_model = new CommandBufferModel(command_hierarchy);
    m_command_buffer_view = new CommandBufferView(command_hierarchy);
    m_command_buffer_view->setModel(m_command_buffer_model);

    // Move the address column to be the 1st column, followed by the IB Level column
    // This allows the expand/collapse icon to be part of what was originally the 1st column (i.e.
    // the pm4 column)
    m_command_buffer_view->header()->moveSection(2, 0);
    m_command_buffer_view->header()->moveSection(2, 1);

    m_search_trigger_button = new QPushButton;
    m_search_trigger_button->setObjectName(kCommandBufferSearchButtonName);
    m_search_trigger_button->setIcon(QIcon(":/images/search.png"));

    QHBoxLayout *options_layout = new QHBoxLayout();
    options_layout->addWidget(m_search_trigger_button);
    options_layout->addStretch();

    m_search_bar = new SearchBar(this);
    m_search_bar->setObjectName(kCommandBufferSearchBarName);
    m_search_bar->hide();

    QVBoxLayout *main_layout = new QVBoxLayout();
    main_layout->addLayout(options_layout);
    main_layout->addWidget(m_search_bar);
    main_layout->addWidget(m_command_buffer_view);
    setLayout(main_layout);
    m_search_bar->setTreeView(m_command_buffer_view);

    QObject::connect(m_search_trigger_button,
                     SIGNAL(clicked()),
                     this,
                     SLOT(OnSearchCommandBuffer()));

    QObject::connect(m_search_bar,
                     SIGNAL(hide_search_bar(bool)),
                     this,
                     SLOT(OnSearchBarVisibilityChange(bool)));
}

//--------------------------------------------------------------------------------------------------
void CommandTabView::SetTopologyToView(const Dive::SharedNodeTopology *topology_ptr)
{
    m_command_buffer_model->SetTopologyToView(topology_ptr);
}

//--------------------------------------------------------------------------------------------------
void CommandTabView::clearSearchBar()
{
    m_search_bar->clearSearch();
    m_search_bar->hide();
    m_search_trigger_button->show();

    DisconnectSearchBar();
}

//--------------------------------------------------------------------------------------------------
void CommandTabView::ResetModel()
{
    m_command_buffer_model->Reset();

    // Reset search results
    m_command_buffer_view->Reset();
    if (m_search_bar->isVisible())
    {
        m_search_bar->clearSearch();
    }
}

//--------------------------------------------------------------------------------------------------
void CommandTabView::OnSelectionChanged(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    m_command_buffer_model->OnSelectionChanged(index);

    // After m_command_buffer_view is filled out in CommandBufferModel::OnSelectionChanged(), do NOT
    // expandAll. For huge trees (e.g. 20+ million nodes), it needs to traverse all expanded nodes
    // to figure out how to setup scrollbar. So the more collapsed it is, the less it has to
    // traverse
    // m_command_buffer_view->expandAll();

    m_command_buffer_view->scrollTo(m_command_buffer_model->scrollToIndex(),
                                    QAbstractItemView::PositionAtBottom);

    // Resize columns to fit
    uint32_t column_count = (uint32_t)m_command_buffer_model->columnCount(QModelIndex());
    for (uint32_t column = 0; column < column_count; ++column)
        m_command_buffer_view->resizeColumnToContents(column);

    // Reset search results
    m_command_buffer_view->Reset();
    if (m_search_bar->isVisible())
    {
        clearSearchBar();
    }
}

//--------------------------------------------------------------------------------------------------
void CommandTabView::OnSearchCommandBuffer()
{
    if (m_search_bar->isVisible())
    {
        clearSearchBar();
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
void CommandTabView::OnSearchBarVisibilityChange(bool isHidden)
{
    if (isHidden)
    {
        m_search_bar->clearSearch();
        m_search_bar->hide();
        m_search_trigger_button->show();
        DisconnectSearchBar();
    }
}

//--------------------------------------------------------------------------------------------------
void CommandTabView::ConnectSearchBar()
{
    QObject::connect(m_search_bar,
                     SIGNAL(new_search(const QString &)),
                     m_command_buffer_view,
                     SLOT(searchCommandBufferByText(const QString &)));
    QObject::connect(m_search_bar,
                     &SearchBar::next_search,
                     m_command_buffer_view,
                     &CommandBufferView::nextCommandInSearch);
    QObject::connect(m_search_bar,
                     &SearchBar::prev_search,
                     m_command_buffer_view,
                     &CommandBufferView::prevCommandInSearch);
    QObject::connect(m_command_buffer_view,
                     &CommandBufferView::updateSearch,
                     m_search_bar,
                     &SearchBar::updateSearchResults);
}

//--------------------------------------------------------------------------------------------------
void CommandTabView::DisconnectSearchBar()
{
    QObject::disconnect(m_search_bar,
                        SIGNAL(new_search(const QString &)),
                        m_command_buffer_view,
                        SLOT(searchCommandBufferByText(const QString &)));
    QObject::disconnect(m_search_bar,
                        &SearchBar::next_search,
                        m_command_buffer_view,
                        &CommandBufferView::nextCommandInSearch);
    QObject::disconnect(m_search_bar,
                        &SearchBar::prev_search,
                        m_command_buffer_view,
                        &CommandBufferView::prevCommandInSearch);
    QObject::disconnect(m_command_buffer_view,
                        &CommandBufferView::updateSearch,
                        m_search_bar,
                        &SearchBar::updateSearchResults);
}
