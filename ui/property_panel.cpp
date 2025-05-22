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

#include "property_panel.h"

#include <QGridLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QVBoxLayout>

//--------------------------------------------------------------------------------------------------
PropertyPanel::PropertyPanel(QFrame* m_parent) : QFrame(m_parent)
{
    setMinimumWidth(250);

    m_selection_info_str = new QLabel(this);
    m_selection_info_str->setObjectName("propertyPanelLabel");
    m_selection_info_str->setTextFormat(Qt::RichText);
    m_selection_info_str->setAlignment(Qt::AlignTop);
    m_selection_info_str->hide();
    m_selection_info_str->setWordWrap(true);

    m_selection_info_sa = new QScrollArea(this);
    m_selection_info_sa->setWidget(m_selection_info_str);
    m_selection_info_sa->setAlignment(Qt::AlignTop);
    m_selection_info_sa->setWidgetResizable(true);
    m_selection_info_sa->hide();
    m_selection_info_sa->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_hover_help_str = new QLabel(this);
    m_hover_help_str->setObjectName("hoverHelpLabel");
    m_hover_help_str->setTextFormat(Qt::RichText);
    m_hover_help_str->hide();
    m_hover_help_str->setWordWrap(true);
    m_hover_help_str->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Ignored);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 10);
    layout->addWidget(m_selection_info_sa);
    layout->addWidget(m_hover_help_str);
    layout->setAlignment(m_hover_help_str, Qt::AlignBottom);

    QObject::connect(m_selection_info_str,
                     &QLabel::linkActivated,
                     this,
                     &PropertyPanel::OnLinkActivated);

    setLayout(layout);
}

//--------------------------------------------------------------------------------------------------
void PropertyPanel::OnHoverStringChange(const QString& string)
{
    // Hide widget if no string to display
    if (string.isEmpty())
    {
        m_hover_help_str->hide();
        return;
    }

    m_hover_help_str->setMaximumWidth(width() - kMargin);
    m_hover_help_str->setText(string);
    m_hover_help_str->show();
}

//--------------------------------------------------------------------------------------------------
void PropertyPanel::OnSelectionInfoChange(const QString& string)
{
    // Hide widget if no string to display
    if (string.isEmpty())
    {
        m_selection_info_str->setText(QString());
        m_selection_info_sa->hide();
        m_selection_info_str->hide();
        return;
    }

    m_selection_info_str->setMaximumWidth(width() - kMargin);
    m_selection_info_str->setText(string);
    m_selection_info_str->show();
    m_selection_info_sa->show();
}

//--------------------------------------------------------------------------------------------------
void PropertyPanel::OnVulkanParams(const QString& string)
{
    if (string.isEmpty())
        return;

    m_selection_info_str->setText(string);
    m_selection_info_str->show();
    m_selection_info_sa->show();
}

//--------------------------------------------------------------------------------------------------
void PropertyPanel::resizeEvent(QResizeEvent* event)
{
    m_selection_info_str->setMaximumWidth(event->size().width() - kMargin);
    m_hover_help_str->setMaximumWidth(event->size().width() - kMargin);
}

//--------------------------------------------------------------------------------------------------
void PropertyPanel::OnLinkActivated(const QString& link)
{
    if (link.startsWith("#shader-"))
    {
        QStringRef addr_str(&link,
                            (sizeof("#shader-") - 1),
                            link.size() - (sizeof("#shader-") - 1));
        bool       ok = false;
        uint64_t   addr = addr_str.toULongLong(&ok, 16);
        if (ok)
        {
            emit crossReference(Dive::CrossRef(Dive::CrossRefType::kShaderAddress, addr));
        }
    }
}