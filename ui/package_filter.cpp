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
#include "QCheckBox"
#include "QHBoxLayout"
#include "QPushButton"
#include "QVBoxLayout"

//--------------------------------------------------------------------------------------------------
PackageFilter::PackageFilter(QWidget* parent) :
    QWidget(parent)

{
    m_all_filter = new QCheckBox("All", this);
    m_debuggable_filter = new QCheckBox("Debuggable", this);
    m_non_debuggable_filter = new QCheckBox("Non-Debuggable", this);
    m_apply = new QPushButton("Apply Filters", this);
    m_apply->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_debuggable_filter->setCheckState(Qt::Checked);
    m_active_filters.insert(m_debuggable_filter);

    connect(m_all_filter, SIGNAL(stateChanged(int)), this, SLOT(SelectAllEventsFilter(int)));
    connect(m_debuggable_filter, SIGNAL(stateChanged(int)), this, SLOT(SelectFilter(int)));
    connect(m_non_debuggable_filter, SIGNAL(stateChanged(int)), this, SLOT(SelectFilter(int)));
    connect(m_apply, SIGNAL(clicked()), this, SLOT(ApplyFilters()));

    QHBoxLayout* filter_options_layout = new QHBoxLayout;
    filter_options_layout->addWidget(m_all_filter);
    filter_options_layout->addWidget(m_debuggable_filter);
    filter_options_layout->addWidget(m_non_debuggable_filter);
    filter_options_layout->addWidget(m_apply);

    setLayout(filter_options_layout);
}

//--------------------------------------------------------------------------------------------------
void PackageFilter::SelectAllEventsFilter(int state)
{
    if (state == Qt::Checked)
    {
        // Block signals so this function does not trigger SelectFilter through the setCheckState
        // calls.
        m_debuggable_filter->blockSignals(true);
        m_non_debuggable_filter->blockSignals(true);

        m_debuggable_filter->setCheckState(Qt::Unchecked);
        m_non_debuggable_filter->setCheckState(Qt::Unchecked);

        m_filters.clear();
        m_filters.insert(m_all_filter);

        // Unblock signals so SelectFilter is triggered through the setCheckState calls.
        m_debuggable_filter->blockSignals(false);
        m_non_debuggable_filter->blockSignals(false);
    }
    else
    {
        m_filters.erase(m_all_filter);
    }
}

//--------------------------------------------------------------------------------------------------
void PackageFilter::SelectFilter(int state)
{
    QCheckBox* current_sender = qobject_cast<QCheckBox*>(sender());

    // Block signals so this function is not recursively triggered by setCheckState calls.
    m_all_filter->blockSignals(true);
    m_debuggable_filter->blockSignals(true);
    m_non_debuggable_filter->blockSignals(true);

    if (state == Qt::Checked)
    {
        if (m_all_filter->isChecked())
        {
            m_all_filter->setCheckState(Qt::Unchecked);
            m_filters.erase(m_all_filter);
        }

        m_filters.insert(current_sender);

        if (m_debuggable_filter->isChecked() && m_non_debuggable_filter->isChecked())
        {
            m_all_filter->setCheckState(Qt::Checked);

            m_debuggable_filter->setCheckState(Qt::Unchecked);
            m_non_debuggable_filter->setCheckState(Qt::Unchecked);

            m_filters.clear();
            m_filters.insert(m_all_filter);
        }
    }
    else
    {
        if (m_all_filter->isChecked())
        {
            m_all_filter->setCheckState(Qt::Unchecked);
            m_filters.erase(m_all_filter);
        }

        m_filters.erase(current_sender);
    }

    // Unblock signals so this function is triggered by setCheckState calls.
    m_all_filter->blockSignals(false);
    m_debuggable_filter->blockSignals(false);
    m_non_debuggable_filter->blockSignals(false);
}

//--------------------------------------------------------------------------------------------------
void PackageFilter::ApplyFilters()
{
    m_active_filters.clear();

    if (m_filters.empty())
    {
        m_all_filter->setCheckState(Qt::Checked);
        m_active_filters.insert(m_all_filter);
        emit FiltersApplied({ m_all_filter->text() });
    }
    else
    {
        QSet<QString> applied_filter_texts;
        for (QCheckBox* selectedCheckBox : m_filters)
        {
            if (selectedCheckBox->text() == "All")
            {
                m_active_filters.clear();
                applied_filter_texts.clear();
                m_active_filters.insert(selectedCheckBox);
                applied_filter_texts.insert(selectedCheckBox->text());
                emit FiltersApplied(applied_filter_texts);
                break;
            }
            m_active_filters.insert(selectedCheckBox);
            applied_filter_texts.insert(selectedCheckBox->text());
        }
        emit FiltersApplied(applied_filter_texts);
        m_filters.clear();
    }
}

//--------------------------------------------------------------------------------------------------
void PackageFilter::closeEvent(QCloseEvent* event)
{
    // Iterate over the copy of filters
    auto unappliedFilters = m_filters;
    for (QCheckBox* checkBox : unappliedFilters)
    {
        checkBox->setCheckState(m_active_filters.count(checkBox) ? Qt::Checked : Qt::Unchecked);
    }

    // Ensure all checkboxes in m_active_filters are checked
    for (QCheckBox* checkBox : m_active_filters)
    {
        if (!checkBox->isChecked())
        {
            checkBox->setCheckState(Qt::Checked);
        }
    }
}
