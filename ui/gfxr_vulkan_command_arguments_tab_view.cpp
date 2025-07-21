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

#include "gfxr_vulkan_command_arguments_tab_view.h"
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <iostream>
#include <string>
#include "gfxr_vulkan_command_filter_proxy_model.h"
#include "gfxr_vulkan_command_arguments_filter_proxy_model.h"
#include "search_bar.h"
#include "shortcuts.h"

#include "dive_core/gfxr_vulkan_command_hierarchy.h"
#include "object_names.h"

// =================================================================================================
// GfxrVulkanCommandArgumentsTabView
// =================================================================================================
GfxrVulkanCommandArgumentsTabView::GfxrVulkanCommandArgumentsTabView(
const Dive::CommandHierarchy               &vulkan_command_hierarchy,
GfxrVulkanCommandArgumentsFilterProxyModel *proxy_model,
GfxrVulkanCommandModel                     *command_hierarchy_model,
QWidget                                    *parent) :
    m_vulkan_command_hierarchy(vulkan_command_hierarchy),
    m_arg_proxy_Model(proxy_model),
    m_command_hierarchy_model(command_hierarchy_model)
{
    m_command_hierarchy_view = new DiveTreeView(m_vulkan_command_hierarchy);

    m_arg_proxy_Model->setSourceModel(m_command_hierarchy_model);
    m_command_hierarchy_view->setModel(m_arg_proxy_Model);

    m_search_trigger_button = new QPushButton;
    m_search_trigger_button->setObjectName(kGfxrVulkanCommandArgumentsSearchButtonName);
    m_search_trigger_button->setIcon(QIcon(":/images/search.png"));

    QHBoxLayout *options_layout = new QHBoxLayout();
    options_layout->addWidget(m_search_trigger_button);
    options_layout->addStretch();

    m_search_bar = new SearchBar(this);
    m_search_bar->setObjectName(kGfxrVulkanCommandArgumentsSearchBarName);
    m_search_bar->hide();

    QVBoxLayout *main_layout = new QVBoxLayout();
    main_layout->addLayout(options_layout);
    main_layout->addWidget(m_search_bar);
    main_layout->addWidget(m_command_hierarchy_view);
    setLayout(main_layout);
    m_search_bar->setTreeView(m_command_hierarchy_view);

    QObject::connect(m_search_trigger_button, SIGNAL(clicked()), this, SLOT(OnSearchCommandArgs()));

    QObject::connect(m_search_bar,
                     SIGNAL(hide_search_bar(bool)),
                     this,
                     SLOT(OnSearchBarVisibilityChange(bool)));
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandArgumentsTabView::SetTopologyToView(const Dive::Topology *topology_ptr)
{
    m_command_hierarchy_model->SetTopologyToView(topology_ptr);
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandArgumentsTabView::ResetModel()
{
    m_command_hierarchy_model->Reset();
    // Reset search results
    m_command_hierarchy_view->reset();
    if (m_search_bar->isVisible())
    {
        m_search_bar->clearSearch();
    }
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandArgumentsTabView::OnSelectionChanged(const QModelIndex &index)
{
    if (!index.isValid())
    {
        return;
    }

    auto *selection_model = qobject_cast<QItemSelectionModel *>(sender());
    if (!selection_model)
    {
        return;
    }

    // Get the proxy model from the selection model.
    auto *proxy_model = qobject_cast<QSortFilterProxyModel *>(selection_model->model());
    if (!proxy_model)
    {
        return;
    }

    // Map the index using the correct proxy model.
    QModelIndex source_index = proxy_model->mapToSource(index);

    if (!source_index.isValid())
    {
        m_arg_proxy_Model->setTargetParentSourceIndex(QModelIndex());
        return;
    }

    m_arg_proxy_Model->setTargetParentSourceIndex(source_index);

    m_command_hierarchy_view->expandAll();
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandArgumentsTabView::OnSearchCommandArgs()
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
void GfxrVulkanCommandArgumentsTabView::OnSearchBarVisibilityChange(bool isHidden)
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
void GfxrVulkanCommandArgumentsTabView::ConnectSearchBar()
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
void GfxrVulkanCommandArgumentsTabView::DisconnectSearchBar()
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
