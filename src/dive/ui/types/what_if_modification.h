/*
 Copyright 2026 Google LLC

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

#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <optional>

#include "dive/common/what_if_modification_types.h"

namespace Dive
{
struct DrawCallFilters
{
    int index_count = 0;
    int vertex_count = 0;
    int instance_count = 0;
    int draw_count = 0;
    QString pso_property = {};
    QString render_pass = {};
};

struct RenderPassFilters
{
    QString command_buffer = {};
    QString render_pass_type = {};
};

struct WhatIfModification
{
    QString ui_text = {};
    Dive::WhatIfType type = {};
    QString command = {};

    QStringList flags = {};
    std::optional<DrawCallFilters> draw_call_filters = std::nullopt;
    std::optional<RenderPassFilters> render_pass_filters = std::nullopt;
};
}  // namespace Dive
// qRegisterMetaType in custom_metatypes.cpp

Q_DECLARE_METATYPE(Dive::WhatIfModification)
