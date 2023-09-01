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
#include "command_buffer_view.h"
#include <QPainter>
#include "command_buffer_model.h"
#include "dive_core/command_hierarchy.h"

static_assert(sizeof(void *) == sizeof(uint64_t),
              "Unable to store a uint64_t into internalPointer()!");

// =================================================================================================
// CommandBufferViewDelegate
// =================================================================================================
CommandBufferViewDelegate::CommandBufferViewDelegate(
const CommandBufferView *command_buffer_view_ptr) :
    QStyledItemDelegate(0),
    m_command_buffer_view_ptr(command_buffer_view_ptr)
{}

//--------------------------------------------------------------------------------------------------
void CommandBufferViewDelegate::paint(QPainter *                  painter,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex &         index) const
{
    // Handle the drawing of the CE column. The CE column is not a tree, but will be rendered with
    // indentations to match the look of the DE column. The only difference is that the CE column
    // items are not expandable/collapsable, since they are not in reality a tree
    const Dive::CommandHierarchy &command_hierarchy = m_command_buffer_view_ptr
                                                      ->GetCommandHierarchy();
    QStyleOptionViewItem new_option = option;
    if (index.column() == CommandBufferModel::kColumnCE)
    {
        bool           is_ce_packet = false;
        uint64_t       node_index = (uint64_t)(index.internalPointer());
        Dive::NodeType node_type = command_hierarchy.GetNodeType(node_index);
        switch (node_type)
        {
        case Dive::NodeType::kPacketNode:
        {
            is_ce_packet = command_hierarchy.GetPacketNodeIsCe(node_index);
            break;
        }
        case Dive::NodeType::kRegNode:
        {
            is_ce_packet = false;  // No reg nodes in CE column
            break;
        }
        case Dive::NodeType::kFieldNode:
        {
            is_ce_packet = command_hierarchy.GetRegFieldNodeIsCe(node_index);
            break;
        }
        default: DIVE_ASSERT(false);
        };

        if (is_ce_packet)
        {
            uint32_t indentation = m_command_buffer_view_ptr->indentation();

            QRect fake_branch_rect;

            // Handle rendering of the fake "branch" item
            {
                // Calculate the branch_rect area and adjust rect for the text
                if (node_type == Dive::NodeType::kPacketNode)
                {
                    const uint32_t kMargin = 5;
                    uint32_t       center_x = option.rect.x() + indentation / 2;
                    uint32_t       center_y = option.rect.y() + option.rect.height() / 2;
                    uint32_t       dimension = std::min<int>(10,
                                                       std::min(option.rect.height() - kMargin * 2,
                                                                indentation - kMargin * 2));
                    fake_branch_rect = QRect(center_x - dimension / 2,
                                             center_y - dimension / 2,
                                             dimension,
                                             dimension);
                    new_option.rect.setLeft(option.rect.x() + indentation);
                }
                else
                {
                    fake_branch_rect = QRect(option.rect.x() + indentation,
                                             option.rect.y(),
                                             indentation,
                                             option.rect.height());
                    new_option.rect.setLeft(option.rect.x() + 2 * indentation);
                }

                // Fill background if selected (since we are adjusting the rect, we have to deal
                // with the selection highlighting now)
                {
                    m_command_buffer_view_ptr->style()->drawPrimitive(QStyle::PE_PanelItemViewItem,
                                                                      &option,
                                                                      painter,
                                                                      m_command_buffer_view_ptr);
                }

                // Render "fake" branch item
                if (node_type == Dive::NodeType::kPacketNode)
                {
                    // Render the "bullet" for the packet item
                    painter->fillRect(fake_branch_rect, Qt::lightGray);
                }
                else
                {
                    // Last element is rendered differently
                    QModelIndex parent = m_command_buffer_view_ptr->model()->parent(index);
                    uint32_t    row_count = m_command_buffer_view_ptr->model()->rowCount(parent);
                    QRect       branch_rect(option.rect.x() + indentation,
                                      option.rect.y(),
                                      indentation,
                                      option.rect.height());
                    DIVE_ASSERT(index.row() >= 0);  // Pre-typecast check
                    if ((uint32_t)index.row() == row_count - 1)
                    {
                        painter->drawImage(branch_rect, QImage(":/images/branch-end.png"));
                    }
                    else
                    {
                        painter->drawImage(branch_rect, QImage(":/images/branch-more.png"));
                    }
                }
            }
        }
    }
    QStyledItemDelegate::paint(painter, new_option, index);
}

// =================================================================================================
// CommandBufferView
// =================================================================================================
CommandBufferView::CommandBufferView(const Dive::CommandHierarchy &command_hierarchy,
                                     QWidget *                     parent) :
    DiveTreeView(command_hierarchy, parent)
{
    setItemDelegate(new CommandBufferViewDelegate(this));
    setAccessibleName("DiveCommandBufferView");
}

