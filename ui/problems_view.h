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

#include <QFrame>
#pragma once

#include <QStyledItemDelegate>

#include "dive_core/cross_ref.h"

// Forward declaration
class QTextEdit;
class QTreeWidget;
class QTreeWidgetItem;
namespace Dive
{
class DataCore;
class LogRecord;
class MarkerData;
class CommandHierarchy;
}  // namespace Dive

//--------------------------------------------------------------------------------------------------
class ProblemsViewDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

//--------------------------------------------------------------------------------------------------
class ProblemsView : public QFrame
{
    Q_OBJECT

public:
    ProblemsView(const Dive::CommandHierarchy &command_hierarchy);
    void Update(const Dive::LogRecord *log_ptr);

signals:
    void crossReferece(Dive::CrossRef);

private slots:
    void OnProblemSelectionChanged();
    void OnProblemItemHover(QTreeWidgetItem *item, int column);

protected:
    virtual void leaveEvent(QEvent *event);

private:
    QTreeWidget                  *m_log_list;
    const Dive::CommandHierarchy &m_command_hierarchy;
};