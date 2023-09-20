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
#include "shader_view.h"

#include "dive_core/cross_ref.h"
#include "dive_core/data_core.h"
#include "dive_core/dive_strings.h"
#include "dive_core/shader_disassembly.h"

#include "shader_text_view.h"

#include <QTextEdit>
#include <QTreeWidget>
#include <QVBoxLayout>

// =================================================================================================
// ShaderWidgetItem
// =================================================================================================
class ShaderWidgetItem : public QTreeWidgetItem
{
public:
    ShaderWidgetItem(Dive::ShaderStage shader_stage,
                     Dive::CrossRef    shader,
                     uint32_t          event_index,
                     QTreeWidget      *view) :
        QTreeWidgetItem(view),
        m_shader_stage(shader_stage),
        m_shader(shader),
        m_event_index(event_index)
    {
    }

    // TODO(tianc): make it so we don't need two different reference systems for crashdump and
    // profiler trace.
    ShaderWidgetItem(Dive::ShaderStage shader_stage,
                     uint32_t          shader_index,
                     uint32_t          event_index,
                     QTreeWidget      *view) :
        ShaderWidgetItem(shader_stage,
                         Dive::CrossRef(Dive::CrossRefType::kNone, shader_index),
                         event_index,
                         view)
    {
    }

    Dive::ShaderStage GetShaderStage() const { return m_shader_stage; }
    uint32_t          GetShaderIndex() const { return static_cast<uint32_t>(m_shader.Id()); }
    uint32_t          GetEventIndex() const { return m_event_index; }
    Dive::CrossRef    GetShaderRef() const { return m_shader; }

private:
    Dive::ShaderStage m_shader_stage;
    Dive::CrossRef    m_shader;
    uint32_t          m_event_index;
};

// =================================================================================================
// ShaderView
// =================================================================================================
ShaderView::ShaderView(const Dive::DataCore &data_core) :
    m_data_core(data_core)
{
    QVBoxLayout *layout = new QVBoxLayout();
    m_shader_list = new QTreeWidget();
    m_shader_list->setColumnCount(4);
    m_shader_list->setHeaderLabels(QStringList() << "Shader Stage"
                                                 << "Address"
                                                 << "Size"
                                                 << "VGPRs"
                                                 << "SGPRs"
                                                 << "Preferred Heap"
                                                 << "PSO Hash");
    m_shader_code_text = new ShaderTextView();
    layout->addWidget(m_shader_list);
    layout->setStretchFactor(m_shader_list, 1);
    layout->addWidget(m_shader_code_text);
    layout->setStretchFactor(m_shader_code_text, 5);
    setLayout(layout);

    QObject::connect(m_shader_list,
                     SIGNAL(itemSelectionChanged()),
                     this,
                     SLOT(OnShaderSelectionChanged()));
}

//--------------------------------------------------------------------------------------------------
void ShaderView::Reset()
{
    m_shader_list->clear();
    m_shader_code_text->clear();
    m_shader_code_text->EnableHoverEvent(false);
}

//--------------------------------------------------------------------------------------------------
void ShaderView::SetupHoverHelp(HoverHelp &hoverhelp)
{
    QObject::connect(m_shader_code_text,
                     &ShaderTextView::HoverEnter,
                     &hoverhelp,
                     &HoverHelp::OnEnter);
    QObject::connect(m_shader_code_text,
                     &ShaderTextView::HoverExit,
                     &hoverhelp,
                     &HoverHelp::OnExit);
}

