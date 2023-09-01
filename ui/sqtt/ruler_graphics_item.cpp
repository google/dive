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

#include "ruler_graphics_item.h"
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QPainter>
#include <QTextStream>
#include <QTransform>
#include "dive_core/common.h"
#include "dive_core/conversions.h"

//--------------------------------------------------------------------------------------------------
RulerGraphicsItem::RulerGraphicsItem()
{
    m_width = 0;
    m_max_cycles = 0;
}

//--------------------------------------------------------------------------------------------------
void RulerGraphicsItem::SetMaxCycles(uint64_t max_cycles)
{
    m_max_cycles = max_cycles;
}

//--------------------------------------------------------------------------------------------------
uint64_t RulerGraphicsItem::GetWidth() const
{
    return m_width;
}

//--------------------------------------------------------------------------------------------------
void RulerGraphicsItem::SetWidth(uint64_t width)
{
    if (m_width != width)
    {
        prepareGeometryChange();
        m_width = width;
    }
}

//--------------------------------------------------------------------------------------------------
void RulerGraphicsItem::SetVisibleRange(int64_t scene_x, int64_t width)
{
    if (m_max_cycles == 0)
        return;

    // Convert to local item coordinate
    int32_t item_x = mapFromScene(scene_x, 0).x();

    m_visible_start = item_x;
    m_visible_width = width;

    m_text_step = DetermineTextStepSize(m_max_cycles, m_width, m_visible_width);
}

//--------------------------------------------------------------------------------------------------
int64_t RulerGraphicsItem::MapToCycle(int64_t scene_x)
{
    double item_x = mapFromScene(scene_x, 0).x();
    double item_coord_to_cycle = (double)m_max_cycles / m_width;
    return item_coord_to_cycle * item_x;
}

//--------------------------------------------------------------------------------------------------
int64_t RulerGraphicsItem::MapToScene(int64_t cycle)
{
    double cycle_to_item_coord = (double)m_width / m_max_cycles;
    double item_x = cycle_to_item_coord * cycle;
    return mapToScene(QPoint(item_x, 0)).x();
}

//--------------------------------------------------------------------------------------------------
uint64_t RulerGraphicsItem::GetCyclesVisible(uint64_t visible_width, uint64_t ruler_width) const
{
    return ((double)m_max_cycles / ruler_width) * visible_width;
}

//--------------------------------------------------------------------------------------------------
QRectF RulerGraphicsItem::boundingRect() const
{
    return QRectF(0, 0, m_width, 39);
}

//--------------------------------------------------------------------------------------------------
QPainterPath RulerGraphicsItem::shape() const
{
    QPainterPath path;
    path.addRect(0, 0, m_width, 39);
    return path;
}

