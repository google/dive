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

#include "event_state_view.h"
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <map>
#include <string>
#include "color_utils.h"
#include "dive_core/command_hierarchy.h"
#include "dive_core/data_core.h"
#include "dive_core/dive_strings.h"
#include "dive_core/shader_disassembly.h"
#include "hover_help_model.h"

#define ADD_FIELD_NOT_SET(_field, _items)             \
    {                                                 \
        QTreeWidgetItem *_item = new QTreeWidgetItem; \
        _item->setText(0, QString(_field));           \
        _item->setText(1, QString("Not Set"));        \
        _items.append(_item);                         \
    }

#define ADD_FIELD_TYPE_STRING(_field, _string, _prev_field_set, _prev_string, _items) \
    {                                                                                 \
        QTreeWidgetItem *_item = new QTreeWidgetItem;                                 \
        _item->setText(0, QString(_field));                                           \
        _item->setText(1, _string);                                                   \
        if (!prev_event_state_it->IsValid() || !_prev_field_set ||                    \
            QString::compare(_prev_string, _string) != 0)                             \
            _item->setForeground(1, QBrush(m_accent_color));                          \
        _items.append(_item);                                                         \
    }

#define ADD_FIELD_TYPE_ENUM(_field, _to_string, _num, _prev_field_set, _prev_num, _items) \
    {                                                                                     \
        QTreeWidgetItem *_item = new QTreeWidgetItem;                                     \
        _item->setText(0, QString(_field));                                               \
        _item->setText(1, QString(_to_string(_num)));                                     \
        if (!prev_event_state_it->IsValid() || !_prev_field_set || _prev_num != _num)     \
            _item->setForeground(1, QBrush(m_accent_color));                              \
        _items.append(_item);                                                             \
    }

#define ADD_FIELD_TYPE_NUMBER(_field, _num, _prev_field_set, _prev_num, _items)       \
    {                                                                                 \
        QTreeWidgetItem *_item = new QTreeWidgetItem;                                 \
        _item->setText(0, QString(_field));                                           \
        _item->setText(1, QString::number(_num));                                     \
        if (!prev_event_state_it->IsValid() || !_prev_field_set || _prev_num != _num) \
            _item->setForeground(1, QBrush(m_accent_color));                          \
        _items.append(_item);                                                         \
    }

#define ADD_FIELD_TYPE_NUMBER_HEX(_field, _num, _prev_field_set, _prev_num, _items)   \
    {                                                                                 \
        QTreeWidgetItem *_item = new QTreeWidgetItem;                                 \
        _item->setText(0, QString(_field));                                           \
        _item->setText(1, "0x" + QString::number(static_cast<uint32_t>(_num), 16));   \
        if (!prev_event_state_it->IsValid() || !_prev_field_set || _prev_num != _num) \
            _item->setForeground(1, QBrush(m_accent_color));                          \
        _items.append(_item);                                                         \
    }

#define ADD_FIELD_TYPE_NUMBER_HEX64(_field, _num, _prev_field_set, _prev_num, _items) \
    {                                                                                 \
        QTreeWidgetItem *_item = new QTreeWidgetItem;                                 \
        _item->setText(0, QString(_field));                                           \
        _item->setText(1, "0x" + QString::number(static_cast<uint64_t>(_num), 16));   \
        if (!prev_event_state_it->IsValid() || !_prev_field_set || _prev_num != _num) \
            _item->setForeground(1, QBrush(QColor(Qt::cyan)));                        \
        _items.append(_item);                                                         \
    }

#define ADD_FIELD_TYPE_BOOL(_field, _bool, _prev_field_set, _prev_bool, _items)         \
    {                                                                                   \
        QTreeWidgetItem *_item = new QTreeWidgetItem;                                   \
        _item->setText(0, QString(_field));                                             \
        _item->setText(1, (_bool ? "true" : "false"));                                  \
        if (!prev_event_state_it->IsValid() || !_prev_field_set || _prev_bool != _bool) \
            _item->setForeground(1, QBrush(m_accent_color));                            \
        _items.append(_item);                                                           \
    }

#define ADD_TREE_BRANCH(branch_name)                                              \
    QTreeWidgetItem *_tree_widget_item = new QTreeWidgetItem(m_event_state_tree); \
    _tree_widget_item->setText(0, branch_name);                                   \
    _tree_widget_item->insertChildren(0, items);

#define ADD_FIELD_DESC(_field, _desc)                    \
    if (m_field_desc.find(_field) == m_field_desc.end()) \
        m_field_desc[_field] = _desc;

// Has to use macro here since prev_event_state_it could be potentially invalid
// ADD_FIELD_TYPE_STRING checks if prev_event_state_it is invalid and delays the evaluation of the
// content from prev_event_state_it
#define ADD_FIELD_STENCIL(_name, _cur, _pre, _is_pre_set, _items)                                 \
    { ADD_FIELD_TYPE_STRING((_name + "_FailOp").c_str(),                                          \
                            GetVkStencilOp(_cur.failOp),                                          \
                            _is_pre_set,                                                          \
                            GetVkStencilOp(_pre.failOp),                                          \
                            _items) ADD_FIELD_TYPE_STRING((_name + "_PassOp").c_str(),            \
                                                          GetVkStencilOp(_cur.passOp),            \
                                                          _is_pre_set,                            \
                                                          GetVkStencilOp(_pre.passOp),            \
                                                          _items)                                 \
      ADD_FIELD_TYPE_STRING((_name + "_DepthFailOp").c_str(),                                     \
                            GetVkStencilOp(_cur.depthFailOp),                                     \
                            _is_pre_set,                                                          \
                            GetVkStencilOp(_pre.depthFailOp),                                     \
                            _items) ADD_FIELD_TYPE_STRING((_name + "_CompareOp").c_str(),         \
                                                          GetVkCompareOp(_cur.compareOp),         \
                                                          _is_pre_set,                            \
                                                          GetVkCompareOp(_pre.compareOp),         \
                                                          _items)                                 \
      ADD_FIELD_TYPE_NUMBER_HEX((_name + "_CompareMask").c_str(),                                 \
                                _cur.compareMask,                                                 \
                                _is_pre_set,                                                      \
                                _pre.compareMask,                                                 \
                                _items) ADD_FIELD_TYPE_NUMBER_HEX((_name + "_WriteMask").c_str(), \
                                                                  _cur.writeMask,                 \
                                                                  _is_pre_set,                    \
                                                                  _pre.writeMask,                 \
                                                                  _items)                         \
      ADD_FIELD_TYPE_NUMBER_HEX((_name + "_Reference").c_str(),                                   \
                                _cur.reference,                                                   \
                                _is_pre_set,                                                      \
                                _pre.reference,                                                   \
                                _items) }

// =================================================================================================
// EventStateView
// =================================================================================================
EventStateView::EventStateView(const Dive::DataCore &data_core) :
    m_data_core(data_core)
{
    QVBoxLayout *layout = new QVBoxLayout();
    m_event_state_tree = new QTreeWidget();
    m_event_state_tree->setColumnCount(2);
    m_event_state_tree->setHeaderLabels(QStringList() << " "
                                                      << " ");
    m_event_state_tree->setAlternatingRowColors(true);
    m_event_state_tree->setMouseTracking(true);
    m_event_state_tree->setAutoScroll(false);
    m_event_state_tree->viewport()->setAttribute(Qt::WA_Hover);

    layout->addWidget(m_event_state_tree);
    layout->setStretchFactor(m_event_state_tree, 1);
    setLayout(layout);

    QObject::connect(m_event_state_tree,
                     SIGNAL(itemEntered(QTreeWidgetItem *, int)),
                     this,
                     SLOT(OnHover(QTreeWidgetItem *, int)));
}

