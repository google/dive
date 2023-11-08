/*
Copyright 2023 Google Inc.

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
// LAYER modified from
// https://android.googlesource.com/platform/cts/+/master/hostsidetests/gputools/layers/jni/glesLayer.cpp

#if defined __ANDROID__
#    include <EGL/egl.h>
#    include <GLES3/gl3.h>
#    include <android/log.h>
#endif

#include <cstring>

#if defined __ANDROID__

#    define xstr(a) str(a)
#    define str(a) #    a
#    define LOG_TAG "glesLayer" xstr(LAYERNAME)
#    define ALOGI(msg, ...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, (msg), __VA_ARGS__)
// Announce if anything loads this layer.  LAYERNAME is defined in Android.mk
class StaticLogMessage
{
public:
    StaticLogMessage(const char* msg) { ALOGI("%s", msg); }
};
StaticLogMessage g_initMessage("glesLayer" xstr(LAYERNAME) " loaded");
typedef __eglMustCastToProperFunctionPointerType EGLFuncPointer;
typedef void* (*PFNEGLGETNEXTLAYERPROCADDRESSPROC)(void*, const char*);
namespace
{
std::unordered_map<std::string, EGLFuncPointer> funcMap;
static int                                      frame_num = 0;

EGLAPI EGLBoolean EGLAPIENTRY glesLayer_eglInitialize(EGLDisplay dpy, EGLint* major, EGLint* minor)
{
    ALOGI("%s %lu %li %li",
          "glesLayer_eglInitialize called with parameters:",
          (unsigned long)dpy,
          major ? (long)*major : 0,
          minor ? (long)*minor : 0);
    if (funcMap.find("eglInitialize") == funcMap.end())
        ALOGI("%s", "Unable to find funcMap entry for eglInitialize");
    EGLFuncPointer entry = funcMap["eglInitialize"];
    typedef EGLBoolean (*PFNEGLINITIALIZEPROC)(EGLDisplay, EGLint*, EGLint*);
    PFNEGLINITIALIZEPROC next = reinterpret_cast<PFNEGLINITIALIZEPROC>(entry);
    return next(dpy, major, minor);
}

EGLAPI EGLBoolean EGLAPIENTRY glesLayer_eglSwapBuffers(EGLDisplay display, EGLSurface surface)
{

    frame_num++;
    const char* msg = "glesLayer_eglSwapBuffers called in glesLayer" xstr(LAYERNAME);
    ALOGI("%s", msg);
    ALOGI("frame %d", frame_num);
    if (funcMap.find("eglSwapBuffers") == funcMap.end())
        ALOGI("%s", "Unable to find funcMap entry for eglSwapBuffers");

    EGLFuncPointer entry = funcMap["eglSwapBuffers"];
    typedef EGLBoolean (*PFNEGLSWAPBUFFERSPROC)(EGLDisplay display, EGLSurface surface);
    PFNEGLSWAPBUFFERSPROC next = reinterpret_cast<PFNEGLSWAPBUFFERSPROC>(entry);

    return next(display, surface);
}

EGLAPI void* EGLAPIENTRY glesLayer_eglGetProcAddress(const char* procname)
{
    const char* msg = "glesLayer_eglGetProcAddress called in glesLayer" xstr(LAYERNAME) " for:";
    ALOGI("%s%s", msg, procname);
    if (funcMap.find("eglGetProcAddress") == funcMap.end())
        ALOGI("%s", "Unable to find funcMap entry for eglGetProcAddress");
    EGLFuncPointer entry = funcMap["eglGetProcAddress"];
    typedef void* (*PFNEGLGETPROCADDRESSPROC)(const char*);
    PFNEGLGETPROCADDRESSPROC next = reinterpret_cast<PFNEGLGETPROCADDRESSPROC>(entry);
    return next(procname);
}
EGLAPI EGLFuncPointer EGLAPIENTRY eglGPA(const char* funcName)
{
#    define GETPROCADDR(func)                                                              \
        if (!strcmp(funcName, #func))                                                      \
        {                                                                                  \
            ALOGI("%s%s%s", "Returning glesLayer_" #func " for ", funcName, " in eglGPA"); \
            return (EGLFuncPointer)glesLayer_##func;                                       \
        }
    GETPROCADDR(eglInitialize);
    GETPROCADDR(eglSwapBuffers);
    GETPROCADDR(eglGetProcAddress);
    // Don't return anything for unrecognized functions
    return nullptr;
}
EGLAPI void EGLAPIENTRY
glesLayer_InitializeLayer(void*                             layer_id,
                          PFNEGLGETNEXTLAYERPROCADDRESSPROC get_next_layer_proc_address)
{
    ALOGI("%s%llu%s%llu",
          "glesLayer_InitializeLayer called with layer_id (",
          (unsigned long long)layer_id,
          ") get_next_layer_proc_address (",
          (unsigned long long)get_next_layer_proc_address);
    // Pick a real function to look up and test the pointer we've been handed
    const char* func = "eglGetProcAddress";
    ALOGI("%s%s%s%llu%s%llu%s",
          "Looking up address of ",
          func,
          " using get_next_layer_proc_address (",
          (unsigned long long)get_next_layer_proc_address,
          ") with layer_id (",
          (unsigned long long)layer_id,
          ")");
}
EGLAPI EGLFuncPointer EGLAPIENTRY glesLayer_GetLayerProcAddress(const char*    funcName,
                                                                EGLFuncPointer next)
{
    EGLFuncPointer entry = eglGPA(funcName);
    if (entry != nullptr)
    {
        ALOGI("%s%s%s%llu%s",
              "Setting up glesLayer version of ",
              funcName,
              " calling down with: next (",
              (unsigned long long)next,
              ")");
        funcMap[std::string(funcName)] = next;
        return entry;
    }
    // If the layer does not intercept the function, just return original func pointer
    return next;
}
}  // namespace

extern "C"
{
    __attribute((visibility("default"))) EGLAPI void AndroidGLESLayer_Initialize(
    void*                             layer_id,
    PFNEGLGETNEXTLAYERPROCADDRESSPROC get_next_layer_proc_address)
    {
        return (void)glesLayer_InitializeLayer(layer_id, get_next_layer_proc_address);
    }
    __attribute((visibility("default"))) EGLAPI void* AndroidGLESLayer_GetProcAddress(
    const char*    funcName,
    EGLFuncPointer next)
    {
        return (void*)glesLayer_GetLayerProcAddress(funcName, next);
    }
}
#endif