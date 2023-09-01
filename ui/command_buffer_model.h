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
#include <QAbstractItemModel>
#include <QList>
#include <QModelIndex>
#include <QVariant>

#include <vector>

// Forward Declarations
namespace Dive
{
class CommandHierarchy;
class Topology;
};  // namespace Dive

class CommandBufferModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum ColumnType : int
    {
        kColumnDE,
        kColumnCE,
        kColumnAddress,  // Swapped to first column in code.
        kColumnCount
    };

public:
    explicit CommandBufferModel(const Dive::CommandHierarchy &command_hierarchy);
    ~CommandBufferModel();

    void Reset();

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

    QList<QModelIndex> search(const QModelIndex &start, const QVariant &value) const;

public slots:
    void OnSelectionChanged(const QModelIndex &index);

private:
    void                          AddPackets(uint64_t node_index);
    void                          AddMarkerPackets(uint64_t marker_node_index);
    uint64_t                      m_selected_node_index = UINT64_MAX;
    std::map<uint64_t, uint64_t>  m_node_index_to_row_map;
    const Dive::CommandHierarchy &m_command_hierarchy;
    const Dive::Topology         *m_topology_ptr = nullptr;
};
