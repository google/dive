/*
 Copyright 2021 Google LLC

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

#pragma once

// Forward declaration
class ProblemsView;
class MostExpensiveEventsView;
class EventSelection;
class QTabWidget;

// Hide/Disable tab depend on QT_VERSION
void SetTabAvailable(QTabWidget *widget, int index, bool available);

namespace Dive
{
class LogRecord;
struct CaptureMetadata;
};  // namespace Dive

class OverviewTabView : public QFrame
{
    Q_OBJECT

public:
    OverviewTabView(const Dive::CaptureMetadata &capture_metadata, EventSelection &event_selection);

    void Update(const Dive::LogRecord *log_ptr);
    void UpdateTabAvailability();

private:
    QTabWidget              *m_tab_widget;
    ProblemsView            *m_problems_view;
    int                      m_problems_view_tab_index;
    MostExpensiveEventsView *m_expensive_events_view;
    int                      m_expensive_events_view_tab_index;
};
