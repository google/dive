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

#include "gfxr_vulkan_command_tab_view.h"
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <iostream>
#include <QMenu>
#include <string>
#include "gfxr_vulkan_command_filter_proxy_model.h"
#include "gfxr_vulkan_command_arguments_filter_proxy_model.h"
#include "search_bar.h"
#include "shortcuts.h"
#include "gfxr_vulkan_command_filter.h"

#include "object_names.h"

// =================================================================================================
// GfxrVulkanCommandTabView
// =================================================================================================
GfxrVulkanCommandTabView::GfxrVulkanCommandTabView(
const Dive::CommandHierarchy      &vulkan_command_hierarchy,
GfxrVulkanCommandFilterProxyModel &proxy_model,
GfxrVulkanCommandModel            &command_hierarchy_model,
QWidget                           *parent) :
    m_vulkan_command_hierarchy(vulkan_command_hierarchy),
    m_proxy_Model(proxy_model),
    m_command_hierarchy_model(command_hierarchy_model)
{
    m_command_hierarchy_view = new DiveTreeView(m_vulkan_command_hierarchy);
    m_command_hierarchy_view->setModel(&m_proxy_Model);
    m_command_hierarchy_view->setContextMenuPolicy(Qt::CustomContextMenu);

    QLabel *filter_combo_box_label = new QLabel(tr("Filter:"));
    m_gfxr_vulkan_command_filter = new GfxrVulkanCommandFilter(*m_command_hierarchy_view,
                                                               m_proxy_Model);
    m_gfxr_vulkan_command_filter->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_search_trigger_button = new QPushButton;
    m_search_trigger_button->setObjectName(kGfxrVulkanCommandSearchButtonName);
    m_search_trigger_button->setIcon(QIcon(":/images/search.png"));

    QHBoxLayout *options_layout = new QHBoxLayout();
    options_layout->addWidget(filter_combo_box_label);
    options_layout->addWidget(m_gfxr_vulkan_command_filter, 1);
    options_layout->addWidget(m_search_trigger_button);
    options_layout->addStretch();

    m_search_bar = new SearchBar(this);
    m_search_bar->setObjectName(kGfxrVulkanCommandSearchBarName);
    m_search_bar->hide();

    QVBoxLayout *main_layout = new QVBoxLayout();
    main_layout->addLayout(options_layout);
    main_layout->addWidget(m_search_bar);
    main_layout->addWidget(m_command_hierarchy_view);
    setLayout(main_layout);
    m_search_bar->setView(m_command_hierarchy_view);

    QObject::connect(m_search_trigger_button, SIGNAL(clicked()), this, SLOT(OnSearchCommands()));

    QObject::connect(m_search_bar,
                     SIGNAL(hide_search_bar(bool)),
                     this,
                     SLOT(OnSearchBarVisibilityChange(bool)));

    connect(m_command_hierarchy_view,
            &QTreeView::customContextMenuRequested,
            this,
            &GfxrVulkanCommandTabView::OnCorrelateCommand);

    connect(m_command_hierarchy_view->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            &GfxrVulkanCommandTabView::OnSelectionChanged);
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandTabView::SetTopologyToView(const Dive::Topology *topology_ptr)
{
    m_command_hierarchy_model.SetTopologyToView(topology_ptr);
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandTabView::ResetModel()
{
    // Reset the filter
    m_gfxr_vulkan_command_filter->Reset();

    m_command_hierarchy_model.Reset();

    // Reset search results
    m_command_hierarchy_view->reset();
    if (m_search_bar->isVisible())
    {
        m_search_bar->clearSearch();
    }
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandTabView::OnSelectionChanged(const QModelIndex &index)
{
    if (!index.isValid() || index.parent() == QModelIndex())
    {
        return;
    }

    QModelIndex sourceIndex = m_proxy_Model.mapToSource(index);

    // Resize columns to fit
    uint32_t column_count = (uint32_t)m_command_hierarchy_model.columnCount(QModelIndex());
    for (uint32_t column = 0; column < column_count; ++column)
        m_command_hierarchy_view->resizeColumnToContents(column);

    m_command_hierarchy_view->expandAll();

    emit SelectCommand(sourceIndex);
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandTabView::OnSearchCommands()
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
void GfxrVulkanCommandTabView::OnSearchBarVisibilityChange(bool isHidden)
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
void GfxrVulkanCommandTabView::ConnectSearchBar()
{
    QObject::connect(m_search_bar,
                     SIGNAL(new_search(const QString &)),
                     m_command_hierarchy_view,
                     SLOT(searchNodeByText(const QString &)));
    QObject::connect(m_search_bar,
                     &SearchBar::next_search,
                     m_command_hierarchy_view,
                     &DiveTreeView::nextNodeInSearch);
    QObject::connect(m_search_bar,
                     &SearchBar::prev_search,
                     m_command_hierarchy_view,
                     &DiveTreeView::prevNodeInSearch);
    QObject::connect(m_command_hierarchy_view,
                     &DiveTreeView::updateSearch,
                     m_search_bar,
                     &SearchBar::updateSearchResults);
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandTabView::DisconnectSearchBar()
{
    QObject::disconnect(m_search_bar,
                        SIGNAL(new_search(const QString &)),
                        m_command_hierarchy_view,
                        SLOT(searchNodeByText(const QString &)));
    QObject::disconnect(m_search_bar,
                        &SearchBar::next_search,
                        m_command_hierarchy_view,
                        &DiveTreeView::nextNodeInSearch);
    QObject::disconnect(m_search_bar,
                        &SearchBar::prev_search,
                        m_command_hierarchy_view,
                        &DiveTreeView::prevNodeInSearch);
    QObject::disconnect(m_command_hierarchy_view,
                        &DiveTreeView::updateSearch,
                        m_search_bar,
                        &SearchBar::updateSearchResults);
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandTabView::ExpandAll()
{
    m_command_hierarchy_view->expandAll();
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandTabView::OnCorrelateCommand(const QPoint &pos)
{
    QModelIndex proxy_model_index = m_command_hierarchy_view->indexAt(pos);
    QModelIndex source_model_index = m_proxy_Model.mapToSource(proxy_model_index);
    uint64_t    node_index = (uint64_t)source_model_index.internalPointer();

    if (proxy_model_index.isValid() && m_vulkan_command_hierarchy.GetNodeType(node_index) ==
                                       Dive::NodeType::kGfxrVulkanDrawCommandNode)
    {
        QMenu    context_menu;
        QAction *binning_action = context_menu.addAction("PM4 Events with BinningPassOnly Filter");
        binning_action->setData(1);
        QAction *first_tile_action = context_menu.addAction(
        "PM4 Events with FirstTilePassOnly Filter");
        first_tile_action->setData(2);

        QAction *selectedAction = context_menu.exec(
        m_command_hierarchy_view->viewport()->mapToGlobal(pos));

        if (selectedAction)
        {
            emit ApplyFilter(source_model_index, selectedAction->data().toInt());
        }
    }
}
