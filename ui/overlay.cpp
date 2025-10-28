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

#include <QTimer>
#include <QDateTime>
#include <memory>
#include <qelapsedtimer.h>
#include <qobject.h>
#include <qtimer.h>

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
