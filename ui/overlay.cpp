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

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QLabel>
#include <QPushButton>
#include <QElapsedTimer>
#include <QVBoxLayout>
#include <memory>

namespace
{
// Don't add visual clutter if the operation is short.
constexpr int kOverlayMinElapsedTimeMs = 2000;
// Update UI once per second so it does not look frozen.
constexpr int kOverlayUpdateIntervalMs = 1000;
}  // namespace

//--------------------------------------------------------------------------------------------------
void OverlayWidget::newParent()
{
    if (!parent())
        return;
    parent()->installEventFilter(this);
    raise();
}

//--------------------------------------------------------------------------------------------------
OverlayWidget::OverlayWidget(QWidget* parent) :
    QWidget(parent)
{
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    newParent();
}

//--------------------------------------------------------------------------------------------------
bool OverlayWidget::eventFilter(QObject* object, QEvent* event)
{
    if (object == parent())
    {
        if (event->type() == QEvent::Resize)
            resize(static_cast<QResizeEvent*>(event)->size());
        else if (event->type() == QEvent::ChildAdded)
            raise();
    }
    return QWidget::eventFilter(object, event);
}

//--------------------------------------------------------------------------------------------------
bool OverlayWidget::event(QEvent* event)
{
    if (event->type() == QEvent::ParentAboutToChange)
    {
        if (parent())
            parent()->removeEventFilter(this);
    }
    else if (event->type() == QEvent::ParentChange)
        newParent();
    return QWidget::event(event);
}

//--------------------------------------------------------------------------------------------------
Overlay::Overlay(QWidget* parent) :
    OverlayWidget(parent)
{
    m_timer = new QTimer(this);
    m_elapsed_timer = std::make_unique<QElapsedTimer>();

    setAttribute(Qt::WA_TranslucentBackground);
    hide();

    QObject::connect(m_timer, &QTimer::timeout, this, &Overlay::OnUpdate);
}

Overlay::~Overlay()
{
    // For ~unique_ptr<QElapsedTimer>()
}

//--------------------------------------------------------------------------------------------------
void Overlay::SetMessage(const QString& message, bool timed)
{
    m_elapsed_timer->invalidate();
    if (message.isEmpty())
    {
        m_timer->stop();
    }
    else if (timed)
    {
        m_elapsed_timer->start();
        m_timer->start(kOverlayUpdateIntervalMs);
    }
    m_message = message;
    update();
}

//--------------------------------------------------------------------------------------------------
void Overlay::OnUpdate()
{
    update();
}

//--------------------------------------------------------------------------------------------------
void Overlay::UpdateSize(const QRect& rect)
{
    m_overlay_rect = rect;
    setGeometry(m_overlay_rect);
}

//--------------------------------------------------------------------------------------------------
void Overlay::paintEvent(QPaintEvent* paint_event)
{
    if (m_message.isEmpty())
        return;
    auto message = m_message;
    if (m_elapsed_timer->isValid())
    {
        auto elapsed = m_elapsed_timer->elapsed();
        if (elapsed >= kOverlayMinElapsedTimeMs)
        {
            message += " (elapsed: " + QString::number(elapsed / 1000) + "s)";
        }
    }

    QPainter painter(this);
    QFont    font = painter.font();
    font.setPixelSize(20);
    painter.setFont(font);
    painter.fillRect(m_overlay_rect, { 100, 100, 100, 128 });
    painter.setPen({ 200, 200, 255 });
    painter.drawText(rect(), message, Qt::AlignHCenter | Qt::AlignVCenter);
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

void OverlayHelper::SetMessage(const QString& message)
{
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
    m_timer->stop();
    m_elapsed_timer->invalidate();
    m_text->setText(QString());
    m_cancel_button->hide();
    m_overlay->hide();
    m_layout->setStackingMode(QStackedLayout::StackOne);
    m_layout->setCurrentWidget(m_container);
}

void OverlayHelper::Initialize(QLayout* layout)
{
    m_timer = new QTimer(this);
    m_elapsed_timer = std::make_unique<QElapsedTimer>();

    m_layout = new QStackedLayout;
    m_container = new QWidget;
    m_layout->addWidget(m_container);
    m_container->setLayout(layout);

    m_overlay = new QWidget;
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
