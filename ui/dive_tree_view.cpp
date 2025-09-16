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
#include "dive_tree_view.h"
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QHeaderView>
#include <QPainter>
#include <QScrollBar>
#include <QTextDocument>
#include <algorithm>
#include <cstdint>
#include <qabstractitemmodel.h>
#ifndef NDEBUG
#    include <iostream>
#endif
#include "command_model.h"
#include "dive_core/command_hierarchy.h"
#include "dive_core/common.h"
#include "dive_core/common/common.h"
#include "dive_core/data_core.h"
#include "hover_help_model.h"
#include "gfxr_vulkan_command_model.h"
#include "gfxr_vulkan_command_filter_proxy_model.h"
#include "gfxr_vulkan_command_arguments_filter_proxy_model.h"

static constexpr uint64_t kInvalidNodeIndex = static_cast<uint64_t>(-1);

// =================================================================================================
// DiveFilterModel
// =================================================================================================
DiveFilterModel::DiveFilterModel(const Dive::CommandHierarchy &command_hierarchy, QObject *parent) :
    QSortFilterProxyModel(parent),
    m_command_hierarchy(command_hierarchy)
{
}

void DiveFilterModel::applyNewFilterMode(FilterMode new_mode)
{
    // Check if the mode is actually changing to avoid unnecessary resets
    if (m_filter_mode == new_mode)
        return;

    beginResetModel();
    m_filter_mode = new_mode;

    // Only collect the draw call indices if gfxr and pm4 data are present.
    if (!m_gfxr_draw_call_indices.empty())
    {
        // Clear the vector of pm4 draw call indices.
        m_pm4_draw_call_indices.clear();
        // Recollect pm4 draw call indices when new filter is applied.
        CollectPm4DrawCallIndices(QModelIndex());
    }
    // invalidateFilter() doesn't invalidate all nodes
    // begin/endResetModel() will cause a full re-evaluation and rebuild of the proxy's internal
    // mapping.
    endResetModel();
}

bool DiveFilterModel::IncludeIndex(uint64_t node_index) const
{
    Dive::CommandHierarchy::FilterListType
    filter_list_type = Dive::CommandHierarchy::kFilterListTypeCount;

    switch (m_filter_mode)
    {
    case kBinningPassOnly:
        filter_list_type = Dive::CommandHierarchy::kBinningPassOnly;
        break;
    case kFirstTilePassOnly:
        filter_list_type = Dive::CommandHierarchy::kFirstTilePassOnly;
        break;
    case kBinningAndFirstTilePass:
        filter_list_type = Dive::CommandHierarchy::kBinningAndFirstTilePass;
        break;
    default:
        return true;
    }

    const auto &filter_exclude_indices = m_command_hierarchy.GetFilterExcludeIndices(
    filter_list_type);

    // If the node index is in the exclude list, we exclude the index.
    if (filter_exclude_indices.find(node_index) != filter_exclude_indices.end())
    {
        return false;
    }

    return true;
}

void DiveFilterModel::SetMode(FilterMode filter_mode)
{
    applyNewFilterMode(filter_mode);
}

void DiveFilterModel::CollectPm4DrawCallIndices(const QModelIndex &parent_index)
{
    if (!parent_index.isValid())
    {
        m_pm4_draw_call_indices.clear();
    }
    else
    {
        // Check if the current parent node is filtered out.
        uint64_t parent_node_index = (uint64_t)parent_index.internalPointer();

        if (!IncludeIndex(parent_node_index))
        {
            return;
        }
    }

    int row_count = sourceModel()->rowCount(parent_index);
    for (int row = 0; row < row_count; ++row)
    {
        QModelIndex index = sourceModel()->index(row, 0, parent_index);
        if (index.isValid())
        {
            uint64_t       node_index = (uint64_t)index.internalPointer();
            Dive::NodeType node_type = m_command_hierarchy.GetNodeType(node_index);
            if (Dive::IsDrawDispatchNode(node_type))
            {
                m_pm4_draw_call_indices.push_back(node_index);
            }

            // Only recurse into children if the current node is not a Vulkan submit node.
            if (node_type != Dive::NodeType::kGfxrVulkanSubmitNode)
            {
                CollectPm4DrawCallIndices(index);
            }
        }
    }
}

