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

#include <QLabel>
#include <iostream>
#include <qevent.h>
#include <qnamespace.h>
#include <string>

#include "QCheckBox"
#include "QHBoxLayout"
#include "QPushButton"
#include "QVBoxLayout"

//--------------------------------------------------------------------------------------------------
PackageFilter::PackageFilter(QWidget* parent) : QWidget(parent)

{
    m_all_filter = new QCheckBox("All", this);
    m_all_filter->setCheckState(Qt::Checked);
    m_active_filters.insert(m_all_filter);
    m_debuggable_filter = new QCheckBox("Debuggable", this);
    m_non_debuggable_filter = new QCheckBox("Non-Debuggable", this);
    m_apply = new QPushButton("Apply Filters", this);
    m_apply->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    connect(this, SIGNAL(rejected()), this, SLOT(onReject()));
    connect(m_all_filter, SIGNAL(stateChanged(int)), this, SLOT(selectAllEventsFilter(int)));
    connect(m_debuggable_filter, SIGNAL(stateChanged(int)), this, SLOT(selectFilter(int)));
    connect(m_non_debuggable_filter, SIGNAL(stateChanged(int)), this, SLOT(selectFilter(int)));
    connect(m_apply, SIGNAL(clicked()), this, SLOT(applyFilters()));

    QHBoxLayout* filter_options_layout = new QHBoxLayout;
    filter_options_layout->addWidget(m_all_filter);
    filter_options_layout->addWidget(m_debuggable_filter);
    filter_options_layout->addWidget(m_non_debuggable_filter);
    filter_options_layout->addWidget(m_apply);

    setLayout(filter_options_layout);
}

//--------------------------------------------------------------------------------------------------
void PackageFilter::selectAllEventsFilter(int state)
{
    if (state == Qt::Checked)
    {
        m_non_debuggable_filter->setCheckState(Qt::Unchecked);
        m_debuggable_filter->setCheckState(Qt::Unchecked);
        m_filters.clear();
    }
}

//--------------------------------------------------------------------------------------------------
void PackageFilter::selectFilter(int state)
{
    if (state == Qt::Checked)
    {
        m_all_filter->setCheckState(Qt::Unchecked);
        auto iterator = std::find_if(m_filters.begin(),
                                     m_filters.end(),
                                     [](const QCheckBox* checkBox) {
                                         return checkBox->text() == "All";
                                     });
        if (iterator != m_filters.end())
        {
            m_filters.erase(iterator);
        }
        m_filters.insert(qobject_cast<QCheckBox*>(sender()));

        if (m_filters.size() == (kTotalFilterCount - 1))
        {
            m_all_filter->setCheckState(Qt::Checked);
        }
    }
    else
    {
        m_filters.erase(qobject_cast<QCheckBox*>(sender()));
    }
}

//--------------------------------------------------------------------------------------------------
void PackageFilter::applyFilters()
{
    m_active_filters.clear();

    if (m_filters.empty())
    {
        m_all_filter->setCheckState(Qt::Checked);
        m_active_filters.insert(m_all_filter);
        emit filtersApplied({ m_all_filter->text() });
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
                emit filtersApplied(applied_filter_texts);
                break;
            }
            m_active_filters.insert(selectedCheckBox);
            applied_filter_texts.insert(selectedCheckBox->text());
        }
        emit filtersApplied(applied_filter_texts);
        m_filters.clear();
    }
}

//--------------------------------------------------------------------------------------------------
void PackageFilter::onReject() { close(); }

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