//--------------------------------------------------------------------------------------------------
void RulerGraphicsItem::paint(QPainter *                      painter,
                              const QStyleOptionGraphicsItem *option,
                              QWidget *                       widget)
{
    if (m_max_cycles == 0)
        return;

    QRectF rect = boundingRect();

    // Draw
    painter->setPen(QColor(0, 0, 0));
    painter->setBrush(QColor(158, 158, 158));
    painter->drawRect(rect);

    uint32_t font_height = RULER_FONT_HEIGHT;

    QFont font;
    font.setFamily(font.defaultFamily());
    font.setPointSize(font_height);
    painter->setFont(font);

    // Conversion factors

    Settings::DisplayUnit unit = Settings::Get()->ReadRulerDisplayUnit();

    double max_unit = 0.0;
    if (unit == Settings::DisplayUnit::kCycle)
        max_unit = m_max_cycles;
    else if (unit == Settings::DisplayUnit::kMs)
        max_unit = ConvertCyclesToMs(m_max_cycles);
    else if (unit == Settings::DisplayUnit::kUs)
        max_unit = ConvertCyclesToUs(m_max_cycles);
    else if (unit == Settings::DisplayUnit::kNs)
        max_unit = ConvertCyclesToNs(m_max_cycles);

    double unit_to_item_coord = m_width / max_unit;

    // Determine first location for the addText()
    // The integer rounding down allows for a starting coord just to the left of the viewport to
    // still be rendered (ie. partial text rendering on the left)
    uint64_t visible_start = std::max((int64_t)0, m_visible_start);
    double   visible_start_unit = visible_start / unit_to_item_coord;
    double   start_unit = (visible_start_unit / m_text_step) * m_text_step;
    uint64_t start_coord_x = start_unit * unit_to_item_coord;
    double   next_unit = 0.0;
    bool     visible = true;
    uint32_t step = 0;
    int64_t  text_coord_step = m_text_step * unit_to_item_coord;
    while (visible)
    {
        int64_t coord_x = start_coord_x + step * text_coord_step;

        next_unit = start_unit + step * m_text_step;

        // Stop drawing beyond the max unit
        if (next_unit > max_unit)
            break;

        QString tick_string = GetTickString(next_unit);

        // Going to re-create the transform matrix with the assumption that there is no existing
        // scaling or rotation
        DIVE_ASSERT(!painter->worldTransform().isScaling());
        DIVE_ASSERT(!painter->worldTransform().isRotating());

        // At really large coordinates, drawText fails. This is a known Qt problem that will not be
        // fixed. So use the worldTransform as a workaround.
        qreal orig_x = painter->worldTransform().dx();
        qreal orig_y = painter->worldTransform().dy();
        painter->setWorldTransform(QTransform::fromTranslate(coord_x + orig_x, 0 + orig_y));

        // Only draw text if all of it will be visible.
        QRect bounds = painter->boundingRect(0, 0, rect.width(), rect.height(), 0, tick_string);
        if (coord_x + bounds.right() < rect.width())
        {
            painter->drawText(0, rect.bottom() - (rect.height() / 2), tick_string);
        }

        // Main tick
        QPen thick_pen, thin_pen;
        thick_pen.setWidth(2);
        thin_pen.setWidth(1);
        painter->setWorldTransform(QTransform::fromTranslate(coord_x + orig_x, 0 + orig_y));
        painter->setPen(thick_pen);
        painter->drawLine(0, rect.bottom() - (rect.height() * 1.0 / 3.0), 0, rect.bottom());

        // All the following mini ticks
        double mini_step = text_coord_step / 10.0;
        for (uint32_t i = 1; i < 10; ++i)
        {
            qreal tx = coord_x + mini_step * i;

            // Outside of visible range
            if (tx > (m_visible_start + m_visible_width))
                break;

            painter->setWorldTransform(QTransform::fromTranslate(tx + orig_x, orig_y));
            painter->setPen(thin_pen);

            qreal line_scale = 1.0 / 7.0;
            if (i == 5)
                line_scale = 1.0 / 4.0;

            painter->drawLine(0, rect.bottom() - (rect.height() * line_scale), 0, rect.bottom());
        }
        // Restore
        painter->setWorldTransform(QTransform::fromTranslate(orig_x, orig_y));

        // Check if any of the mini ticks are visible. If they are, run next pass.
        coord_x += text_coord_step;
        visible = (coord_x < (m_visible_start + m_visible_width));
        step++;
    }
}

//--------------------------------------------------------------------------------------------------
void RulerGraphicsItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    Settings::DisplayUnit unit = Settings::Get()->ReadRulerDisplayUnit();

    QMenu menu;

    QAction cycle_action("Display in cycles");
    cycle_action.setCheckable(true);
    cycle_action.setChecked(unit == Settings::DisplayUnit::kCycle);
    menu.addAction(&cycle_action);

    QAction ms_action("Display in milliseconds (ms)");
    ms_action.setCheckable(true);
    ms_action.setChecked(unit == Settings::DisplayUnit::kMs);
    menu.addAction(&ms_action);

    QAction us_action("Display in microseconds (us)");
    us_action.setCheckable(true);
    us_action.setChecked(unit == Settings::DisplayUnit::kUs);
    menu.addAction(&us_action);

    QAction ns_action("Display in nanoseconds (ns)");
    ns_action.setCheckable(true);
    ns_action.setChecked(unit == Settings::DisplayUnit::kNs);
    menu.addAction(&ns_action);

    QAction *selected_action_ptr = menu.exec(event->screenPos());
    if (selected_action_ptr == &cycle_action)
        Settings::Get()->WriteRulerDisplayUnit(Settings::DisplayUnit::kCycle);
    else if (selected_action_ptr == &ms_action)
        Settings::Get()->WriteRulerDisplayUnit(Settings::DisplayUnit::kMs);
    else if (selected_action_ptr == &us_action)
        Settings::Get()->WriteRulerDisplayUnit(Settings::DisplayUnit::kUs);
    else if (selected_action_ptr == &ns_action)
        Settings::Get()->WriteRulerDisplayUnit(Settings::DisplayUnit::kNs);

    // Recalculate space between ticks
    m_text_step = DetermineTextStepSize(m_max_cycles, m_width, m_visible_width);

    update();
}

