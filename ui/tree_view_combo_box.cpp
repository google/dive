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

#include "tree_view_combo_box.h"

#include <QHeaderView>
#include <QTreeView>

//--------------------------------------------------------------------------------------------------
TreeViewComboBox::TreeViewComboBox(QWidget *parent) : QComboBox(parent), m_tree_view(nullptr)
{
    m_tree_view = new QTreeView(this);
    m_tree_view->setFrameShape(QFrame::NoFrame);
    m_tree_view->setEditTriggers(QTreeView::NoEditTriggers);
    m_tree_view->setSelectionBehavior(QTreeView::SelectRows);
    m_tree_view->setRootIsDecorated(true);
    m_tree_view->setWordWrap(true);
    m_tree_view->setAllColumnsShowFocus(true);
    m_tree_view->header()->setVisible(false);
    setView(m_tree_view);
}

//--------------------------------------------------------------------------------------------------
void TreeViewComboBox::showPopup()
{
    setRootModelIndex(QModelIndex());
    m_tree_view->expandAll();
    m_tree_view->setItemsExpandable(false);
    m_tree_view->setMinimumWidth(this->view()->sizeHintForColumn(0));
    QComboBox::showPopup();
}

//--------------------------------------------------------------------------------------------------
void TreeViewComboBox::selectIndex(const QModelIndex &index)
{
    setRootModelIndex(index.parent());
    setCurrentIndex(index.row());
}