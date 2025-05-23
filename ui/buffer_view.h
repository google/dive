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
#pragma once

// Forward declaration
class QTableWidget;
class QTreeWidget;
class QTreeWidgetItem;
namespace Dive
{
class DataCore;
}

//--------------------------------------------------------------------------------------------------
class BufferView : public QFrame
{
    Q_OBJECT

public:
    BufferView(const Dive::DataCore &data_core);

private slots:
    void OnEventSelected(uint32_t event_index);
    void OnBufferSelectionChanged();

private:
    const Dive::DataCore &m_data_core;
    QTableWidget         *m_memory_view;
    QTreeWidget          *m_buffer_list;
    uint32_t              m_event_index;

    const uint32_t kNumDwordsPerRow = 4;

    // Store identity information for each buffer in the m_buffer_list
    std::vector<uint32_t> m_buffer_indices;
};
