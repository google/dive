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
#include <qpixmap.h>
#include <QPoint>
#include "object_names.h"

FrameTabView::FrameTabView(QWidget *parent) :
    QWidget(parent)
{
    m_image_label = new QLabel();
    m_image_label->setAlignment(Qt::AlignCenter);
    m_image_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    m_image_label->setScaledContents(true);

    m_image = new QPixmap();
    m_image_label->setPixmap(*m_image);

    QVBoxLayout *main_layout = new QVBoxLayout(this);
    main_layout->addWidget(m_image_label);
}

void FrameTabView::OnCaptureScreenshotLoaded(const QString &file_path)
{
    if (m_image->load(file_path))
    {
        m_image_label->setPixmap(*m_image);
        m_image_label->update();
    }
    else
    {
        m_image_label->setPixmap(QPixmap());
        m_image_label->update();
    }
}

void FrameTabView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if (!m_image->isNull())
    {
        QSize label_size = m_image_label->size();

        QPixmap scaled_image = m_image->scaled(label_size,
                                               Qt::KeepAspectRatio,
                                               Qt::SmoothTransformation);

        m_image_label->setPixmap(scaled_image);
    }
}
