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
#include "search_dialog.h"

#include "dive_core/command_hierarchy.h"

// =================================================================================================
// CommandTabView
// =================================================================================================
CommandTabView::CommandTabView(const Dive::CommandHierarchy &command_hierarchy, QWidget *parent) :
    m_command_hierarchy(command_hierarchy)
{
    m_command_buffer_model = new CommandBufferModel(command_hierarchy);
    m_command_buffer_view = new CommandBufferView(command_hierarchy);
    m_command_buffer_view->setModel(m_command_buffer_model);

    // Put address column (column 3) to the left of the tree. This forces the expand/collapse
    // icon to be part of the 2nd column (originally 1st)
    m_command_buffer_view->header()->moveSection(2, 0);

    m_search_trigger_button = new QPushButton;
    m_search_trigger_button->setIcon(QIcon(":/images/search.png"));

    QHBoxLayout *options_layout = new QHBoxLayout();
    options_layout->addWidget(m_search_trigger_button);
    options_layout->addStretch();

    QVBoxLayout *main_layout = new QVBoxLayout();
    main_layout->addLayout(options_layout);
    main_layout->addWidget(m_command_buffer_view);
    setLayout(main_layout);

    QObject::connect(m_search_trigger_button,
                     SIGNAL(clicked()),
                     this,
                     SLOT(OnSearchCommandBuffer()));
}

void CommandTabView::SetTopologyToView(const Dive::Topology *topology_ptr)
{
    m_command_buffer_model->SetTopologyToView(topology_ptr);
}

//--------------------------------------------------------------------------------------------------
void CommandTabView::ResetModel()
{
    m_command_buffer_model->Reset();

    // Reset search results
    m_command_buffer_view->Reset();
}

//--------------------------------------------------------------------------------------------------
void CommandTabView::OnSelectionChanged(const QModelIndex &index)
{
    m_command_buffer_model->OnSelectionChanged(index);

    // After m_command_buffer_view is filled out in CommandBufferModel::OnSelectionChanged(), expand
    // the tree
    m_command_buffer_view->expandAll();

    // Resize columns to fit
    uint32_t column_count = (uint32_t)m_command_buffer_model->columnCount(QModelIndex());
    for (uint32_t column = 0; column < column_count; ++column)
        m_command_buffer_view->resizeColumnToContents(column);

    // Reset search results
    m_command_buffer_view->Reset();
    if (m_search_dialog != nullptr)
        m_search_dialog->resetSearchResults();
}

//--------------------------------------------------------------------------------------------------
void CommandTabView::OnSearchCommandBuffer()
{
    if (m_search_dialog == nullptr)
    {
        m_search_dialog = new SearchDialog(this, "Commands");
        QObject::connect(m_search_dialog,
                         SIGNAL(new_search(const QString &)),
                         m_command_buffer_view,
                         SLOT(searchCommandBufferByText(const QString &)));
        QObject::connect(m_search_dialog,
                         &SearchDialog::next_search,
                         m_command_buffer_view,
                         &CommandBufferView::nextCommandInSearch);
        QObject::connect(m_search_dialog,
                         &SearchDialog::prev_search,
                         m_command_buffer_view,
                         &CommandBufferView::prevCommandInSearch);
        QObject::connect(m_command_buffer_view,
                         &CommandBufferView::updateSearch,
                         m_search_dialog,
                         &SearchDialog::updateSearchResults);
    }

    m_search_dialog->show();
    m_search_dialog->raise();
    m_search_dialog->activateWindow();
}
