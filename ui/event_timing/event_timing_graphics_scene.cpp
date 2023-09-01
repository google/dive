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

#include "event_timing_graphics_scene.h"

#include <QPainter>

//--------------------------------------------------------------------------------------------------
EventTimingGraphicsScene::EventTimingGraphicsScene()
{
    m_mouse_pos.setX(0.0);
    m_mouse_pos.setY(0.0);
}

void EventTimingGraphicsScene::drawForeground(QPainter* painter, const QRectF& rect)
{
    if (!m_mouse_pos.isNull())
    {
        // Draw a dotted vertical line at the mouse cursor position
        painter->setPen(QPen(Qt::white, 0.5, Qt::DashDotLine));
        painter->drawLine(m_mouse_pos.x(), rect.top(), m_mouse_pos.x(), rect.bottom());
    }
    else
    {
        QGraphicsScene::drawForeground(painter, rect);
    }
}

void EventTimingGraphicsScene::onMouseCursorChanged(QPointF new_mouse_pos)
{
    m_mouse_pos = new_mouse_pos;
    invalidate();  // Redraw the scene.
}