//--------------------------------------------------------------------------------------------------
void EventStateView::OnEventSelected(uint64_t node_index)
{
    m_event_state_tree->clear();
    if (node_index == UINT64_MAX)
        return;

    m_accent_color = GetTextAccentColor();

    auto &metadata = m_data_core.GetCaptureMetadata();
    auto &command_hierarchy = m_data_core.GetCommandHierarchy();
    auto &event_state = metadata.m_event_state;

    auto previous_event_state = [&](auto it, auto IsType) {
        auto prev_it = std::prev(it);
        while (prev_it->IsValid())
        {
            auto id = static_cast<Dive::EventStateId>(prev_it->id());
            // Check if it's draw or dispatch events
            const Dive::EventInfo &prev_event_info = metadata
                                                     .m_event_info[static_cast<uint32_t>(id)];
            if (IsType(prev_event_info.m_type))
                break;
            else
                prev_it = std::prev(prev_it);
        }
        return prev_it;
    };

    Dive::NodeType node_type = command_hierarchy.GetNodeType(node_index);
    if (node_type == Dive::NodeType::kEventNode)
    {
        auto event_id = m_data_core.GetCommandHierarchy().GetEventNodeId(node_index);
        // Check if it's draw or dispatch events
        if (!(event_id < metadata.m_event_info.size()))
            return;
        const Dive::EventInfo &event_info = metadata.m_event_info[event_id];
        if (event_info.m_type == Dive::Util::EventType::kDraw)
        {
            auto event_state_it = GetStateInfoForEvent(event_state, event_id);
            auto prev_event_state_it = previous_event_state(event_state_it,
                                                            [](Dive::Util::EventType type) {
                                                                return type ==
                                                                       Dive::Util::EventType::kDraw;
                                                            });
            DisplayDrawEventStateInfo(event_state_it, prev_event_state_it);
        }
        else if (Dive::EventInfo::IsResolve(event_info.m_type))
        {
            auto event_state_it = GetStateInfoForEvent(event_state, event_id);
            auto prev_event_state_it = previous_event_state(event_state_it,
                                                            [](Dive::Util::EventType type) {
                                                                return Dive::EventInfo::IsResolve(
                                                                type);
                                                            });
            DisplayResolveState(event_state_it, prev_event_state_it);
            DisplayResolveGmemInfo(event_state_it, prev_event_state_it);
            DisplayResolveSysmemInfo(event_state_it, prev_event_state_it);
        }
        else if (Dive::EventInfo::IsGmemClear(event_info.m_type))
        {
            auto event_state_it = GetStateInfoForEvent(event_state, event_id);
            auto prev_event_state_it = previous_event_state(event_state_it,
                                                            [](Dive::Util::EventType type) {
                                                                return Dive::EventInfo::IsGmemClear(
                                                                type);
                                                            });
            DisplayResolveState(event_state_it, prev_event_state_it);
            DisplayResolveGmemInfo(event_state_it, prev_event_state_it);
        }
    }

    // Resize columns to fit
    m_event_state_tree->expandAll();
    uint32_t column_count = (uint32_t)m_event_state_tree->columnCount();
    for (uint32_t column = 0; column < column_count; ++column)
        m_event_state_tree->resizeColumnToContents(column);
}

Dive::EventStateInfo::ConstIterator EventStateView::GetStateInfoForEvent(
const Dive::EventStateInfo &state,
uint32_t                    event_id)
{
    return state.find(static_cast<Dive::EventStateId>(event_id));
}

//--------------------------------------------------------------------------------------------------
void EventStateView::DisplayDrawEventStateInfo(
Dive::EventStateInfo::ConstIterator event_state_it,
Dive::EventStateInfo::ConstIterator prev_event_state_it)
{
    // Build up map to lookup description
    BuildDrawDescriptionMap(event_state_it);

    // Vulkan states
    DisplayInputAssemblyState(event_state_it, prev_event_state_it);
    DisplayTessellationState(event_state_it, prev_event_state_it);
    DisplayFillViewportState(event_state_it, prev_event_state_it);
    DisplayRasterizerState(event_state_it, prev_event_state_it);
    DisplayFillMultisamplingState(event_state_it, prev_event_state_it);
    DisplayColorBlendState(event_state_it, prev_event_state_it);
    DisplayDepthState(event_state_it, prev_event_state_it);
    DisplayStencilState(event_state_it, prev_event_state_it);

    // Hardware-specific non-Vulkan states
    DisplayHardwareSpecificStates(event_state_it, prev_event_state_it);
}

