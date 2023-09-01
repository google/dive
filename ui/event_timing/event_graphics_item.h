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
// Handles the rendering of all wavefronts
// =====================================================================================================================

#pragma once
#include <QGraphicsItem>
#include "dive_core/common/gpudefs.h"

#define EVENT_HEIGHT 30

class EventGraphicsItem : public QGraphicsItem
{
public:
    EventGraphicsItem();

    // Set width of item
    void SetWidth(uint64_t width);

    // Set height of item
    void SetHeight(uint64_t offset_height);

    // Set the leftmost QGraphicsScene coordinate of item that is visible
    void SetVisibleRange(int64_t scene_x, int64_t scene_y, int64_t width, int64_t height);

    // Get color for the event
    QColor GetEventColor(uint32_t event);

    // Set m_color_by_index
    void setColorByIndex(int index);

    // Shader stage colors
    QColor GetShaderStageColor(Dive::ShaderStage shader_stage) const
    {
        const uint32_t kStageCount = (uint32_t)Dive::ShaderStage::kShaderStageCount;

        QColor stage_to_color[kStageCount] = { QColor(0, 150, 0),      // kShaderStageCs
                                               QColor(0, 215, 215),    // kShaderStageGs
                                               QColor(160, 160, 160),  // kShaderStageHs
                                               QColor(110, 110, 255),  // kShaderStagePs
                                               QColor(255, 70, 70) };  // kShaderStageVs
        return stage_to_color[(uint32_t)shader_stage];
    }

    // hardware context colors
    QColor GetHardwareContextColor(uint8_t ctx_index) const
    {
        const uint32_t kCtxCount = 7;

        QColor hw_ctx_color[kCtxCount] = { QColor(0, 215, 215),   QColor(160, 160, 160),
                                           QColor(110, 110, 255), QColor(255, 100, 150),
                                           QColor(255, 0, 0),     QColor(0, 100, 100),
                                           QColor(0, 150, 0) };
        return ctx_index >= kCtxCount ? QColor(255, 255, 255) : hw_ctx_color[(uint32_t)ctx_index];
    }

    // QGraphicsItem overrides
    virtual QRectF       boundingRect() const Q_DECL_OVERRIDE;
    virtual QPainterPath shape() const Q_DECL_OVERRIDE;
    virtual void         paint(QPainter                       *painter,
                               const QStyleOptionGraphicsItem *option,
                               QWidget                        *widget) Q_DECL_OVERRIDE;

private:
    void DrawEvents(QPainter *painter);
    void CalcRectCoord(uint64_t  start_cycle,
                       uint64_t  end_cycle,
                       uint64_t *start_x,
                       uint64_t *end_x);

    int64_t  m_visible_start_x = 0;
    int64_t  m_visible_start_y = 0;
    int64_t  m_visible_width = 0;
    int64_t  m_visible_height = 0;
    uint64_t m_width = 0;
    uint64_t m_height = 0;
    uint32_t m_color_by_index = 0;
};