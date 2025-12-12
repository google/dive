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

#include <functional>
#include <QObject>
#include <QScopedPointer>

class QElapsedTimer;
class QLabel;
class QLayout;
class QPushButton;
class QStackedLayout;
class QTimer;
class QVBoxLayout;
class QWidget;

class OverlayHelper : public QObject
{
    Q_OBJECT
public:
    using CancelFunc = std::function<void()>;
    explicit OverlayHelper(QObject* parent);
    ~OverlayHelper();

    void Initialize(QLayout* layout, QWidget* parent = nullptr);
    void Initialize(QWidget* widget);

    QLayout* GetLayout() const;

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
    QScopedPointer<QElapsedTimer> m_elapsed_timer;

    CancelFunc m_cancel_func;

    QStackedLayout* m_layout = nullptr;
    QWidget*        m_overlay = nullptr;
    QWidget*        m_container = nullptr;

    QVBoxLayout* m_overlay_layout = nullptr;
    QLabel*      m_text = nullptr;
    QPushButton* m_cancel_button = nullptr;

    QString m_message;
};
