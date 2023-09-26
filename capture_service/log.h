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

#pragma once

#if defined __ANDROID__
#    include <android/log.h>
#    define TAG "Dive"
#    define __LOGE(...) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)
#    define __LOGW(...) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)
#    define __LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#    define __LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#else
#    include <cstdio>
#    define __LOGE(...) printf(__VA_ARGS__)
#    define __LOGW(...) printf(__VA_ARGS__)
#    define __LOGI(...) printf(__VA_ARGS__)
#    define __LOGD(...) printf(__VA_ARGS__)
#endif

#ifndef NDEBUG
#    define LOGE(...) __LOGE(__VA_ARGS__)
#    define LOGW(...) __LOGW(__VA_ARGS__)
#    define LOGI(...) __LOGI(__VA_ARGS__)
#    define LOGD(...) __LOGD(__VA_ARGS__)
#else
#    define LOGE(...) __LOGE(__VA_ARGS__)
#    define LOGW(...) __LOGW(__VA_ARGS__)
#    define LOGI(...) __LOGI(__VA_ARGS__)
#    define LOGD(...) ((void)0)
#endif