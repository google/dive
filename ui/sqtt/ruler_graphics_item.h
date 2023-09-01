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

// =====================================================================================================================
// Handles the rendering of the ruler item
// In particular, the item is informed what area is visible, and only renders the visible region
// =====================================================================================================================

#pragma once
#include <QGraphicsItem>
#include "settings.h"

#define RULER_FONT_HEIGHT 9

class RulerGraphicsItem : public QGraphicsItem
{
public:
    RulerGraphicsItem();

    // Set range
    void SetMaxCycles(uint64_t max_cycles);

    // Get the width of the ruler
    uint64_t GetWidth() const;
    void     SetWidth(uint64_t width);

    // Set the leftmost QGraphicsScene coordinate of ruler that is visible
    // This controls what is rendered in the viewport
    void SetVisibleRange(int64_t scene_x, int64_t width);

    // Conversion functions
    int64_t MapToCycle(int64_t scene_x);
    int64_t MapToScene(int64_t cycle);

    // Given a potential new ruler width, calculate how many cycles would be visible
    // This is used to determine whether to zoom or not
    uint64_t GetCyclesVisible(uint64_t visible_width, uint64_t ruler_width) const;

    // QGraphicsItem overrides
    virtual QRectF       boundingRect() const Q_DECL_OVERRIDE;
    virtual QPainterPath shape() const Q_DECL_OVERRIDE;
    virtual void         paint(QPainter *                      painter,
                               const QStyleOptionGraphicsItem *option,
                               QWidget *                       widget) Q_DECL_OVERRIDE;

protected:
    virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) Q_DECL_OVERRIDE;

private:
    QString GetTickString(double value) const;
    double  DetermineTextStepSize(uint64_t max_units,
                                  uint64_t max_width,
                                  uint64_t visible_width) const;

    uint64_t m_width;
    int64_t  m_visible_start = 0;
    int64_t  m_visible_width = 0;

    // Cycle values
    uint64_t m_max_cycles;

    // Each major tick step, in whatever unit is enabled
    double m_text_step;
};