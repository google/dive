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
#include "buffer_view.h"
#include "dive_core/data_core.h"
#include "dive_core/dive_strings.h"

#include <QTableWidget>
#include <QTreeWidget>
#include <QVBoxLayout>

// =================================================================================================
// BufferWidgetItem
// =================================================================================================
class BufferWidgetItem : public QTreeWidgetItem
{
public:
    BufferWidgetItem(uint32_t buffer_index, QTreeWidget *view) :
        QTreeWidgetItem(view),
        m_buffer_index(buffer_index)
    {
    }
    uint32_t GetBufferIndex() const { return m_buffer_index; }

private:
    uint32_t m_buffer_index;
};

// =================================================================================================
// BufferView
// =================================================================================================
BufferView::BufferView(const Dive::DataCore &data_core) :
    m_data_core(data_core),
    m_event_index(UINT32_MAX)
{
    QVBoxLayout *layout = new QVBoxLayout();
    m_buffer_list = new QTreeWidget();
    m_buffer_list->setColumnCount(9);
    m_buffer_list->setHeaderLabels(QStringList() << "Shader Stage"
                                                 << "Address"
                                                 << "Size"
                                                 << "Format"
                                                 << "Dest-X"
                                                 << "Dest-Y"
                                                 << "Dest-Z"
                                                 << "Dest-W"
                                                 << "Preferred Heap");
    m_memory_view = new QTableWidget();
    m_memory_view->setColumnCount(kNumDwordsPerRow + 1);
    QStringList header;
    header << "Address";
    for (uint32_t i = 0; i < kNumDwordsPerRow; i++)
        header << "";
    m_memory_view->setHorizontalHeaderLabels(header);
    m_memory_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_memory_view->setFocusPolicy(Qt::NoFocus);
    m_memory_view->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_buffer_list);
    layout->setStretchFactor(m_buffer_list, 1);
    layout->addWidget(m_memory_view);
    layout->setStretchFactor(m_memory_view, 5);
    setLayout(layout);

    QObject::connect(m_buffer_list,
                     SIGNAL(itemSelectionChanged()),
                     this,
                     SLOT(OnBufferSelectionChanged()));
}

//--------------------------------------------------------------------------------------------------
void BufferView::OnEventSelected(uint32_t event_index)
{
    m_buffer_list->clear();
    m_buffer_indices.clear();
    if (event_index == UINT32_MAX)
        return;

    const Dive::CaptureMetadata &metadata = m_data_core.GetCaptureMetadata();

    // Add the buffer(s) to the list
    const uint32_t         kShaderStageCount = (uint32_t)Dive::ShaderStage::kShaderStageCount;
    const Dive::EventInfo &event_info = metadata.m_event_info[event_index];
    for (uint32_t shader_stage = 0; shader_stage < kShaderStageCount; ++shader_stage)
    {
        uint32_t num_buffers = (uint32_t)event_info.m_buffer_indices[shader_stage].size();
        for (uint32_t buffer = 0; buffer < num_buffers; ++buffer)
        {
            uint32_t buffer_index = event_info.m_buffer_indices[shader_stage][buffer];
            const Dive::BufferInfo &buffer_info = metadata.m_buffers[buffer_index];
            BufferWidgetItem       *treeItem = new BufferWidgetItem(buffer_index, m_buffer_list);

            // Column 0
            treeItem->setText(0, tr(kShaderStageStrings[shader_stage]));

            // Column 1
            char str_buffer[256];
            sprintf(str_buffer, "%p", (void *)buffer_info.m_addr);
            treeItem->setText(1, tr(str_buffer));

            // Column 2
            static_assert(sizeof(unsigned long long) == sizeof(uint64_t), "%llu failure!");
            sprintf(str_buffer, "%llu", (unsigned long long)buffer_info.m_size);
            treeItem->setText(2, tr(str_buffer));

            // Column 3
            sprintf(str_buffer,
                    "%s_%s",
                    kBufferDataFormatStrings[buffer_info.m_data_format],
                    kBufferNumFormatStrings[buffer_info.m_num_format]);
            treeItem->setText(3, tr(str_buffer));

            // Column 4
            sprintf(str_buffer, "%s", kSqSelStrings[(uint32_t)buffer_info.m_dst_sel_x]);
            treeItem->setText(4, tr(str_buffer));

            // Column 5
            sprintf(str_buffer, "%s", kSqSelStrings[(uint32_t)buffer_info.m_dst_sel_y]);
            treeItem->setText(5, tr(str_buffer));

            // Column 6
            sprintf(str_buffer, "%s", kSqSelStrings[(uint32_t)buffer_info.m_dst_sel_z]);
            treeItem->setText(6, tr(str_buffer));

            // Column 7
            sprintf(str_buffer, "%s", kSqSelStrings[(uint32_t)buffer_info.m_dst_sel_w]);
            treeItem->setText(7, tr(str_buffer));

            // Column 8
            // Find preferred heap
            const Dive::CaptureData          &capture_data = m_data_core.GetCaptureData();
            const Dive::MemoryManager        &memory = capture_data.GetMemoryManager();
            const Dive::MemoryAllocationInfo &mem_alloc_info = memory.GetMemoryAllocationInfo();
            const Dive::MemoryAllocationData *mem_alloc_ptr;
            mem_alloc_ptr = mem_alloc_info.FindGlobalAllocation(buffer_info.m_addr,
                                                                buffer_info.m_size);
            DIVE_ASSERT(mem_alloc_ptr != nullptr);
            switch ((Dive::MemoryAllocationData::GpuHeap)mem_alloc_ptr->m_preferred_heap)
            {
            case Dive::MemoryAllocationData::GpuHeap::GpuHeapLocal:
                sprintf(str_buffer, "Local");
                break;
            case Dive::MemoryAllocationData::GpuHeap::GpuHeapInvisible:
                sprintf(str_buffer, "Invisible");
                break;
            case Dive::MemoryAllocationData::GpuHeap::GpuHeapGartUswc:
                sprintf(str_buffer, "GartUswc");
                break;
            case Dive::MemoryAllocationData::GpuHeap::GpuHeapGartCacheable:
                sprintf(str_buffer, "GartCacheable");
                break;
            default: DIVE_ASSERT(false);
            }
            treeItem->setText(8, tr(str_buffer));

            // Store the buffer index for retrieval on item selection
            m_buffer_indices.push_back(buffer_index);
        }
    }

    // Resize columns to fit
    uint32_t column_count = (uint32_t)m_buffer_list->columnCount();
    for (uint32_t column = 0; column < column_count; ++column)
        m_buffer_list->resizeColumnToContents(column);

    // Clear m_memory_view *after* clearing the m_buffer_list, because the act of clearing the
    // m_buffer_list can signal a selection change and fill out the table
    m_memory_view->setRowCount(0);

    m_event_index = event_index;
}

