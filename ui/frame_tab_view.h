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

class FrameTabView : public QWidget
{
    Q_OBJECT
public:
    explicit FrameTabView(QWidget *parent = nullptr);
    ~FrameTabView() override;

public slots:
    void OnCaptureScreenshotLoaded(const QString &file_path);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QLabel  *m_image_label;
    QPixmap *m_image;
};
