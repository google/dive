/*
 Copyright 2021 Google LLC

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
#include <QColor>
#include <stdint.h>

#include "dive_core/common/gpudefs.h"

#ifndef _MSC_VER
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wunused-variable"
#endif

namespace Dive
{

static QColor kShaderStageColor[Dive::kShaderStageCount] = {
    QColor(0, 200, 83),     // kShaderStageCs
    QColor(0, 215, 215),    // kShaderStageGs
    QColor(160, 160, 160),  // kShaderStageHs
    QColor(83, 109, 254),   // kShaderStagePs
    QColor(255, 82, 82)
};  // kShaderStageVs

static QColor kShaderStageBorderColor[Dive::kShaderStageCount] = {
    QColor(0, 75, 0),     // kShaderStageCs
    QColor(0, 107, 107),  // kShaderStageGs
    QColor(128, 90, 30),  // kShaderStageHs
    QColor(55, 55, 200),  // kShaderStagePs
    QColor(200, 35, 35)
};  // kShaderStageVs

}  // namespace Dive

#ifndef _MSC_VER
#    pragma GCC diagnostic pop
#endif