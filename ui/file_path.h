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

#pragma once

#include <filesystem>

#include <QMetaType>
#include <QString>

#include "utils/component_files.h"

namespace Dive
{

// Qt compatible std::filesystem::path .
struct FilePath
{
    std::filesystem::path value;

    std::string ToString() const { return value.string(); }
    QString     ToQString() const { return QString::fromStdString(value.string()); }
};

}  // namespace Dive
// qRegisterMetaType in custom_metatypes.cpp
Q_DECLARE_METATYPE(Dive::FilePath)
Q_DECLARE_METATYPE(Dive::ComponentFilePaths)
