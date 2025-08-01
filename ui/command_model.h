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

// Forward Declarations
namespace Dive
{
class CommandHierarchy;
class SharedNodeTopology;
};  // namespace Dive

class CommandModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum class ViewMode
    {
        kEngine,
        kSubmit,
        kEvent
    };

    explicit CommandModel(const Dive::CommandHierarchy &command_hierarchy);
    ~CommandModel();

    void Reset();
    void BeginResetModel();
    void EndResetModel();
    void SetTopologyToView(const Dive::SharedNodeTopology *topology_ptr);

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

    static QVariant GetNodeUIId(uint64_t                        node_index,
                                const Dive::CommandHierarchy   &command_hierarchy,
                                const Dive::SharedNodeTopology *topology_ptr);

    QList<QModelIndex> search(const QModelIndex &start, const QVariant &value) const;

private:
    enum class UIBarrierIdVariant
    {
        First,
        Last,
        Full
    };
    bool     EventNodeHasMarker(uint64_t node_index) const;
    char     GetEventNodeStream(uint64_t node_index) const;
    uint32_t GetEventNodeIndexInStream(uint64_t node_index) const;
    void     BuildNodeLookup(const QModelIndex &parent = QModelIndex()) const;

    const Dive::CommandHierarchy              &m_command_hierarchy;
    const Dive::SharedNodeTopology            *m_topology_ptr;
    mutable std::vector<QPersistentModelIndex> m_node_lookup;
};