//--------------------------------------------------------------------------------------------------
QString RulerGraphicsItem::GetTickString(double value) const
{
    Settings::DisplayUnit unit = Settings::Get()->ReadRulerDisplayUnit();

    QString     tick_string;
    QTextStream out(&tick_string);

    out.setRealNumberNotation(QTextStream::FixedNotation);  // Equivalent of %f
    out.setRealNumberPrecision(0);
    out.setLocale(QLocale::system());  // To format #s (eg. English locale: 1000 -> 1,000)

    if (unit == Settings::DisplayUnit::kCycle)
    {
        out << value << " cycle";
    }
    else if (unit == Settings::DisplayUnit::kMs)
    {
        out.setRealNumberPrecision(4);  // #.####
        out << value << " ms";
    }
    else if (unit == Settings::DisplayUnit::kUs)
    {
        out.setRealNumberPrecision(2);  // #.##
        out << value << " us";
    }
    else if (unit == Settings::DisplayUnit::kNs)
    {
        out << value << " ns";
    }
    return tick_string;
}

//--------------------------------------------------------------------------------------------------
double RulerGraphicsItem::DetermineTextStepSize(uint64_t max_cycles,
                                                uint64_t max_width,
                                                uint64_t visible_width) const
{
    double text_step_size = 0.0;

    const uint32_t kMinStepCount = 5;
    const uint32_t kMaxStepCount = 10;

    uint32_t step_count = UINT32_MAX;

    // Start at lowest value possible
    Settings::DisplayUnit unit = Settings::Get()->ReadRulerDisplayUnit();

    double max_units = max_cycles;
    ;
    if (unit == Settings::DisplayUnit::kCycle)
    {
        text_step_size = 1.0;
        max_units = max_cycles;
    }
    else if (unit == Settings::DisplayUnit::kMs)
    {
        text_step_size = 0.01;
        max_units = ConvertCyclesToMs(max_cycles);
    }
    else if (unit == Settings::DisplayUnit::kUs)
    {
        text_step_size = 1;
        max_units = ConvertCyclesToUs(max_cycles);
    }
    else if (unit == Settings::DisplayUnit::kNs)
    {
        text_step_size = 1250;
        max_units = ConvertCyclesToNs(max_cycles);
    }

    double item_coord_to_unit = max_units / max_width;
    double visible_units = visible_width * item_coord_to_unit;

    // First, determine a power-of-10 value that will meet the kMaxStepCount requirement
    while (step_count > kMaxStepCount)
    {
        text_step_size *= 10;
        step_count = visible_units / text_step_size;
    }

    // Next, keep halving text_step_size until it exceeds kMinStepCount
    while (step_count < kMinStepCount)
    {
        text_step_size /= 2.0;
        step_count = visible_units / text_step_size;
    }

    // Lastly, ensure the text can fit, using max_units as the longest possible label string
    QFont font;
    font.setFamily(font.defaultFamily());
    font.setPointSize(RULER_FONT_HEIGHT);
    QFontMetrics fm(font);
    int          pixel_width = fm.horizontalAdvance(GetTickString(max_units));
    int64_t      text_coord_step = text_step_size / item_coord_to_unit;
    while (((double)text_coord_step * 0.85) < pixel_width)
    {
        text_step_size *= 2;
        text_coord_step = text_step_size / item_coord_to_unit;
    }
    return text_step_size;
}