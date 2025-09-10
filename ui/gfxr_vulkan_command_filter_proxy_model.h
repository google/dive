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

#ifndef GFXRVULKANCOMMANDFILTERPROXYMODEL_H
#define GFXRVULKANCOMMANDFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include "dive_core/command_hierarchy.h"
#include "gfxr_vulkan_command_model.h"

namespace Dive
{
class CommandHierarchy;
class Topology;
};  // namespace Dive

class GfxrVulkanCommandFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    enum FilterMode : uint32_t
    {
        kNone,
        kDrawDispatchOnly,
        kFilterModeCount
    };

    GfxrVulkanCommandFilterProxyModel(const Dive::CommandHierarchy &command_hierarchy,
                                      QObject                      *parent = nullptr);
    void SetFilter(FilterMode filter_mode);

    // The default parent index is QModelIndex() so that the top-level items (submits) are recursed
    // through. When the sourceModel()->rowCount is called on QModelIndex(), the row count is the
    // number of those top-level items. In the initial call, QModelIndex() is not valid so the list
    // of indices is cleared. This is so we don't have any of the previous collected indices.
    void CollectGfxrDrawCallIndices(const QModelIndex &parent_index = QModelIndex());
    const std::vector<uint64_t> &GetGfxrDrawCallIndices() { return m_gfxr_draw_call_indices; }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    void ApplyNewFilterMode(FilterMode new_mode);

    const Dive::CommandHierarchy &m_command_hierarchy;
    FilterMode                    m_filter_mode;
    std::vector<uint64_t>         m_gfxr_draw_call_indices;
};

#endif  // GFXRVULKANCOMMANDFILTERPROXYMODEL_H
