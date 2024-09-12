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
#include "dive_core/event_state.h"
#pragma once

// Forward declaration
class QTreeWidget;
class QTreeWidgetItem;
namespace Dive
{
class DataCore;
class EventStateInfo;
}  // namespace Dive

//--------------------------------------------------------------------------------------------------
class EventStateView : public QFrame
{
    Q_OBJECT

public:
    EventStateView(const Dive::DataCore &data_core);

private slots:
    void OnEventSelected(uint64_t node_index);
    void OnHover(QTreeWidgetItem *item_ptr, int column);

protected:
    virtual void leaveEvent(QEvent *event) override;

private:
    std::map<std::string, std::string> m_field_desc;
    const Dive::DataCore              &m_data_core;
    QTreeWidget                       *m_event_state_tree;

    Dive::EventStateInfo::ConstIterator GetStateInfoForEvent(const Dive::EventStateInfo &state,
                                                             uint32_t                    event_id);
    void BuildDescriptionMap(Dive::EventStateInfo::ConstIterator event_state_it);
    void DisplayEventStateInfo(Dive::EventStateInfo::ConstIterator event_state_it,
                               Dive::EventStateInfo::ConstIterator prev_event_state_it);
    void DisplayInputAssemblyState(Dive::EventStateInfo::ConstIterator event_state_it,
                                   Dive::EventStateInfo::ConstIterator);
    void DisplayTessellationState(Dive::EventStateInfo::ConstIterator event_state_it,
                                  Dive::EventStateInfo::ConstIterator prev_event_state_it);
    void DisplayRasterizerState(Dive::EventStateInfo::ConstIterator event_state_it,
                                Dive::EventStateInfo::ConstIterator prev_event_state_it);
    void DisplayFillViewportState(Dive::EventStateInfo::ConstIterator event_state_it,
                                  Dive::EventStateInfo::ConstIterator prev_event_state_it);
    void DisplayFillMultisamplingState(Dive::EventStateInfo::ConstIterator event_state_it,
                                       Dive::EventStateInfo::ConstIterator prev_event_state_it);
    void DisplayDepthState(Dive::EventStateInfo::ConstIterator event_state_it,
                           Dive::EventStateInfo::ConstIterator prev_event_state_it);
    void DisplayStencilState(Dive::EventStateInfo::ConstIterator event_state_it,
                             Dive::EventStateInfo::ConstIterator prev_event_state_it);
    void DisplayColorBlendState(Dive::EventStateInfo::ConstIterator event_state_it,
                                Dive::EventStateInfo::ConstIterator prev_event_state_it);
    void DisplayHardwareSpecificStates(Dive::EventStateInfo::ConstIterator event_state_it,
                                       Dive::EventStateInfo::ConstIterator prev_event_state_it);
};