//--------------------------------------------------------------------------------------------------
void EventStateView::BuildDrawDescriptionMap(Dive::EventStateInfo::ConstIterator event_state_it)
{
    if (m_field_desc.size())
        return;

    ADD_FIELD_DESC(event_state_it->GetTopologyName(), event_state_it->GetTopologyDescription());
    ADD_FIELD_DESC(event_state_it->GetPrimRestartEnabledName(),
                   event_state_it->GetPrimRestartEnabledDescription());
    ADD_FIELD_DESC(event_state_it->GetPatchControlPointsName(),
                   event_state_it->GetPatchControlPointsDescription());
    ADD_FIELD_DESC(event_state_it->GetViewportName(), event_state_it->GetViewportDescription());
    ADD_FIELD_DESC(event_state_it->GetScissorName(), event_state_it->GetScissorDescription());
    ADD_FIELD_DESC(event_state_it->GetDepthClampEnabledName(),
                   event_state_it->GetDepthClampEnabledDescription());
    ADD_FIELD_DESC(event_state_it->GetRasterizerDiscardEnabledName(),
                   event_state_it->GetRasterizerDiscardEnabledDescription());
    ADD_FIELD_DESC(event_state_it->GetPolygonModeName(),
                   event_state_it->GetPolygonModeDescription());
    ADD_FIELD_DESC(event_state_it->GetCullModeName(), event_state_it->GetCullModeDescription());
    ADD_FIELD_DESC(event_state_it->GetFrontFaceName(), event_state_it->GetFrontFaceDescription());
    ADD_FIELD_DESC(event_state_it->GetDepthBiasEnabledName(),
                   event_state_it->GetDepthBiasEnabledDescription());
    ADD_FIELD_DESC(event_state_it->GetDepthBiasConstantFactorName(),
                   event_state_it->GetDepthBiasConstantFactorDescription());
    ADD_FIELD_DESC(event_state_it->GetDepthBiasClampName(),
                   event_state_it->GetDepthBiasClampDescription());
    ADD_FIELD_DESC(event_state_it->GetDepthBiasSlopeFactorName(),
                   event_state_it->GetDepthBiasSlopeFactorDescription());
    ADD_FIELD_DESC(event_state_it->GetLineWidthName(), event_state_it->GetLineWidthDescription());
    ADD_FIELD_DESC(event_state_it->GetRasterizationSamplesName(),
                   event_state_it->GetRasterizationSamplesDescription());
    ADD_FIELD_DESC(event_state_it->GetSampleShadingEnabledName(),
                   event_state_it->GetSampleShadingEnabledDescription());
    ADD_FIELD_DESC(event_state_it->GetMinSampleShadingName(),
                   event_state_it->GetMinSampleShadingDescription());
    ADD_FIELD_DESC(event_state_it->GetSampleMaskName(), event_state_it->GetSampleMaskDescription());
    ADD_FIELD_DESC(event_state_it->GetAlphaToCoverageEnabledName(),
                   event_state_it->GetAlphaToCoverageEnabledDescription());
    ADD_FIELD_DESC(event_state_it->GetDepthTestEnabledName(),
                   event_state_it->GetDepthTestEnabledDescription());
    ADD_FIELD_DESC(event_state_it->GetDepthWriteEnabledName(),
                   event_state_it->GetDepthWriteEnabledDescription());
    ADD_FIELD_DESC(event_state_it->GetDepthCompareOpName(),
                   event_state_it->GetDepthCompareOpDescription());
    ADD_FIELD_DESC(event_state_it->GetDepthBoundsTestEnabledName(),
                   event_state_it->GetDepthBoundsTestEnabledDescription());
    ADD_FIELD_DESC(event_state_it->GetStencilTestEnabledName(),
                   event_state_it->GetStencilTestEnabledDescription());
    ADD_FIELD_DESC(event_state_it->GetStencilOpStateFrontName(),
                   event_state_it->GetStencilOpStateFrontDescription());
    ADD_FIELD_DESC(event_state_it->GetStencilOpStateBackName(),
                   event_state_it->GetStencilOpStateBackDescription());
    ADD_FIELD_DESC(event_state_it->GetMinDepthBoundsName(),
                   event_state_it->GetMinDepthBoundsDescription());
    ADD_FIELD_DESC(event_state_it->GetMaxDepthBoundsName(),
                   event_state_it->GetMaxDepthBoundsDescription());
    ADD_FIELD_DESC(event_state_it->GetLogicOpEnabledName(),
                   event_state_it->GetLogicOpEnabledDescription());
    ADD_FIELD_DESC(event_state_it->GetLogicOpName(), event_state_it->GetLogicOpDescription());
    ADD_FIELD_DESC(event_state_it->GetAttachmentName(), event_state_it->GetAttachmentDescription());
    ADD_FIELD_DESC(event_state_it->GetBlendConstantName(),
                   event_state_it->GetBlendConstantDescription());
    ADD_FIELD_DESC(event_state_it->GetLRZEnabledName(), event_state_it->GetLRZEnabledDescription());
    ADD_FIELD_DESC(event_state_it->GetLRZWriteName(), event_state_it->GetLRZWriteDescription());
    ADD_FIELD_DESC(event_state_it->GetLRZDirStatusName(),
                   event_state_it->GetLRZDirStatusDescription());
    ADD_FIELD_DESC(event_state_it->GetLRZDirWriteName(),
                   event_state_it->GetLRZDirWriteDescription());
    ADD_FIELD_DESC(event_state_it->GetZTestModeName(), event_state_it->GetZTestModeDescription());
    ADD_FIELD_DESC(event_state_it->GetBinWName(), event_state_it->GetBinWDescription());
    ADD_FIELD_DESC(event_state_it->GetBinHName(), event_state_it->GetBinHDescription());
    ADD_FIELD_DESC(event_state_it->GetRenderModeName(), event_state_it->GetRenderModeDescription());
    ADD_FIELD_DESC(event_state_it->GetBuffersLocationName(),
                   event_state_it->GetBuffersLocationDescription());
    ADD_FIELD_DESC(event_state_it->GetThreadSizeName(), event_state_it->GetThreadSizeDescription());
    ADD_FIELD_DESC(event_state_it->GetEnableAllHelperLanesName(),
                   event_state_it->GetEnableAllHelperLanesDescription());
    ADD_FIELD_DESC(event_state_it->GetEnablePartialHelperLanesName(),
                   event_state_it->GetEnablePartialHelperLanesDescription());
}

//--------------------------------------------------------------------------------------------------
void EventStateView::DisplayResolveState(Dive::EventStateInfo::ConstIterator event_state_it,
                                         Dive::EventStateInfo::ConstIterator prev_event_state_it)
{
    BuildResolveDescriptionMap(event_state_it);

    QList<QTreeWidgetItem *> items;

    // ResolveScissor
    if (event_state_it->IsResolveScissorSet())
    {
        QString  scissor_child;
        VkRect2D rect = event_state_it->ResolveScissor();
        scissor_child = "x: " + QString::number(rect.offset.x) +
                        ", y: " + QString::number(rect.offset.y) +
                        ", width: " + QString::number(rect.extent.width) +
                        ", height: " + QString::number(rect.extent.height);

        QString  prev_scissor_child;
        VkRect2D prev_rect;
        if (prev_event_state_it->IsValid())
        {
            prev_rect = prev_event_state_it->ResolveScissor();
            prev_scissor_child = "x: " + QString::number(prev_rect.offset.x) +
                                 ", y: " + QString::number(prev_rect.offset.y) +
                                 ", width: " + QString::number(prev_rect.extent.width) +
                                 ", height: " + QString::number(prev_rect.extent.height);
        }

        ADD_FIELD_TYPE_STRING(event_state_it->GetResolveScissorName(),
                              scissor_child,
                              prev_event_state_it->IsResolveScissorSet(),
                              prev_scissor_child,
                              items);
    }
    else
        ADD_FIELD_NOT_SET(event_state_it->GetResolveScissorName(), items);

    ADD_TREE_BRANCH("Resolve Properties")
}

//--------------------------------------------------------------------------------------------------
void EventStateView::BuildResolveDescriptionMap(Dive::EventStateInfo::ConstIterator event_state_it)
{
    if (m_field_desc.size())
        return;

    ADD_FIELD_DESC(event_state_it->GetResolveScissorName(),
                   event_state_it->GetResolveScissorDescription());
}

//--------------------------------------------------------------------------------------------------
void EventStateView::DisplayResolveGmemInfo(Dive::EventStateInfo::ConstIterator event_state_it,
                                            Dive::EventStateInfo::ConstIterator prev_event_state_it)
{
    BuildResolveGmemDescriptionMap(event_state_it);

    QList<QTreeWidgetItem *> items;

    // ResolveBaseGmem
    if (event_state_it->IsResolveBaseGmemSet())
    {
        ADD_FIELD_TYPE_NUMBER_HEX(event_state_it->GetResolveBaseGmemName(),
                                  event_state_it->ResolveBaseGmem(),
                                  prev_event_state_it->IsResolveBaseGmemSet(),
                                  prev_event_state_it->ResolveBaseGmem(),
                                  items);
    }
    else
        ADD_FIELD_NOT_SET(event_state_it->GetResolveBaseGmemName(), items);

    ADD_TREE_BRANCH("Gmem")
}

//--------------------------------------------------------------------------------------------------
void EventStateView::BuildResolveGmemDescriptionMap(
Dive::EventStateInfo::ConstIterator event_state_it)
{
    ADD_FIELD_DESC(event_state_it->GetResolveBaseGmemName(),
                   event_state_it->GetResolveBaseGmemDescription());
}

