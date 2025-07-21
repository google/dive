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

#include "gfxr_vulkan_command_arguments_filter_proxy_model.h"
#include <cstdint>
#include <iostream>
#include <string>

GfxrVulkanCommandArgumentsFilterProxyModel::GfxrVulkanCommandArgumentsFilterProxyModel(
QObject                      *parent,
const Dive::CommandHierarchy *command_hierarchy) :
    QSortFilterProxyModel(parent),
    m_command_hierarchy(command_hierarchy)
{
}

void GfxrVulkanCommandArgumentsFilterProxyModel::setTargetParentSourceIndex(
const QModelIndex &sourceIndex)
{
    if (m_targetParentSourceIndex != sourceIndex)
    {
        beginResetModel();
        m_targetParentSourceIndex = sourceIndex;
        endResetModel();
    }
}

bool GfxrVulkanCommandArgumentsFilterProxyModel::isDescendant(
const QModelIndex &potentialDescendant,
const QModelIndex &potentialAncestor) const
{
    if (!potentialAncestor.isValid())
    {
        return false;
    }
    if (!potentialDescendant.isValid())
    {
        return false;
    }
    if (potentialDescendant == potentialAncestor)
    {
        return true;
    }

    QModelIndex currentParent = potentialDescendant.parent();
    while (currentParent.isValid())
    {
        if (currentParent == potentialAncestor)
        {
            return true;
        }
        currentParent = currentParent.parent();
    }
    return false;
}

QVariant GfxrVulkanCommandArgumentsFilterProxyModel::data(const QModelIndex &index, int role) const
{
    QVariant value = QSortFilterProxyModel::data(index, role);

    if (role == Qt::ForegroundRole)
    {
        QModelIndex sourceIndex = mapToSource(index);

        if (!m_targetParentSourceIndex.isValid())
        {
            return QVariant(QColor(Qt::gray));
        }

        if (isDescendant(sourceIndex, m_targetParentSourceIndex))
        {
            return QVariant(QColor(Qt::white));
        }
        else
        {
            return QVariant(QColor(Qt::gray));
        }
    }

    return value;
}

bool GfxrVulkanCommandArgumentsFilterProxyModel::filterAcceptsRow(
int                source_row,
const QModelIndex &source_parent) const
{
    if (!m_targetParentSourceIndex.isValid())
    {
        return false;
    }

    QModelIndex currentSourceIndex = sourceModel()->index(source_row, 0, source_parent);

    if (currentSourceIndex == m_targetParentSourceIndex)
    {
        return true;
    }

    // Accept all ancestors of the selected item.
    QModelIndex ancestorOfTarget = m_targetParentSourceIndex.parent();
    while (ancestorOfTarget.isValid())
    {
        if (ancestorOfTarget == currentSourceIndex)
        {
            return true;
        }
        ancestorOfTarget = ancestorOfTarget.parent();
    }

    // Accept all descendants of the selected item. If it is an argument node,
    // accept the node.
    if (isDescendant(currentSourceIndex, m_targetParentSourceIndex))
    {
        uint64_t current_node_index = currentSourceIndex.internalId();

        if (m_command_hierarchy)
        {
            Dive::NodeType current_node_type = m_command_hierarchy->GetNodeType(current_node_index);

            if (current_node_type == Dive::NodeType::kGfxrVulkanCommandArgNode)
            {
                return true;
            }
        }
    }
    return false;
}
