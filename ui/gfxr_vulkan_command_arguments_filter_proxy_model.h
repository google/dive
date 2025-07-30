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

#ifndef GFXRVULKANCOMMANDARGUMENTSFILTERPROXYMODEL_H
#define GFXRVULKANCOMMANDARGUMENTSFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QColor>
#include "dive_core/command_hierarchy.h"

class GfxrVulkanCommandArgumentsFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    GfxrVulkanCommandArgumentsFilterProxyModel(
    QObject                      *parent = nullptr,
    const Dive::CommandHierarchy *command_hierarchy = nullptr);

    Q_INVOKABLE void SetTargetParentSourceIndex(const QModelIndex &sourceIndex);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QPersistentModelIndex m_targetParentSourceIndex;
    bool                  IsDescendant(const QModelIndex &potentialDescendant,
                                       const QModelIndex &potentialAncestor) const;

    const Dive::CommandHierarchy *m_command_hierarchy;
    int                           m_filterFunctionNodeIndex;
};

#endif  // GFXRVULKANCOMMANDARGUMENTSFILTERPROXYMODEL_H