//--------------------------------------------------------------------------------------------------
void EventStateView::DisplayResolveSysmemInfo(
Dive::EventStateInfo::ConstIterator event_state_it,
Dive::EventStateInfo::ConstIterator prev_event_state_it)
{
    BuildResolveSysmemDescriptionMap(event_state_it);

    QList<QTreeWidgetItem *> items;

    // ResolveBaseSysmem
    if (event_state_it->IsResolveBaseSysmemSet())
    {
        ADD_FIELD_TYPE_NUMBER_HEX64(event_state_it->GetResolveBaseSysmemName(),
                                    event_state_it->ResolveBaseSysmem(),
                                    prev_event_state_it->IsResolveBaseSysmemSet(),
                                    prev_event_state_it->ResolveBaseSysmem(),
                                    items);
    }
    else
        ADD_FIELD_NOT_SET(event_state_it->GetResolveBaseSysmemName(), items);

    ADD_TREE_BRANCH("Sysmem")
}

//--------------------------------------------------------------------------------------------------
void EventStateView::BuildResolveSysmemDescriptionMap(
Dive::EventStateInfo::ConstIterator event_state_it)
{
    ADD_FIELD_DESC(event_state_it->GetResolveBaseSysmemName(),
                   event_state_it->GetResolveBaseSysmemDescription());
}

//--------------------------------------------------------------------------------------------------
void EventStateView::DisplayInputAssemblyState(
Dive::EventStateInfo::ConstIterator event_state_it,
Dive::EventStateInfo::ConstIterator prev_event_state_it)
{
    QList<QTreeWidgetItem *> items;

    // Topology
    if (event_state_it->IsTopologySet())
        ADD_FIELD_TYPE_STRING(event_state_it->GetTopologyName(),
                              GetVkPrimitiveTopology(event_state_it->Topology()),
                              prev_event_state_it->IsTopologySet(),
                              GetVkPrimitiveTopology(prev_event_state_it->Topology()),
                              items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetTopologyName(), items)

    // PrimRestartEnabled
    if (event_state_it->IsPrimRestartEnabledSet())
        ADD_FIELD_TYPE_BOOL(event_state_it->GetPrimRestartEnabledName(),
                            event_state_it->PrimRestartEnabled(),
                            prev_event_state_it->IsPrimRestartEnabledSet(),
                            prev_event_state_it->PrimRestartEnabled(),
                            items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetPrimRestartEnabledName(), items)

    ADD_TREE_BRANCH("Input Assembly")
}

//--------------------------------------------------------------------------------------------------
void EventStateView::DisplayTessellationState(
Dive::EventStateInfo::ConstIterator event_state_it,
Dive::EventStateInfo::ConstIterator prev_event_state_it)
{
    QList<QTreeWidgetItem *> items;

    // PatchControlPoints
    if (event_state_it->IsPatchControlPointsSet())
        ADD_FIELD_TYPE_NUMBER(event_state_it->GetPatchControlPointsName(),
                              event_state_it->PatchControlPoints(),
                              prev_event_state_it->IsPatchControlPointsSet(),
                              prev_event_state_it->PatchControlPoints(),
                              items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetPatchControlPointsName(), items)

    ADD_TREE_BRANCH("Tessellation")
}

//--------------------------------------------------------------------------------------------------
void EventStateView::DisplayRasterizerState(Dive::EventStateInfo::ConstIterator event_state_it,
                                            Dive::EventStateInfo::ConstIterator prev_event_state_it)
{
    QList<QTreeWidgetItem *> items;

    // DepthClampEnabled
    if (event_state_it->IsDepthClampEnabledSet())
        ADD_FIELD_TYPE_BOOL(event_state_it->GetDepthClampEnabledName(),
                            event_state_it->DepthClampEnabled(),
                            prev_event_state_it->IsDepthClampEnabledSet(),
                            prev_event_state_it->DepthClampEnabled(),
                            items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetDepthClampEnabledName(), items)

    // RasterizerDiscardEnabled
    if (event_state_it->IsRasterizerDiscardEnabledSet())
        ADD_FIELD_TYPE_BOOL(event_state_it->GetRasterizerDiscardEnabledName(),
                            event_state_it->RasterizerDiscardEnabled(),
                            prev_event_state_it->IsRasterizerDiscardEnabledSet(),
                            prev_event_state_it->RasterizerDiscardEnabled(),
                            items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetRasterizerDiscardEnabledName(), items)

    // PolygonMode
    if (event_state_it->IsPolygonModeSet())
        ADD_FIELD_TYPE_STRING(event_state_it->GetPolygonModeName(),
                              GetVkPolygonMode(event_state_it->PolygonMode()),
                              prev_event_state_it->IsPolygonModeSet(),
                              GetVkPolygonMode(prev_event_state_it->PolygonMode()),
                              items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetPolygonModeName(), items)

    // CullMode
    if (event_state_it->IsCullModeSet())
        ADD_FIELD_TYPE_STRING(event_state_it->GetCullModeName(),
                              GetVkCullModeFlags(event_state_it->CullMode()),
                              prev_event_state_it->IsCullModeSet(),
                              GetVkCullModeFlags(prev_event_state_it->CullMode()),
                              items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetCullModeName(), items)

    // FrontFace
    if (event_state_it->IsFrontFaceSet())
        ADD_FIELD_TYPE_STRING(event_state_it->GetFrontFaceName(),
                              GetVkFrontFace(event_state_it->FrontFace()),
                              prev_event_state_it->IsFrontFaceSet(),
                              GetVkFrontFace(prev_event_state_it->FrontFace()),
                              items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetFrontFaceName(), items)

    // DepthBiasEnabled
    if (event_state_it->IsDepthBiasEnabledSet())
        ADD_FIELD_TYPE_BOOL(event_state_it->GetDepthBiasEnabledName(),
                            event_state_it->DepthBiasEnabled(),
                            prev_event_state_it->IsDepthBiasEnabledSet(),
                            prev_event_state_it->DepthBiasEnabled(),
                            items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetDepthBiasEnabledName(), items)

    // DepthBiasConstantFactor
    if (event_state_it->IsDepthBiasConstantFactorSet())
        ADD_FIELD_TYPE_NUMBER(event_state_it->GetDepthBiasConstantFactorName(),
                              event_state_it->DepthBiasConstantFactor(),
                              prev_event_state_it->IsDepthBiasConstantFactorSet(),
                              prev_event_state_it->DepthBiasConstantFactor(),
                              items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetDepthBiasConstantFactorName(), items)

    // DepthBiasClamp
    if (event_state_it->IsDepthBiasClampSet())
        ADD_FIELD_TYPE_NUMBER(event_state_it->GetDepthBiasClampName(),
                              event_state_it->DepthBiasClamp(),
                              prev_event_state_it->IsDepthBiasClampSet(),
                              prev_event_state_it->DepthBiasClamp(),
                              items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetDepthBiasClampName(), items)

    // DepthBiasSlopeFactor
    if (event_state_it->IsDepthBiasSlopeFactorSet())
        ADD_FIELD_TYPE_NUMBER(event_state_it->GetDepthBiasSlopeFactorName(),
                              event_state_it->DepthBiasSlopeFactor(),
                              prev_event_state_it->IsDepthBiasSlopeFactorSet(),
                              prev_event_state_it->DepthBiasSlopeFactor(),
                              items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetDepthBiasSlopeFactorName(), items)

    // LineWidth
    if (event_state_it->IsLineWidthSet())
        ADD_FIELD_TYPE_NUMBER(event_state_it->GetLineWidthName(),
                              event_state_it->LineWidth(),
                              prev_event_state_it->IsLineWidthSet(),
                              prev_event_state_it->LineWidth(),
                              items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetLineWidthName(), items)

    ADD_TREE_BRANCH("Rasterizer")
}

