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
#include "dive_tree_view.h"

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QHeaderView>
#include <QPainter>
#include <QScrollBar>
#include <QTextDocument>
#include <algorithm>
#include <cstdint>
#ifndef NDEBUG
#    include <iostream>
#endif
#include "command_model.h"
#include "dive_core/command_hierarchy.h"
#include "dive_core/common.h"
#include "dive_core/common/common.h"
#include "dive_core/data_core.h"
#include "hover_help_model.h"

// =================================================================================================
// DiveTreeViewDelegate
// =================================================================================================
DiveTreeViewDelegate::DiveTreeViewDelegate(const DiveTreeView *dive_tree_view_ptr) :
    QStyledItemDelegate(0), m_dive_tree_view_ptr(dive_tree_view_ptr)
{
    m_hover_help_ptr = HoverHelp::Get();
}

//--------------------------------------------------------------------------------------------------
void DiveTreeViewDelegate::paint(QPainter                   *painter,
                                 const QStyleOptionViewItem &option,
                                 const QModelIndex          &index) const
{
    // Hover help messages
    if (option.state & QStyle::State_MouseOver)
    {
        const Dive::CommandHierarchy &command_hierarchy = m_dive_tree_view_ptr
                                                          ->GetCommandHierarchy();
        uint64_t node_index = (uint64_t)(index.internalPointer());
        m_hover_help_ptr->SetCommandHierarchyNodeItem(command_hierarchy, node_index);
    }

    // Write the command hierarchy description
    if (index.column() == 0)
    {
        QStyleOptionViewItem options = option;
        initStyleOption(&options, index);

        QStyle *style = options.widget ? options.widget->style() : QApplication::style();

        uint64_t node_index = (uint64_t)(index.internalPointer());
        options.text = QString(m_dive_tree_view_ptr->GetCommandHierarchy().GetNodeDesc(node_index));

        QTextDocument doc;
        int           first_pos = options.text.indexOf('(');
        int           last_pos = options.text.lastIndexOf(')');
        bool          is_parameterized = first_pos != -1 && last_pos != -1 &&
                                last_pos == options.text.length() - 1;

        // Call to the base class function is needed to handle hover effects correctly
        if (options.state & QStyle::State_MouseOver || options.state & QStyle::State_Selected ||
            !is_parameterized)
        {
            QStyledItemDelegate::paint(painter, options, index);
            return;
        }
        else
        {
            doc.setHtml(options.text.left(first_pos) + "<span style=\"color:#ccffff;\">" +
                        options.text.right(last_pos - first_pos + 1) + "<span>");

            /// Painting item without text
            options.text = QString();
            style->drawControl(QStyle::CE_ItemViewItem, &options, painter);

            QAbstractTextDocumentLayout::PaintContext ctx;

            // Highlighting text if item is selected
            if (options.state & QStyle::State_Selected)
            {
                ctx.palette.setColor(QPalette::Text,
                                     options.palette.color(QPalette::Active,
                                                           QPalette::HighlightedText));
            }

            QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &options);
            painter->save();
            painter->translate(textRect.topLeft());
            painter->setClipRect(textRect.translated(-textRect.topLeft()));
            doc.documentLayout()->draw(painter, ctx);
            painter->restore();
            return;
        }
    }

    QStyledItemDelegate::paint(painter, option, index);
}

//--------------------------------------------------------------------------------------------------
QSize DiveTreeViewDelegate::sizeHint(const QStyleOptionViewItem &option,
                                     const QModelIndex          &index) const
{
    QSize size_hint = QStyledItemDelegate::sizeHint(option, index);
    return QSize(size_hint.width(), size_hint.height() + kMargin);
}

