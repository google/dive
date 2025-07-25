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

#include "gfxr_vulkan_command_model.h"
#include <QString>
#include <QStringList>
#include <QTreeWidget>
#include <cstdint>
#include <iostream>
#include <string>

#include "dive_core/command_hierarchy.h"

static_assert(sizeof(void *) == sizeof(uint64_t),
              "Unable to store a uint64_t into internalPointer()!");

// =================================================================================================
// GfxrVulkanCommandModel
// =================================================================================================
GfxrVulkanCommandModel::GfxrVulkanCommandModel(const Dive::CommandHierarchy &command_hierarchy) :
    m_command_hierarchy(command_hierarchy)
{
    m_topology_ptr = nullptr;
}

//--------------------------------------------------------------------------------------------------
GfxrVulkanCommandModel::~GfxrVulkanCommandModel() {}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandModel::Reset()
{
    emit beginResetModel();
    m_topology_ptr = nullptr;
    emit endResetModel();
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandModel::BeginResetModel()
{
    emit beginResetModel();
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandModel::EndResetModel()
{
    emit endResetModel();
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandModel::SetTopologyToView(const Dive::Topology *topology_ptr)
{
    BeginResetModel();
    m_topology_ptr = topology_ptr;
    m_node_lookup.clear();
    EndResetModel();
}

//--------------------------------------------------------------------------------------------------
QVariant GfxrVulkanCommandModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    uint64_t node_index = (uint64_t)(index.internalPointer());
    if (role == Qt::ForegroundRole)
    {
        return QVariant();
    }
    else if (role != Qt::DisplayRole)
        return QVariant();

    // 1st column
    return QString(m_command_hierarchy.GetNodeDesc(node_index));
}

//--------------------------------------------------------------------------------------------------
Qt::ItemFlags GfxrVulkanCommandModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemFlags();

    return QAbstractItemModel::flags(index);
}

//--------------------------------------------------------------------------------------------------
QVariant GfxrVulkanCommandModel::headerData(int             section,
                                            Qt::Orientation orientation,
                                            int             role) const
{
    return QVariant();
}

//--------------------------------------------------------------------------------------------------
QModelIndex GfxrVulkanCommandModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    uint64_t node_index;
    if (!parent.isValid())
    {
        // Root level in the model, which is actually one-level down in the topology, since the root
        // node is ignored
        node_index = m_topology_ptr->GetChildNodeIndex(Dive::Topology::kRootNodeIndex, row);
    }
    else
    {
        uint64_t parent_node_index = (uint64_t)(parent.internalPointer());
        node_index = m_topology_ptr->GetChildNodeIndex(parent_node_index, row);
    }
    return createIndex(row, column, node_index);
}

//--------------------------------------------------------------------------------------------------
QModelIndex GfxrVulkanCommandModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    uint64_t child_node_index = (uint64_t)(index.internalPointer());
    uint64_t parent_node_index = m_topology_ptr->GetParentNodeIndex(child_node_index);

    // Root item. No parent.
    if (parent_node_index == UINT64_MAX)
        return QModelIndex();

    uint64_t row = m_topology_ptr->GetChildIndex(parent_node_index);
    return createIndex(row, 0, (void *)parent_node_index);
}

//--------------------------------------------------------------------------------------------------
int GfxrVulkanCommandModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;
    if (m_topology_ptr == nullptr || m_topology_ptr->GetNumNodes() == 0)
        return 0;

    uint64_t parent_node_index;
    if (!parent.isValid())  // Root level
        parent_node_index = Dive::Topology::kRootNodeIndex;
    else
        parent_node_index = (uint64_t)(parent.internalPointer());

    return m_topology_ptr->GetNumChildren(parent_node_index);
}

//--------------------------------------------------------------------------------------------------
int GfxrVulkanCommandModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

//--------------------------------------------------------------------------------------------------
QModelIndex GfxrVulkanCommandModel::findNode(uint64_t node_index) const
{
    if (m_node_lookup.size() != m_command_hierarchy.size())
        BuildNodeLookup();
    if (node_index < m_command_hierarchy.size() && m_node_lookup[node_index].isValid())
        return m_node_lookup[node_index];
    return QModelIndex();
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandModel::BuildNodeLookup(const QModelIndex &parent) const
{
    if (!parent.isValid())
    {
        m_node_lookup.clear();
        m_node_lookup.resize(m_command_hierarchy.size());
    }
    int n = rowCount(parent);
    for (int r = 0; r < n; ++r)
    {
        auto     ix = index(r, 0, parent);
        uint64_t node_index = (uint64_t)ix.internalPointer();
        if (node_index < m_node_lookup.size())
            m_node_lookup[node_index] = QPersistentModelIndex(ix);
        BuildNodeLookup(ix);
    }
}

//--------------------------------------------------------------------------------------------------
QList<QModelIndex> GfxrVulkanCommandModel::search(const QModelIndex &start,
                                                  const QVariant    &value) const
{
    QList<QModelIndex>  result;
    Qt::CaseSensitivity cs = Qt::CaseInsensitive;

    QString     text;
    QModelIndex p = parent(start);
    int         from = start.row();
    int         to = rowCount(p);

    for (int r = from; r < to; ++r)
    {
        QModelIndex idx = index(r, start.column(), p);
        if (!idx.isValid())
            continue;
        QVariant v = data(idx, Qt::DisplayRole);

        if (text.isEmpty())
            text = value.toString();
        QString t = v.toString();
        if (t.contains(text, cs))
            result.append(idx);

        // Search the hierarchy
        if (hasChildren(idx))
            result += search(index(0, idx.column(), idx), (text.isEmpty() ? value : text));
    }

    return result;
}

uint64_t GfxrVulkanCommandModel::getNumNodes() const
{
    return m_command_hierarchy.size();
}
