/*
 Copyright 2020 Google LLC
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
#pragma once
#include <QObject>
#include <unordered_set>

#include "dive_core/command_hierarchy.h"
#include "dive_core/cross_ref.h"

// The model class for the selection system
class EventSelection : public QObject
{
    Q_OBJECT

public:
    EventSelection(const Dive::CommandHierarchy &command_hierarchy);
    void Reset();

private:
    void updateCurrentNode(uint64_t node_index);

    const Dive::CommandHierarchy &m_command_hierarchy;
    uint64_t                      m_current_node = UINT64_MAX;
    Dive::CrossRef                m_current_ref;

signals:
    void currentNodeChanged(uint64_t node_index, uint64_t prev_node_index);
    void vulkanParams(const QString &);
    void crossReference(Dive::CrossRef ref);
};