// =================================================================================================
// DiveTreeView
// =================================================================================================
DiveTreeView::DiveTreeView(const Dive::CommandHierarchy &command_hierarchy, QWidget *parent) :
    QTreeView(parent), m_command_hierarchy(command_hierarchy), curr_node_selected(QModelIndex())
{
    setHorizontalScrollBar(new QScrollBar);
    horizontalScrollBar()->setEnabled(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setAlternatingRowColors(true);
    setAutoScroll(false);
    setItemDelegate(new DiveTreeViewDelegate(this));
    viewport()->setAttribute(Qt::WA_Hover);
    setAccessibleName("DiveCommandHierarchy");
}

//--------------------------------------------------------------------------------------------------
bool DiveTreeView::RenderBranch(const QModelIndex &index) const { return true; }

//--------------------------------------------------------------------------------------------------
void DiveTreeView::setCurrentNode(uint64_t node_index)
{
    auto        m = dynamic_cast<CommandModel *>(model());
    QModelIndex ix = m->findNode(node_index);
    ix = m->index(ix.row(), 1, ix.parent());
    curr_node_selected = ix;
    scrollTo(ix);
    setCurrentIndex(ix);
}

//--------------------------------------------------------------------------------------------------
void DiveTreeView::expandNode(const QModelIndex &index)
{
    uint64_t node_index = (uint64_t)(index.internalPointer());
    if (m_command_hierarchy.GetNodeType(node_index) == Dive::NodeType::kMarkerNode)
    {
        Dive::CommandHierarchy::MarkerType marker_type = m_command_hierarchy.GetMarkerNodeType(
        node_index);
        if (marker_type == Dive::CommandHierarchy::MarkerType::kBeginEnd)
        {
            emit labelExpanded(node_index);
        }
    }
}

//--------------------------------------------------------------------------------------------------
void DiveTreeView::collapseNode(const QModelIndex &index)
{
    uint64_t node_index = (uint64_t)(index.internalPointer());
    if (m_command_hierarchy.GetNodeType(node_index) == Dive::NodeType::kMarkerNode)
    {
        Dive::CommandHierarchy::MarkerType marker_type = m_command_hierarchy.GetMarkerNodeType(
        node_index);
        if (marker_type == Dive::CommandHierarchy::MarkerType::kBeginEnd)
        {
            emit labelCollapsed(node_index);
        }
    }
}

//--------------------------------------------------------------------------------------------------
void DiveTreeView::gotoPrevEvent() { gotoEvent(true); }

//--------------------------------------------------------------------------------------------------
void DiveTreeView::gotoNextEvent() { gotoEvent(false); }

//--------------------------------------------------------------------------------------------------
void DiveTreeView::gotoEvent(bool is_above)
{
    QModelIndex current_idx = currentIndex();
    if (!current_idx.isValid())
        return;

    auto m = dynamic_cast<CommandModel *>(model());
    auto next_node_idx = is_above ? indexAbove(current_idx) : indexBelow(current_idx);
    while (next_node_idx.isValid())
    {
        auto     event_id_idx = m->index(next_node_idx.row(), 1, next_node_idx.parent());
        auto     event_id = m->data(event_id_idx, Qt::DisplayRole);
        uint64_t node_idx = (uint64_t)(next_node_idx.internalPointer());
        auto     node_type = m_command_hierarchy.GetNodeType(node_idx);

        if (event_id != QVariant() && (node_type == Dive::NodeType::kDrawDispatchBlitNode ||
                                       (node_type == Dive::NodeType::kMarkerNode &&
                                        m_command_hierarchy.GetMarkerNodeType(node_idx) !=
                                        Dive::CommandHierarchy::MarkerType::kBeginEnd)))
            break;

        current_idx = next_node_idx;
        next_node_idx = is_above ? indexAbove(current_idx) : indexBelow(current_idx);
    }
    if (next_node_idx.isValid())
    {
        next_node_idx = m->index(next_node_idx.row(), 1, next_node_idx.parent());
        scrollTo(next_node_idx);
        setCurrentIndex(next_node_idx);
    }
}

//--------------------------------------------------------------------------------------------------
void DiveTreeView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    curr_node_selected = current;
    emit currentNodeChanged((uint64_t)current.internalPointer(),
                            (uint64_t)previous.internalPointer());
}

//--------------------------------------------------------------------------------------------------
const Dive::CommandHierarchy &DiveTreeView::GetCommandHierarchy() const
{
    return m_command_hierarchy;
}

