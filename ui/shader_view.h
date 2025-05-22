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

#pragma once
#include <QFrame>

#include "dive_core/cross_ref.h"

// Forward declaration
class QTreeWidget;
class QTreeWidgetItem;
namespace Dive
{
class DataCore;
}
class HoverHelp;
class ShaderTextView;

//--------------------------------------------------------------------------------------------------
class ShaderView : public QFrame
{
    Q_OBJECT

public:
    ShaderView(const Dive::DataCore &data_core);
    void Reset();
    void SetupHoverHelp(HoverHelp &);

    bool OnCrossReference(Dive::CrossRef);

signals:
    // void ShaderSelected();

protected:
    virtual void paintEvent(QPaintEvent *) override;

private slots:
    void OnEventSelected(uint64_t node_index);
    void OnShaderSelectionChanged();

private:
    const Dive::DataCore &m_data_core;
    uint64_t              m_node_index;
    ShaderTextView       *m_shader_code_text;
    QTreeWidget          *m_shader_list;
};