void DiveFilterModel::CollectGfxrDrawCallIndices(const QModelIndex &parent_index)
{
    if (!parent_index.isValid())
    {
        m_gfxr_draw_call_indices.clear();
    }

    int row_count = sourceModel()->rowCount(parent_index);
    for (int row = 0; row < row_count; ++row)
    {
        QModelIndex index = sourceModel()->index(row, 0, parent_index);
        if (index.isValid())

        {
            uint64_t       node_index = (uint64_t)index.internalPointer();
            Dive::NodeType node_type = m_command_hierarchy.GetNodeType(node_index);

            // If a node is a gfxr draw call, add its index to the list.
            if (node_type == Dive::NodeType::kGfxrVulkanDrawCommandNode)
            {
                m_gfxr_draw_call_indices.push_back(node_index);
            }

            // Only recurse into children if the current node is gfxr submit node.
            if (node_type != Dive::NodeType::kSubmitNode)
            {
                CollectGfxrDrawCallIndices(index);
            }
        }
    }
}

void DiveFilterModel::ClearDrawCallIndices()
{
    m_pm4_draw_call_indices.clear();
    m_gfxr_draw_call_indices.clear();
}

bool DiveFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    uint64_t    node_index = (uint64_t)index.internalPointer();

    Dive::NodeType current_node_type = m_command_hierarchy.GetNodeType(node_index);

    if (current_node_type == Dive::NodeType::kGfxrVulkanSubmitNode)
    {
        return false;
    }

    if (!index.isValid())
    {
        return false;
    }

    if (m_filter_mode == kNone)
    {
        return true;
    }

    return IncludeIndex(node_index);
}

// =================================================================================================
// DiveTreeViewDelegate
// =================================================================================================
DiveTreeViewDelegate::DiveTreeViewDelegate(const DiveTreeView *dive_tree_view_ptr) :
    QStyledItemDelegate(0),
    m_dive_tree_view_ptr(dive_tree_view_ptr)
{
    m_hover_help_ptr = HoverHelp::Get();
}

