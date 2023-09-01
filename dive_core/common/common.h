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

// The common folder is used by both the GUI tool and the PAL capture code
// A macro is defined for the GUI tool
#ifndef DIVE_GUI_TOOL
#    define DIVE_PAL_CAPTURE
#endif

#ifdef DIVE_PAL_CAPTURE
#    include "palAssert.h"
#    define DIVE_ASSERT PAL_ASSERT
#    define DIVE_ERROR_MSG(_pReasonFmt, ...) PAL_ALERT_ALWAYS_MSG(_pReasonFmt, ##__VA_ARGS__)
#else

void DIVE_LOG(const char* file, int line, const char* format, ...);

#    include <assert.h>
#    include <stdio.h>
#    define DIVE_ASSERT assert
#    define DIVE_ERROR_MSG(...) DIVE_LOG(__FILE__, __LINE__, __VA_ARGS__)
#    define DIVE_TRACE(...) DIVE_LOG(__FILE__, __LINE__, __VA_ARGS__)
#endif

// This does not get compiled out, so safe to call it in RELEASE
#define DIVE_VERIFY(cond)   \
    if (!(cond))            \
    {                       \
        DIVE_ASSERT(false); \
    }
