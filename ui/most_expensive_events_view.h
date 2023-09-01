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

// Forward declaration
class QTextEdit;
class QTreeWidget;
class QTreeWidgetItem;
namespace Dive
{
class DataCore;
class LogRecord;
class MarkerData;
struct CaptureMetadata;
}  // namespace Dive

//--------------------------------------------------------------------------------------------------
class MostExpensiveEventsViewDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    void  paint(QPainter                   *painter,
                const QStyleOptionViewItem &option,
                const QModelIndex          &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

//--------------------------------------------------------------------------------------------------
class MostExpensiveEventsView : public QFrame
{
    Q_OBJECT

public:
    MostExpensiveEventsView(const Dive::CaptureMetadata &capture_metadata);
    void Update();

private slots:
    void OnCustomContextMenuRequested(QPoint pos);

protected:
    virtual void leaveEvent(QEvent *event) override;

private:
    QString GetDurationString(uint64_t cycle) const;

    QTreeWidget                 *m_event_list;
    const Dive::CaptureMetadata &m_capture_metadata;
};