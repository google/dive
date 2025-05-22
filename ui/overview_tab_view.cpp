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

#include "overview_tab_view.h"

#include <QApplication>
#include <QSplitter>
#include <QVBoxLayout>

#include "dive_core/data_core.h"
#include "event_selection_model.h"
#include "most_expensive_events_view.h"
#include "problems_view.h"

// =================================================================================================
// OverviewTabView
// =================================================================================================
OverviewTabView::OverviewTabView(const Dive::CaptureMetadata &capture_metadata,
                                 EventSelection              &event_selection)
{
    m_problems_view = new ProblemsView(capture_metadata.m_command_hierarchy);
    m_expensive_events_view = new MostExpensiveEventsView(capture_metadata);

    m_tab_widget = new QTabWidget();
    m_problems_view_tab_index = m_tab_widget->addTab(m_problems_view, "Problems");
    m_expensive_events_view_tab_index = m_tab_widget->addTab(m_expensive_events_view,
                                                             "Most Expensive Events");

    QVBoxLayout *main_layout = new QVBoxLayout();
    main_layout->addWidget(m_tab_widget);
    setLayout(main_layout);
}

//--------------------------------------------------------------------------------------------------
void OverviewTabView::Update(const Dive::LogRecord *log_ptr)
{
    // Show warning icon if any entries detected
    if (log_ptr->GetNumEntries() > 0)
    {
        QIcon warning_icon = qApp->style()->standardIcon(QStyle::SP_MessageBoxWarning);
        m_tab_widget->setTabIcon(m_tab_widget->indexOf(m_problems_view), warning_icon);
    }
    else
    {
        m_tab_widget->setTabIcon(m_tab_widget->indexOf(m_problems_view), QIcon());
    }

    m_problems_view->Update(log_ptr);
    m_expensive_events_view->Update();
}

//--------------------------------------------------------------------------------------------------
void OverviewTabView::UpdateTabAvailability()
{
    SetTabAvailable(m_tab_widget, m_expensive_events_view_tab_index, true);
}
