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

#include "gpu_timing_tab_view.h"

#include <QAbstractItemModel>
#include <QDebug>
#include <QItemSelectionModel>

#include "ui/gpu_timing_model.h"

GpuTimingTabView::GpuTimingTabView(GpuTimingModel               &gpu_timing_model,
                                   const Dive::CommandHierarchy &command_hierarchy,
                                   QWidget                      *parent) :
    QWidget(parent),
    m_model(gpu_timing_model),
    m_command_hierarchy(command_hierarchy)
{
    m_table_view = new QTableView(this);
    m_table_view->setModel(&m_model);

    // Used otherwise the table does not expand to fit available space
    m_main_layout = new QVBoxLayout(this);
    m_main_layout->addWidget(m_table_view);

    QObject::connect(&m_model,
                     &QAbstractItemModel::modelReset,
                     this,
                     &GpuTimingTabView::OnModelReset);
}

//--------------------------------------------------------------------------------------------------
void GpuTimingTabView::OnModelReset()
{
    ResizeColumns();
}

//--------------------------------------------------------------------------------------------------
void GpuTimingTabView::CollectIndicesFromModel(const QAbstractItemModel &command_hierarchy_model,
                                               const QModelIndex        &parent_index)
{
    // Initial call clears the map, and subsequent recursed calls repopulate it
    if (!parent_index.isValid())
    {
        qDebug() << "GpuTimingTabView::CollectIndicesFromModel()";
        m_timed_event_indices.clear();
    }

    for (int row = 0; row < command_hierarchy_model.rowCount(parent_index); ++row)
    {
        QModelIndex index = command_hierarchy_model.index(row, 0, parent_index);
        if (!index.isValid())
        {
            continue;
        }
        uint64_t       node_index = (uint64_t)index.internalPointer();
        Dive::NodeType node_type = m_command_hierarchy.GetNodeType(node_index);
        std::string    node_desc = m_command_hierarchy.GetNodeDesc(node_index);
        CollectTimingIndex(node_type, node_desc, index);

        // Recurse into valid children
        CollectIndicesFromModel(command_hierarchy_model, index);
    }
}

//--------------------------------------------------------------------------------------------------
void GpuTimingTabView::CollectTimingIndex(Dive::NodeType     node_type,
                                          const std::string &node_desc,
                                          const QModelIndex &model_index)
{
    if (!model_index.isValid())
    {
        qDebug() << "Tried to collect invalid model index for m_timed_event_indices";
        return;
    }

    switch (node_type)
    {
    case Dive::NodeType::kGfxrRootFrameNode:  // AvailableGpuTiming::ObjectType::kFrame
    case Dive::NodeType::
    kGfxrVulkanCommandBufferNode:  // AvailableGpuTiming::ObjectType::kCommandBuffer
    case Dive::NodeType::
    kGfxrVulkanRenderPassCommandNode:  // AvailableGpuTiming::ObjectType::kRenderPass
    {
        if (node_desc.find("End") != std::string::npos)
        {
            // For paired types (e.g. Command Buffers and RenderPasses), only the *Begin* calls are
            // used for correlation, *End* is rejected
            return;
        }
        uint64_t index_address = (uint64_t)model_index.internalPointer();
        m_timed_event_indices.push_back(index_address);
    }
    default:
        return;
    }
}

//--------------------------------------------------------------------------------------------------
void GpuTimingTabView::ResizeColumns()
{
    // Resize columns to fit the content
    uint32_t column_count = (uint32_t)m_model.columnCount(QModelIndex());
    for (uint32_t column = 0; column < column_count; ++column)
    {
        m_table_view->resizeColumnToContents(column);
    }
}

//--------------------------------------------------------------------------------------------------
int GpuTimingTabView::EventIndexToRow(const QModelIndex &model_index)
{
    if (!model_index.isValid())
    {
        qDebug() << "Tried to convert invalid model index to row";
        return -1;
    }

    uint64_t index_address = (uint64_t)model_index.internalPointer();

    const auto it = std::find(m_timed_event_indices.cbegin(),
                              m_timed_event_indices.cend(),
                              index_address);
    if (it == m_timed_event_indices.cend())
    {
        return -1;
    }

    return std::distance(m_timed_event_indices.cbegin(), it);
}

//--------------------------------------------------------------------------------------------------
void GpuTimingTabView::OnEventSelectionChanged(const QModelIndex &model_index)
{
    // Verify that the number of rows in the model is consistent with the rows of
    // m_timed_event_indices
    if (m_model.rowCount() != static_cast<int>(m_timed_event_indices.size()))
    {
        qDebug()
        << "GpuTimingTabView::OnEventSelectionChanged() ERROR: inconsistent model row count ("
        << m_model.rowCount() << ") and count of collected indices of timed Vulkan events: "
        << m_timed_event_indices.size();
    }

    int row = EventIndexToRow(model_index);
    if (row < 0)
    {
        m_table_view->clearSelection();
        return;
    }
    m_table_view->selectRow(row);
}