//--------------------------------------------------------------------------------------------------
void DiveTreeViewDelegate::paint(QPainter                   *painter,
                                 const QStyleOptionViewItem &option,
                                 const QModelIndex          &index) const
{
    uint64_t source_node_index = m_dive_tree_view_ptr->GetNodeSourceIndex(index);

    // Hover help messages
    if (option.state & QStyle::State_MouseOver)
    {
        const Dive::CommandHierarchy &command_hierarchy = m_dive_tree_view_ptr
                                                          ->GetCommandHierarchy();
        m_hover_help_ptr->SetCommandHierarchyNodeItem(command_hierarchy, source_node_index);
    }

    // Write the command hierarchy description
    if (index.column() == 0)
    {
        QStyleOptionViewItem options = option;
        initStyleOption(&options, index);

        QStyle *style = options.widget ? options.widget->style() : QApplication::style();

        options.text = QString(
        m_dive_tree_view_ptr->GetCommandHierarchy().GetNodeDesc(source_node_index));

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
    QTreeView(parent),
    m_command_hierarchy(command_hierarchy),
    m_curr_node_selected(QModelIndex())
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
bool DiveTreeView::RenderBranch(const QModelIndex &index) const
{
    return true;
}

//--------------------------------------------------------------------------------------------------
void DiveTreeView::setCurrentNode(uint64_t node_index)
{
    QAbstractItemModel *model_ptr = GetCommandModel();
    CommandModel       *command_model = qobject_cast<CommandModel *>(model_ptr);
    QModelIndex         source_node_model_idx;
    if (command_model)
    {
        source_node_model_idx = command_model->findNode(node_index);
    }
    else
    {
        GfxrVulkanCommandModel *gfxr_vulkan_command_model = qobject_cast<GfxrVulkanCommandModel *>(
        model_ptr);

        source_node_model_idx = gfxr_vulkan_command_model->findNode(node_index);
    }

    QModelIndex proxy_model_idx = GetProxyModelIndexFromSource(source_node_model_idx);

    proxy_model_idx = model()->index(proxy_model_idx.row(), 1, proxy_model_idx.parent());

    m_curr_node_selected = proxy_model_idx;
    scrollTo(proxy_model_idx);
    setCurrentIndex(proxy_model_idx);
}

//--------------------------------------------------------------------------------------------------
uint64_t DiveTreeView::GetNodeSourceIndex(const QModelIndex &proxy_model_index) const
{
    QModelIndex source_model_index = GetNodeSourceModelIndex(proxy_model_index);

    if (!source_model_index.isValid())
    {
        return kInvalidNodeIndex;
    }

    return (uint64_t)(source_model_index.internalPointer());
}

//--------------------------------------------------------------------------------------------------
QAbstractItemModel *DiveTreeView::GetCommandModel()
{
    DiveFilterModel *filter_model = qobject_cast<DiveFilterModel *>(model());

    GfxrVulkanCommandFilterProxyModel
    *vulkan_command_proxy_model = qobject_cast<GfxrVulkanCommandFilterProxyModel *>(model());

    GfxrVulkanCommandArgumentsFilterProxyModel
    *vulkan_command_arg_proxy_model = qobject_cast<GfxrVulkanCommandArgumentsFilterProxyModel *>(
    model());

    if (filter_model)
    {
        CommandModel *command_model = qobject_cast<CommandModel *>(filter_model->sourceModel());

        DIVE_ASSERT(command_model);

        return command_model;
    }
    else if (vulkan_command_proxy_model)
    {
        GfxrVulkanCommandModel *gfxr_vulkan_command_model = qobject_cast<GfxrVulkanCommandModel *>(
        vulkan_command_proxy_model->sourceModel());

        DIVE_ASSERT(gfxr_vulkan_command_model);

        return gfxr_vulkan_command_model;
    }
    else if (vulkan_command_arg_proxy_model)
    {
        GfxrVulkanCommandModel *gfxr_vulkan_command_model = qobject_cast<GfxrVulkanCommandModel *>(
        vulkan_command_arg_proxy_model->sourceModel());

        DIVE_ASSERT(gfxr_vulkan_command_model);

        return gfxr_vulkan_command_model;
    }
    else
    {
        CommandModel *command_model = qobject_cast<CommandModel *>(model());

        DIVE_ASSERT(command_model);

        return command_model;
    }
}

//--------------------------------------------------------------------------------------------------
QModelIndex DiveTreeView::GetNodeSourceModelIndex(const QModelIndex &proxy_model_index) const
{
    const DiveFilterModel *filter_model = qobject_cast<const DiveFilterModel *>(model());

    const GfxrVulkanCommandFilterProxyModel
    *vulkan_command_proxy_model = qobject_cast<const GfxrVulkanCommandFilterProxyModel *>(model());

    const GfxrVulkanCommandArgumentsFilterProxyModel *vulkan_command_arg_proxy_model = qobject_cast<
    const GfxrVulkanCommandArgumentsFilterProxyModel *>(model());

    if (filter_model)
    {
        return filter_model->mapToSource(proxy_model_index);
    }
    else if (vulkan_command_proxy_model)
    {
        return vulkan_command_proxy_model->mapToSource(proxy_model_index);
    }
    else if (vulkan_command_arg_proxy_model)
    {
        return vulkan_command_arg_proxy_model->mapToSource(proxy_model_index);
    }
    else
    {
        return proxy_model_index;
    }
}

//--------------------------------------------------------------------------------------------------
QModelIndex DiveTreeView::GetProxyModelIndexFromSource(const QModelIndex &source_model_index) const
{
    const DiveFilterModel *filter_model = qobject_cast<const DiveFilterModel *>(model());

    const GfxrVulkanCommandFilterProxyModel
    *vulkan_command_proxy_model = qobject_cast<const GfxrVulkanCommandFilterProxyModel *>(model());

    const GfxrVulkanCommandArgumentsFilterProxyModel *vulkan_command_arg_proxy_model = qobject_cast<
    const GfxrVulkanCommandArgumentsFilterProxyModel *>(model());

    if (filter_model)
    {
        return filter_model->mapFromSource(source_model_index);
    }
    else if (vulkan_command_proxy_model)
    {
        return vulkan_command_proxy_model->mapFromSource(source_model_index);
    }
    else if (vulkan_command_arg_proxy_model)
    {
        return vulkan_command_arg_proxy_model->mapFromSource(source_model_index);
    }
    else
    {
        return source_model_index;
    }
}

//--------------------------------------------------------------------------------------------------
void DiveTreeView::expandNode(const QModelIndex &index)
{
    uint64_t node_index = GetNodeSourceIndex(index);
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
    uint64_t node_index = GetNodeSourceIndex(index);
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
void DiveTreeView::gotoPrevEvent()
{
    GotoEvent(true);
}

//--------------------------------------------------------------------------------------------------
void DiveTreeView::gotoNextEvent()
{
    GotoEvent(false);
}

//--------------------------------------------------------------------------------------------------
void DiveTreeView::GotoEvent(bool is_above)
{
    QModelIndex current_proxy_idx = currentIndex();
    if (!current_proxy_idx.isValid())
        return;

    const DiveFilterModel *filter_model = qobject_cast<const DiveFilterModel *>(model());

    const GfxrVulkanCommandFilterProxyModel
    *vulkan_command_proxy_model = qobject_cast<const GfxrVulkanCommandFilterProxyModel *>(model());

    const GfxrVulkanCommandArgumentsFilterProxyModel *vulkan_command_arg_proxy_model = qobject_cast<
    const GfxrVulkanCommandArgumentsFilterProxyModel *>(model());

    if (!filter_model || !vulkan_command_proxy_model || !vulkan_command_arg_proxy_model)
        return;

    // This gets the source model
    QAbstractItemModel     *model_ptr = GetCommandModel();
    CommandModel           *command_model = qobject_cast<CommandModel *>(model_ptr);
    GfxrVulkanCommandModel *gfxr_vulkan_command_model = qobject_cast<GfxrVulkanCommandModel *>(
    model_ptr);

    QModelIndex next_proxy_idx = current_proxy_idx;

    while (next_proxy_idx.isValid())
    {
        QModelIndex source_node_idx;
        if (filter_model)
        {
            source_node_idx = filter_model->mapToSource(next_proxy_idx);
        }
        else if (vulkan_command_proxy_model)
        {
            source_node_idx = vulkan_command_proxy_model->mapToSource(next_proxy_idx);
        }
        else
        {
            source_node_idx = vulkan_command_arg_proxy_model->mapToSource(next_proxy_idx);
        }

        if (!source_node_idx.isValid())
        {
            break;
        }

        uint64_t node_idx = (uint64_t)(source_node_idx.internalPointer());
        auto     node_type = m_command_hierarchy.GetNodeType(node_idx);

        auto event_id_idx = command_model ?
                            command_model->index(source_node_idx.row(),
                                                 1,
                                                 source_node_idx.parent()) :
                            gfxr_vulkan_command_model->index(source_node_idx.row(),
                                                             1,
                                                             source_node_idx.parent());

        auto event_id = command_model ?
                        command_model->data(event_id_idx, Qt::DisplayRole) :
                        gfxr_vulkan_command_model->data(event_id_idx, Qt::DisplayRole);

        if (event_id != QVariant() && (Dive::IsDrawDispatchBlitNode(node_type) ||
                                       (node_type == Dive::NodeType::kMarkerNode &&
                                        m_command_hierarchy.GetMarkerNodeType(node_idx) !=
                                        Dive::CommandHierarchy::MarkerType::kBeginEnd)))
        {
            if (filter_model)
            {
                next_proxy_idx = filter_model->mapFromSource(source_node_idx);
                next_proxy_idx = filter_model->index(next_proxy_idx.row(),
                                                     1,
                                                     next_proxy_idx.parent());
            }
            else if (vulkan_command_proxy_model)
            {
                next_proxy_idx = vulkan_command_proxy_model->mapFromSource(source_node_idx);
                next_proxy_idx = vulkan_command_proxy_model->index(next_proxy_idx.row(),
                                                                   1,
                                                                   next_proxy_idx.parent());
            }
            else
            {
                next_proxy_idx = vulkan_command_arg_proxy_model->mapFromSource(source_node_idx);
                next_proxy_idx = vulkan_command_arg_proxy_model->index(next_proxy_idx.row(),
                                                                       1,
                                                                       next_proxy_idx.parent());
            }

            scrollTo(next_proxy_idx);
            setCurrentIndex(next_proxy_idx);
            return;
        }

        if (is_above)
            next_proxy_idx = indexAbove(next_proxy_idx);
        else
            next_proxy_idx = indexBelow(next_proxy_idx);
    }
}

//--------------------------------------------------------------------------------------------------
void DiveTreeView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    m_curr_node_selected = current;
    QModelIndex current_source_index = GetNodeSourceModelIndex(current);
    QModelIndex previous_source_index = GetNodeSourceModelIndex(previous);
    emit        sourceCurrentChanged(current_source_index, previous_source_index);
}

//--------------------------------------------------------------------------------------------------
const Dive::CommandHierarchy &DiveTreeView::GetCommandHierarchy() const
{
    return m_command_hierarchy;
}

//--------------------------------------------------------------------------------------------------
void DiveTreeView::RetainCurrentNode()
{
    if (!m_curr_node_selected.isValid())
        return;

    QAbstractItemModel *model_ptr = GetCommandModel();

    CommandModel           *command_model = qobject_cast<CommandModel *>(model_ptr);
    GfxrVulkanCommandModel *gfxr_vulkan_command_model = qobject_cast<GfxrVulkanCommandModel *>(
    model_ptr);
    QModelIndex source_node_model_idx;
    uint64_t    source_node_idx = GetNodeSourceIndex(m_curr_node_selected);

    if (command_model)
    {
        source_node_model_idx = command_model->findNode(source_node_idx);
    }
    else
    {
        DIVE_ASSERT(gfxr_vulkan_command_model);

        source_node_model_idx = gfxr_vulkan_command_model->findNode(source_node_idx);
    }

    QModelIndex proxy_model_idx = GetProxyModelIndexFromSource(source_node_model_idx);
    proxy_model_idx = model()->index(proxy_model_idx.row(), 1, proxy_model_idx.parent());

    if (isExpanded(proxy_model_idx.parent()))
    {
        scrollTo(proxy_model_idx, QAbstractItemView::PositionAtCenter);
        setCurrentIndex(proxy_model_idx);
    }
    else
    {
        // Reset selection and scroll to the approximate area where the previous node was
        QModelIndex index = command_model ?
                            command_model->parent(source_node_model_idx) :
                            gfxr_vulkan_command_model->parent(source_node_model_idx);

        setCurrentIndex(QModelIndex());

        QModelIndex proxy_parent_model_idx;
        while (index.isValid())
        {
            proxy_parent_model_idx = GetProxyModelIndexFromSource(index);
            if (isExpanded(proxy_parent_model_idx))
                break;
            index = command_model ? command_model->parent(index) :
                                    gfxr_vulkan_command_model->parent(index);
        }

        if (index.isValid())
        {
            proxy_parent_model_idx = model()->index(proxy_parent_model_idx.row(),
                                                    1,
                                                    proxy_parent_model_idx.parent());
            scrollTo(proxy_parent_model_idx, QAbstractItemView::PositionAtCenter);
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
        GotoEvent(true);
    }
    // Select the next numbered draw call
    else if (event->key() == Qt::Key_S)
    {
        GotoEvent(false);
    }
    else
    {
        QTreeView::keyPressEvent(event);
    }
}

//--------------------------------------------------------------------------------------------------
void DiveTreeView::SetAndScrollToNode(QModelIndex &proxy_model_idx)
{
    QAbstractItemModel     *model_ptr = GetCommandModel();
    GfxrVulkanCommandModel *gfxr_vulkan_command_model = qobject_cast<GfxrVulkanCommandModel *>(
    model_ptr);

    if (!gfxr_vulkan_command_model)
    {
        proxy_model_idx = model()->index(proxy_model_idx.row(), 1, proxy_model_idx.parent());
    }

    scrollTo(proxy_model_idx);
    setCurrentIndex(proxy_model_idx);
}

//--------------------------------------------------------------------------------------------------
void DiveTreeView::searchNodeByText(const QString &search_text)
{
    m_search_indexes.clear();
    m_search_index_it = m_search_indexes.begin();

    if (search_text.isEmpty())
        return;

    // Get the currently active model (which is DiveFilterModel)
    const DiveFilterModel *filter_model = qobject_cast<const DiveFilterModel *>(model());
    if (!filter_model)
    {
        // Fallback or error handling if somehow the model isn't a DiveFilterModel
        QAbstractItemModel *model_ptr = GetCommandModel();

        CommandModel *command_model = qobject_cast<CommandModel *>(model_ptr);

        GfxrVulkanCommandModel *gfxr_vulkan_command_model = qobject_cast<GfxrVulkanCommandModel *>(
        model_ptr);

        const GfxrVulkanCommandFilterProxyModel
        *vulkan_command_proxy_model = dynamic_cast<const GfxrVulkanCommandFilterProxyModel *>(
        model());
        if (command_model)
        {
            // This search is on the source model and returns source indexes
            m_search_indexes = command_model->search(command_model->index(0, 0),
                                                     QVariant::fromValue(search_text));
        }
        else if (gfxr_vulkan_command_model)
        {
            if (vulkan_command_proxy_model)
            {
                m_search_indexes = vulkan_command_proxy_model
                                   ->match(vulkan_command_proxy_model->index(0, 0),
                                           Qt::DisplayRole,
                                           QVariant::fromValue(search_text),
                                           -1,
                                           Qt::MatchContains | Qt::MatchRecursive | Qt::MatchWrap);
            }
            else
            {
                const GfxrVulkanCommandArgumentsFilterProxyModel
                *vulkan_command_arg_proxy_model = dynamic_cast<
                const GfxrVulkanCommandArgumentsFilterProxyModel *>(model());

                m_search_indexes = vulkan_command_arg_proxy_model
                                   ->match(vulkan_command_arg_proxy_model->index(0, 0),
                                           Qt::DisplayRole,
                                           QVariant::fromValue(search_text),
                                           -1,
                                           Qt::MatchContains | Qt::MatchRecursive | Qt::MatchWrap);
            }
        }
    }
    else
    {
        // Perform the search on the filtered model
        // QSortFilterProxyModel::match returns QModelIndexes from the proxy model
        m_search_indexes = filter_model
                           ->match(filter_model
                                   ->index(0, 0),  // Start from the root of the filtered model
                                   Qt::DisplayRole,
                                   QVariant::fromValue(search_text),
                                   -1,  // Search all occurrences
                                   Qt::MatchContains |
                                   Qt::MatchRecursive);  // Case-insensitive, recursive search
    }

    m_search_index_it = m_search_indexes.begin();

    if (!m_search_indexes.isEmpty())
    {
        // This is a proxy index
        QModelIndex curr_idx = currentIndex();
        if (curr_idx.isValid() && curr_idx != *m_search_index_it)
        {
            // Pass the source node_index of the current selection to GetNearestSearchNode
            m_search_index_it = m_search_indexes.begin() +
                                GetNearestSearchNode(GetNodeSourceIndex(curr_idx));
        }
        QModelIndex proxy_model_idx = *m_search_index_it;
        SetAndScrollToNode(proxy_model_idx);
    }
    emit updateSearch(m_search_index_it - m_search_indexes.begin(),
                      m_search_indexes.isEmpty() ? 0 : m_search_indexes.size());
}

//--------------------------------------------------------------------------------------------------
void DiveTreeView::nextNodeInSearch()
{
    if (m_search_indexes.isEmpty())
        return;

    QModelIndex curr_proxy_idx = currentIndex();

    if (!curr_proxy_idx.isValid())
        return;

    // Get the source node index of the currently selected item
    uint64_t source_node_idx = GetNodeSourceIndex(curr_proxy_idx);

    // If the current iterator is not pointing to the currently selected item,
    // or if the currently selected item is not valid/is not a search result,
    // re-synchronize the iterator to the nearest search result.
    if (!curr_proxy_idx.isValid() || GetNodeSourceIndex(*m_search_index_it) != source_node_idx)
    {
        int nearest_idx = GetNearestSearchNode(source_node_idx);
        m_search_index_it = m_search_indexes.begin() + nearest_idx;

        // If the nearest node found is the same as the current node, advance it once
        // to move to the 'next' one in the sequence.
        if (GetNodeSourceIndex(*m_search_index_it) == source_node_idx)
        {
            if ((m_search_index_it + 1) != m_search_indexes.end())
            {
                ++m_search_index_it;
            }
            else
            {
                // We are at the last item, loop to the beginning or stay
                m_search_index_it = m_search_indexes.begin();  // Loop around
            }
        }
    }
    else  // The iterator already points to the currently selected item
    {
        // Move to the next item in the search list.
        if ((m_search_index_it + 1) != m_search_indexes.end())
        {
            ++m_search_index_it;
        }
        else
        {
            // Reached the end, loop back to the beginning.
            m_search_index_it = m_search_indexes.begin();
        }
    }

    // This is a proxy index
    QModelIndex idx_to_select = *m_search_index_it;
    SetAndScrollToNode(idx_to_select);
    emit updateSearch(m_search_index_it - m_search_indexes.begin(), m_search_indexes.size());
}

//--------------------------------------------------------------------------------------------------
void DiveTreeView::prevNodeInSearch()
{
    if (m_search_indexes.isEmpty())
        return;

    QModelIndex curr_proxy_idx = currentIndex();

    if (!curr_proxy_idx.isValid())
        return;

    // Get the source node index of the currently selected item
    uint64_t source_node_idx = GetNodeSourceIndex(curr_proxy_idx);

    if (!curr_proxy_idx.isValid() || GetNodeSourceIndex(*m_search_index_it) != source_node_idx)
    {
        int nearest_idx = GetNearestSearchNode(source_node_idx);
        m_search_index_it = m_search_indexes.begin() + nearest_idx;

        // If the nearest node found is the same as the current node, decrement it once
        // to move to the 'previous' one in the sequence.
        if (GetNodeSourceIndex(*m_search_index_it) == source_node_idx)
        {
            if (m_search_index_it != m_search_indexes.begin())
            {
                --m_search_index_it;
            }
            else
            {
                // We are at the first item, loop to the end
                m_search_index_it = m_search_indexes.end() - 1;  // Loop around
            }
        }
    }
    else  // The iterator already points to the currently selected item
    {
        // Move to the previous item in the search list.
        if (m_search_index_it != m_search_indexes.begin())
        {
            --m_search_index_it;
        }
        else
        {
            // Reached the beginning, loop back to the end.
            m_search_index_it = m_search_indexes.end() - 1;
        }
    }

    // This is a proxy index
    QModelIndex idx_to_select = *m_search_index_it;
    SetAndScrollToNode(idx_to_select);
    emit updateSearch(m_search_index_it - m_search_indexes.begin(), m_search_indexes.size());
}

//--------------------------------------------------------------------------------------------------
int DiveTreeView::GetNearestSearchNode(uint64_t source_node_idx)
{
    auto get_node_hierarchy_index = [this](int index) {
        // m_search_indexes[index] contains proxy indices.
        // We need to map it to the source model to get the original node_index
        // that is comparable to source_node_idx.
        const uint64_t source_index = GetNodeSourceIndex(m_search_indexes[index]);
        return source_index;
    };

    auto get_nearest = [&get_node_hierarchy_index](int x, int y, uint64_t target) {
        if (target - get_node_hierarchy_index(x) >= get_node_hierarchy_index(y) - target)
            return y;
        else
            return x;
    };

    int n = m_search_indexes.size();
    int left = 0, right = n, mid = 0;

    // Handle empty search results
    if (n == 0)
        return 0;

    if (source_node_idx <= get_node_hierarchy_index(left))
        return left;

    if (source_node_idx >= get_node_hierarchy_index(right - 1))
        return right - 1;

    while (left < right)
    {
        mid = (left + right) / 2;

        if (source_node_idx == get_node_hierarchy_index(mid))
            return mid;
        if (source_node_idx < get_node_hierarchy_index(mid))
        {
            if (mid > 0 && source_node_idx > get_node_hierarchy_index(mid - 1))
                return get_nearest(mid - 1, mid, source_node_idx);
            right = mid;
        }
        else
        {
            if (mid < n - 1 && source_node_idx < get_node_hierarchy_index(mid + 1))
                return get_nearest(mid, mid + 1, source_node_idx);
            left = mid + 1;
        }
    }
    return mid;
}
