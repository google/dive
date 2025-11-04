/*
Copyright 2025 Google Inc.

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

#ifndef GFXRECON_DECODE_DIVE_PM4_CAPTURE_H
#define GFXRECON_DECODE_DIVE_PM4_CAPTURE_H

#include "util/defines.h"

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

#if defined(__ANDROID__)

bool IsPm4CaptureEnabledByProperty();

class DivePM4Capture
{
public:
    using CaptureFunc = void (*)();

    static DivePM4Capture& GetInstance()
    {
        static DivePM4Capture instance;
        return instance;
    }

    bool IsPM4CaptureEnabled() const
    {
        return m_is_initialized && m_pm4_start_func != nullptr && m_pm4_stop_func != nullptr;
    }

    bool TryStartCapture();
    bool TryStopCapture();
    bool IsCapturing() const { return m_is_capturing; }

private:
    DivePM4Capture();

    bool        m_is_initialized = false;
    bool        m_is_capturing = false;
    CaptureFunc m_pm4_start_func = nullptr;
    CaptureFunc m_pm4_stop_func = nullptr;
};

#endif
GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)

#endif  // GFXRECON_DECODE_DIVE_PM4_CAPTURE_H