//--------------------------------------------------------------------------------------------------
bool CommandBufferView::RenderBranch(const QModelIndex &index) const
{
    uint64_t node_index = (uint64_t)(index.internalPointer());

    // Do not render branch controls for rows that contain CE info
    Dive::NodeType node_type = m_command_hierarchy.GetNodeType(node_index);
    switch (node_type)
    {
    case Dive::NodeType::kPacketNode: return !m_command_hierarchy.GetPacketNodeIsCe(node_index);
    case Dive::NodeType::kRegNode:
    case Dive::NodeType::kFieldNode: return !m_command_hierarchy.GetRegFieldNodeIsCe(node_index);
    default: DIVE_ASSERT(false);
    };

    return true;
}

//--------------------------------------------------------------------------------------------------
void CommandBufferView::setAndScrollToIndex(QModelIndex &idx)
{
    auto m = dynamic_cast<CommandBufferModel *>(model());
    idx = m->index(idx.row(), 1, idx.parent());
    scrollTo(idx);
    setCurrentIndex(idx);
}

//--------------------------------------------------------------------------------------------------
void CommandBufferView::Reset()
{
    search_indexes.clear();
    search_index_it = search_indexes.begin();
}

//--------------------------------------------------------------------------------------------------
void CommandBufferView::searchCommandBufferByText(const QString &search_text)
{
    search_indexes.clear();
    search_index_it = search_indexes.begin();

    if (search_text.isEmpty())
        return;

    auto m = dynamic_cast<CommandBufferModel *>(model());
    search_indexes = m->search(m->index(0, 0), QVariant::fromValue(search_text));
    search_index_it = search_indexes.begin();

    if (!search_indexes.isEmpty())
    {
        QModelIndex curr_idx = currentIndex();
        if (curr_idx.isValid() && curr_idx != *search_index_it)
        {
            search_index_it = search_indexes.begin() +
                              getNearestSearchCommand((uint64_t)(curr_idx.internalPointer()));
        }
        setAndScrollToIndex(*search_index_it);
    }
    emit updateSearch(0, search_indexes.isEmpty() ? 0 : search_indexes.size());
}

//--------------------------------------------------------------------------------------------------
void CommandBufferView::nextCommandInSearch()
{
    if (!search_indexes.isEmpty() && search_index_it != search_indexes.end() &&
        (search_index_it + 1) != search_indexes.end())
    {
        QModelIndex curr_idx = currentIndex();
        if (curr_idx.isValid() && curr_idx != *search_index_it)
        {
            search_index_it = search_indexes.begin() +
                              getNearestSearchCommand((uint64_t)(curr_idx.internalPointer()));
            if (*search_index_it == curr_idx)
                ++search_index_it;
        }
        else
        {
            ++search_index_it;
        }
        setAndScrollToIndex(*search_index_it);
        emit updateSearch(search_index_it - search_indexes.begin(), search_indexes.size());
    }
}

//--------------------------------------------------------------------------------------------------
void CommandBufferView::prevCommandInSearch()
{
    if (!search_indexes.isEmpty() && search_index_it != search_indexes.begin())
    {
        QModelIndex curr_idx = currentIndex();
        if (curr_idx.isValid() && curr_idx != *search_index_it)
        {
            search_index_it = search_indexes.begin() +
                              getNearestSearchCommand((uint64_t)(curr_idx.internalPointer()));
            if (*search_index_it == curr_idx)
                --search_index_it;
        }
        else
        {
            --search_index_it;
        }
        setAndScrollToIndex(*search_index_it);
        emit updateSearch(search_index_it - search_indexes.begin(), search_indexes.size());
    }
}

//--------------------------------------------------------------------------------------------------
int CommandBufferView::getNearestSearchCommand(uint64_t target_index)
{
    auto get_internal_pointer = [this](int index) {
        return (uint64_t)(search_indexes[index].internalPointer());
    };

    auto get_nearest = [this](int x, int y, uint64_t target) {
        if (target - (uint64_t)(search_indexes[x].internalPointer()) >=
            (uint64_t)(search_indexes[y].internalPointer()) - target)
            return y;
        else
            return x;
    };

    int n = search_indexes.size();
    int left = 0, right = n, mid = 0;

    if (target_index <= get_internal_pointer(left))
        return left;

    if (target_index >= get_internal_pointer(right - 1))
        return right - 1;

    while (left < right)
    {
        mid = (left + right) / 2;

        if (target_index == get_internal_pointer(mid))
            return mid;
        if (target_index < get_internal_pointer(mid))
        {
            if (mid > 0 && target_index > get_internal_pointer(mid - 1))
                return get_nearest(mid - 1, mid, target_index);
            right = mid;
        }
        else
        {
            if (mid < n - 1 && target_index < get_internal_pointer(mid + 1))
                return get_nearest(mid, mid + 1, target_index);
            left = mid + 1;
        }
    }
    return mid;
}
