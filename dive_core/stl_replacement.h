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

// Provides a replacement of some STL containers. The reason for this is that Windows DEBUG versions
// of STL libraries are notoriously slow (multiple orders-of-magnitude slower than RELEASE), so a
// replacement of simplified and less-safe version of these STL containers is warranted to make
// DEBUG useful. Only the simplest functions are provided here

namespace Dive
{

template<class Type> class Vector
{
public:
    Vector();
    Vector(Vector<Type> &&a);
    Vector(const Vector<Type> &a);
    Vector(uint64_t size);
    Vector(std::initializer_list<Type> a);
    ~Vector();
    Type         &operator[](uint64_t i) const;
    Vector<Type> &operator=(const Vector<Type> &a);
    Vector<Type> &operator=(Vector<Type> &&a);
    Type         *data() const;
    Type         &front() const;
    Type         &back() const;
    uint64_t      size() const;
    uint64_t      capacity() const;
    bool          empty() const;
    void          push_back(const Type &a);
    void          push_back(Type &&a);
    void          pop_back();

    template<typename... Args> void emplace_back(Args &&...args);

    void resize(uint64_t size);
    void resize(uint64_t size, const Type &a);
    void reserve(uint64_t size);
    void clear();

    Type *begin() { return m_buffer; }
    Type *end() { return m_buffer + m_size; }

    Type const *begin() const { return m_buffer; }
    Type const *end() const { return m_buffer + m_size; }

private:
    void     internal_clear();
    Type    *m_buffer;
    uint64_t m_reserved;
    uint64_t m_size;
};

}  // namespace Dive

#include "stl_replacement.hpp"

template<typename T>
// using DiveVector = std::vector<T>;
using DiveVector = Dive::Vector<T>;