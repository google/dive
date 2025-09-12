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
#include <QTableView>
#include <QVBoxLayout>
#include <vector>

#include "dive_core/command_hierarchy.h"

#pragma once

// Forward declaration
class GpuTimingModel;

class GpuTimingTabView : public QWidget
{
    Q_OBJECT

public:
    explicit GpuTimingTabView(GpuTimingModel               &gpu_timing_model,
                              const Dive::CommandHierarchy &command_hierarchy,
                              QWidget                      *parent = nullptr);

    // Recursive function to traverse model in order and collect indices of events that will have
    // GPU timing
    void CollectIndicesFromModel(const QAbstractItemModel &command_hierarchy_model,
                                 const QModelIndex        &parent_index);

public slots:
    void OnModelReset();
    void OnEventSelectionChanged(const QModelIndex &model_index);

private:
    void ResizeColumns();

    // If the node matches a type within Dive::AvailableGpuTiming::ObjectType, then store the model
    // index in m_timed_event_indices
    void CollectTimingIndex(Dive::NodeType     node_type,
                            const std::string &node_desc,
                            const QModelIndex &model_index);

    // Using m_timed_event_indices, converts Qt model index of an element to the id of the
    // corresponding row in this GPU timing info table
    int EventIndexToRow(const QModelIndex &model_index);

    GpuTimingModel               &m_model;
    const Dive::CommandHierarchy &m_command_hierarchy;
    QTableView                   *m_table_view;
    QVBoxLayout                  *m_main_layout;

    // Reverse-lookup vector used for correlating the selected Vulkan event with the highlighted
    // row in the GPU timing data
    //
    // The Qt index of all events for which there is GPU timing data
    std::vector<uint64_t> m_timed_event_indices = {};
};
