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

#include "gfxr_vulkan_command_filter_proxy_model.h"

GfxrVulkanCommandFilterProxyModel::GfxrVulkanCommandFilterProxyModel(
QObject                      *parent,
const Dive::CommandHierarchy *command_hierarchy) :
    QSortFilterProxyModel(parent),
    m_command_hierarchy(command_hierarchy)
{
}

bool GfxrVulkanCommandFilterProxyModel::filterAcceptsRow(int                sourceRow,
                                                         const QModelIndex &sourceParent) const
{
    QModelIndex indexInSource = sourceModel()->index(sourceRow, 0, sourceParent);

    if (!indexInSource.isValid())
    {
        return true;
    }

    uint64_t                      node_index = (uint64_t)indexInSource.internalPointer();
    const GfxrVulkanCommandModel *sourceMyModel = qobject_cast<const GfxrVulkanCommandModel *>(
    sourceModel());
    if (!sourceMyModel)
    {
        return false;
    }

    if (node_index >= sourceMyModel->getNumNodes())
    {
        return false;
    }

    if (m_command_hierarchy->GetNodeType(node_index) == Dive::NodeType::kGfxrVulkanSubmitNode)
    {
        return true;
    }

    if (m_command_hierarchy->GetNodeType(node_index) ==
        Dive::NodeType::kGfxrVulkanCommandBufferNode)
    {
        return true;
    }

    if (m_command_hierarchy->GetNodeType(node_index) == Dive::NodeType::kGfxrVulkanCommandNode)
    {
        return true;
    }

    if (m_command_hierarchy->GetNodeType(node_index) == Dive::NodeType::kGfxrVulkanCommandArgNode)
    {
        return false;
    }

    // Do not include non-gfxr submits and their descendents.
    if (m_command_hierarchy->GetNodeType(node_index) == Dive::NodeType::kSubmitNode)
    {
        return false;
    }

    return true;
}
