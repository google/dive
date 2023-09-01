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

#pragma once
#include <QFrame>

// Forward declaration
class EventTimingGraphicsView;
class EventTimingGraphicsScene;
class RulerGraphicsItem;
class EventGraphicsItem;
class QFrame;
class QGraphicsScene;
class QWidget;
class QComboBox;
class QLabel;

#define SCALE_FACTOR 1.3

//--------------------------------------------------------------------------------------------------
class EventTimingView : public QFrame
{
    Q_OBJECT

public:
    EventTimingView();
    void Reset();

private slots:
    void Update();
    void OnMouseWheel(QPoint mouse_pos, int angle_delta);
    void OnColorByIndexChange(int index);

protected:
    virtual void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;

private:
    QWidget   *m_shader_legend;
    QWidget   *m_hardware_legend;
    QComboBox *m_color_combo_box_ptr;

    EventTimingGraphicsScene *m_event_timing_scene_ptr;
    EventTimingGraphicsView  *m_event_timing_view_ptr;
    RulerGraphicsItem        *m_ruler_item_ptr;
    EventGraphicsItem        *m_event_graphics_item_ptr;
};