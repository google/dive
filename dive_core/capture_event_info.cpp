/*
 Copyright 2019 Google LLC

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

#include <assert.h>

#include "capture_event_info.h"
#include "dive_core/common/pm4_packets/me_pm4_packets.h"

namespace Dive
{
// =================================================================================================
// Helper Functions
// =================================================================================================
SyncType GetSyncType(const IMemoryManager &mem_manager,
                     uint32_t              submit_index,
                     uint64_t              addr,
                     uint32_t              opcode)
{
    return SyncType::kNone;
}

//--------------------------------------------------------------------------------------------------
bool IsDrawDispatchBlitSyncEvent(const IMemoryManager &mem_manager,
                                 uint32_t              submit_index,
                                 uint64_t              addr,
                                 uint32_t              opcode)
{
    if (IsDrawDispatchEventOpcode(opcode))
        return true;

    if (IsBlitEvent(mem_manager, submit_index, addr, opcode))
        return true;

    SyncType sync_type = GetSyncType(mem_manager, submit_index, addr, opcode);
    if (sync_type != SyncType::kNone)
        return true;

    return false;
}

//--------------------------------------------------------------------------------------------------
bool IsBlitEvent(const IMemoryManager &mem_manager,
                 uint32_t              submit_index,
                 uint64_t              addr,
                 uint32_t              opcode)
{
    if (opcode == CP_BLIT)
        return true;

    if (opcode == CP_EVENT_WRITE)
    {
        PM4_CP_EVENT_WRITE packet;
        DIVE_VERIFY(mem_manager.CopyMemory(&packet, submit_index, addr, sizeof(packet)));
        if (packet.bitfields0.EVENT == BLIT)  // aka CCU_RESOLVE
            return true;
    }
    return false;
}

}  // namespace Dive