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

#include "color_utils.h"

#include <QApplication>
#include <QPalette>

QColor GetTextAccentColor(QPalette::ColorGroup cg)
{
    return GetTextAccentColor(QApplication::palette(), cg);
}

QColor GetTextAccentColor(const QPalette& palette, QPalette::ColorGroup cg)
{
    // Use link color for text accent.
    return palette.color(cg, QPalette::Link);
}
