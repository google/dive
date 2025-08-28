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

#include "gfxr_vulkan_command_filter.h"
#include "gfxr_vulkan_command_filter_proxy_model.h"
#include "dive_tree_view.h"
#include <iostream>
#include <qapplication.h>
#include <qstandarditemmodel.h>

static constexpr const char
*kGfxrVulkanCommandFilterStrings[GfxrVulkanCommandFilterProxyModel::kFilterModeCount] = {
    "None",
    "Draw/Dispatch"
};
static constexpr GfxrVulkanCommandFilterProxyModel::FilterMode
kDefaultGfxrVulkanCommandFilterMode = GfxrVulkanCommandFilterProxyModel::kDrawDispatchOnly;
// =================================================================================================
// GfxrVulkanCommandFilter
// =================================================================================================
GfxrVulkanCommandFilter::GfxrVulkanCommandFilter(DiveTreeView &command_hierarchy_view,
                                                 GfxrVulkanCommandFilterProxyModel &proxy_model,
                                                 QWidget                           *parent) :
    m_command_hierarchy_view(command_hierarchy_view),
    m_proxy_Model(proxy_model)
{
    // Set model for the gfxr command filter combo box
    QStandardItemModel *gfxr_command_filter_combo_box_model = new QStandardItemModel();
    for (uint32_t i = 0; i < GfxrVulkanCommandFilterProxyModel::kFilterModeCount; i++)
    {
        QStandardItem *item = new QStandardItem(kGfxrVulkanCommandFilterStrings[i]);
        gfxr_command_filter_combo_box_model->appendRow(item);
    }
    this->setModel(gfxr_command_filter_combo_box_model);
    this->setCurrentIndex(kDefaultGfxrVulkanCommandFilterMode);

    QObject::connect(this,
                     SIGNAL(currentTextChanged(const QString &)),
                     this,
                     SLOT(OnFilterGfxrVulkanCommandChange(const QString &)));
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandFilter::Reset()
{
    setCurrentIndex(kDefaultGfxrVulkanCommandFilterMode);
}

//--------------------------------------------------------------------------------------------------
void GfxrVulkanCommandFilter::OnFilterGfxrVulkanCommandChange(const QString &filter_mode)
{
    GfxrVulkanCommandFilterProxyModel::FilterMode new_filter;

    if (filter_mode == kGfxrVulkanCommandFilterStrings[GfxrVulkanCommandFilterProxyModel::kNone])
    {
        new_filter = GfxrVulkanCommandFilterProxyModel::kNone;
    }
    else if (filter_mode ==
             kGfxrVulkanCommandFilterStrings[GfxrVulkanCommandFilterProxyModel::kDrawDispatchOnly])
    {
        new_filter = GfxrVulkanCommandFilterProxyModel::kDrawDispatchOnly;
    }
    else
    {
        new_filter = GfxrVulkanCommandFilterProxyModel::kNone;
    }

    m_proxy_Model.SetFilter(new_filter);

    m_command_hierarchy_view.scrollToTop();

    m_command_hierarchy_view.expandAll();
}
