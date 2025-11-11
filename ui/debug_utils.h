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

#include <chrono>
#include <type_traits>
#include <utility>

template<typename Func> class DebugScopeTimer
{
public:
    DebugScopeTimer(Func&& func) :
        m_func(std::forward<Func>(func)),
        m_start(std::chrono::steady_clock::now())
    {
    }
    ~DebugScopeTimer() { Invoke(); }
    DebugScopeTimer(const DebugScopeTimer&) = delete;
    DebugScopeTimer(DebugScopeTimer&&) = delete;
    DebugScopeTimer& operator=(const DebugScopeTimer&) = delete;
    DebugScopeTimer& operator=(DebugScopeTimer&&) = delete;

private:
    double Duration() const
    {
        return std::chrono::duration_cast<std::chrono::duration<double>>(
               std::chrono::steady_clock::now() - m_start)
        .count();
    }
    void Invoke() { m_func(Duration()); }

    std::decay_t<Func> m_func;

    std::chrono::steady_clock::time_point m_start;
};

template<typename Func> DebugScopeTimer(Func&&) -> DebugScopeTimer<Func>;
