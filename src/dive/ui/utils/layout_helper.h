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

#include <QLayout>
#include <QObject>
#include <QWidget>
#include <utility>

#include "dive/ui/utils/lint.h"

// Examples
//
// Setup layout for a widget:
//   auto layout = NewWidgetLayout<QHBoxLayout>(widget);
//   auto* label = layout.New<QLabel>(tr("text"));
//
// Add a new item to an existing layout:
//   auto* label = NewWidgetLayout(layout).New<QLabel>(tr("text"));
//

template <typename LayoutT>
struct LayoutHelper
{
    static_assert((std::is_convertible_v<LayoutT*, QLayout*>));

    LayoutT* m_layout = nullptr;
    template <typename T, typename... Args>
    T* NewWidget(Args&&... args)
    {
        static_assert((std::is_convertible_v<T*, QWidget*>));
        auto widget = DiveLint::QtOwned(new T(std::forward<Args>(args)...));
        m_layout->addWidget(widget);
        return widget;
    }

    template <typename T, typename... Args>
    LayoutHelper<T> NewLayout(Args&&... args)
    {
        static_assert((std::is_convertible_v<T*, QWidget*>));
        auto layout = DiveLint::QtOwned(new T(std::forward<Args>(args)...));
        m_layout->addLayout(layout);
        return LayoutHelper<T>{layout};
    }

    template <typename T, typename... Args>
    auto New(Args&&... args)
    {
        if constexpr (std::is_convertible_v<T*, QLayout*>)
        {
            return NewLayout<T, Args...>(std::forward<Args>(args)...);
        }
        else
        {
            return NewWidget<T, Args...>(std::forward<Args>(args)...);
        }
    }
};

template <typename T, typename... Args>
LayoutHelper<T> NewWidgetLayout(Args&&... args)
{
    static_assert((std::is_convertible_v<T*, QLayout*>));
    static_assert(((std::is_convertible_v<Args, QWidget*>) || ...));
    return LayoutHelper<T>{DiveLint::QtNew<T>(std::forward<Args>(args)...)};
}
