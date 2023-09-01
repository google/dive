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

// Warning: This is a common file that is shared with the Dive GUI tool!

#pragma once
#include <stdint.h>

namespace Dive
{

//--------------------------------------------------------------------------------------------------
// Provides interface for memory accesses
class IMemoryManager
{
public:
    // Copy the given va/size from the memory blocks
    virtual bool CopyMemory(void*    buffer_ptr,
                            uint32_t submit_index,
                            uint64_t va_addr,
                            uint64_t size) const = 0;

    // For resources of unknown size (eg. shaders). The caller is responsible (via return value of
    // callback) with notifying when end of resource is reached. Otherwise MemoryManager will keep
    // calling the callback until no more contiguous memory is available for copying.
    typedef bool (*PfnGetMemory)(const void* data_ptr,
                                 uint64_t    va_addr,
                                 uint64_t    size,
                                 void*       user_ptr);
    virtual bool GetMemoryOfUnknownSizeViaCallback(uint32_t     submit_index,
                                                   uint64_t     va_addr,
                                                   PfnGetMemory data_callback,
                                                   void*        user_ptr) const = 0;

    // Given an address, find the maximum contiguous amount of memory accessible from that point
    virtual uint64_t GetMaxContiguousSize(uint32_t submit_index, uint64_t va_addr) const = 0;

    // Determine whether the given range is valid (ie: covered by memory blocks or maps)
    virtual bool IsValid(uint32_t submit_index, uint64_t addr, uint64_t size) const = 0;
};

}  // namespace Dive
