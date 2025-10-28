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

#include "frame_tab_view.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <qobject.h>
#include <qpixmap.h>
#include <QPoint>
#include <QPushButton>
#include <QScrollArea>
#include "object_names.h"
#include <qtimer.h>

static constexpr double kZoomStepFactor = 1.25;

FrameTabView::FrameTabView(QWidget *parent) :
    QWidget(parent)
{
    m_image_label = new QLabel(this);
    m_image_label->setAlignment(Qt::AlignCenter);
    m_image_label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_image_label->setPixmap(m_image);

    m_scroll_area = new QScrollArea(this);
    m_scroll_area->setBackgroundRole(QPalette::Dark);
    m_scroll_area->setWidget(m_image_label);
    m_scroll_area->setAlignment(Qt::AlignCenter);

    m_actual_size_button = new QPushButton("1:1", this);
    m_fit_to_fill_button = new QPushButton("Fit", this);
    m_zoom_in_button = new QPushButton("Zoom In (+)", this);
    m_zoom_out_button = new QPushButton("Zoom Out (-)", this);

    QHBoxLayout *controls_layout = new QHBoxLayout();
    controls_layout->addWidget(m_actual_size_button);
    controls_layout->addWidget(m_fit_to_fill_button);
    controls_layout->addWidget(m_zoom_in_button);
    controls_layout->addWidget(m_zoom_out_button);
    controls_layout->addStretch();

    QVBoxLayout *main_layout = new QVBoxLayout(this);
    main_layout->addLayout(controls_layout);
    main_layout->addWidget(m_scroll_area);

    QObject::connect(m_actual_size_button,
                     &QPushButton::clicked,
                     this,
                     &FrameTabView::OnActualSize);
    QObject::connect(m_fit_to_fill_button, &QPushButton::clicked, this, &FrameTabView::OnFitToView);
    QObject::connect(m_zoom_in_button, &QPushButton::clicked, this, &FrameTabView::OnZoomIn);
    QObject::connect(m_zoom_out_button, &QPushButton::clicked, this, &FrameTabView::OnZoomOut);
}

//--------------------------------------------------------------------------------------------------
void FrameTabView::OnCalculateInitialScale()
{
    if (!m_image.isNull() && m_initial_scale_needed)
    {
        QSize viewport_size = m_scroll_area->viewport()->size();
        QSize image_size = m_image.size();

        double width_ratio = static_cast<double>(viewport_size.width()) /
                             static_cast<double>(image_size.width());
        double height_ratio = static_cast<double>(viewport_size.height()) /
                              static_cast<double>(image_size.height());

        // Use the smaller ratio to guarantee the whole image fits.
        m_scale_factor = qMin(width_ratio, height_ratio);

        ScaleAndDisplayImage();

        // Mark the initial scaling as complete.
        m_initial_scale_needed = false;
        m_initial_scale_factor = m_scale_factor;
    }
}

//--------------------------------------------------------------------------------------------------
void FrameTabView::OnCaptureScreenshotLoaded(const QString &file_path)
{
    if (m_image.load(file_path))
    {
        m_scale_factor = 1.0;
        m_initial_scale_needed = true;

        // Defer the scaling calculation until the event queue has processed
        QTimer::singleShot(10, this, &FrameTabView::OnCalculateInitialScale);
    }
    else
    {
        m_image_label->setPixmap(QPixmap());
        m_image_label->update();
    }
}

//--------------------------------------------------------------------------------------------------
void FrameTabView::ScaleAndDisplayImage()
{
    if (m_image.isNull())
        return;

    QSize new_size = m_image.size() * m_scale_factor;

    QPixmap scaled_image = m_image.scaled(new_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    m_image_label->setPixmap(scaled_image);
    m_image_label->resize(scaled_image.size());
}

//--------------------------------------------------------------------------------------------------
void FrameTabView::OnFitToView()
{
    if (m_image.isNull())
        return;

    QSize viewport_size = m_scroll_area->viewport()->size();
    QSize image_size = m_image.size();

    if (viewport_size.isEmpty() || image_size.isEmpty())
        return;

    double width_ratio = static_cast<double>(viewport_size.width()) /
                         static_cast<double>(image_size.width());
    double height_ratio = static_cast<double>(viewport_size.height()) /
                          static_cast<double>(image_size.height());

    m_scale_factor = qMax(width_ratio, height_ratio);

    ScaleAndDisplayImage();
}

//--------------------------------------------------------------------------------------------------
void FrameTabView::OnActualSize()
{
    if (!m_image.isNull())
    {
        m_scale_factor = m_initial_scale_factor;
        ScaleAndDisplayImage();
    }
}

//--------------------------------------------------------------------------------------------------
void FrameTabView::OnZoomIn()
{
    // Calculate the next potential scale factor
    double next_scale = m_scale_factor * kZoomStepFactor;

    // Guard against exceeding the maximum zoom limit
    if (next_scale <= m_max_scale_factor)
    {
        m_scale_factor = next_scale;
        ScaleAndDisplayImage();
    }
    else if (m_scale_factor < m_max_scale_factor)
    {
        m_scale_factor = m_max_scale_factor;
        ScaleAndDisplayImage();
    }
}

//--------------------------------------------------------------------------------------------------
void FrameTabView::OnZoomOut()
{
    m_scale_factor /= kZoomStepFactor;
    ScaleAndDisplayImage();
}
