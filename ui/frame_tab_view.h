/*
 Copyright 2025 Google LLC
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

#include <QWidget>
#pragma once

// Forward declaration
class QLabel;
class QPixmap;
class QPushButton;
class QScrollArea;

class FrameTabView : public QWidget
{
    Q_OBJECT
public:
    explicit FrameTabView(QWidget *parent = nullptr);

public slots:
    void OnCaptureScreenshotLoaded(const QString &file_path);

private slots:
    void OnActualSize();
    void OnFitToView();
    void OnZoomIn();
    void OnZoomOut();

private:
    void ScaleAndDisplayImage();

    QLabel      *m_image_label;
    QScrollArea *m_scroll_area;
    QPushButton *m_actual_size_button;
    QPushButton *m_fit_to_fill_button;
    QPushButton *m_zoom_in_button;
    QPushButton *m_zoom_out_button;
    QPixmap      m_image;
    qreal        m_scale_factor = 1.0;
    qreal        m_initial_scale_factor = 1.0;
    qreal        m_max_scale_factor = 5.0;
};