//--------------------------------------------------------------------------------------------------
void BufferView::OnBufferSelectionChanged()
{
    // Obtain the buffer_index of the selected item
    const BufferWidgetItem *item_ptr = (const BufferWidgetItem *)m_buffer_list->currentItem();
    uint32_t                buffer_index = item_ptr->GetBufferIndex();

    // Grab the buffer metadata
    const Dive::CaptureMetadata &metadata = m_data_core.GetCaptureMetadata();
    const Dive::BufferInfo      &buffer_info = metadata.m_buffers[buffer_index];

    // Set size of the table
    uint32_t num_dwords = buffer_info.m_size / sizeof(uint32_t);
    DIVE_ASSERT(buffer_info.m_size % sizeof(uint32_t) == 0);
    uint32_t num_rows = num_dwords / kNumDwordsPerRow;
    num_rows += (num_dwords % kNumDwordsPerRow == 0) ? 0 : 1;
    m_memory_view->setRowCount(num_rows);

    // Grab all the data
    std::vector<uint32_t> buffer_memory;
    buffer_memory.resize(num_dwords);
    const Dive::EventInfo     &event_info = metadata.m_event_info[m_event_index];
    const Dive::CaptureData   &capture_data = m_data_core.GetCaptureData();
    const Dive::MemoryManager &mem_manager = capture_data.GetMemoryManager();
    bool                       success = mem_manager.CopyMemory(&buffer_memory[0],
                                          event_info.m_submit_index,
                                          buffer_info.m_addr,
                                          buffer_info.m_size);
    DIVE_VERIFY(success);

    // Add each dword to the table
    uint32_t cur_col = 0;
    uint32_t cur_row = 0;
    uint32_t dword = 0;
    while (dword < buffer_info.m_size / sizeof(uint32_t))
    {
        uint64_t dword_addr = buffer_info.m_addr + dword * sizeof(uint32_t);

        char buffer[256];
        if (cur_col == 0)
        {
            sprintf(buffer, "%p", (void *)dword_addr);
            QTableWidgetItem *addr_item = new QTableWidgetItem(buffer);
            addr_item->setBackground(QColor(192, 192, 192));
            m_memory_view->setItem(cur_row, cur_col, addr_item);
        }
        else
        {
            sprintf(buffer, "%f", *((float *)&buffer_memory[dword]));
            m_memory_view->setItem(cur_row, cur_col, new QTableWidgetItem(buffer));
            dword++;
        }

        cur_col++;
        if (cur_col > kNumDwordsPerRow)
        {
            cur_row++;
            cur_col = 0;
        }
    }
}