//--------------------------------------------------------------------------------------------------
void EventStateView::DisplayFillMultisamplingState(
Dive::EventStateInfo::ConstIterator event_state_it,
Dive::EventStateInfo::ConstIterator prev_event_state_it)
{
    QList<QTreeWidgetItem *> items;

    // RasterizationSamples
    if (event_state_it->IsRasterizationSamplesSet())
        ADD_FIELD_TYPE_STRING(event_state_it->GetRasterizationSamplesName(),
                              GetVkSampleCountFlags(event_state_it->RasterizationSamples()),
                              prev_event_state_it->IsRasterizationSamplesSet(),
                              GetVkSampleCountFlags(prev_event_state_it->RasterizationSamples()),
                              items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetRasterizationSamplesName(), items)

    // SampleShadingEnabled
    if (event_state_it->IsSampleShadingEnabledSet())
        ADD_FIELD_TYPE_BOOL(event_state_it->GetSampleShadingEnabledName(),
                            event_state_it->SampleShadingEnabled(),
                            prev_event_state_it->IsSampleShadingEnabledSet(),
                            prev_event_state_it->SampleShadingEnabled(),
                            items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetSampleShadingEnabledName(), items)

    // MinSampleShading
    if (event_state_it->IsMinSampleShadingSet())
        ADD_FIELD_TYPE_NUMBER(event_state_it->GetMinSampleShadingName(),
                              event_state_it->MinSampleShading(),
                              prev_event_state_it->IsMinSampleShadingSet(),
                              prev_event_state_it->MinSampleShading(),
                              items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetMinSampleShadingName(), items)

    // SampleMask
    if (event_state_it->IsSampleMaskSet())
        ADD_FIELD_TYPE_NUMBER(event_state_it->GetSampleMaskName(),
                              event_state_it->SampleMask(),
                              prev_event_state_it->IsSampleMaskSet(),
                              prev_event_state_it->SampleMask(),
                              items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetSampleMaskName(), items)

    // AlphaToCoverageEnabled
    if (event_state_it->IsAlphaToCoverageEnabledSet())
        ADD_FIELD_TYPE_BOOL(event_state_it->GetAlphaToCoverageEnabledName(),
                            event_state_it->AlphaToCoverageEnabled(),
                            prev_event_state_it->IsAlphaToCoverageEnabledSet(),
                            prev_event_state_it->AlphaToCoverageEnabled(),
                            items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetAlphaToCoverageEnabledName(), items)

    ADD_TREE_BRANCH("Msaa")
}

//--------------------------------------------------------------------------------------------------
void EventStateView::DisplayFillViewportState(
Dive::EventStateInfo::ConstIterator event_state_it,
Dive::EventStateInfo::ConstIterator prev_event_state_it)
{
    QList<QTreeWidgetItem *> items;

    // Viewport
    {
        QList<QTreeWidgetItem *> child_items;
        for (uint16_t viewport_id = 0; viewport_id < 16; ++viewport_id)
        {
            if (event_state_it->IsViewportSet(viewport_id))
            {
                QString    vp_child;
                VkViewport vp = event_state_it->Viewport(viewport_id);
                vp_child = "x: " + QString::number(vp.x) + ", y: " + QString::number(vp.y) +
                           ", width: " + QString::number(vp.width) +
                           ", height: " + QString::number(vp.height) +
                           ", minDepth: " + QString::number(vp.minDepth) +
                           ", maxDepth: " + QString::number(vp.maxDepth);

                QString    prev_vp_child;
                VkViewport prev_vp;
                if (prev_event_state_it->IsValid())
                {
                    prev_vp = prev_event_state_it->Viewport(viewport_id);
                    prev_vp_child = "x: " + QString::number(prev_vp.x) +
                                    ", y: " + QString::number(prev_vp.y) +
                                    ", width: " + QString::number(prev_vp.width) +
                                    ", height: " + QString::number(prev_vp.height) +
                                    ", minDepth: " + QString::number(prev_vp.minDepth) +
                                    ", maxDepth: " + QString::number(prev_vp.maxDepth);
                }

                ADD_FIELD_TYPE_STRING(QString::number(viewport_id),
                                      vp_child,
                                      prev_event_state_it->IsViewportSet(viewport_id),
                                      prev_vp_child,
                                      child_items);
            }
            else
                ADD_FIELD_NOT_SET(QString::number(viewport_id), child_items);
        }

        QTreeWidgetItem *child_widget_item = new QTreeWidgetItem((QTreeWidget *)0,
                                                                 QStringList(event_state_it
                                                                             ->GetViewportName()));
        child_widget_item->insertChildren(0, child_items);
        items.append(child_widget_item);
    }

    // Scissor
    {
        QList<QTreeWidgetItem *> child_items;
        for (uint16_t scissor_id = 0; scissor_id < 16; ++scissor_id)
        {
            if (event_state_it->IsScissorSet(scissor_id))
            {
                QString  scissor_child;
                VkRect2D rect = event_state_it->Scissor(scissor_id);
                scissor_child = "x: " + QString::number(rect.offset.x) +
                                ", y: " + QString::number(rect.offset.y) +
                                ", width: " + QString::number(rect.extent.width) +
                                ", height: " + QString::number(rect.extent.height);

                QString  prev_scissor_child;
                VkRect2D prev_rect;
                if (prev_event_state_it->IsValid())
                {
                    prev_rect = prev_event_state_it->Scissor(scissor_id);
                    prev_scissor_child = "x: " + QString::number(prev_rect.offset.x) +
                                         ", y: " + QString::number(prev_rect.offset.y) +
                                         ", width: " + QString::number(prev_rect.extent.width) +
                                         ", height: " + QString::number(prev_rect.extent.height);
                }

                ADD_FIELD_TYPE_STRING(QString::number(scissor_id),
                                      scissor_child,
                                      prev_event_state_it->IsScissorSet(scissor_id),
                                      prev_scissor_child,
                                      child_items);
            }
            else
                ADD_FIELD_NOT_SET(QString::number(scissor_id), child_items);
        }

        QTreeWidgetItem *child_widget_item = new QTreeWidgetItem((QTreeWidget *)0,
                                                                 QStringList(
                                                                 event_state_it->GetScissorName()));
        child_widget_item->insertChildren(0, child_items);
        items.append(child_widget_item);
    }

    ADD_TREE_BRANCH("Viewport")
}

