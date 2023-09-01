/*
 Copyright 2020 Google LLC

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
#include <cstddef>
#include <memory>
#include <new>

namespace Dive
{
//--------------------------------------------------------------------------------------------------
// StructOfArraysIterator is an iterator type for any of the generated structure-of-array classes
// (e.g. marker_types.h).
//
// This enables full read and write access to the element fields:
//     ```
//     for(auto it=soa.begin(); it != soa.end(); ++it) {
//       auto val = it->MyField();
//       it->SetMyOtherField(val+1);
//     }
//     ```
template<typename Class, typename Id, typename RefT> class StructOfArraysIterator
{
public:
    using difference_type = ptrdiff_t;
    using value_type = RefT;
    using pointer = RefT*;
    using reference = RefT;
    using iterator_category = std::random_access_iterator_tag;
    StructOfArraysIterator() = default;
    StructOfArraysIterator(StructOfArraysIterator&& other) :
        m_ref(other.m_ref.m_obj_ptr, other.m_ref.m_id)
    {}
    StructOfArraysIterator(const StructOfArraysIterator& other) :
        m_ref(other.m_ref.m_obj_ptr, other.m_ref.m_id)
    {}
    StructOfArraysIterator(Class* obj, Id id) : m_ref(obj, id) {}

    StructOfArraysIterator& operator=(const StructOfArraysIterator& other)
    {
        assign(other.obj_ptr(), other->id());
        return *this;
    }

    StructOfArraysIterator& operator=(std::nullptr_t)
    {
        clear();
        return *this;
    }

    StructOfArraysIterator& operator++()
    {
        m_ref.m_id = Id((typename Id::basic_type)m_ref.m_id + 1);
        return *this;
    }
    StructOfArraysIterator operator++(int)
    {
        auto temp = *this;
        m_ref.m_id = Id((typename Id::basic_type)m_ref.m_id + 1);
        return temp;
    }
    StructOfArraysIterator& operator--()
    {
        m_ref.m_id = Id((typename Id::basic_type)m_ref.m_id - 1);
        return *this;
    }
    StructOfArraysIterator operator--(int)
    {
        auto temp = *this;
        m_ref.m_id = Id((typename Id::basic_type)m_ref.m_id - 1);
        return temp;
    }
    StructOfArraysIterator& operator+=(difference_type n)
    {
        difference_type i = (difference_type)m_ref.m_id + n;
        if (i < std::numeric_limits<typename Id::basic_type>::min() ||
            i > std::numeric_limits<typename Id::basic_type>::max())
            m_ref.m_id = Id(std::numeric_limits<typename Id::basic_type>::max());
        else
            m_ref.m_id = Id(static_cast<typename Id::basic_type>(i));
        return *this;
    }
    StructOfArraysIterator& operator-=(difference_type n)
    {
        difference_type i = (difference_type)m_ref.m_id - n;
        if (i < std::numeric_limits<typename Id::basic_type>::min() ||
            i > std::numeric_limits<typename Id::basic_type>::max())
            m_ref.m_id = Id(std::numeric_limits<typename Id::basic_type>::max());
        else
            m_ref.m_id = Id(static_cast<typename Id::basic_type>(i));
        return *this;
    }
    StructOfArraysIterator operator+(difference_type n) const
    {
        difference_type i = (difference_type)m_ref.m_id + n;
        if (i < std::numeric_limits<typename Id::basic_type>::min() ||
            i > std::numeric_limits<typename Id::basic_type>::max())
            i = std::numeric_limits<typename Id::basic_type>::max();
        return StructOfArraysIterator(m_ref.m_obj_ptr, Id(static_cast<typename Id::basic_type>(i)));
    }
    friend StructOfArraysIterator operator+(difference_type n, const StructOfArraysIterator& it)
    {
        difference_type i = (difference_type)it.m_ref.id() + n;
        if (i < std::numeric_limits<typename Id::basic_type>::min() ||
            i > std::numeric_limits<typename Id::basic_type>::max())
            i = std::numeric_limits<typename Id::basic_type>::max();
        return StructOfArraysIterator(&it.m_ref.obj(), Id(static_cast<typename Id::basic_type>(i)));
    }
    StructOfArraysIterator operator-(difference_type n) const
    {
        difference_type i = (difference_type)m_ref.m_id - n;
        if (i < std::numeric_limits<typename Id::basic_type>::min() ||
            i > std::numeric_limits<typename Id::basic_type>::max())
            i = std::numeric_limits<typename Id::basic_type>::max();
        return StructOfArraysIterator(m_ref.m_obj_ptr, Id(static_cast<typename Id::basic_type>(i)));
    }
    RefT        operator*() const { return m_ref; }
    const RefT* operator->() const { return &m_ref; }
    RefT        operator[](difference_type n) const { return *((*this) + n); }

    friend difference_type operator-(const StructOfArraysIterator& left,
                                     const StructOfArraysIterator& right)
    {
        return left.diff(right.obj_ptr(), right->id());
    }
    friend bool operator==(const StructOfArraysIterator& left, const StructOfArraysIterator& right)
    {
        return left.eq(right.obj_ptr(), right->id());
    }
    friend bool operator==(const StructOfArraysIterator& left, std::nullptr_t)
    {
        return !left->IsValid();
    }
    friend bool operator==(std::nullptr_t, const StructOfArraysIterator& right)
    {
        return !right->IsValid();
    }
    friend bool operator!=(const StructOfArraysIterator& left, const StructOfArraysIterator& right)
    {
        return !left.eq(right.obj_ptr(), right->id());
    }
    friend bool operator!=(const StructOfArraysIterator& left, std::nullptr_t)
    {
        return left->IsValid();
    }
    friend bool operator!=(std::nullptr_t, const StructOfArraysIterator& right)
    {
        return right->IsValid();
    }
    friend bool operator<(const StructOfArraysIterator& left, const StructOfArraysIterator& right)
    {
        return left.lt(right.obj_ptr(), right->id());
    }
    friend bool operator<=(const StructOfArraysIterator& left, const StructOfArraysIterator& right)
    {
        return !right.lt(left.obj_ptr(), left->id());
    }
    friend bool operator>(const StructOfArraysIterator& left, const StructOfArraysIterator& right)
    {
        return right.lt(left.obj_ptr(), left->id());
    }
    friend bool operator>=(const StructOfArraysIterator& left, const StructOfArraysIterator& right)
    {
        return !left.lt(right.obj_ptr(), right->id());
    }

    template<typename _Class, typename _Id, typename _ConstRefT, typename _RefT>
    friend class StructOfArraysConstIterator;

protected:
    void assign(Class* obj_ptr, Id id)
    {
        m_ref.m_obj_ptr = obj_ptr;
        m_ref.m_id = id;
    }
    void clear()
    {
        m_ref.m_obj_ptr = nullptr;
        m_ref.m_id = Id();
    }
    difference_type diff(std::add_const_t<Class>* obj_ptr, Id id) const
    {
        DIVE_ASSERT(m_ref.m_obj_ptr == obj_ptr);
        return (difference_type)(*this)->id() - (difference_type)id;
    }
    Class* obj_ptr() const { return m_ref.m_obj_ptr; }
    bool   eq(std::add_const_t<Class>* obj_ptr, Id id) const
    {
        return ((*this).obj_ptr() == obj_ptr) &&
               (((*this)->id() == id) ||
                (!(*this)->IsValid() && (obj_ptr == nullptr || !obj_ptr->IsValidId(id))));
    }
    bool lt(std::add_const_t<Class>* obj_ptr, Id id) const
    {
        return ((*this).obj_ptr() == obj_ptr) && ((*this)->id() < id);
    }

private:
    RefT m_ref;
};

template<typename Class, typename Id, typename ConstRefT, typename RefT>
class StructOfArraysConstIterator : public StructOfArraysIterator<const Class, Id, ConstRefT>
{
public:
    StructOfArraysConstIterator() = default;
    StructOfArraysConstIterator(StructOfArraysConstIterator&& other) = default;
    StructOfArraysConstIterator(const StructOfArraysConstIterator& other) = default;
    StructOfArraysConstIterator(const StructOfArraysIterator<Class, Id, RefT>& other) :
        StructOfArraysIterator<const Class, Id, ConstRefT>(other.obj_ptr(), other->id())
    {}
    StructOfArraysConstIterator(const Class* obj, Id id) :
        StructOfArraysIterator<const Class, Id, ConstRefT>(obj, id)
    {}

    StructOfArraysConstIterator& operator=(const StructOfArraysConstIterator& other)
    {
        this->assign(other.obj_ptr(), other->id());
        return *this;
    }
    StructOfArraysConstIterator& operator=(const StructOfArraysIterator<Class, Id, RefT>& other)
    {
        this->assign(other.obj_ptr(), other->id());
        return *this;
    }
    StructOfArraysConstIterator& operator=(std::nullptr_t)
    {
        this->clear();
        return *this;
    }
    using difference_type = typename StructOfArraysIterator<const Class, Id, ConstRefT>::
    difference_type;
    friend difference_type operator-(const StructOfArraysConstIterator&             left,
                                     const StructOfArraysIterator<Class, Id, RefT>& right)
    {
        return left - StructOfArraysConstIterator(right);
    }
    friend difference_type operator-(const StructOfArraysIterator<Class, Id, RefT>& left,
                                     const StructOfArraysConstIterator&             right)
    {
        return StructOfArraysConstIterator(left) - right;
    }
    friend bool operator==(const StructOfArraysConstIterator&             left,
                           const StructOfArraysIterator<Class, Id, RefT>& right)
    {
        return left == StructOfArraysConstIterator(right);
    }
    friend bool operator==(const StructOfArraysIterator<Class, Id, RefT>& left,
                           const StructOfArraysConstIterator&             right)
    {
        return StructOfArraysConstIterator(left) == right;
    }
    friend bool operator!=(const StructOfArraysConstIterator&             left,
                           const StructOfArraysIterator<Class, Id, RefT>& right)
    {
        return left != StructOfArraysConstIterator(right);
    }
    friend bool operator!=(const StructOfArraysIterator<Class, Id, RefT>& left,
                           const StructOfArraysConstIterator&             right)
    {
        return StructOfArraysConstIterator(left) != right;
    }
    friend bool operator<(const StructOfArraysConstIterator&             left,
                          const StructOfArraysIterator<Class, Id, RefT>& right)
    {
        return left < StructOfArraysConstIterator(right);
    }
    friend bool operator<(const StructOfArraysIterator<Class, Id, RefT>& left,
                          const StructOfArraysConstIterator&             right)
    {
        return StructOfArraysConstIterator(left) < right;
    }
    friend bool operator<=(const StructOfArraysConstIterator&             left,
                           const StructOfArraysIterator<Class, Id, RefT>& right)
    {
        return left <= StructOfArraysConstIterator(right);
    }
    friend bool operator<=(const StructOfArraysIterator<Class, Id, RefT>& left,
                           const StructOfArraysConstIterator&             right)
    {
        return StructOfArraysConstIterator(left) <= right;
    }
    friend bool operator>(const StructOfArraysConstIterator&             left,
                          const StructOfArraysIterator<Class, Id, RefT>& right)
    {
        return left > StructOfArraysConstIterator(right);
    }
    friend bool operator>(const StructOfArraysIterator<Class, Id, RefT>& left,
                          const StructOfArraysConstIterator&             right)
    {
        return StructOfArraysConstIterator(left) > right;
    }
    friend bool operator>=(const StructOfArraysConstIterator&             left,
                           const StructOfArraysIterator<Class, Id, RefT>& right)
    {
        return left >= StructOfArraysConstIterator(right);
    }
    friend bool operator>=(const StructOfArraysIterator<Class, Id, RefT>& left,
                           const StructOfArraysConstIterator&             right)
    {
        return StructOfArraysConstIterator(left) >= right;
    }
};
}  // namespace Dive