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

#include <QStyledItemDelegate>
#include "dive_tree_view.h"
#pragma once

// Forward Declarations
class CommandBufferView;
namespace Dive
{
class CommandHierarchy;
};

//--------------------------------------------------------------------------------------------------
class CommandBufferViewDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    CommandBufferViewDelegate(const CommandBufferView *command_buffer_view_ptr);

    void paint(QPainter                   *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex          &index) const override;

private:
    const CommandBufferView *m_command_buffer_view_ptr;
};

//--------------------------------------------------------------------------------------------------
class CommandBufferView : public DiveTreeView
{
    Q_OBJECT

public:
    CommandBufferView(const Dive::CommandHierarchy &command_hierarchy, QWidget *parent = nullptr);
    void Reset();

public slots:
    // Search CommandBufferView by input text
    void searchCommandBufferByText(const QString &search_text);
    // Navigate to the next item in search
    void nextCommandInSearch();
    // Navigate to the previous item in search
    void prevCommandInSearch();

signals:
    void updateSearch(uint64_t curr_item_pos, uint64_t total_search_results);

private:
    void setAndScrollToIndex(QModelIndex &idx);
    int  getNearestSearchCommand(uint64_t target_index);

    QList<QModelIndex>           search_indexes;
    QList<QModelIndex>::Iterator search_index_it;
};