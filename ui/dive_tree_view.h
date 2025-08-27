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

#pragma once

#include <QList>
#include <QListIterator>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QTreeView>
#include <qabstractitemmodel.h>
#include <qsortfilterproxymodel.h>

// Forward declarations
class CommandModel;
class DiveTreeView;
class HoverHelp;
class QWidget;
class GfxrVulkanCommandFilterProxyModel;
class GfxrVulkanCommandArgumentsFilterProxyModel;

namespace Dive
{
class CommandHierarchy;
class DataCore;
};  // namespace Dive

class DiveFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    enum FilterMode : uint32_t
    {
        kNone,
        kBinningPassOnly,
        kFirstTilePassOnly,
        kBinningAndFirstTilePass,
        kFilterModeCount
    };

    DiveFilterModel(const Dive::CommandHierarchy &command_hierarchy, QObject *parent = nullptr);
    void SetMode(FilterMode filter_mode);
    void CollectGfxrDrawCallIndices(const QModelIndex &parent_index = QModelIndex());
    void AddPm4DrawCallIndex(uint64_t index) const;
    const std::vector<uint64_t> &GetPm4DrawCallIndices() { return m_pm4_draw_call_indices; }
    const std::vector<uint64_t> &GetGfxrDrawCallIndices() { return m_gfxr_draw_call_indices; }
public slots:
    void applyNewFilterMode(FilterMode new_mode);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    const Dive::CommandHierarchy &m_command_hierarchy;
    FilterMode                    m_filter_mode = kNone;
    mutable std::vector<uint64_t> m_pm4_draw_call_indices;
    std::vector<uint64_t>         m_gfxr_draw_call_indices;
};

//--------------------------------------------------------------------------------------------------
class DiveTreeViewDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    DiveTreeViewDelegate(const DiveTreeView *dive_tree_view_ptr);

    void paint(QPainter                   *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex          &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    const int           kMargin = 5;
    const DiveTreeView *m_dive_tree_view_ptr;
    HoverHelp          *m_hover_help_ptr;
};

//--------------------------------------------------------------------------------------------------
class DiveTreeView : public QTreeView
{
    Q_OBJECT

public:
    DiveTreeView(const Dive::CommandHierarchy &command_hierarchy, QWidget *parent = nullptr);

    virtual bool RenderBranch(const QModelIndex &index) const;

    const Dive::CommandHierarchy &GetCommandHierarchy() const;

    void RetainCurrentNode();

    void ExpandToLevel(int level);

    void SetDataCore(Dive::DataCore *data_core) { m_data_core = data_core; }

    uint64_t GetNodeSourceIndex(const QModelIndex &proxy_model_index) const;

public slots:
    void setCurrentNode(uint64_t node_index);
    void expandNode(const QModelIndex &index);
    void collapseNode(const QModelIndex &index);
    void gotoPrevEvent();
    void gotoNextEvent();

    // Search DiveTreeView by input text
    void searchNodeByText(const QString &search_text);
    // Navigate to the next node in search
    void nextNodeInSearch();
    // Navigate to the previous node in search
    void prevNodeInSearch();

protected:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
    const Dive::CommandHierarchy &m_command_hierarchy;

signals:
    void labelExpanded(uint64_t node_index);
    void labelCollapsed(uint64_t node_index);
    void updateSearch(uint64_t curr_item_pos, uint64_t total_search_results);
    void sourceCurrentChanged(const QModelIndex &currentSourceIndex,
                              const QModelIndex &previousSourceIndex);

private:
    void GotoEvent(bool is_above);
    void SetAndScrollToNode(QModelIndex &proxy_model_idx);
    int  GetNearestSearchNode(uint64_t source_node_idx);

    QAbstractItemModel *GetCommandModel();
    QModelIndex         GetNodeSourceModelIndex(const QModelIndex &proxy_model_index) const;

    QModelIndex                  m_curr_node_selected;
    QList<QModelIndex>           m_search_indexes;
    QList<QModelIndex>::Iterator m_search_index_it;
    Dive::DataCore              *m_data_core = nullptr;
};