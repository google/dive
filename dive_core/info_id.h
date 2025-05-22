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
#include <cstdint>
#include <limits>
#include <type_traits>

#include "common.h"

namespace Dive
{
template<typename I, size_t kMaxBits, typename = std::is_unsigned<I>> constexpr I FullBitMask()
{
    static_assert(kMaxBits <= sizeof(I) * 8,
                  "FullBitMask: kMaxBits cannot exceed the number of bits in the integer type");
    if constexpr (kMaxBits == sizeof(I) * 8)
        return std::numeric_limits<I>::max();
    else
        return (I(1u) << kMaxBits) - 1;
}

template<typename Tag, typename I = uint32_t, size_t kMaxBits = 32> class InfoIdT
{
public:
    using basic_type = I;
    InfoIdT() : m_id(std::numeric_limits<I>::max()) {}
    explicit InfoIdT(I id) : m_id(id)
    {
        const I mask = FullBitMask<I, kMaxBits>();
        if (m_id >= mask)
        {
            DIVE_ASSERT((m_id & mask) == mask);
            m_id = std::numeric_limits<I>::max();
        }
    }
    template<typename Num,
             typename = typename std::enable_if<
             std::is_integral<Num>::value &&
             std::numeric_limits<Num>::min() <= std::numeric_limits<I>::min() &&
             std::numeric_limits<I>::max() <= std::numeric_limits<Num>::max(),
             Num>::type>
    explicit operator Num() const
    {
        return m_id;
    }
    bool operator==(InfoIdT other) const { return m_id == other.m_id; }
    bool operator!=(InfoIdT other) const { return m_id != other.m_id; }
    bool operator<(InfoIdT other) const { return m_id < other.m_id; }
    bool operator<=(InfoIdT other) const { return m_id <= other.m_id; }
    bool operator>(InfoIdT other) const { return m_id > other.m_id; }
    bool operator>=(InfoIdT other) const { return m_id >= other.m_id; }

    InfoIdT &operator++()
    {
        ++m_id;
        return *this;
    }
    InfoIdT operator++(int)
    {
        InfoIdT old = *this;
        ++m_id;
        return old;
    }
    InfoIdT &operator--()
    {
        --m_id;
        return *this;
    }
    InfoIdT operator--(int)
    {
        InfoIdT old = *this;
        --m_id;
        return old;
    }

private:
    I m_id;
};
}  // namespace Dive