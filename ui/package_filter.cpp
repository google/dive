/*
 Copyright 2024 Google LLC
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

#include "package_filter.h"
#include <qevent.h>
#include <qnamespace.h>
#include <QLabel>
#include <iostream>
#include <string>
#include "QButtonGroup"
#include "QRadioButton"
#include "QHBoxLayout"
#include "QPushButton"
#include "QVBoxLayout"

//--------------------------------------------------------------------------------------------------
PackageFilter::PackageFilter(QWidget* parent) :
    QWidget(parent)

{
    m_filter_button_group = new QButtonGroup(this);
    m_all_filter = new QRadioButton("All", this);
    m_debuggable_filter = new QRadioButton("Debuggable", this);
    m_non_debuggable_filter = new QRadioButton("Non-Debuggable", this);
    m_apply = new QPushButton("Apply Filters", this);
    m_apply->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_filter_button_group->addButton(m_all_filter, 0);
    m_filter_button_group->addButton(m_debuggable_filter, 1);
    m_filter_button_group->addButton(m_non_debuggable_filter, 2);

    m_debuggable_filter->setChecked(true);
    m_active_filter_text = m_debuggable_filter->text();

    connect(m_apply, &QPushButton::clicked, this, &PackageFilter::ApplyFilters);

    QHBoxLayout* filter_options_layout = new QHBoxLayout;
    filter_options_layout->addWidget(m_all_filter);
    filter_options_layout->addWidget(m_debuggable_filter);
    filter_options_layout->addWidget(m_non_debuggable_filter);
    filter_options_layout->addStretch();
    filter_options_layout->addWidget(m_apply);

    setLayout(filter_options_layout);
}

//--------------------------------------------------------------------------------------------------
void PackageFilter::ApplyFilters()
{
    QRadioButton* checked_button = qobject_cast<QRadioButton*>(
    m_filter_button_group->checkedButton());
    if (checked_button)
    {
        m_active_filter_text = checked_button->text();
    }
    else
    {
        m_active_filter_text = "All";
        m_all_filter->setChecked(true);
    }

    emit FiltersApplied(m_active_filter_text);
}

//--------------------------------------------------------------------------------------------------
void PackageFilter::closeEvent(QCloseEvent* event)
{
    QList<QAbstractButton*> buttons = m_filter_button_group->buttons();
    for (QAbstractButton* button : buttons)
    {
        if (button->text() == m_active_filter_text)
        {
            if (!button->isChecked())
            {
                button->setChecked(true);
            }
            break;
        }
    }
    QWidget::closeEvent(event);
}
