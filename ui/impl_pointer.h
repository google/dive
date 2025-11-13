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

#include <memory>
#include <utility>

// Pointer to implementation wrapper
template<typename T> class ImplPointer
{
public:
    template<typename... Args>
    explicit ImplPointer(Args&&... args) :
        m_ptr(std::make_unique<T>(std::forward<Args>(args)...))
    {
    }
    ~ImplPointer() = default;

    ImplPointer(const ImplPointer&) = delete;
    ImplPointer(ImplPointer&&) = delete;
    ImplPointer& operator=(const ImplPointer&) = delete;
    ImplPointer& operator=(ImplPointer&&) = delete;

    T* operator->() { return m_ptr.get(); }
    T& operator*() { return *m_ptr; }

    const T* operator->() const { return m_ptr.get(); }
    const T& operator*() const { return *m_ptr; }

    // std::unique_ptr::get()
    T* get() const { return m_ptr.get(); }

private:
    std::unique_ptr<T> m_ptr;
};
