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

#include <QFrame>

#pragma once
// Forward declaration
class CommandBufferView;
class CommandBufferModel;
class QGroupBox;
class QLineEdit;
class QPushButton;
class SearchDialog;
namespace Dive
{
class CommandHierarchy;
class Topology;
};  // namespace Dive

class CommandTabView : public QFrame
{
    Q_OBJECT

public:
    CommandTabView(const Dive::CommandHierarchy &command_hierarchy, QWidget *parent = nullptr);

    void SetTopologyToView(const Dive::Topology *topology_ptr);

    void ResetModel();

public slots:
    void OnSelectionChanged(const QModelIndex &index);
    void OnSearchCommandBuffer();

signals:
    // Update property panel for node information.
    void SendNodeProperty(const QString &);

private:
    CommandBufferView  *m_command_buffer_view;
    CommandBufferModel *m_command_buffer_model;
    QPushButton        *m_search_trigger_button;
    SearchDialog       *m_search_dialog = nullptr;

    const Dive::CommandHierarchy &m_command_hierarchy;
};
