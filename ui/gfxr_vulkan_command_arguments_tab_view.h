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

#include <QFrame>
#include <QSortFilterProxyModel>

#pragma once
// Forward declaration
class QGroupBox;
class QLineEdit;
class QPushButton;
class SearchBar;
class GfxrVulkanCommandFilterProxyModel;
class GfxrVulkanCommandArgumentsFilterProxyModel;
class DiveTreeView;
class GfxrVulkanCommandModel;
namespace Dive
{
class CommandHierarchy;
class Topology;
};  // namespace Dive

//--------------------------------------------------------------------------------------------------
class GfxrVulkanCommandArgumentsTabView : public QFrame
{
    Q_OBJECT

public:
    GfxrVulkanCommandArgumentsTabView(const Dive::CommandHierarchy &vulkan_command_hierarchy,
                                      GfxrVulkanCommandArgumentsFilterProxyModel *proxy_model,
                                      GfxrVulkanCommandModel *command_hierarchy_model,
                                      QWidget                *parent = nullptr);

    void SetTopologyToView(const Dive::Topology *topology_ptr);

    void ResetModel();

public slots:
    void OnSelectionChanged(const QModelIndex &index);
    void OnSearchCommandArgs();
    void OnSearchBarVisibilityChange(bool isHidden);
    void ConnectSearchBar();
    void DisconnectSearchBar();

signals:
    // Update property panel for node information.
    void SendNodeProperty(const QString &);
    void HideOtherSearchBars();

private:
    DiveTreeView *m_command_hierarchy_view;
    QPushButton  *m_search_trigger_button;
    SearchBar    *m_search_bar = nullptr;

    const Dive::CommandHierarchy               &m_vulkan_command_hierarchy;
    GfxrVulkanCommandArgumentsFilterProxyModel *m_arg_proxy_model;
    GfxrVulkanCommandModel                     *m_command_hierarchy_model;
};
