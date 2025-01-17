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
#include <algorithm>
#include "common/common.h"

namespace Dive
{

//--------------------------------------------------------------------------------------------------
template<class Type>
Vector<Type>::Vector() :
    m_buffer(nullptr),
    m_reserved(0),
    m_size(0)
{
}

//--------------------------------------------------------------------------------------------------
template<class Type>
Vector<Type>::Vector(Vector &&a) :
    m_buffer(a.m_buffer),
    m_reserved(a.m_reserved),
    m_size(a.m_size)
{
    a.m_buffer = nullptr;
    a.m_reserved = 0;
    a.m_size = 0;
}

//--------------------------------------------------------------------------------------------------
template<class Type>
Vector<Type>::Vector(const Vector &a) :
    m_buffer(nullptr),
    m_reserved(0)
{
    // Do not call resize() directly, since it invokes default constructor
    // And not all classes have default constructors
    reserve(a.m_size);
    m_size = a.m_size;
    std::copy(a.m_buffer, a.m_buffer + a.m_size, m_buffer);
}

//--------------------------------------------------------------------------------------------------
template<class Type>
Vector<Type>::Vector(uint64_t size) :
    m_buffer(nullptr),
    m_reserved(0),
    m_size(0)
{
    reserve(size);

    // Call the constructor for all the new elements
    for (uint64_t i = m_size; i < size; ++i)
        new (&m_buffer[i]) Type();
    m_size = size;
}

//--------------------------------------------------------------------------------------------------
template<class Type>
Vector<Type>::Vector(std::initializer_list<Type> a) :
    m_buffer(nullptr),
    m_reserved(0),
    m_size(0)
{
    reserve(a.size());
    m_size = a.size();
    std::copy(a.begin(), a.end(), m_buffer);
}

//--------------------------------------------------------------------------------------------------
template<class Type> Vector<Type>::~Vector()
{
    internal_clear();
}

//--------------------------------------------------------------------------------------------------
template<class Type> Type &Vector<Type>::operator[](uint64_t i) const
{
    DIVE_ASSERT(i < m_size);
    return m_buffer[i];
}

//--------------------------------------------------------------------------------------------------
template<class Type> Vector<Type> &Vector<Type>::operator=(const Vector<Type> &a)
{
    if (&a != this)
    {
        // Do not call resize() directly, since it invokes default constructor
        // And not all classes have default constructors
        reserve(a.m_size);
        m_size = a.m_size;
        std::copy(a.m_buffer, a.m_buffer + a.m_size, m_buffer);
    }
    return *this;
}

//--------------------------------------------------------------------------------------------------
template<class Type> Vector<Type> &Vector<Type>::operator=(Vector<Type> &&a)
{
    if (&a != this)
    {
        m_buffer = a.m_buffer;
        m_reserved = a.m_reserved;
        m_size = a.m_size;
        a.m_buffer = nullptr;
        a.m_reserved = 0;
        a.m_size = 0;
    }
    return *this;
}

//--------------------------------------------------------------------------------------------------
template<class Type> Type *Vector<Type>::data() const
{
    return m_buffer;
}

//--------------------------------------------------------------------------------------------------
template<class Type> Type &Vector<Type>::front() const
{
    return *m_buffer;
}

//--------------------------------------------------------------------------------------------------
template<class Type> Type &Vector<Type>::back() const
{
    return m_buffer[m_size - 1];
}

//--------------------------------------------------------------------------------------------------
template<class Type> uint64_t Vector<Type>::size() const
{
    return m_size;
}

//--------------------------------------------------------------------------------------------------
template<class Type> uint64_t Vector<Type>::capacity() const
{
    return m_reserved;
}

//--------------------------------------------------------------------------------------------------
template<class Type> bool Vector<Type>::empty() const
{
    return m_size == 0;
}

//--------------------------------------------------------------------------------------------------
template<class Type> void Vector<Type>::push_back(const Type &a)
{
    reserve(m_size + 1);
    new (&m_buffer[m_size]) Type(a);
    ++m_size;
}

//--------------------------------------------------------------------------------------------------
template<class Type> void Vector<Type>::push_back(Type &&a)
{
    emplace_back(std::move(a));
}

//--------------------------------------------------------------------------------------------------
template<class Type> void Vector<Type>::pop_back()
{
    if (m_size > 0)
    {
        m_buffer[m_size - 1].~Type();
        --m_size;
    }
}

//--------------------------------------------------------------------------------------------------
template<class Type> template<typename... Args> void Vector<Type>::emplace_back(Args &&...args)
{
    reserve(m_size + 1);

    // Construct the element in place
    new (m_buffer + m_size) Type(std::forward<Args>(args)...);
    ++m_size;
}

//--------------------------------------------------------------------------------------------------
template<class Type> void Vector<Type>::resize(uint64_t size)
{
    reserve(size);

    if (!std::is_trivially_constructible<Type>::value)
    {
        // Call the constructor for all the new elements
        for (uint64_t i = m_size; i < size; ++i)
            new (&m_buffer[i]) Type();
    }
    m_size = size;
}

//--------------------------------------------------------------------------------------------------
template<class Type> void Vector<Type>::resize(uint64_t size, const Type &a)
{
    reserve(size);

    // Call the copy constructor for all the new elements
    for (uint64_t i = m_size; i < size; ++i)
        new (&m_buffer[i]) Type(a);
    m_size = size;
}

//--------------------------------------------------------------------------------------------------
template<class Type> void Vector<Type>::reserve(uint64_t size)
{
    if (size > m_reserved)
    {
        // Round up to nearest power of 2
        m_reserved = size;
        m_reserved--;
        m_reserved |= m_reserved >> 1;
        m_reserved |= m_reserved >> 2;
        m_reserved |= m_reserved >> 4;
        m_reserved |= m_reserved >> 8;
        m_reserved |= m_reserved >> 16;
        m_reserved |= m_reserved >> 32;
        m_reserved++;

        // Can't directly 'new' an array of Type, because Type is not guaranteed to have a default
        // constructor. So use an 'operator new' instead, which doesn't call the constructor
        Type *new_buffer = (Type *)operator new[](m_reserved * sizeof(Type));
        if (m_buffer != nullptr)
        {
            // Do a move-constructor
            // Hopefully(!) the compiler knows to optimize this to memmove/memcpy for raw data
            for (uint32_t i = 0; i < m_size; ++i)
            {
                new (&new_buffer[i]) Type(std::move(m_buffer[i]));
                m_buffer[i].~Type();
            }
            // operator delete[] does not call destructors.
            // Destructors have already been explicitly called
            operator delete[](m_buffer);
        }
        m_buffer = new_buffer;
    }
}

//--------------------------------------------------------------------------------------------------
template<class Type> void Vector<Type>::clear()
{
    internal_clear();
}

//--------------------------------------------------------------------------------------------------
template<class Type> void Vector<Type>::internal_clear()
{
    if (m_buffer != nullptr)
    {
        // Need to explicitly call the destructors of each element, since deallocation happens
        // as a typecast to void*
        for (uint32_t i = 0; i < m_size; ++i)
            m_buffer[i].~Type();

        // Allocated as raw void* type in reserve(), so deallocate in the same way
        operator delete[](m_buffer);
    }
    m_buffer = nullptr;
    m_reserved = 0;
    m_size = 0;
}

}  // namespace Dive
