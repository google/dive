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
#include "event_timing_graphics_view.h"
#include <QApplication>
#include <QEasingCurve>
#include <QMouseEvent>
#include <QScroller>
#include <QWheelEvent>

//--------------------------------------------------------------------------------------------------
EventTimingGraphicsView::EventTimingGraphicsView()
{
    m_scroller_ptr = QScroller::scroller(this);
    QScrollerProperties scroll_property = m_scroller_ptr->scrollerProperties();
    scroll_property.setScrollMetric(QScrollerProperties::ScrollingCurve,
                                    QEasingCurve(QEasingCurve::OutExpo));
    scroll_property.setScrollMetric(QScrollerProperties::DecelerationFactor, 1.0);
    m_scroller_ptr->setScrollerProperties(scroll_property);
}

//--------------------------------------------------------------------------------------------------
void EventTimingGraphicsView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control)
    {
        QApplication::setOverrideCursor(Qt::DragMoveCursor);
        m_scroller_ptr->grabGesture(this, QScroller::LeftMouseButtonGesture);
    }
}

//--------------------------------------------------------------------------------------------------
void EventTimingGraphicsView::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Control)
    {
        QApplication::restoreOverrideCursor();
        m_scroller_ptr->ungrabGesture(this);
    }
}

//--------------------------------------------------------------------------------------------------
void EventTimingGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    // QPointF scene_mouse_pos = this->mapToScene(event->pos());
    emit OnMouseCursor(this->mapToScene(event->pos()));
    QGraphicsView::mouseMoveEvent(event);
}

//--------------------------------------------------------------------------------------------------
void EventTimingGraphicsView::wheelEvent(QWheelEvent *event)
{
    // It's possible to get a x-axis-only wheel event on trackpads!
    // Only deal with y-axis ones (ie. mouse wheels)
    if (event->angleDelta().y() == 0)
        return;
    emit OnMouseWheel(event->pos(), event->angleDelta().y());
}