//--------------------------------------------------------------------------------------------------
void EventStateView::DisplayDepthState(Dive::EventStateInfo::ConstIterator event_state_it,
                                       Dive::EventStateInfo::ConstIterator prev_event_state_it)
{
    QList<QTreeWidgetItem *> items;

    // DepthTestEnabled
    if (event_state_it->IsDepthTestEnabledSet())
        ADD_FIELD_TYPE_BOOL(event_state_it->GetDepthTestEnabledName(),
                            event_state_it->DepthTestEnabled(),
                            prev_event_state_it->IsDepthTestEnabledSet(),
                            prev_event_state_it->DepthTestEnabled(),
                            items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetDepthTestEnabledName(), items)

    // DepthWriteEnabled
    if (event_state_it->IsDepthWriteEnabledSet())
        ADD_FIELD_TYPE_BOOL(event_state_it->GetDepthWriteEnabledName(),
                            event_state_it->DepthWriteEnabled(),
                            prev_event_state_it->IsDepthWriteEnabledSet(),
                            prev_event_state_it->DepthWriteEnabled(),
                            items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetDepthWriteEnabledName(), items)

    // DepthCompareOp
    if (event_state_it->IsDepthCompareOpSet())
        ADD_FIELD_TYPE_STRING(event_state_it->GetDepthCompareOpName(),
                              GetVkCompareOp(event_state_it->DepthCompareOp()),
                              prev_event_state_it->IsDepthCompareOpSet(),
                              GetVkCompareOp(prev_event_state_it->DepthCompareOp()),
                              items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetDepthCompareOpName(), items)

    // DepthBoundsTestEnabled
    if (event_state_it->IsDepthBoundsTestEnabledSet())
        ADD_FIELD_TYPE_BOOL(event_state_it->GetDepthBoundsTestEnabledName(),
                            event_state_it->DepthBoundsTestEnabled(),
                            prev_event_state_it->IsDepthBoundsTestEnabledSet(),
                            prev_event_state_it->DepthBoundsTestEnabled(),
                            items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetDepthBoundsTestEnabledName(), items)

    // MinDepthBounds
    if (event_state_it->IsMinDepthBoundsSet())
        ADD_FIELD_TYPE_NUMBER(event_state_it->GetMinDepthBoundsName(),
                              event_state_it->MinDepthBounds(),
                              prev_event_state_it->IsMinDepthBoundsSet(),
                              prev_event_state_it->MinDepthBounds(),
                              items)

    else
        ADD_FIELD_NOT_SET(event_state_it->GetMinDepthBoundsName(), items)

    // MaxDepthBounds
    if (event_state_it->IsMaxDepthBoundsSet())
        ADD_FIELD_TYPE_NUMBER(event_state_it->GetMaxDepthBoundsName(),
                              event_state_it->MaxDepthBounds(),
                              prev_event_state_it->IsMaxDepthBoundsSet(),
                              prev_event_state_it->MaxDepthBounds(),
                              items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetMaxDepthBoundsName(), items)

    ADD_TREE_BRANCH("Depth")
}

//--------------------------------------------------------------------------------------------------
void EventStateView::DisplayStencilState(Dive::EventStateInfo::ConstIterator event_state_it,
                                         Dive::EventStateInfo::ConstIterator prev_event_state_it)
{
    QList<QTreeWidgetItem *> items;

    // StencilTestEnabled
    if (event_state_it->IsStencilTestEnabledSet())
        ADD_FIELD_TYPE_BOOL(event_state_it->GetStencilTestEnabledName(),
                            event_state_it->StencilTestEnabled(),
                            prev_event_state_it->IsStencilTestEnabledSet(),
                            prev_event_state_it->StencilTestEnabled(),
                            items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetStencilTestEnabledName(), items)

    // Stencil Front
    if (event_state_it->IsStencilOpStateFrontSet())
    {
        ADD_FIELD_STENCIL(std::string(event_state_it->GetStencilOpStateFrontName()),
                          event_state_it->StencilOpStateFront(),
                          prev_event_state_it->StencilOpStateFront(),
                          prev_event_state_it->IsStencilOpStateFrontSet(),
                          items)
    }
    else
        ADD_FIELD_NOT_SET(event_state_it->GetStencilOpStateFrontName(), items)

    // Stencil Back
    if (event_state_it->IsStencilOpStateBackSet())
    {
        ADD_FIELD_STENCIL(std::string(event_state_it->GetStencilOpStateBackName()),
                          event_state_it->StencilOpStateBack(),
                          prev_event_state_it->StencilOpStateBack(),
                          prev_event_state_it->IsStencilOpStateBackSet(),
                          items)
    }
    else
        ADD_FIELD_NOT_SET(event_state_it->GetStencilOpStateBackName(), items)

    ADD_TREE_BRANCH("Stencil")
}

//--------------------------------------------------------------------------------------------------
void EventStateView::DisplayColorBlendState(Dive::EventStateInfo::ConstIterator event_state_it,
                                            Dive::EventStateInfo::ConstIterator prev_event_state_it)
{
    QList<QTreeWidgetItem *> items;

    // Attachment
    {
        QList<QTreeWidgetItem *> child_items;
        for (uint16_t i = 0; i < 8; ++i)
        {
            if (event_state_it->IsAttachmentSet(i))
            {
                QList<QTreeWidgetItem *> attachment_items;
                // LogicOpEnabled
                if (event_state_it->IsLogicOpEnabledSet(i))
                    ADD_FIELD_TYPE_BOOL(event_state_it->GetLogicOpEnabledName(),
                                        event_state_it->LogicOpEnabled(i),
                                        prev_event_state_it->IsLogicOpEnabledSet(i),
                                        prev_event_state_it->LogicOpEnabled(i),
                                        attachment_items)
                else
                    ADD_FIELD_NOT_SET(event_state_it->GetLogicOpEnabledName(), attachment_items)

                // LogicOp
                if (event_state_it->IsLogicOpSet(i))
                {
                    ADD_FIELD_TYPE_STRING(event_state_it->GetLogicOpName(),
                                          GetVkLogicOp(event_state_it->LogicOp(i)),
                                          prev_event_state_it->IsLogicOpSet(i),
                                          GetVkLogicOp(prev_event_state_it->LogicOp(i)),
                                          attachment_items)
                }
                else
                {
                    ADD_FIELD_NOT_SET(event_state_it->GetLogicOpName(), attachment_items)
                }

                const VkPipelineColorBlendAttachmentState curr_attach = event_state_it->Attachment(
                i);
                const bool prev_set = prev_event_state_it->IsAttachmentSet(i);
                VkPipelineColorBlendAttachmentState prev_attach = {};
                if (prev_event_state_it->IsValid())
                {
                    prev_attach = prev_event_state_it->Attachment(i);
                }
                ADD_FIELD_TYPE_BOOL("BlendEnabled",
                                    curr_attach.blendEnable,
                                    prev_set,
                                    prev_attach.blendEnable,
                                    attachment_items);
                ADD_FIELD_TYPE_ENUM("SrcColorBlendFactor",
                                    GetVkBlendFactor,
                                    curr_attach.srcColorBlendFactor,
                                    prev_set,
                                    prev_attach.srcColorBlendFactor,
                                    attachment_items);
                ADD_FIELD_TYPE_ENUM("DstColorBlendFactor",
                                    GetVkBlendFactor,
                                    curr_attach.dstColorBlendFactor,
                                    prev_set,
                                    prev_attach.dstColorBlendFactor,
                                    attachment_items);
                ADD_FIELD_TYPE_ENUM("ColorBlendOp",
                                    GetVkBlendOp,
                                    curr_attach.colorBlendOp,
                                    prev_set,
                                    prev_attach.colorBlendOp,
                                    attachment_items);
                ADD_FIELD_TYPE_ENUM("SrcAlphaBlendFactor",
                                    GetVkBlendFactor,
                                    curr_attach.srcAlphaBlendFactor,
                                    prev_set,
                                    prev_attach.srcAlphaBlendFactor,
                                    attachment_items);
                ADD_FIELD_TYPE_ENUM("DstAlphaBlendFactor",
                                    GetVkBlendFactor,
                                    curr_attach.dstAlphaBlendFactor,
                                    prev_set,
                                    prev_attach.dstAlphaBlendFactor,
                                    attachment_items);
                ADD_FIELD_TYPE_ENUM("AlphaBlendOp",
                                    GetVkBlendOp,
                                    curr_attach.alphaBlendOp,
                                    prev_set,
                                    prev_attach.alphaBlendOp,
                                    attachment_items);
                ADD_FIELD_TYPE_NUMBER_HEX("ColorWriteMask",
                                          curr_attach.colorWriteMask,
                                          prev_set,
                                          prev_attach.colorWriteMask,
                                          attachment_items);

                QTreeWidgetItem *attachment_item = new QTreeWidgetItem((QTreeWidget *)0,
                                                                       QStringList(
                                                                       QString::number(i)));
                attachment_item->insertChildren(0, attachment_items);
                child_items.append(attachment_item);
            }
            else
                ADD_FIELD_NOT_SET(QString::number(i), child_items);
        }

        QTreeWidgetItem
        *child_widget_item = new QTreeWidgetItem((QTreeWidget *)0,
                                                 QStringList(event_state_it->GetAttachmentName()));
        child_widget_item->insertChildren(0, child_items);
        items.append(child_widget_item);
    }

    // BlendConstant
    if (event_state_it->IsBlendConstantSet(0))
    {
        QString blend_constant, prev_blend_constant;
        blend_constant = "R: " + QString::number(event_state_it->BlendConstant(0)) +
                         ", G: " + QString::number(event_state_it->BlendConstant(1)) +
                         ", B: " + QString::number(event_state_it->BlendConstant(2)) +
                         ", A: " + QString::number(event_state_it->BlendConstant(3));

        if (prev_event_state_it->IsValid())
        {
            prev_blend_constant = "R: " + QString::number(prev_event_state_it->BlendConstant(0)) +
                                  ", G: " + QString::number(prev_event_state_it->BlendConstant(1)) +
                                  ", B: " + QString::number(prev_event_state_it->BlendConstant(2)) +
                                  ", A: " + QString::number(prev_event_state_it->BlendConstant(3));
        }

        ADD_FIELD_TYPE_STRING(event_state_it->GetBlendConstantName(),
                              blend_constant,
                              prev_event_state_it->IsBlendConstantSet(0),
                              prev_blend_constant,
                              items)
    }
    else
        ADD_FIELD_NOT_SET(event_state_it->GetBlendConstantName(), items)

    ADD_TREE_BRANCH("Color Blend")
}

