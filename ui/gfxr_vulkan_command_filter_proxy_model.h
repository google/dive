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

    GfxrVulkanCommandFilterProxyModel(QObject                      *parent = nullptr,
                                      const Dive::CommandHierarchy *command_hierarchy = nullptr);
    void SetFilter(FilterMode filter_mode);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    void ApplyNewFilterMode(FilterMode new_mode);

    const Dive::CommandHierarchy *m_command_hierarchy;
    FilterMode                    m_filter_mode;
};

#endif  // GFXRVULKANCOMMANDFILTERPROXYMODEL_H
