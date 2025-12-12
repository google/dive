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

#include "overlay.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QLabel>
#include <QObject>
#include <QPushButton>
#include <QStackedLayout>
#include <QTimer>
#include <QtWidgets>
#include <QVBoxLayout>
#include <QWidget>

namespace
{
// Don't add visual clutter if the operation is short.
constexpr int kOverlayMinElapsedTimeMs = 2000;
// Update UI once per second so it does not look frozen.
constexpr int kOverlayUpdateIntervalMs = 1000;
}  // namespace

//--------------------------------------------------------------------------------------------------
OverlayHelper::OverlayHelper(QObject* parent) :
    QObject(parent)
{
}

OverlayHelper::~OverlayHelper() = default;

//--------------------------------------------------------------------------------------------------
QLayout* OverlayHelper::GetLayout() const
{
    return m_layout;
}

//--------------------------------------------------------------------------------------------------
void OverlayHelper::Initialize(QLayout* layout, QWidget* parent)
{
    auto container = new QWidget(parent);
    container->setLayout(layout);
    Initialize(container);
}

void OverlayHelper::Initialize(QWidget* widget)
{
    m_container = widget;

    m_timer = new QTimer(this);
    m_elapsed_timer.reset(new QElapsedTimer);

    m_layout = new QStackedLayout;
    m_layout->addWidget(m_container);

    m_overlay = new QWidget();
    {
        // It does not seems translucent background is required?
        // m_overlay->setAttribute(Qt::WA_TranslucentBackground);
        auto palette = m_overlay->palette();
        m_overlay->setAutoFillBackground(true);
        palette.setColor(QPalette::All, QPalette::Window, { 100, 100, 100, 128 });
        m_overlay->setPalette(palette);
    }
    m_layout->addWidget(m_overlay);
    m_overlay_layout = new QVBoxLayout;
    m_overlay->setLayout(m_overlay_layout);
    m_text = new QLabel;
    {
        m_text->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        m_text->setWordWrap(true);
        m_text->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        auto font = m_text->font();
        font.setPixelSize(20);
        m_text->setFont(font);
        auto palette = m_text->palette();
        palette.setColor(QPalette::All, QPalette::WindowText, { 200, 200, 255 });
        m_text->setPalette(palette);
    }
    m_overlay_layout->addWidget(m_text);
    m_cancel_button = new QPushButton;
    {
        m_cancel_button->setHidden(true);
        m_cancel_button->setText("Cancel");
    }
    m_overlay_layout->addWidget(m_cancel_button);

    QObject::connect(m_timer, &QTimer::timeout, this, &OverlayHelper::OnUpdate);
    QObject::connect(m_cancel_button, &QPushButton::clicked, this, &OverlayHelper::OnCancel);

    Clear();
}

//--------------------------------------------------------------------------------------------------
void OverlayHelper::OnUpdate()
{
    if (m_message.isEmpty())
    {
        m_text->setText("");
    }
    else
    {
        auto message = m_message;
        if (m_elapsed_timer->isValid())
        {
            auto elapsed = m_elapsed_timer->elapsed();
            if (elapsed >= kOverlayMinElapsedTimeMs)
            {
                message += " (elapsed: " + QString::number(elapsed / 1000) + "s)";
            }
        }

        m_text->setText(message);
    }
}

void OverlayHelper::OnCancel()
{
    if (m_cancel_func)
    {
        m_cancel_func();
    }
}

//--------------------------------------------------------------------------------------------------
void OverlayHelper::SetMessage(const QString& message)
{
    if (message.isEmpty())
    {
        return Clear();
    }
    m_timer->stop();
    m_elapsed_timer->invalidate();
    m_cancel_func = {};
    m_cancel_button->hide();

    m_message = message;
    m_text->setText(message);
    m_layout->setStackingMode(QStackedLayout::StackAll);
    m_layout->setCurrentWidget(m_overlay);
    if (m_overlay->isHidden())
        m_overlay->show();
}

void OverlayHelper::SetMessageIsTimed()
{
    m_elapsed_timer->start();
    m_timer->start(kOverlayUpdateIntervalMs);
}

void OverlayHelper::SetMessageCancelFunc(CancelFunc func)
{
    m_cancel_func = func;
    m_cancel_button->show();
}

void OverlayHelper::Clear()
{
    m_message.clear();

    m_overlay->hide();
    m_layout->setCurrentWidget(m_container);
    m_layout->setStackingMode(QStackedLayout::StackOne);

    m_text->setText(QString());

    m_timer->stop();
    m_elapsed_timer->invalidate();
    m_cancel_func = {};
    m_cancel_button->hide();
}
