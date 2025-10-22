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

#include <memory>
#include <functional>
#include <QWidget>
#include <QtWidgets>

class QTimer;
class QElapsedTimer;
class QLabel;
class QPushButton;
class QVBoxLayout;
class QStackedLayout;

class OverlayWidget : public QWidget
{
    Q_OBJECT
    void newParent();

public:
    OverlayWidget(QWidget* parent = nullptr);

protected:
    // Catches resize and child events from the parent widget
    bool eventFilter(QObject* object, QEvent* event) Q_DECL_OVERRIDE;

    // Tracks parent widget changes
    bool event(QEvent* event) Q_DECL_OVERRIDE;
};

class Overlay : public OverlayWidget
{
    Q_OBJECT
public:
    Overlay(QWidget* parent = nullptr);
    ~Overlay();

    // Set message to be displayed on the overlay
    void SetMessage(const QString& message, bool timed = false);

    // Update the size of the overlay widget
    void UpdateSize(const QRect& rect);

private slots:
    void OnUpdate();

protected:
    void paintEvent(QPaintEvent* paint_event) Q_DECL_OVERRIDE;

private:
    // Timer for UI refreshing.
    QTimer* m_timer = nullptr;
    // Elapsed time since last timed message / event.
    std::unique_ptr<QElapsedTimer> m_elapsed_timer;

    QString m_message;
    QRect   m_overlay_rect;
};

class OverlayHelper : public QObject
{
    Q_OBJECT
public:
    using CancelFunc = std::function<void()>;
    explicit OverlayHelper(QObject* parent) :
        QObject(parent)
    {
    }

    void            Initialize(QLayout* layout);
    QStackedLayout* GetLayout() const { return m_layout; }

    // Set message to be displayed on the overlay
    void SetMessage(const QString& message);
    void SetMessageIsTimed();
    void SetMessageCancelFunc(CancelFunc func);
    void Clear();

private slots:
    void OnUpdate();
    void OnCancel();

private:
    // Timer for UI refreshing.
    QTimer* m_timer = nullptr;
    // Elapsed time since last timed message / event.
    std::unique_ptr<QElapsedTimer> m_elapsed_timer;

    CancelFunc m_cancel_func;

    QStackedLayout* m_layout = nullptr;
    QWidget*        m_overlay = nullptr;
    QWidget*        m_container = nullptr;

    QVBoxLayout* m_overlay_layout = nullptr;
    QLabel*      m_text = nullptr;
    QPushButton* m_cancel_button = nullptr;

    QString m_message;
};
