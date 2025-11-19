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

#pragma once

#include <string>
namespace Dive
{

// Returns a string with the following structure:
//
// (major).(minor).(revision)-(dev|release|internal|canary)-(7 digit SHA)
std::string GetHostShortVersionString();

// Returns a string with the following structure:
//
// clang-format off
// -----
// Device Libraries Build: (major).(minor).(revision)-(dev|release|canary)-(arm64-v8a|armeabi-v7a|x86|x86_64)
// Device Libraries Build Type: (Debug | Release)
// Device Libraries SHA: (40 digit SHA)
// -----
// clang-format on
std::string GetDeviceLibrariesVersionInfo();

// Returns a short blurb about Dive with product description and copyright info
std::string GetDiveDescription();

// Returns a string with the following structure:
//
// clang-format off
// -----
// Host Tools Build: (major).(minor).(revision)-(dev|release|internal|canary)-(linux|mac|win)-(7 digit SHA)
// Host Tools Build Type: (Debug | Release)
// Host Tools SHA: (40 digit SHA)
//
// Device Libraries Build: (major).(minor).(revision)-(dev|release|canary)-(arm64-v8a|armeabi-v7a|x86|x86_64)
// Device Libraries Build Type: (Debug | Release)
// Device Libraries SHA: (40 digit SHA)
//
// Profiling Plugin SHA: (40 digit SHA)
// -----
// clang-format on
//
// NOTE: Other than host tools info, info is retrieved from within DIVE_INSTALL_DIR_PATH
std::string GetLongVersionString();

// Returns both the host short version string and long version string
std::string GetCompleteVersionString();

}  // namespace Dive
