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
#include "event_graphics_item.h"

#include <QPainter>

//--------------------------------------------------------------------------------------------------
EventGraphicsItem::EventGraphicsItem() { setAcceptHoverEvents(true); }

//--------------------------------------------------------------------------------------------------
void EventGraphicsItem::SetWidth(uint64_t width)
{
    if (m_width != width)
    {
        prepareGeometryChange();
        m_width = width;
    }
}

void EventGraphicsItem::SetHeight(uint64_t height) {}

//--------------------------------------------------------------------------------------------------
void EventGraphicsItem::SetVisibleRange(int64_t scene_x,
                                        int64_t scene_y,
                                        int64_t width,
                                        int64_t height)
{
    // Convert to local item coordinate
    int32_t item_x = mapFromScene(scene_x, 0).x();
    int32_t item_y = mapFromScene(0, scene_y).y();

    m_visible_start_x = item_x;
    m_visible_start_y = item_y;
    m_visible_width = width;
    m_visible_height = height;
}

//--------------------------------------------------------------------------------------------------
QRectF EventGraphicsItem::boundingRect() const { return QRectF(0, 0, m_width, m_height); }

//--------------------------------------------------------------------------------------------------
QPainterPath EventGraphicsItem::shape() const
{
    QPainterPath path;
    path.addRect(0, 0, m_width, m_height);
    return path;
}

//--------------------------------------------------------------------------------------------------
void EventGraphicsItem::paint(QPainter                       *painter,
                              const QStyleOptionGraphicsItem *option,
                              QWidget                        *widget)
{
    QFont font;
    font.setFamily(font.defaultFamily());
    font.setPixelSize(11);
    painter->setFont(font);

    DrawEvents(painter);
}

//--------------------------------------------------------------------------------------------------
void EventGraphicsItem::DrawEvents(QPainter *painter) {}

//--------------------------------------------------------------------------------------------------
void EventGraphicsItem::CalcRectCoord(uint64_t  start_cycle,
                                      uint64_t  end_cycle,
                                      uint64_t *start_x,
                                      uint64_t *end_x)
{
}

//--------------------------------------------------------------------------------------------------
QColor EventGraphicsItem::GetEventColor(uint32_t event) { return QColor(250, 218, 94); }

//--------------------------------------------------------------------------------------------------
void EventGraphicsItem::setColorByIndex(int index) { m_color_by_index = index; }