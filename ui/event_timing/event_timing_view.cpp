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
#include "event_timing_view.h"
#include "dive_core/common.h"
#include "dive_core/common/gpudefs.h"
#include "event_graphics_item.h"
#include "event_timing_graphics_scene.h"
#include "event_timing_graphics_view.h"
#include "ui/sqtt/ruler_graphics_item.h"

#include <QComboBox>
#include <QGraphicsScene>
#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QScrollBar>
#include <QVBoxLayout>

// =================================================================================================
// EventTimingView
// =================================================================================================
EventTimingView::EventTimingView()
{
    // Add Color by combo box
    QHBoxLayout *combo_box_layout_ptr = new QHBoxLayout();
    m_color_combo_box_ptr = new QComboBox(this);
    m_color_combo_box_ptr->addItem("Color by shader");
    m_color_combo_box_ptr->addItem("Color by hardware context");
    m_color_combo_box_ptr->setCurrentIndex(0);
    combo_box_layout_ptr->addWidget(m_color_combo_box_ptr);
    combo_box_layout_ptr->addStretch();

    // Define graphics view and graphics scene
    m_event_timing_view_ptr = new EventTimingGraphicsView();
    m_event_timing_scene_ptr = new EventTimingGraphicsScene();

    // Define graphics items
    m_ruler_item_ptr = new RulerGraphicsItem();
    m_event_graphics_item_ptr = new EventGraphicsItem();

    // Set position of graphics items
    m_ruler_item_ptr->setPos(0, 0);
    m_event_graphics_item_ptr->setPos(0, 0);

    // Ensure the ruler always appears on the top
    m_event_graphics_item_ptr->setZValue(0);
    m_ruler_item_ptr->setZValue(1);

    m_event_timing_scene_ptr->addItem(m_ruler_item_ptr);
    m_event_timing_scene_ptr->addItem(m_event_graphics_item_ptr);
    m_event_timing_view_ptr->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_event_timing_view_ptr->setScene(m_event_timing_scene_ptr);

    // Add shader color legend
    m_shader_legend = new QWidget(this);
    QHBoxLayout *shader_legend_layout = new QHBoxLayout();

    const uint32_t kStageCount = (uint32_t)Dive::ShaderStage::kShaderStageCount;
    QString        shader_names[kStageCount] = {
               "Vertex Shader", "Pixel Shader", "Compute Shader", "Geometry Shader", "Hull Shader"
    };
    Dive::ShaderStage shader_stage_order[kStageCount] = { Dive::ShaderStage::kShaderStageVs,
                                                          Dive::ShaderStage::kShaderStagePs,
                                                          Dive::ShaderStage::kShaderStageCs,
                                                          Dive::ShaderStage::kShaderStageGs,
                                                          Dive::ShaderStage::kShaderStageHs };
    for (uint32_t i = 0; i < kStageCount; i++)
    {
        // Add color for the shader
        QLabel *color_label = new QLabel(this);
        color_label->setAutoFillBackground(true);
        QPalette          palette = color_label->palette();
        Dive::ShaderStage shader_stage = shader_stage_order[i];
        palette.setColor(QPalette::Window,
                         m_event_graphics_item_ptr->GetShaderStageColor(shader_stage));
        color_label->setPalette(palette);
        shader_legend_layout->addWidget(color_label);

        // Add name of the shader
        QLabel *text_label = new QLabel(this);
        text_label->setText(shader_names[i]);
        shader_legend_layout->addWidget(text_label);
    }
    shader_legend_layout->addStretch();
    m_shader_legend->setLayout(shader_legend_layout);
    m_shader_legend->show();

    // Add hardware context color legend
    m_hardware_legend = new QWidget(this);
    QHBoxLayout   *hardware_legend_layout = new QHBoxLayout();
    const uint32_t kCtxCount = 7;
    QString        hardware_context_names[kCtxCount] = { "Ctx 1", "Ctx 2", "Ctx 3", "Ctx 4",
                                                         "Ctx 5", "Ctx 6", "Ctx 7" };
    for (uint32_t i = 0; i < kCtxCount; i++)
    {
        // Add color for the shader
        QLabel *color_label = new QLabel(this);
        color_label->setAutoFillBackground(true);
        QPalette palette = color_label->palette();
        palette.setColor(QPalette::Window, m_event_graphics_item_ptr->GetHardwareContextColor(i));
        color_label->setPalette(palette);
        hardware_legend_layout->addWidget(color_label);

        // Add name of the shader
        QLabel *text_label = new QLabel(this);
        text_label->setText(hardware_context_names[i]);
        hardware_legend_layout->addWidget(text_label);
    }
    hardware_legend_layout->addStretch();
    m_hardware_legend->setLayout(hardware_legend_layout);
    m_hardware_legend->hide();

    QVBoxLayout *main_layout_ptr = new QVBoxLayout();
    main_layout_ptr->addLayout(combo_box_layout_ptr);
    main_layout_ptr->addWidget(m_event_timing_view_ptr);
    main_layout_ptr->addWidget(m_shader_legend);
    main_layout_ptr->addWidget(m_hardware_legend);
    setLayout(main_layout_ptr);

    Reset();

    QObject::connect(m_color_combo_box_ptr,
                     SIGNAL(currentIndexChanged(int)),
                     this,
                     SLOT(OnColorByIndexChange(int)));
    QObject::connect(m_event_timing_view_ptr->horizontalScrollBar(),
                     SIGNAL(valueChanged(int)),
                     this,
                     SLOT(Update()));
    QObject::connect(m_event_timing_view_ptr->verticalScrollBar(),
                     SIGNAL(valueChanged(int)),
                     this,
                     SLOT(Update()));
    QObject::connect(m_event_timing_view_ptr,
                     SIGNAL(OnMouseWheel(QPoint, int)),
                     this,
                     SLOT(OnMouseWheel(QPoint, int)));
    QObject::connect(m_event_timing_view_ptr,
                     SIGNAL(OnMouseCursor(QPointF)),
                     m_event_timing_scene_ptr,
                     SLOT(onMouseCursorChanged(QPointF)));
}

