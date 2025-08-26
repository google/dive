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

#include "dive_pm4_capture.h"

#include "util/logging.h"
#if defined(__ANDROID__)

#    include <dlfcn.h>

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

DivePM4Capture::DivePM4Capture()
{

    GFXRECON_LOG_INFO("Initializing PM4 capture");
    void* handle = dlopen(NULL, RTLD_LAZY);
    if (!handle)
    {
        GFXRECON_LOG_ERROR("Error in dlopen: %s", dlerror());
        return;
    }
    // Clear any existing errors
    dlerror();

    m_pm4_start_func = reinterpret_cast<CaptureFunc>(dlsym(handle, "StartCapture"));
    const char* dlsym_error = dlerror();
    if (dlsym_error != nullptr)
    {
        GFXRECON_LOG_INFO("dlsym error for StartCapture: %s", dlsym_error);
        return;
    }

    m_pm4_stop_func = reinterpret_cast<CaptureFunc>(dlsym(handle, "StopCapture"));
    dlsym_error = dlerror();
    if (dlsym_error != nullptr)
    {
        GFXRECON_LOG_INFO("dlsym error for StopCapture: %s", dlsym_error);
        return;
    }
    m_is_initialized = true;
}

bool DivePM4Capture::TryStartCapture()
{
    if (!m_is_initialized || m_pm4_start_func == nullptr)
    {
        return false;
    }
    if (m_is_capturing)
    {
        GFXRECON_LOG_INFO("PM4 capture already started");
        return false;
    }
    // Call into libwrap to start capture
    m_pm4_start_func();
    m_is_capturing = true;
    GFXRECON_LOG_INFO("PM4 capture started");

    return true;
}

bool DivePM4Capture::TryStopCapture()
{
    if (!m_is_initialized || m_pm4_stop_func == nullptr)
    {
        return false;
    }
    if (!m_is_capturing)
    {
        GFXRECON_LOG_INFO("PM4 capture not started");
        return false;
    }

    // Call into libwrap to stop capture
    m_pm4_stop_func();
    m_is_capturing = false;
    GFXRECON_LOG_INFO("PM4 capture stopped");
    return true;
}

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)
#endif