//--------------------------------------------------------------------------------------------------
void EventStateView::DisplayHardwareSpecificStates(
Dive::EventStateInfo::ConstIterator event_state_it,
Dive::EventStateInfo::ConstIterator prev_event_state_it)
{
    QList<QTreeWidgetItem *> depth_target_items, binning_items, thread_items, ubwc_items;

    if (event_state_it->IsLRZEnabledSet())
        ADD_FIELD_TYPE_BOOL(event_state_it->GetLRZEnabledName(),
                            event_state_it->LRZEnabled(),
                            prev_event_state_it->IsLRZEnabledSet(),
                            prev_event_state_it->LRZEnabled(),
                            depth_target_items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetLRZEnabledName(), depth_target_items)

    if (event_state_it->IsLRZWriteSet())
        ADD_FIELD_TYPE_BOOL(event_state_it->GetLRZWriteName(),
                            event_state_it->LRZWrite(),
                            prev_event_state_it->IsLRZWriteSet(),
                            prev_event_state_it->LRZWrite(),
                            depth_target_items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetLRZWriteName(), depth_target_items)

    auto LRZDirStatusToStr = [](const a6xx_lrz_dir_status &status) -> QString {
        std::string s;
        switch (status)
        {
        case LRZ_DIR_LE:
            s = "Less Equal";
            break;
        case LRZ_DIR_GE:
            s = "Greater Equal";
            break;
        case LRZ_DIR_INVALID:
            s = "Invalid";
            break;
        default:
            s = "Undefined";
            break;  // TODO(wangra): we have cases where this value is 0, same
                    // with cffdump
        }
        return QString::fromStdString(s);
    };

    if (event_state_it->IsLRZDirStatusSet())
        ADD_FIELD_TYPE_STRING(event_state_it->GetLRZDirStatusName(),
                              LRZDirStatusToStr(event_state_it->LRZDirStatus()),
                              prev_event_state_it->IsLRZDirStatusSet(),
                              LRZDirStatusToStr(prev_event_state_it->LRZDirStatus()),
                              depth_target_items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetLRZDirStatusName(), depth_target_items)

    if (event_state_it->IsLRZDirWriteSet())
        ADD_FIELD_TYPE_BOOL(event_state_it->GetLRZDirWriteName(),
                            event_state_it->LRZDirWrite(),
                            prev_event_state_it->IsLRZDirWriteSet(),
                            prev_event_state_it->LRZDirWrite(),
                            depth_target_items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetLRZDirWriteName(), depth_target_items)

    auto ZTestModeToStr = [](const a6xx_ztest_mode &mode) -> QString {
        std::string s;
        switch (mode)
        {
        case A6XX_EARLY_Z:
            s = "Early Z";
            break;
        case A6XX_LATE_Z:
            s = "Late Z";
            break;
        case A6XX_EARLY_Z_LATE_Z:
            s = "Early Z Late Z";
            break;
        case A6XX_INVALID_ZTEST:
            s = "Invalid ZTest";
            break;
        default:
            DIVE_ASSERT(false);
            break;
        }
        return QString::fromStdString(s);
    };

    if (event_state_it->IsZTestModeSet())
        ADD_FIELD_TYPE_STRING(event_state_it->GetZTestModeName(),
                              ZTestModeToStr(event_state_it->ZTestMode()),
                              prev_event_state_it->IsZTestModeSet(),
                              ZTestModeToStr(prev_event_state_it->ZTestMode()),
                              depth_target_items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetZTestModeName(), depth_target_items)

    if (event_state_it->IsBinWSet())
        ADD_FIELD_TYPE_NUMBER(event_state_it->GetBinWName(),
                              event_state_it->BinW(),
                              prev_event_state_it->IsBinWSet(),
                              prev_event_state_it->BinW(),
                              binning_items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetZTestModeName(), depth_target_items)

    if (event_state_it->IsBinHSet())
        ADD_FIELD_TYPE_NUMBER(event_state_it->GetBinHName(),
                              event_state_it->BinH(),
                              prev_event_state_it->IsBinHSet(),
                              prev_event_state_it->BinH(),
                              binning_items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetZTestModeName(), binning_items)

    auto RenderModeToStr = [](const a6xx_render_mode &mode) -> QString {
        std::string s;
        switch (mode)
        {
        case RENDERING_PASS:
            s = "Rendering Pass";
            break;
        case BINNING_PASS:
            s = "Binning Pass";
            break;
        default:
            DIVE_ASSERT(false);
            break;
        }
        return QString::fromStdString(s);
    };

    if (event_state_it->IsRenderModeSet())
        ADD_FIELD_TYPE_STRING(event_state_it->GetRenderModeName(),
                              RenderModeToStr(event_state_it->RenderMode()),
                              prev_event_state_it->IsRenderModeSet(),
                              RenderModeToStr(prev_event_state_it->RenderMode()),
                              binning_items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetRenderModeName(), binning_items)

    // only valid for A6xx
    auto BuffersLocationToStr = [](const a6xx_buffers_location &location) -> QString {
        std::string s;
        switch (location)
        {
        case BUFFERS_IN_GMEM:
            s = "Buffers in GMEM";
            break;
        case BUFFERS_IN_SYSMEM:
            s = "Buffers in SYSMEM";
            break;
        default:
            s = "Unknown";
            break;
        }
        return QString::fromStdString(s);
    };

    if (event_state_it->IsBuffersLocationSet())
        ADD_FIELD_TYPE_STRING(event_state_it->GetBuffersLocationName(),
                              BuffersLocationToStr(event_state_it->BuffersLocation()),
                              prev_event_state_it->IsBuffersLocationSet(),
                              BuffersLocationToStr(prev_event_state_it->BuffersLocation()),
                              binning_items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetBuffersLocationName(), binning_items)

    auto ThreadSizeToStr = [](const a6xx_threadsize &size) -> QString {
        std::string s;
        switch (size)
        {
        case THREAD64:
            s = "Thread 64";
            break;
        case THREAD128:
            s = "Thread 128";
            break;
        default:
            DIVE_ASSERT(false);
            break;
        }
        return QString::fromStdString(s);
    };

    if (event_state_it->IsThreadSizeSet())
        ADD_FIELD_TYPE_STRING(event_state_it->GetThreadSizeName(),
                              ThreadSizeToStr(event_state_it->ThreadSize()),
                              prev_event_state_it->IsThreadSizeSet(),
                              ThreadSizeToStr(prev_event_state_it->ThreadSize()),
                              thread_items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetThreadSizeName(), thread_items)

    if (event_state_it->IsEnableAllHelperLanesSet())
        ADD_FIELD_TYPE_BOOL(event_state_it->GetEnableAllHelperLanesName(),
                            event_state_it->EnableAllHelperLanes(),
                            prev_event_state_it->IsEnableAllHelperLanesSet(),
                            prev_event_state_it->EnableAllHelperLanes(),
                            thread_items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetEnableAllHelperLanesName(), thread_items)

    if (event_state_it->IsEnablePartialHelperLanesSet())
        ADD_FIELD_TYPE_BOOL(event_state_it->GetEnablePartialHelperLanesName(),
                            event_state_it->EnablePartialHelperLanes(),
                            prev_event_state_it->IsEnablePartialHelperLanesSet(),
                            prev_event_state_it->EnablePartialHelperLanes(),
                            thread_items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetEnablePartialHelperLanesName(), thread_items)

    // UBWC on attatchments
    for (uint16_t i = 0; i < 8; ++i)
    {
        // UBWCEnabled
        if (event_state_it->IsUBWCEnabledSet(i))
            ADD_FIELD_TYPE_BOOL(event_state_it->GetUBWCEnabledName() + QString("_") +
                                QString::number(i),
                                event_state_it->UBWCEnabled(i),
                                prev_event_state_it->IsUBWCEnabledSet(i),
                                prev_event_state_it->UBWCEnabled(i),
                                ubwc_items)
        else
            ADD_FIELD_NOT_SET(event_state_it->GetUBWCEnabledName() + QString("_") +
                              QString::number(i),
                              ubwc_items)

        // UBWCLosslessEnabled
        if (event_state_it->IsUBWCLosslessEnabledSet(i))
            ADD_FIELD_TYPE_BOOL(event_state_it->GetUBWCLosslessEnabledName() + QString("_") +
                                QString::number(i),
                                event_state_it->UBWCLosslessEnabled(i),
                                prev_event_state_it->IsUBWCLosslessEnabledSet(i),
                                prev_event_state_it->UBWCLosslessEnabled(i),
                                ubwc_items)
        else
            ADD_FIELD_NOT_SET(event_state_it->GetUBWCLosslessEnabledName() + QString("_") +
                              QString::number(i),
                              ubwc_items)
    }

    // UBWC on Depth Stencil
    {
        if (event_state_it->IsUBWCEnabledOnDSSet())
            ADD_FIELD_TYPE_BOOL(event_state_it->GetUBWCEnabledOnDSName(),
                                event_state_it->UBWCEnabledOnDS(),
                                prev_event_state_it->IsUBWCEnabledOnDSSet(),
                                prev_event_state_it->UBWCEnabledOnDS(),
                                ubwc_items)
        else
            ADD_FIELD_NOT_SET(event_state_it->GetUBWCEnabledOnDSName(), ubwc_items)

        if (event_state_it->IsUBWCLosslessEnabledOnDSSet())
            ADD_FIELD_TYPE_BOOL(event_state_it->GetUBWCLosslessEnabledOnDSName(),
                                event_state_it->UBWCLosslessEnabledOnDS(),
                                prev_event_state_it->IsUBWCLosslessEnabledOnDSSet(),
                                prev_event_state_it->UBWCLosslessEnabledOnDS(),
                                ubwc_items)
        else
            ADD_FIELD_NOT_SET(event_state_it->GetUBWCLosslessEnabledOnDSName(), ubwc_items)
    }

    QTreeWidgetItem *tree_widget_item = new QTreeWidgetItem(m_event_state_tree);
    tree_widget_item->setText(0, "GPU-specific");

    QTreeWidgetItem *thread_item = new QTreeWidgetItem;
    thread_item->setText(0, "Threads Invocation");
    thread_item->insertChildren(0, thread_items);
    tree_widget_item->insertChild(0, thread_item);

    QTreeWidgetItem *depth_target_item = new QTreeWidgetItem;
    depth_target_item->setText(0, "Depth Targets");
    depth_target_item->insertChildren(0, depth_target_items);
    tree_widget_item->insertChild(0, depth_target_item);

    QTreeWidgetItem *binning_item = new QTreeWidgetItem;
    binning_item->setText(0, "Tiling and Binning");
    binning_item->insertChildren(0, binning_items);
    tree_widget_item->insertChild(0, binning_item);

    QTreeWidgetItem *ubwc_item = new QTreeWidgetItem;
    ubwc_item->setText(0, "UBWC status");
    ubwc_item->insertChildren(0, ubwc_items);
    tree_widget_item->insertChild(0, ubwc_item);
}

//--------------------------------------------------------------------------------------------------
void EventStateView::OnHover(QTreeWidgetItem *item_ptr, int column)
{
    HoverHelp *hover_help_ptr = HoverHelp::Get();
    QString    field_name = item_ptr->text(0);
    if (m_field_desc.find(field_name.toStdString()) != m_field_desc.end())
    {
        hover_help_ptr->SetCurItem(HoverHelp::Item::kNone,
                                   UINT32_MAX,
                                   UINT32_MAX,
                                   UINT32_MAX,
                                   m_field_desc[field_name.toStdString()].c_str());
    }
    else
        hover_help_ptr->SetCurItem(HoverHelp::Item::kNone);
}

//--------------------------------------------------------------------------------------------------
void EventStateView::leaveEvent(QEvent *event)
{
    HoverHelp *hover_help_ptr = HoverHelp::Get();
    hover_help_ptr->SetCurItem(HoverHelp::Item::kNone);
}