//--------------------------------------------------------------------------------------------------
void EventTimingView::Reset()
{
    // Make the scene width to be 5x the visible width
    m_ruler_item_ptr->SetWidth(m_event_timing_view_ptr->contentsRect().width() * 5);
    m_event_graphics_item_ptr->SetWidth(m_event_timing_view_ptr->contentsRect().width() * 5);
    m_event_graphics_item_ptr->SetHeight(m_ruler_item_ptr->boundingRect().height());

    // TODO(wangra): cleanup fixme!
    // m_ruler_item_ptr->SetMaxCycles(m_sqtt_data.GetMaxCycles());

    // Update viewport before calling Update() to make sure the mapToScene() returns appropriate
    // values
    m_event_timing_view_ptr->viewport()->update();

    Update();
}

//--------------------------------------------------------------------------------------------------
void EventTimingView::Update()
{
    // Set the position of the ruler to be on top of the scene
    m_ruler_item_ptr->setY(m_event_timing_view_ptr->mapToScene(QPoint(0, 0)).y());

    // Set the visible width of all graphics items
    int64_t visible_width = m_event_timing_view_ptr->contentsRect().width();
    int64_t visible_height = m_event_timing_view_ptr->contentsRect().height();
    int32_t scene_left_x = m_event_timing_view_ptr->mapToScene(QPoint(0, 0)).x();
    int32_t scene_left_y = m_event_timing_view_ptr->mapToScene(QPoint(0, 0)).y();
    m_ruler_item_ptr->SetVisibleRange(scene_left_x, visible_width);
    m_event_graphics_item_ptr->SetVisibleRange(scene_left_x,
                                               scene_left_y,
                                               visible_width,
                                               visible_height);

    // Update viewport (or else paint() of new region might not take into effect)
    m_event_timing_view_ptr->viewport()->update();
}

