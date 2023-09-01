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

#include <QWidget>
#include <QtWidgets>

class OverlayWidget : public QWidget
{
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
public:
    Overlay(QWidget* parent = nullptr);

    // Set message to be displayed on the overlay
    void SetMessage(const QString& message);

    // Update the size of the overlay widget
    void UpdateSize(const QRect& rect);

protected:
    void paintEvent(QPaintEvent* paint_event) Q_DECL_OVERRIDE;

private:
    QString m_message;
    QRect   m_overlay_rect;
};