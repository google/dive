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

#include "dive_core/data_core.h"
#include <QFrame>
#include <QSortFilterProxyModel>

#pragma once
// Forward declaration
class QLabel;
class QGroupBox;
class QLineEdit;
class QPushButton;
class SearchBar;
class GfxrVulkanCommandFilterProxyModel;
class GfxrVulkanCommandArgFilterProxyModel;
class DiveTreeView;
class GfxrVulkanCommandModel;
class GfxrVulkanCommandFilter;

namespace Dive
{
class CommandHierarchy;
class Topology;
};  // namespace Dive

//--------------------------------------------------------------------------------------------------
class GfxrVulkanCommandTabView : public QFrame
{
    Q_OBJECT

public:
    GfxrVulkanCommandTabView(const Dive::CommandHierarchy      &vulkan_command_hierarchy,
                             GfxrVulkanCommandFilterProxyModel &proxy_model,
                             GfxrVulkanCommandModel            &command_hierarchy_model,
                             QWidget                           *parent = nullptr);

    void SetTopologyToView(const Dive::Topology *topology_ptr);

    void ResetModel();

public slots:
    void OnSelectionChanged(const QModelIndex &index);
    void OnSearchCommands();
    void OnSearchBarVisibilityChange(bool isHidden);
    void ConnectSearchBar();
    void DisconnectSearchBar();
    void ExpandAll();
    void OnCorrelateCommand(const QPoint &pos);

signals:
    // Update property panel for node information.
    void SendNodeProperty(const QString &);
    void HideOtherSearchBars();
    void ApplyFilter(const QModelIndex &, int integer_value);
    void SelectCommand(const QModelIndex &);

private:
    DiveTreeView            *m_command_hierarchy_view;
    QPushButton             *m_search_trigger_button;
    SearchBar               *m_search_bar;
    GfxrVulkanCommandFilter *m_gfxr_vulkan_command_filter;

    const Dive::CommandHierarchy      &m_vulkan_command_hierarchy;
    GfxrVulkanCommandFilterProxyModel &m_proxy_Model;
    GfxrVulkanCommandModel            &m_command_hierarchy_model;
};
