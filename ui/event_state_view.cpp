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
            _item->setForeground(1, QBrush(QColor(Qt::cyan)));                        \
        _items.append(_item);                                                         \
    }

#define ADD_FIELD_TYPE_NUMBER(_field, _num, _prev_field_set, _prev_num, _items)       \
    {                                                                                 \
        QTreeWidgetItem *_item = new QTreeWidgetItem;                                 \
        _item->setText(0, QString(_field));                                           \
        _item->setText(1, QString::number(_num));                                     \
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
            _item->setForeground(1, QBrush(QColor(Qt::cyan)));                          \
        _items.append(_item);                                                           \
    }

#define ADD_TREE_BRANCH(branch_name)                                              \
    QTreeWidgetItem *_tree_widget_item = new QTreeWidgetItem(m_event_state_tree); \
    _tree_widget_item->setText(0, branch_name);                                   \
    _tree_widget_item->insertChildren(0, items);

#define ADD_FIELD_DESC(_field, _desc)                    \
    if (m_field_desc.find(_field) == m_field_desc.end()) \
        m_field_desc[_field] = _desc;

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

    auto &metadata = m_data_core.GetCaptureMetadata();
    auto &command_hierarchy = m_data_core.GetCommandHierarchy();
    auto &event_state = metadata.m_event_state;

    auto previous_event_state = [&](auto it) {
        auto prev_it = std::prev(it);
        while (prev_it->IsValid())
        {
            auto id = static_cast<Dive::EventStateId>(prev_it->id());
            // Check if it's draw or dispatch events
            const Dive::EventInfo &prev_event_info = metadata
                                                     .m_event_info[static_cast<uint32_t>(id)];
            if (prev_event_info.m_type == Dive::EventInfo::EventType::kDraw ||
                prev_event_info.m_type == Dive::EventInfo::EventType::kDispatch)
                break;
            else
                prev_it = std::prev(prev_it);
        }
        return prev_it;
    };

    auto display_event_state_info = [&](uint64_t event_node_index) {
        auto event_id = m_data_core.GetCommandHierarchy().GetEventNodeId(event_node_index);
        // Check if it's draw or dispatch events
        if (!(event_id < metadata.m_event_info.size()))
            return;
        const Dive::EventInfo &event_info = metadata.m_event_info[event_id];
        if (event_info.m_type == Dive::EventInfo::EventType::kDraw ||
            event_info.m_type == Dive::EventInfo::EventType::kDispatch)
        {
            auto event_state_it = GetStateInfoForEvent(event_state, event_id);
            auto prev_event_state_it = previous_event_state(event_state_it);
            DisplayEventStateInfo(event_state_it, prev_event_state_it);
        }
    };

    Dive::NodeType node_type = command_hierarchy.GetNodeType(node_index);
    if (node_type == Dive::NodeType::kDrawDispatchBlitNode)
    {
        display_event_state_info(node_index);
    }
    else if (node_type == Dive::NodeType::kMarkerNode &&
             command_hierarchy.GetMarkerNodeType(node_index) ==
             Dive::CommandHierarchy::MarkerType::kDiveMetadata)
    {
        uint64_t event_node_index = UINT64_MAX;
        auto     topology = m_data_core.GetCommandHierarchy().GetAllEventHierarchyTopology();
        auto     num_children = topology.GetNumChildren(node_index);
        for (uint64_t i = 0; i < num_children; i++)
        {
            auto           child_node_index = topology.GetChildNodeIndex(node_index, i);
            Dive::NodeType child_node_type = command_hierarchy.GetNodeType(child_node_index);
            if (child_node_type == Dive::NodeType::kDrawDispatchBlitNode)
            {
                event_node_index = child_node_index;
            }
        }
        if (event_node_index != UINT64_MAX)
            display_event_state_info(event_node_index);
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
void EventStateView::DisplayEventStateInfo(Dive::EventStateInfo::ConstIterator event_state_it,
                                           Dive::EventStateInfo::ConstIterator prev_event_state_it)
{
    // Build up map to lookup description
    BuildDescriptionMap(event_state_it);

    // Vulkan states
    DisplayInputAssemblyState(event_state_it, prev_event_state_it);
    DisplayTessellationState(event_state_it, prev_event_state_it);
    DisplayFillViewportState(event_state_it, prev_event_state_it);
    DisplayRasterizerState(event_state_it, prev_event_state_it);
    DisplayFillMultisamplingState(event_state_it, prev_event_state_it);
    DisplayDepthState(event_state_it, prev_event_state_it);
    DisplayColorBlendState(event_state_it, prev_event_state_it);

    // Hardware-specific non-Vulkan states
    DisplayHardwareSpecificStates(event_state_it, prev_event_state_it);
}

//--------------------------------------------------------------------------------------------------
void EventStateView::BuildDescriptionMap(Dive::EventStateInfo::ConstIterator event_state_it)
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
    ADD_FIELD_DESC(event_state_it->GetZAddrName(), event_state_it->GetZAddrDescription());
    ADD_FIELD_DESC(event_state_it->GetHTileAddrName(), event_state_it->GetHTileAddrDescription());
    ADD_FIELD_DESC(event_state_it->GetHiZEnabledName(), event_state_it->GetHiZEnabledDescription());
    ADD_FIELD_DESC(event_state_it->GetHiSEnabledName(), event_state_it->GetHiSEnabledDescription());
    ADD_FIELD_DESC(event_state_it->GetZCompressEnabledName(),
                   event_state_it->GetZCompressEnabledDescription());
    ADD_FIELD_DESC(event_state_it->GetStencilCompressEnabledName(),
                   event_state_it->GetStencilCompressEnabledDescription());
    ADD_FIELD_DESC(event_state_it->GetCompressedZFetchEnabledName(),
                   event_state_it->GetCompressedZFetchEnabledDescription());
    ADD_FIELD_DESC(event_state_it->GetZFormatName(), event_state_it->GetZFormatDescription());
    ADD_FIELD_DESC(event_state_it->GetZOrderName(), event_state_it->GetZOrderDescription());
    ADD_FIELD_DESC(event_state_it->GetVSLateAllocName(),
                   event_state_it->GetVSLateAllocDescription());
    ADD_FIELD_DESC(event_state_it->GetDccEnabledName(), event_state_it->GetDccEnabledDescription());
    ADD_FIELD_DESC(event_state_it->GetColorFormatName(),
                   event_state_it->GetColorFormatDescription());
    ADD_FIELD_DESC(event_state_it->GetMip0HeightName(), event_state_it->GetMip0HeightDescription());
    ADD_FIELD_DESC(event_state_it->GetMip0WidthName(), event_state_it->GetMip0WidthDescription());
    ADD_FIELD_DESC(event_state_it->GetVgprName(), event_state_it->GetVgprDescription());
    ADD_FIELD_DESC(event_state_it->GetSgprName(), event_state_it->GetSgprDescription());
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

    // DepthBoundsTestEnabled
    if (event_state_it->IsDepthBoundsTestEnabledSet())
        ADD_FIELD_TYPE_BOOL(event_state_it->GetDepthBoundsTestEnabledName(),
                            event_state_it->DepthBoundsTestEnabled(),
                            prev_event_state_it->IsDepthBoundsTestEnabledSet(),
                            prev_event_state_it->DepthBoundsTestEnabled(),
                            items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetDepthBoundsTestEnabledName(), items)

    // StencilTestEnabled
    if (event_state_it->IsStencilTestEnabledSet())
        ADD_FIELD_TYPE_BOOL(event_state_it->GetStencilTestEnabledName(),
                            event_state_it->StencilTestEnabled(),
                            prev_event_state_it->IsStencilTestEnabledSet(),
                            prev_event_state_it->StencilTestEnabled(),
                            items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetStencilTestEnabledName(), items)

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
void EventStateView::DisplayColorBlendState(Dive::EventStateInfo::ConstIterator event_state_it,
                                            Dive::EventStateInfo::ConstIterator prev_event_state_it)
{
    QList<QTreeWidgetItem *> items;

    // LogicOpEnabled
    if (event_state_it->IsLogicOpEnabledSet())
        ADD_FIELD_TYPE_BOOL(event_state_it->GetLogicOpEnabledName(),
                            event_state_it->LogicOpEnabled(),
                            prev_event_state_it->IsLogicOpEnabledSet(),
                            prev_event_state_it->LogicOpEnabled(),
                            items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetLogicOpEnabledName(), items)

    // LogicOp
    if (event_state_it->IsLogicOpSet())
    {
        ADD_FIELD_TYPE_STRING(event_state_it->GetLogicOpName(),
                              GetVkLogicOp(event_state_it->LogicOp()),
                              prev_event_state_it->IsLogicOpSet(),
                              GetVkLogicOp(prev_event_state_it->LogicOp()),
                              items)
    }
    else
    {
        ADD_FIELD_NOT_SET(event_state_it->GetLogicOpName(), items)
    }

    // Attachment
    {
        QList<QTreeWidgetItem *> child_items;
        for (uint16_t i = 0; i < 8; ++i)
        {
            if (event_state_it->IsAttachmentSet(i))
            {
                QString                             value;
                VkPipelineColorBlendAttachmentState attach = event_state_it->Attachment(i);

                value = "srcColorBlendFactor: " +
                        QString(GetVkBlendFactor(attach.srcColorBlendFactor)) +
                        ", dstColorBlendFactor: " +
                        QString(GetVkBlendFactor(attach.dstColorBlendFactor)) +
                        ", colorBlendOp: " + QString(GetVkBlendOp(attach.colorBlendOp)) +
                        ", srcAlphaBlendFactor: " +
                        QString(GetVkBlendFactor(attach.srcAlphaBlendFactor)) +
                        ", dstAlphaBlendFactor: " +
                        QString(GetVkBlendFactor(attach.dstAlphaBlendFactor)) +
                        ", alphaBlendOp: " + QString(GetVkBlendOp(attach.alphaBlendOp));

                QString                             prev_value;
                VkPipelineColorBlendAttachmentState prev_attach;

                if (prev_event_state_it->IsValid())
                {
                    prev_attach = prev_event_state_it->Attachment(i);
                    prev_value = "srcColorBlendFactor: " +
                                 QString(GetVkBlendFactor(prev_attach.srcColorBlendFactor)) +
                                 ", dstColorBlendFactor: " +
                                 QString(GetVkBlendFactor(prev_attach.dstColorBlendFactor)) +
                                 ", colorBlendOp: " +
                                 QString(GetVkBlendOp(prev_attach.colorBlendOp)) +
                                 ", srcAlphaBlendFactor: " +
                                 QString(GetVkBlendFactor(prev_attach.srcAlphaBlendFactor)) +
                                 ", dstAlphaBlendFactor: " +
                                 QString(GetVkBlendFactor(prev_attach.dstAlphaBlendFactor)) +
                                 ", alphaBlendOp: " +
                                 QString(GetVkBlendOp(prev_attach.alphaBlendOp));
                }

                ADD_FIELD_TYPE_STRING(QString::number(i),
                                      value,
                                      prev_event_state_it->IsAttachmentSet(i),
                                      prev_value,
                                      child_items);
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
    QList<QTreeWidgetItem *> items, depth_target_items, color_target_items;

    // ZAddr
    if (event_state_it->IsZAddrSet())
        ADD_FIELD_TYPE_STRING(event_state_it->GetZAddrName(),
                              "0x" + QString::number(event_state_it->ZAddr(), 16).toUpper(),
                              prev_event_state_it->IsZAddrSet(),
                              "0x" + QString::number(prev_event_state_it->ZAddr(), 16).toUpper(),
                              depth_target_items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetZAddrName(), depth_target_items)

    // HTileAddr
    if (event_state_it->IsHTileAddrSet())
        ADD_FIELD_TYPE_STRING(event_state_it->GetHTileAddrName(),
                              "0x" + QString::number(event_state_it->HTileAddr(), 16).toUpper(),
                              prev_event_state_it->IsHTileAddrSet(),
                              "0x" +
                              QString::number(prev_event_state_it->HTileAddr(), 16).toUpper(),
                              depth_target_items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetHTileAddrName(), depth_target_items)

    // HiZEnabled
    if (event_state_it->IsHiZEnabledSet())
        ADD_FIELD_TYPE_BOOL(event_state_it->GetHiZEnabledName(),
                            event_state_it->HiZEnabled(),
                            prev_event_state_it->IsHiZEnabledSet(),
                            prev_event_state_it->HiZEnabled(),
                            depth_target_items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetHiZEnabledName(), depth_target_items)

    // HiSEnabled
    if (event_state_it->IsHiSEnabledSet())
        ADD_FIELD_TYPE_BOOL(event_state_it->GetHiSEnabledName(),
                            event_state_it->HiSEnabled(),
                            prev_event_state_it->IsHiSEnabledSet(),
                            prev_event_state_it->HiSEnabled(),
                            depth_target_items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetHiSEnabledName(), depth_target_items)

    // ZCompressEnabled
    if (event_state_it->IsZCompressEnabledSet())
        ADD_FIELD_TYPE_BOOL(event_state_it->GetZCompressEnabledName(),
                            event_state_it->ZCompressEnabled(),
                            prev_event_state_it->IsZCompressEnabledSet(),
                            prev_event_state_it->ZCompressEnabled(),
                            depth_target_items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetZCompressEnabledName(), depth_target_items)

    // StencilCompressEnabled
    if (event_state_it->IsStencilCompressEnabledSet())
        ADD_FIELD_TYPE_BOOL(event_state_it->GetStencilCompressEnabledName(),
                            event_state_it->StencilCompressEnabled(),
                            prev_event_state_it->IsStencilCompressEnabledSet(),
                            prev_event_state_it->StencilCompressEnabled(),
                            depth_target_items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetStencilCompressEnabledName(), depth_target_items)

    // CompressedZFetchEnabled
    if (event_state_it->IsCompressedZFetchEnabledSet())
        ADD_FIELD_TYPE_BOOL(event_state_it->GetCompressedZFetchEnabledName(),
                            event_state_it->CompressedZFetchEnabled(),
                            prev_event_state_it->IsCompressedZFetchEnabledSet(),
                            prev_event_state_it->CompressedZFetchEnabled(),
                            depth_target_items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetCompressedZFetchEnabledName(), depth_target_items)

    // ZFormat
    if (event_state_it->IsZFormatSet())
        ADD_FIELD_TYPE_STRING(event_state_it->GetZFormatName(),
                              GetZFormat(event_state_it->ZFormat()),
                              prev_event_state_it->IsZFormatSet(),
                              GetZFormat(prev_event_state_it->ZFormat()),
                              depth_target_items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetZFormatName(), depth_target_items)

    // ZOrder
    if (event_state_it->IsZOrderSet())
        ADD_FIELD_TYPE_STRING(event_state_it->GetZOrderName(),
                              GetZOrder(event_state_it->ZOrder()),
                              prev_event_state_it->IsZOrderSet(),
                              GetZOrder(prev_event_state_it->ZOrder()),
                              depth_target_items)
    else
        ADD_FIELD_NOT_SET(event_state_it->GetZOrderName(), depth_target_items)

    // VSLateAlloc
    if (event_state_it->IsVSLateAllocSet())
    {
        ADD_FIELD_TYPE_NUMBER(event_state_it->GetVSLateAllocName(),
                              event_state_it->VSLateAlloc(),
                              prev_event_state_it->IsVSLateAllocSet(),
                              prev_event_state_it->VSLateAlloc(),
                              depth_target_items)
    }
    else
    {
        ADD_FIELD_NOT_SET(event_state_it->GetVSLateAllocName(), depth_target_items)
    }

    // DccEnabled
    {
        QList<QTreeWidgetItem *> child_items;
        for (uint16_t i = 0; i < 8; ++i)
        {
            QString field_name = "Target# " + QString::number(i);
            if (event_state_it->IsDccEnabledSet(i))
            {
                ADD_FIELD_TYPE_BOOL(field_name,
                                    event_state_it->DccEnabled(i),
                                    prev_event_state_it->IsDccEnabledSet(i),
                                    prev_event_state_it->DccEnabled(i),
                                    child_items);
            }
            else
                ADD_FIELD_NOT_SET(field_name, child_items);
        }

        QTreeWidgetItem
        *child_widget_item = new QTreeWidgetItem((QTreeWidget *)0,
                                                 QStringList(event_state_it->GetDccEnabledName()));
        child_widget_item->insertChildren(0, child_items);
        color_target_items.append(child_widget_item);
    }

    // ColorFormat
    {
        QList<QTreeWidgetItem *> child_items;
        for (uint16_t i = 0; i < 8; ++i)
        {
            QString field_name = "Target# " + QString::number(i);
            if (event_state_it->IsColorFormatSet(i))
            {
                ADD_FIELD_TYPE_STRING(field_name,
                                      QString(GetColorFormat(event_state_it->ColorFormat(i))),
                                      prev_event_state_it->IsColorFormatSet(i),
                                      QString(GetColorFormat(prev_event_state_it->ColorFormat(i))),
                                      child_items);
            }
            else
                ADD_FIELD_NOT_SET(field_name, child_items);
        }

        QTreeWidgetItem
        *child_widget_item = new QTreeWidgetItem((QTreeWidget *)0,
                                                 QStringList(event_state_it->GetColorFormatName()));
        child_widget_item->insertChildren(0, child_items);
        color_target_items.append(child_widget_item);
    }

    // Mip0Height
    {
        QList<QTreeWidgetItem *> child_items;
        for (uint16_t i = 0; i < 8; ++i)
        {
            QString field_name = "Target# " + QString::number(i);
            if (event_state_it->IsMip0HeightSet(i))
            {
                ADD_FIELD_TYPE_NUMBER(field_name,
                                      event_state_it->Mip0Height(i),
                                      prev_event_state_it->IsMip0HeightSet(i),
                                      prev_event_state_it->Mip0Height(i),
                                      child_items);
            }
            else
                ADD_FIELD_NOT_SET(field_name, child_items);
        }

        QTreeWidgetItem
        *child_widget_item = new QTreeWidgetItem((QTreeWidget *)0,
                                                 QStringList(event_state_it->GetMip0HeightName()));
        child_widget_item->insertChildren(0, child_items);
        color_target_items.append(child_widget_item);
    }

    // Mip0Width
    {
        QList<QTreeWidgetItem *> child_items;
        for (uint16_t i = 0; i < 8; ++i)
        {
            QString field_name = "Target# " + QString::number(i);
            if (event_state_it->IsMip0WidthSet(i))
            {
                ADD_FIELD_TYPE_NUMBER(field_name,
                                      event_state_it->Mip0Width(i),
                                      prev_event_state_it->IsMip0WidthSet(i),
                                      prev_event_state_it->Mip0Width(i),
                                      child_items);
            }
            else
                ADD_FIELD_NOT_SET(field_name, child_items);
        }

        QTreeWidgetItem *child_widget_item = new QTreeWidgetItem((QTreeWidget *)0,
                                                                 QStringList(event_state_it
                                                                             ->GetMip0WidthName()));
        child_widget_item->insertChildren(0, child_items);
        color_target_items.append(child_widget_item);
    }

    QString shader_stage[] = { "CS", "GS", "HS", "PS", "VS" };
    // Vgpr
    {
        QList<QTreeWidgetItem *> child_items;
        for (uint32_t stage = 0; stage < Dive::kShaderStageCount; ++stage)
        {
            if (event_state_it->IsVgprSet((Dive::ShaderStage)stage))
            {
                ADD_FIELD_TYPE_NUMBER(shader_stage[stage],
                                      event_state_it->Vgpr((Dive::ShaderStage)stage),
                                      prev_event_state_it->IsVgprSet((Dive::ShaderStage)stage),
                                      prev_event_state_it->Vgpr((Dive::ShaderStage)stage),
                                      child_items);
            }
            else
                ADD_FIELD_NOT_SET(shader_stage[stage], child_items);
        }

        QTreeWidgetItem *child_widget_item = new QTreeWidgetItem((QTreeWidget *)0,
                                                                 QStringList(
                                                                 event_state_it->GetVgprName()));
        child_widget_item->insertChildren(0, child_items);
        items.append(child_widget_item);
    }

    // Sgpr
    {
        QList<QTreeWidgetItem *> child_items;
        for (uint32_t stage = 0; stage < Dive::kShaderStageCount; ++stage)
        {
            if (event_state_it->IsSgprSet((Dive::ShaderStage)stage))
            {
                ADD_FIELD_TYPE_NUMBER(shader_stage[stage],
                                      event_state_it->Sgpr((Dive::ShaderStage)stage),
                                      prev_event_state_it->IsSgprSet((Dive::ShaderStage)stage),
                                      prev_event_state_it->Sgpr((Dive::ShaderStage)stage),
                                      child_items);
            }
            else
                ADD_FIELD_NOT_SET(shader_stage[stage], child_items);
        }

        QTreeWidgetItem *child_widget_item = new QTreeWidgetItem((QTreeWidget *)0,
                                                                 QStringList(
                                                                 event_state_it->GetSgprName()));
        child_widget_item->insertChildren(0, child_items);
        items.append(child_widget_item);
    }

    QTreeWidgetItem *tree_widget_item = new QTreeWidgetItem(m_event_state_tree);
    tree_widget_item->setText(0, "GPU-specific");
    QTreeWidgetItem *depth_target_item = new QTreeWidgetItem;
    depth_target_item->setText(0, "Depth Targets");
    depth_target_item->insertChildren(0, depth_target_items);
    QTreeWidgetItem *color_target_item = new QTreeWidgetItem;
    color_target_item->setText(0, "Color Targets");
    color_target_item->insertChildren(0, color_target_items);
    tree_widget_item->insertChild(0, depth_target_item);
    tree_widget_item->insertChild(0, color_target_item);
    tree_widget_item->insertChildren(0, items);
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
