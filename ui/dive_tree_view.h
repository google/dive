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

#pragma once

#include <QList>
#include <QListIterator>
#include <QStyledItemDelegate>
#include <QTreeView>

// Forward declarations
class QWidget;
class DiveTreeView;
class HoverHelp;
namespace Dive
{
class CommandHierarchy;
class DataCore;
};  // namespace Dive

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
    void currentNodeChanged(uint64_t node_index, uint64_t prev_node_index);
    void labelExpanded(uint64_t node_index);
    void labelCollapsed(uint64_t node_index);
    void updateSearch(uint64_t curr_item_pos, uint64_t total_search_results);

private:
    void gotoEvent(bool is_above);
    void setAndScrollToNode(QModelIndex &idx);
    int  getNearestSearchNode(uint64_t target_index);

    QModelIndex                  curr_node_selected;
    QList<QModelIndex>           search_indexes;
    QList<QModelIndex>::Iterator search_index_it;
    Dive::DataCore              *m_data_core = nullptr;
};