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

#ifndef DIVE_WRAP_H_
#define DIVE_WRAP_H_

#if defined __ANDROID__
#    include <android/log.h>
#    define TAG "libwrap"
#    define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#    define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#endif

#ifdef __cplusplus
extern "C"
{
#endif
    extern int  IsCapturing();
    extern int  IsGfrxReplayCapture();
<<<<<<< Updated upstream
=======
    extern void StartCapture();
    extern void StopCapture();
>>>>>>> Stashed changes
    extern void SetCaptureName(const char* name, const char* frame_num);
#ifdef __cplusplus
}
#endif

#endif
