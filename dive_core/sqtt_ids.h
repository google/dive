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
#include <type_traits>

#include "info_id.h"

namespace Dive
{
constexpr uint32_t kNumMe = 3;
constexpr uint32_t kNumPipesPerMe = 4;
constexpr uint32_t kNumSqttStreams = 1 + kNumPipesPerMe;  // 1 graphics, 4 compute

class SqttStreamId
{
public:
    using basic_type = uint8_t;
    static const SqttStreamId kGraphics;
    static const SqttStreamId kNone;
    inline SqttStreamId() : m_id(0) {}
    explicit inline SqttStreamId(uint8_t id) : m_id(id) {}
    SqttStreamId(uint8_t me_id, uint8_t pipe_id);
    bool        operator==(const SqttStreamId &other) const { return m_id == other.m_id; }
    bool        operator!=(const SqttStreamId &other) const { return m_id != other.m_id; }
    inline bool IsValid() const { return m_id < kNumSqttStreams; }

    template<typename Num,
             typename = typename std::enable_if<std::is_integral<Num>::value, Num>::type>
    explicit operator Num() const
    {
        return m_id;
    }

    const char *string() const;

private:
    uint8_t m_id = UINT8_MAX;
};

}  // namespace Dive