//--------------------------------------------------------------------------------------------------
void EventTimingView::OnMouseWheel(QPoint mouse_pos, int angle_delta)
{
    DIVE_ASSERT(angle_delta != 0);

    // Note: angle_delta is relative amount the wheel was rotated, in eighths of a degree. Most
    // mouse wheels work in steps of 15 degrees, but there are some finer-resolution wheels that
    // work in less than 120 units (ie. less than 15 degrees). Trackpads can also work in less than
    // 120 units

    // Determine what cycle is pointed to by mouse
    double prev_scene_mouse_pt_x = m_event_timing_view_ptr->mapToScene(QPoint(mouse_pos.x(), 0))
                                   .x();
    int64_t prev_cycle_mouse_pt = m_ruler_item_ptr->MapToCycle(prev_scene_mouse_pt_x);

    // Determine how off-center, in scene-coordinates, mouse position is
    uint64_t visible_width = m_event_timing_view_ptr->contentsRect().width();
    uint64_t visible_height = m_event_timing_view_ptr->contentsRect().height();
    double   prev_scene_center = m_event_timing_view_ptr->mapToScene(QPoint(visible_width / 2, 0))
                               .x();
    double
    prev_scene_center_y = m_event_timing_view_ptr->mapToScene(QPoint(0, visible_height / 2.0)).y();
    double scene_distance = prev_scene_mouse_pt_x - prev_scene_center;

    // If both start/end cycles are visible already, do not allow a zoom-out
    if (angle_delta < 0)
    {
        // TODO(wangra): cleanup fixme!
        // double  scene_right = m_event_timing_view_ptr->mapToScene(QPoint(visible_width, 0)).x();
        // int64_t cycle_right = m_ruler_item_ptr->MapToCycle(scene_right);
        // DIVE_ASSERT(m_sqtt_data.GetMaxCycles() <= INT64_MAX);  // To account for using int64
        // if (cycle_right >= (int64_t)m_sqtt_data.GetMaxCycles())
        {
            double  scene_left = m_event_timing_view_ptr->mapToScene(QPoint(0, 0)).x();
            int64_t cycle_left = m_ruler_item_ptr->MapToCycle(scene_left);
            if (cycle_left <= 0)
                return;
        }
    }

    double new_width = std::max(1.0, SCALE_FACTOR * m_ruler_item_ptr->GetWidth());
    if (angle_delta < 0)
        new_width = std::max(1.0, m_ruler_item_ptr->GetWidth() / SCALE_FACTOR);

    // Maximum width is bounded by INT32_MAX (assuming position of (0,0)), since the Qt code
    // uses code like this:
    //      viewBoundingRect.adjust(-int(rectAdjust), -int(rectAdjust), rectAdjust, rectAdjust);
    if ((m_ruler_item_ptr->pos().x() + new_width) > INT32_MAX)
        return;

    // If number of cycles visible would get below a certain threshold, then do not allow a zoom-in
    const uint64_t kMinCyclesVisible = 50;
    if (angle_delta > 0)
    {
        if (m_ruler_item_ptr->GetCyclesVisible(visible_width, new_width) < kMinCyclesVisible)
            return;
    }

    m_ruler_item_ptr->SetWidth(new_width);
    m_event_graphics_item_ptr->SetWidth(new_width);

    // Update the scene with the new ruler item size
    m_event_timing_scene_ptr->setSceneRect(m_event_timing_scene_ptr->itemsBoundingRect());

    double new_scene_mouse_pt = m_ruler_item_ptr->MapToScene(prev_cycle_mouse_pt);
    double new_scene_center = new_scene_mouse_pt - scene_distance;
    m_event_timing_view_ptr->centerOn(new_scene_center, prev_scene_center_y);

    // Update visible range in the graphics items
    Update();
}

//--------------------------------------------------------------------------------------------------
void EventTimingView::resizeEvent(QResizeEvent *event)
{
    Update();
}

void EventTimingView::OnColorByIndexChange(int index)
{
    if (index == 0)
    {
        m_shader_legend->show();
        m_hardware_legend->hide();
    }
    else if (index == 1)
    {
        m_shader_legend->hide();
        m_hardware_legend->show();
    }
    m_event_graphics_item_ptr->setColorByIndex(index);
}