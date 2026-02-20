/*
 Copyright 2026 Google LLC

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

#ifndef UI_DIVE_UI_LIB_EXPORT_H_
#define UI_DIVE_UI_LIB_EXPORT_H_

#if defined(_MSC_VER)
// Check if we are building the DLL (DIVE_UI_LIB_BUILD is defined)
#ifdef DIVE_UI_LIB_BUILD
#define DIVE_UI_LIB_EXPORT __declspec(dllexport)
#else
#define DIVE_UI_LIB_EXPORT __declspec(dllimport)
#endif
#else
// On other platforms, this macro does nothing
#define DIVE_UI_LIB_EXPORT
#endif

#endif  // UI_DIVE_UI_LIB_EXPORT_H_