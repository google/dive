/*
 Copyright 2022 Google LLC

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

namespace Dive
{

enum class CrossRefType : int
{
    kNone,
    kEvent,
    kBarrier,
    kNodeIndex,
    kShaderAddress,
    kGFRIndex,  // It's a NodeIndex happened to be reference to GFR
    // kWavestate
};

class CrossRef
{
public:
    typedef uint64_t IdType;

    CrossRef(CrossRefType type = CrossRefType::kNone, IdType id = 0) :
        m_type(type),
        m_id(id)
    {
    }

    CrossRef(const CrossRef&) = default;
    CrossRef& operator=(const CrossRef&) = default;

    inline bool operator==(const CrossRef& rhs) { return m_type == rhs.m_type && m_id == rhs.m_id; }
    inline bool operator!=(const CrossRef& rhs) { return !(*this == rhs); }

    inline CrossRefType Type() const { return m_type; }
    inline IdType       Id() const { return m_id; }

private:
    CrossRefType m_type;
    IdType       m_id;
};

}  // namespace Dive
