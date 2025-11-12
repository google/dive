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
#include <utility>

template<typename FuncT> class DebugScoped;

#ifdef NDEBUG
template<typename FuncT> class DebugScoped
{
public:
    explicit DebugScoped(FuncT&&) {};
    ~DebugScoped() { (void)this; }

    DebugScoped(const DebugScoped&) = delete;
    DebugScoped(DebugScoped&&) = default;
    DebugScoped& operator=(const DebugScoped&) = delete;
    DebugScoped& operator=(DebugScoped&&) = delete;
};
#else
template<typename FuncT> class DebugScoped
{
public:
    explicit DebugScoped(FuncT&& func) :
        m_func(std::forward<FuncT>(func))
    {
    }
    ~DebugScoped() { Invoke(); }

    DebugScoped(const DebugScoped&) = delete;
    DebugScoped(DebugScoped&& other) { std::swap(m_func, other.m_func); }
    DebugScoped& operator=(const DebugScoped&) = delete;
    DebugScoped& operator=(DebugScoped&&) = delete;

private:
    std::optional<FuncT> m_func;

    void Invoke()
    {
        if (m_func)
        {
            (*m_func)();
        }
    }
};
#endif

template<typename FuncT, typename... ArgsT> DebugScoped(FuncT&&) -> DebugScoped<FuncT>;

template<typename FuncT> inline auto DebugScopedStopwatch(FuncT&& func)
{
    return DebugScoped([func = func, start = std::chrono::steady_clock::now()]() {
        auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(
                        std::chrono::steady_clock::now() - start)
                        .count();
        func(duration);
    });
}
