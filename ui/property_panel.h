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

#include <QFrame>
#include <QScrollArea>

#include "dive_core/cross_ref.h"

// Forward declarations
class QLabel;

// This class provides panel to display properties
class PropertyPanel : public QFrame
{
    Q_OBJECT

public:
    PropertyPanel(QFrame *m_parent = nullptr);

private:
    const int kMargin = 10;

    // Selection info string
    QScrollArea *m_selection_info_sa;
    QLabel *     m_selection_info_str;

    // Hover help string
    QLabel *m_hover_help_str;

protected:
    virtual void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;

signals:
    void crossReference(Dive::CrossRef);

public slots:

    // Update property panel on change in hover string
    void OnHoverStringChange(const QString &);

    // Update property panel on change in selection info
    void OnSelectionInfoChange(const QString &);

    // Update property panel on addition of vulkan params
    void OnVulkanParams(const QString &);

private slots:
    void OnLinkActivated(const QString &);
};