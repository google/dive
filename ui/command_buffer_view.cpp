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
    QStyledItemDelegate(0), m_command_buffer_view_ptr(command_buffer_view_ptr)
{
}

//--------------------------------------------------------------------------------------------------
void CommandBufferViewDelegate::paint(QPainter                   *painter,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex          &index) const
{
    // Draw vertical border around a ib-level cell
    if (index.column() == CommandBufferModel::kColumnIbLevel)
    {
        painter->save();
        painter->setPen(QPen(Qt::darkGray, 1));
        painter->drawLine(option.rect.left(),
                          option.rect.top(),
                          option.rect.left(),
                          option.rect.bottom());
        painter->drawLine(option.rect.right(),
                          option.rect.top(),
                          option.rect.right(),
                          option.rect.bottom());

        painter->restore();
    }
    QStyledItemDelegate::paint(painter, option, index);
}

// =================================================================================================
// CommandBufferView
// =================================================================================================
CommandBufferView::CommandBufferView(const Dive::CommandHierarchy &command_hierarchy,
                                     QWidget                      *parent) :
    DiveTreeView(command_hierarchy, parent)
{
    setItemDelegate(new CommandBufferViewDelegate(this));
    setAccessibleName("DiveCommandBufferView");
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