//--------------------------------------------------------------------------------------------------
void DiveTreeView::RetainCurrentNode()
{
    if (!curr_node_selected.isValid())
        return;
    auto     m = dynamic_cast<CommandModel *>(model());
    uint64_t node_index = (uint64_t)(curr_node_selected.internalPointer());
    curr_node_selected = m->findNode(node_index);
    curr_node_selected = m->index(curr_node_selected.row(), 1, curr_node_selected.parent());
    if (isExpanded(curr_node_selected.parent()))
    {
        scrollTo(curr_node_selected, QAbstractItemView::PositionAtCenter);
        setCurrentIndex(curr_node_selected);
    }
    else
    {
        // Reset selection and scroll to the approximate area where the previous node was
        QModelIndex index = m->parent(curr_node_selected);
        setCurrentIndex(QModelIndex());
        while (index.isValid() && !isExpanded(m->parent(index)))
            index = m->parent(index);
        if (index.isValid())
        {
            index = m->index(index.row(), 1, index.parent());
            scrollTo(index, QAbstractItemView::PositionAtCenter);
        }
    }
}

//--------------------------------------------------------------------------------------------------
void DiveTreeView::ExpandToLevel(int level)
{
    DIVE_ASSERT(level > 0);
    if (level == 1)
    {
        /* Collapse to submits for level 1 */
        collapseAll();
    }
    else
    {
        level -= 2;
        expandToDepth(level);
    }
}

//--------------------------------------------------------------------------------------------------
void DiveTreeView::keyPressEvent(QKeyEvent *event)
{
    // Select the previous numbered draw call
    if (event->key() == Qt::Key_W)
    {
        gotoEvent(true);
    }
    // Select the next numbered draw call
    else if (event->key() == Qt::Key_S)
    {
        gotoEvent(false);
    }
    else
    {
        QTreeView::keyPressEvent(event);
    }
}

//--------------------------------------------------------------------------------------------------
void DiveTreeView::setAndScrollToNode(QModelIndex &idx)
{
    auto     m = dynamic_cast<CommandModel *>(model());
    uint64_t node_index = (uint64_t)(idx.internalPointer());
    idx = m->findNode(node_index);
    idx = m->index(idx.row(), 1, idx.parent());
    scrollTo(idx);
    setCurrentIndex(idx);
}

//--------------------------------------------------------------------------------------------------
void DiveTreeView::searchNodeByText(const QString &search_text)
{
    search_indexes.clear();
    search_index_it = search_indexes.begin();

    if (search_text.isEmpty())
        return;

    auto m = dynamic_cast<CommandModel *>(model());
    search_indexes = m->search(m->index(0, 0), QVariant::fromValue(search_text));
    search_index_it = search_indexes.begin();

    if (!search_indexes.isEmpty())
    {
        QModelIndex curr_idx = currentIndex();
        if (curr_idx.isValid() && curr_idx != *search_index_it)
        {
            search_index_it = search_indexes.begin() +
                              getNearestSearchNode((uint64_t)(curr_idx.internalPointer()));
        }
        setAndScrollToNode(*search_index_it);
    }
    emit updateSearch(search_index_it - search_indexes.begin(),
                      search_indexes.isEmpty() ? 0 : search_indexes.size());
}

//--------------------------------------------------------------------------------------------------
void DiveTreeView::nextNodeInSearch()
{
    if (!search_indexes.isEmpty() && search_index_it != search_indexes.end() &&
        (search_index_it + 1) != search_indexes.end())
    {
        QModelIndex curr_idx = currentIndex();
        if (curr_idx.isValid() && curr_idx != *search_index_it)
        {
            search_index_it = search_indexes.begin() +
                              getNearestSearchNode((uint64_t)(curr_idx.internalPointer()));
            if (*search_index_it == curr_idx)
                ++search_index_it;
        }
        else
        {
            ++search_index_it;
        }
        setAndScrollToNode(*search_index_it);
        emit updateSearch(search_index_it - search_indexes.begin(), search_indexes.size());
    }
}

//--------------------------------------------------------------------------------------------------
void DiveTreeView::prevNodeInSearch()
{
    if (!search_indexes.isEmpty() && search_index_it != search_indexes.begin())
    {
        QModelIndex curr_idx = currentIndex();
        if (curr_idx.isValid() && curr_idx != *search_index_it)
        {
            search_index_it = search_indexes.begin() +
                              getNearestSearchNode((uint64_t)(curr_idx.internalPointer()));
            if (*search_index_it == curr_idx)
                --search_index_it;
        }
        else
        {
            --search_index_it;
        }
        setAndScrollToNode(*search_index_it);
        emit updateSearch(search_index_it - search_indexes.begin(), search_indexes.size());
    }
}

//--------------------------------------------------------------------------------------------------
int DiveTreeView::getNearestSearchNode(uint64_t target_index)
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