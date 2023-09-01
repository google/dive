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
#include "command_buffer_model.h"
#include <QString>
#include <QStringList>
#include <QTreeWidget>

#include <iostream>
#include <sstream>
#include "dive_core/command_hierarchy.h"

static_assert(sizeof(void *) == sizeof(uint64_t),
              "Unable to store a uint64_t into internalPointer()!");

static const char *CommandBufferColumnNames[] = {
    "DE Command Buffer",
    "CE Command Buffer",
    "Address",
};

static_assert(sizeof(CommandBufferColumnNames) / sizeof(CommandBufferColumnNames[0]) ==
              static_cast<size_t>(CommandBufferModel::kColumnCount),
              "Mismatched CommandBuffer columns");

// =================================================================================================
// CommandBufferModel
// =================================================================================================
CommandBufferModel::CommandBufferModel(const Dive::CommandHierarchy &command_hierarchy) :
    m_command_hierarchy(command_hierarchy)
{
}

//--------------------------------------------------------------------------------------------------
CommandBufferModel::~CommandBufferModel() {}

//--------------------------------------------------------------------------------------------------
void CommandBufferModel::Reset()
{
    emit beginResetModel();
    m_selected_node_index = UINT64_MAX;
    emit endResetModel();
}

//--------------------------------------------------------------------------------------------------
void CommandBufferModel::SetTopologyToView(const Dive::Topology *topology_ptr)
{
    emit beginResetModel();
    m_topology_ptr = topology_ptr;
    m_selected_node_index = UINT64_MAX;
    emit endResetModel();
}

//--------------------------------------------------------------------------------------------------
int CommandBufferModel::columnCount(const QModelIndex &parent) const
{
    return kColumnCount;
}

//--------------------------------------------------------------------------------------------------
QVariant CommandBufferModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    uint64_t       node_index = (uint64_t)(index.internalPointer());
    Dive::NodeType node_type = m_command_hierarchy.GetNodeType(node_index);

    // Column 2: Address (will be swapped via moveSection() to be visually the 0th column)
    if (index.column() == CommandBufferModel::kColumnAddress)
    {
        if (node_type == Dive::NodeType::kPacketNode)
        {
            uint64_t           addr = m_command_hierarchy.GetPacketNodeAddr(node_index);
            std::ostringstream addr_string_stream;
            addr_string_stream << "0x" << std::hex << addr;
            return QString::fromStdString(addr_string_stream.str());
        }
        else
        {
            return QVariant();
        }
    }
    int target_column = 0;
    switch (node_type)
    {
    case Dive::NodeType::kPacketNode:
    {
        target_column = m_command_hierarchy.GetPacketNodeIsCe(node_index) ? 1 : 0;
        break;
    }
    case Dive::NodeType::kRegNode:
    case Dive::NodeType::kFieldNode:
    {
        target_column = m_command_hierarchy.GetRegFieldNodeIsCe(node_index) ? 1 : 0;
        break;
    }
    default: DIVE_ASSERT(false);
    };

    // Column 0 for DE packets, and column 1 is for CE packets
    if (index.column() != target_column)
        return QVariant();
    return QString(m_command_hierarchy.GetNodeDesc(node_index));
}

//--------------------------------------------------------------------------------------------------
Qt::ItemFlags CommandBufferModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemFlags();

    return QAbstractItemModel::flags(index);
}

//--------------------------------------------------------------------------------------------------
QVariant CommandBufferModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        if (section < CommandBufferModel::kColumnCount)
            return QVariant(tr(CommandBufferColumnNames[section]));
    }
    return QVariant();
}

//--------------------------------------------------------------------------------------------------
QModelIndex CommandBufferModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    // Root level
    if (!parent.isValid())
    {
        // Get node index from the second child set. This is because the second child set contains
        // packet nodes. First child set never does.
        uint64_t node_index = m_topology_ptr->GetSharedChildNodeIndex(m_selected_node_index, row);
        return createIndex(row, column, (void *)node_index);
    }

    uint64_t parent_node_index = (uint64_t)(parent.internalPointer());
    uint64_t child_node_index = m_topology_ptr->GetChildNodeIndex(parent_node_index, row);
    if (child_node_index != UINT64_MAX)
        return createIndex(row, column, (void *)child_node_index);
    else
        return QModelIndex();
}

//--------------------------------------------------------------------------------------------------
QModelIndex CommandBufferModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    uint64_t child_node_index = (uint64_t)(index.internalPointer());

    // Root item. No parent.
    if (m_command_hierarchy.GetNodeType(child_node_index) == Dive::NodeType::kPacketNode)
        return QModelIndex();

    uint64_t row;
    uint64_t parent_node_index = m_topology_ptr->GetParentNodeIndex(child_node_index);

    // Packet nodes serve as root nodes in this model, and packet nodes in particular are "shared"
    // nodes with many parents. So use a map to determine which child this packet node with respect
    // to the selected node.
    if (m_command_hierarchy.GetNodeType(parent_node_index) != Dive::NodeType::kPacketNode)
        row = m_topology_ptr->GetChildIndex(parent_node_index);
    else
    {
        auto it = m_node_index_to_row_map.find(parent_node_index);
        DIVE_ASSERT(it != m_node_index_to_row_map.end());
        row = it->second;
    }
    return createIndex(row, 0, (void *)parent_node_index);
}

//--------------------------------------------------------------------------------------------------
int CommandBufferModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;
    if (m_selected_node_index == UINT64_MAX)
        return 0;

    if (!parent.isValid())  // Root level
    {
        // Second child set contains packet nodes. First child set never does.
        uint64_t num_children = m_topology_ptr->GetNumSharedChildren(m_selected_node_index);
        return num_children;
    }

    uint64_t parent_node_index = (uint64_t)(parent.internalPointer());
    uint64_t num_children = m_topology_ptr->GetNumChildren(parent_node_index);
    return num_children;
}

//--------------------------------------------------------------------------------------------------
void CommandBufferModel::OnSelectionChanged(const QModelIndex &index)
{
    uint64_t selected_node_index = (uint64_t)(index.internalPointer());
    if (m_selected_node_index == selected_node_index)  // Selected same item
        return;

    emit beginResetModel();
    m_selected_node_index = selected_node_index;

    // Keep track of the child-index for each packet node;
    uint64_t num_children = m_topology_ptr->GetNumSharedChildren(m_selected_node_index);
    for (uint64_t child = 0; child < num_children; ++child)
    {
        uint64_t child_node_index = m_topology_ptr->GetSharedChildNodeIndex(m_selected_node_index,
                                                                            child);
        DIVE_ASSERT(m_command_hierarchy.GetNodeType(child_node_index) ==
                    Dive::NodeType::kPacketNode);
        m_node_index_to_row_map[child_node_index] = child;
    }
    emit endResetModel();
}

//--------------------------------------------------------------------------------------------------
QList<QModelIndex> CommandBufferModel::search(const QModelIndex &start, const QVariant &value) const
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
