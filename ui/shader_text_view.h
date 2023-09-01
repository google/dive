/*
 Copyright 2022 Google LLC

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
#include <QTextEdit>

#include "hover_help_model.h"

//--------------------------------------------------------------------------------------------------
// ShaderTextView enables hoverhelp for QTextEdit
// Note: It is not implemented in shader_view.cpp due to qt does not play nicely with Q_OBJECT in a
//       cpp file.
class ShaderTextView : public QTextEdit
{
    Q_OBJECT

public:
    ShaderTextView();
    void EnableHoverEvent(bool enabled = true);

private:
    bool m_hover_enabled = false;
signals:
    void HoverEnter(HoverHelp::Item item,
                    uint32_t        param1 = UINT32_MAX,
                    uint32_t        param2 = UINT32_MAX,
                    uint32_t        param3 = UINT32_MAX,
                    const char *    custom_string = nullptr);
    void HoverExit(HoverHelp::Item item,
                   uint32_t        param1 = UINT32_MAX,
                   uint32_t        param2 = UINT32_MAX,
                   uint32_t        param3 = UINT32_MAX,
                   const char *    custom_string = nullptr);
protected slots:
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);
};