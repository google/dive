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

#pragma once

void DIVE_LOG_INTERNAL(const char* file, int line, const char* format, ...);

#include <assert.h>

#define DIVE_ERROR_MSG(...) DIVE_LOG_INTERNAL(__FILE__, __LINE__, __VA_ARGS__)
#define DIVE_LOG(...) DIVE_LOG_INTERNAL(__FILE__, __LINE__, __VA_ARGS__)

#ifndef NDEBUG  // DEBUG
#    define DIVE_ASSERT(p) assert(p)
#    define DIVE_DEBUG_LOG(...) DIVE_LOG_INTERNAL(__FILE__, __LINE__, __VA_ARGS__)
#else
#    define DIVE_ASSERT(...) ((void)0)
#    define DIVE_DEBUG_LOG(...) ((void)0)
#endif

// This does not get compiled out, so safe to call it in RELEASE
#define DIVE_VERIFY(cond)   \
    if (!(cond))            \
    {                       \
        DIVE_ASSERT(false); \
    }
