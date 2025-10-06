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

#ifndef GFXRVULKANCOMMANDMODEL_H
#define GFXRVULKANCOMMANDMODEL_H

#include <QAbstractItemModel>
#include <QList>
#include <QModelIndex>
#include <QVariant>
#include <cstdint>

// Forward Declarations
namespace Dive
{
class CommandHierarchy;
class Topology;
class ArgsFilterProxyModel;
};  // namespace Dive

class GfxrVulkanCommandModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit GfxrVulkanCommandModel(const Dive::CommandHierarchy &command_hierarchy);
    ~GfxrVulkanCommandModel();

    void Reset();
    void BeginResetModel();
    void EndResetModel();
    void SetTopologyToView(const Dive::Topology *topology_ptr);

    QVariant      data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant      headerData(int             section,
                             Qt::Orientation orientation,
                             int             role = Qt::DisplayRole) const override;
    QModelIndex   index(int                row,
                        int                column,
                        const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex   parent(const QModelIndex &index) const override;
    int           rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int           columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex   findNode(uint64_t node_index) const;

    QList<QModelIndex> search(const QModelIndex &start, const QVariant &value) const;

    uint64_t getNumNodes() const;

private:
    void BuildNodeLookup(const QModelIndex &parent = QModelIndex()) const;

    const Dive::CommandHierarchy                        &m_command_hierarchy;
    const Dive::Topology                                *m_topology_ptr;
    const std::unordered_map<std::string, const char *> &m_vulkan_command_tool_tip_summaries;
    mutable std::vector<QPersistentModelIndex>           m_node_lookup;
};
#endif