//--------------------------------------------------------------------------------------------------
void ShaderView::OnEventSelected(uint64_t node_index)
{
    m_shader_list->clear();
    if (node_index == UINT64_MAX)
        return;

    const Dive::CaptureMetadata  &metadata = m_data_core.GetCaptureMetadata();
    const Dive::CommandHierarchy &command_hierarchy = m_data_core.GetCommandHierarchy();
    const uint32_t kShaderStageCount = (uint32_t)Dive::ShaderStage::kShaderStageCount;

    auto update_shader_list = [&](uint64_t event_node_index) {
        uint32_t event_id = command_hierarchy.GetEventNodeId(event_node_index);
        if (event_id >= metadata.m_event_info.size())
        {
            return;
        }
        const Dive::EventInfo &event = metadata.m_event_info[event_id];
        for (uint32_t shader_stage = 0; shader_stage < kShaderStageCount; ++shader_stage)
        {
            // Do not add shaders that are not used by the event
            if (event.m_type != Dive::EventInfo::EventType::kDraw &&
                event.m_type != Dive::EventInfo::EventType::kDispatch)
                continue;
            if ((event.m_type == Dive::EventInfo::EventType::kDraw) &&
                (shader_stage == (uint32_t)Dive::ShaderStage::kShaderStageCs))
                continue;
            if ((event.m_type == Dive::EventInfo::EventType::kDispatch) &&
                (shader_stage != (uint32_t)Dive::ShaderStage::kShaderStageCs))
                continue;

            uint32_t shader_index = event.m_shader_indices[shader_stage];
            if (shader_index != UINT32_MAX)
            {
                const Dive::ShaderInfo &shader_info = metadata
                                                      .m_shaders[shader_stage][shader_index];
                ShaderWidgetItem *treeItem = new ShaderWidgetItem((Dive::ShaderStage)shader_stage,
                                                                  shader_index,
                                                                  event_id,
                                                                  m_shader_list);

                // Column 0
                treeItem->setText(0, tr(kShaderStageStrings[shader_stage]));

                // Column 1
                const uint32_t buffer_size = 256;
                char           buffer[buffer_size];
                snprintf(buffer, buffer_size, "%p", (void *)shader_info.m_addr);
                treeItem->setText(1, tr(buffer));

                // Column 2
                static_assert(sizeof(unsigned long long) == sizeof(uint64_t), "%llu failure!");
                snprintf(buffer, 256, "%llu", (unsigned long long)shader_info.m_size);
                treeItem->setText(2, tr(buffer));

                // Column 3&4: VGPRs, SGPRs: TODO(rwang): removed from cleanup
                const Dive::CaptureData &capture_data = m_data_core.GetCaptureData();

                // Column 5: Preferred heap
                const Dive::MemoryManager        &memory = capture_data.GetMemoryManager();
                const Dive::MemoryAllocationInfo &mem_alloc_info = memory.GetMemoryAllocationInfo();
                const Dive::MemoryAllocationData *mem_alloc_ptr;
                mem_alloc_ptr = mem_alloc_info.FindInternalAllocation(shader_info.m_addr,
                                                                      shader_info.m_size);
                DIVE_ASSERT(mem_alloc_ptr != nullptr);
                switch ((Dive::MemoryAllocationData::GpuHeap)mem_alloc_ptr->m_preferred_heap)
                {
                case Dive::MemoryAllocationData::GpuHeap::GpuHeapLocal:
                    snprintf(buffer, buffer_size, "Local");
                    break;
                case Dive::MemoryAllocationData::GpuHeap::GpuHeapInvisible:
                    snprintf(buffer, buffer_size, "Invisible");
                    break;
                case Dive::MemoryAllocationData::GpuHeap::GpuHeapGartUswc:
                    snprintf(buffer, buffer_size, "GartUswc");
                    break;
                case Dive::MemoryAllocationData::GpuHeap::GpuHeapGartCacheable:
                    snprintf(buffer, buffer_size, "GartCacheable");
                    break;
                default: DIVE_ASSERT(false);
                }
                treeItem->setText(5, tr(buffer));
            }
        }
    };

    Dive::NodeType node_type = command_hierarchy.GetNodeType(node_index);
    if (node_type == Dive::NodeType::kDrawDispatchBlitNode)
    {
        update_shader_list(node_index);
    }
    else if (node_type == Dive::NodeType::kMarkerNode)
    {
        auto topology = m_data_core.GetCommandHierarchy().GetAllEventHierarchyTopology();
        auto num_children = topology.GetNumChildren(node_index);
        for (uint64_t i = 0; i < num_children; i++)
        {
            auto           child_node_index = topology.GetChildNodeIndex(node_index, i);
            Dive::NodeType child_node_type = command_hierarchy.GetNodeType(child_node_index);
            if (child_node_type == Dive::NodeType::kDrawDispatchBlitNode)
            {
                update_shader_list(child_node_index);
            }
        }
    }

    // Resize columns to fit
    uint32_t column_count = (uint32_t)m_shader_list->columnCount();
    for (uint32_t column = 0; column < column_count; ++column)
        m_shader_list->resizeColumnToContents(column);

    // Clear m_shader_code_text *after* clearing the m_shader_list, because the act of clearing the
    // m_buffer_list can signal a selection change and fill out the table
    m_shader_code_text->clear();

    // Auto-select first shader item from the shader list
    m_shader_list->setCurrentItem(m_shader_list->topLevelItem(0));
}

//--------------------------------------------------------------------------------------------------
void ShaderView::OnShaderSelectionChanged()
{
    m_shader_code_text->EnableHoverEvent(false);
    const ShaderWidgetItem *item_ptr = (const ShaderWidgetItem *)m_shader_list->currentItem();
    m_shader_code_text->clear();

    // Determine shader type by matching column 0 string
    uint32_t selected_stage = (uint32_t)item_ptr->GetShaderStage();
    uint32_t shader_index = item_ptr->GetShaderIndex();

    const Dive::CaptureMetadata &metadata = m_data_core.GetCaptureMetadata();
    const Dive::ShaderInfo      &shader_info = metadata.m_shaders[selected_stage][shader_index];
    m_shader_code_text->append(shader_info.m_disassembly.GetListing().c_str());
    m_shader_code_text->moveCursor(QTextCursor::Start);
    m_shader_code_text->setReadOnly(true);
}

//--------------------------------------------------------------------------------------------------
bool ShaderView::OnCrossReference(Dive::CrossRef ref)
{
    if (ref.Type() == Dive::CrossRefType::kShaderAddress)
    {
        char buffer[16 + 2 + 2] = {};  // {"0x", 16 digit, null}
        snprintf(buffer, 16 + 2 + 1, "%p", reinterpret_cast<void *>(ref.Id()));

        // Search column 1 (address)
        auto res = m_shader_list->findItems(tr(buffer), Qt::MatchContains, 1);

        if (!res.empty())
        {
            m_shader_list->setCurrentItem(*res.begin());
            return true;
        }
    }
    return false;
}
