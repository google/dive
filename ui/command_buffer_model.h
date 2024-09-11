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
#include "dive_core/common.h"

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
        kColumnPm4,
        kColumnIbLevel,  // Swapped to second column in code.
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

    QModelIndex scrollToIndex() const;

    QList<QModelIndex> search(const QModelIndex &start, const QVariant &value) const;

public slots:
    void OnSelectionChanged(const QModelIndex &index);

private:
    bool CreateNodeToParentMap(uint64_t parent_row, uint64_t parent_node_index, bool is_cur_event);
    void SetIsSelected(uint64_t node_index);
    bool IsSelected(uint64_t node_index) const;
    void searchAddressColumn(QList<QModelIndex>        &search_results,
                             int                        row,
                             const QModelIndex         &parent,
                             const QString             &text,
                             const Qt::CaseSensitivity &case_sensitivity) const;

    uint64_t m_selected_node_index = UINT64_MAX;

    // Need a bit to indicate if it's part of same event, for both set of children
    // Need a QModelIndex per both set of children
    // Bit to determine if parent is a shared node or not
    std::vector<QModelIndex> m_node_parent_list;
    std::vector<uint8_t>     m_node_is_selected_bit_list;
    QModelIndex              m_scroll_to_index;

    const Dive::CommandHierarchy &m_command_hierarchy;
    const Dive::Topology         *m_topology_ptr = nullptr;
    bool                          m_show_level_column = true;
};
