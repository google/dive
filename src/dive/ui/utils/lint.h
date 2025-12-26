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

#include <QObject>
#include <type_traits>
#include <utility>

#include "dive/lint/gsl.h"

namespace DiveLint
{

// Tell lint that this pointer uses Qt ownership semantics.
template <typename T>
constexpr T* QtOwned(gsl::owner<T*> ptr)
{
    return ptr;  // NOLINT
}

// Create a new Qt object.
// - Suppress clang-tidy/cppcoreguidelines-owning-memory
// - Heuristically checked that the object have parent.
template <typename T, typename... Args>
T* QtNew(Args&&... args)
{
    // Check at least one parameter can be converted to QObject for parent.
    // Otherwise explicitly annotate so.
    static_assert(((std::is_convertible_v<Args, QObject*>) || ...) ||
                  ((std::is_convertible_v<Args, QObject&>) || ...));
    return QtOwned(new T(std::forward<Args>(args)...));
}

// Tell lint that we intend this pointer to be unowned for now.
// e.g. we are adding it to a model: model->appendRow(QtNewUnowned<Item>())
template <typename T, typename... Args>
T* QtNewUnowned(Args&&... args)
{
    return QtOwned(new T(std::forward<Args>(args)...));
}

}  // namespace DiveLint
