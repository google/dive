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
#include "sqtt_ids.h"

namespace Dive
{

// =================================================================================================
// SqttStreamId
// =================================================================================================
const SqttStreamId SqttStreamId::kGraphics = SqttStreamId();
const SqttStreamId SqttStreamId::kNone = SqttStreamId(UINT8_MAX);

//--------------------------------------------------------------------------------------------------
SqttStreamId::SqttStreamId(uint8_t me_id, uint8_t pipe_id)
{
    if (me_id == 0)
    {
        DIVE_ASSERT(pipe_id == 0);
        m_id = 0;
    }
    else if (me_id == 1)
    {
        DIVE_ASSERT(pipe_id < kNumPipesPerMe);
        m_id = pipe_id + 1;
    }
    else
    {
        assert(false);
    }
}

const char *SqttStreamId::string() const
{
    switch (m_id)
    {
    case 0: return "Gfx";
    case 1: return "Ace A";
    case 2: return "Ace B";
    case 3: return "Ace C";
    case 4: return "Ace D";
    default: return nullptr;
    }
}

}  // namespace Dive
