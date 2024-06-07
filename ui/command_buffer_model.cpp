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
    "Command Buffer",
    "IB Level",
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
QModelIndex CommandBufferModel::scrollToIndex() const
{
    return m_scroll_to_index;
}

//--------------------------------------------------------------------------------------------------
QVariant CommandBufferModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role == Qt::TextAlignmentRole)
    {
        if (index.column() == CommandBufferModel::kColumnIbLevel)
            return int(Qt::AlignHCenter | Qt::AlignVCenter);
        else
            return int(Qt::AlignLeft | Qt::AlignVCenter);
    }

    uint64_t node_index = (uint64_t)(index.internalPointer());
    if (role == Qt::ForegroundRole && IsCurEvent(node_index))
        return QColor(255, 128, 128);

    if (role != Qt::DisplayRole)
        return QVariant();

    Dive::NodeType node_type = m_command_hierarchy.GetNodeType(node_index);

    // Column 2: Address (will be swapped via moveSection() to be visually the 0th column)
    // This forces the expand/collapse icon to be part of the pm4 column
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
    else if (index.column() == CommandBufferModel::kColumnIbLevel)
    {
        if (node_type == Dive::NodeType::kPacketNode)
        {
            uint64_t           ib_level = m_command_hierarchy.GetPacketNodeIbLevel(node_index);
            std::ostringstream ib_level_string_stream;
            ib_level_string_stream << ib_level;
            return QString::fromStdString(ib_level_string_stream.str());
        }
        else
        {
            return QVariant();
        }
    }
    else  // if (index.column() == CommandBufferModel::kColumnPm4)
    {
#ifndef NDEBUG  // DEBUG
        // Print out node_index, which helps a lot in debugging command hierarchy issues
        std::ostringstream addr_string_stream;
        addr_string_stream << m_command_hierarchy.GetNodeDesc(node_index) << " (" << node_index
                           << ")";
        return QString::fromStdString(addr_string_stream.str());
#else
        return QString(m_command_hierarchy.GetNodeDesc(node_index));
#endif
    }
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
        {
            if (section != kColumnIbLevel)
                return QVariant(tr(CommandBufferColumnNames[section]));
        }
    }
    else if (role == Qt::TextAlignmentRole)
    {
        if (section == CommandBufferModel::kColumnPm4)
            return int(Qt::AlignHCenter | Qt::AlignVCenter);
        else
            return int(Qt::AlignLeft | Qt::AlignVCenter);
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
        uint64_t root_node_index = m_topology_ptr->GetSharedChildRootNodeIndex(
        m_selected_node_index);
        uint64_t node_index = m_topology_ptr->GetSharedChildNodeIndex(root_node_index, row);
        return createIndex(row, column, (void *)node_index);
    }

    uint64_t parent_node_index = (uint64_t)(parent.internalPointer());

    // Children order is the "normal" children followed by the "shared" children
    uint64_t child_node_index = UINT64_MAX;
    if ((uint32_t)row < m_topology_ptr->GetNumChildren(parent_node_index))
    {
        child_node_index = m_topology_ptr->GetChildNodeIndex(parent_node_index, row);
    }
    else if (m_topology_ptr->GetNumSharedChildren(parent_node_index) > 0)
    {
        uint32_t index = row - m_topology_ptr->GetNumChildren(parent_node_index);
        child_node_index = m_topology_ptr->GetSharedChildNodeIndex(parent_node_index, index);
    }
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
    return m_node_parent_list[child_node_index];
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
        uint64_t root_node_index = m_topology_ptr->GetSharedChildRootNodeIndex(
        m_selected_node_index);
        uint64_t num_children = m_topology_ptr->GetNumSharedChildren(root_node_index);
        return num_children;
    }

    // Return sum of shared children + normal children
    //  Normal Children: The packet fields
    //  Shared Children: Additional packets (e.g. for packets from INDIRECT_BUFFERS packet)
    uint64_t parent_node_index = (uint64_t)(parent.internalPointer());
    uint64_t num_children = m_topology_ptr->GetNumChildren(parent_node_index) +
                            m_topology_ptr->GetNumSharedChildren(parent_node_index);
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
    uint64_t root_node_index = m_topology_ptr->GetSharedChildRootNodeIndex(m_selected_node_index);

    // Resize the look-up lists
    // The bit lists are 1-bit per node, but they're arrays of uint8_ts, so round up
    size_t bit_list_size = (m_command_hierarchy.size() + 7) / 8;
    m_node_parent_list.resize(m_command_hierarchy.size());
    m_node_is_cur_event_bit_list.clear();
    m_node_is_cur_event_bit_list.resize(bit_list_size);
    CreateNodeToParentMap(UINT64_MAX, root_node_index, false);

    // Determine the scroll-to position
    m_scroll_to_index = QModelIndex();
    uint64_t end_node_index = m_topology_ptr->GetEndSharedChildNodeIndex(m_selected_node_index);
    uint64_t parent_node_index = (uint64_t)(m_node_parent_list[end_node_index].internalPointer());
    uint64_t num_children = m_topology_ptr->GetNumSharedChildren(parent_node_index);
    for (uint64_t child = 0; child < num_children; ++child)
    {
        uint64_t child_node_index = m_topology_ptr->GetSharedChildNodeIndex(parent_node_index,
                                                                            child);
        // Cache the row index for the specific end node
        // Recall a parent has normal children + shared children (e.g. normal fields + packets)
        // The row has to account for both
        if (child_node_index == end_node_index)
        {
            uint64_t row = m_topology_ptr->GetNumChildren(parent_node_index) + child;
            m_scroll_to_index = createIndex(row, 0, (void *)end_node_index);

            // If the shared node has fields, then the last field should be the scroll-to position
            // instead
            if (m_topology_ptr->GetNumChildren(end_node_index) > 0)
            {
                uint32_t i = m_topology_ptr->GetNumChildren(end_node_index) - 1;
                uint64_t last_node_index = m_topology_ptr->GetChildNodeIndex(end_node_index, i);
                m_scroll_to_index = createIndex(i, 0, (void *)last_node_index);
            }
        }
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

//--------------------------------------------------------------------------------------------------
bool CommandBufferModel::CreateNodeToParentMap(uint64_t parent_row,
                                               uint64_t parent_node_index,
                                               bool     is_parent_cur_event)
{
    bool is_cur_event = is_parent_cur_event;
    if (is_parent_cur_event)
        SetIsCurEvent(parent_node_index);

    // Because shared (i.e. packet) nodes can have multiple parents, a map is created to match
    // those packets to the parent as seen during a specific traversal
    // This is a recursive function to traverse through all nodes in the tree

    uint64_t start_node_index = m_topology_ptr->GetStartSharedChildNodeIndex(m_selected_node_index);
    uint64_t end_node_index = m_topology_ptr->GetEndSharedChildNodeIndex(m_selected_node_index);

    // To keep things simple, also include non-shared nodes in this mapping
    // This way the parent() function will be a simple lookup regardless of packet type
    if (parent_row != UINT64_MAX)
    {
        uint64_t num_children = m_topology_ptr->GetNumChildren(parent_node_index);
        for (uint64_t child = 0; child < num_children; ++child)
        {
            uint64_t child_node_index = m_topology_ptr->GetChildNodeIndex(parent_node_index, child);
            QModelIndex model_index = QModelIndex();
            // Check parent_row against UINT64_MAX, since only the shared children of the root node
            // should be considered
            if (parent_row != UINT64_MAX)
                model_index = createIndex(parent_row, 0, (void *)parent_node_index);
            DIVE_ASSERT(child_node_index < m_node_parent_list.size());
            m_node_parent_list[child_node_index] = model_index;

            CreateNodeToParentMap(child, child_node_index, is_cur_event);
        }
    }

    // Now for the shared nodes...
    uint64_t num_children = m_topology_ptr->GetNumSharedChildren(parent_node_index);
    for (uint64_t child = 0; child < num_children; ++child)
    {
        uint64_t    child_node_index = m_topology_ptr->GetSharedChildNodeIndex(parent_node_index,
                                                                            child);
        QModelIndex model_index = QModelIndex();
        if (parent_row != UINT64_MAX)
            model_index = createIndex(parent_row, 0, (void *)parent_node_index);
        DIVE_ASSERT(child_node_index < m_node_parent_list.size());
        m_node_parent_list[child_node_index] = model_index;

        is_cur_event |= (child_node_index == start_node_index);

        // if cur_event==true, and return value is false, then that means the event ended in one
        // of the children. Will no longer be part of current event in the future
        // If cur_event==false, and return value is true, then that means event started in one
        // of the children. Will continue be part of the current event going forward
        is_cur_event = CreateNodeToParentMap(child, child_node_index, is_cur_event);

        if (child_node_index == end_node_index)
            is_cur_event = false;
    }
    return is_cur_event;
}

//--------------------------------------------------------------------------------------------------
void CommandBufferModel::SetIsCurEvent(uint64_t node_index)
{
    uint32_t array_index = node_index / 8;
    uint32_t bit_element = node_index % 8;
    uint8_t  mask = 0x1 << bit_element;
    m_node_is_cur_event_bit_list[array_index] |= mask;
}

//--------------------------------------------------------------------------------------------------
bool CommandBufferModel::IsCurEvent(uint64_t node_index) const
{
    uint32_t array_index = node_index / 8;
    uint32_t bit_element = node_index % 8;
    uint8_t  mask = 0x1 << bit_element;
    return (m_node_is_cur_event_bit_list[array_index] & mask) != 0;
}