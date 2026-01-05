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

#pragma once

#include <filesystem>

#include "absl/status/statusor.h"

namespace Dive
{

constexpr char kHandlerBinary[] = "crashpad_handler";
constexpr char kDbDirectory[] = "crash_database";
constexpr char kMetricsDirectory[] = "crash_metrics";
constexpr char kCrashReportUrl[] = "https://clients2.google.com/cr/report";
constexpr char kProductName[] = "Dive";
constexpr char kFormat[] = "minidump";
constexpr char kNoRateLimitFlag[] = "--no-rate-limit";
constexpr int kMaxCrashpadVersionLength = 30;

// Returns the platform-specific writable directory for application data.
// Windows: %LOCALAPPDATA%/Dive
// macOS: ~/Library/Application Support/Dive
// Linux: ~/.local/share/Dive
absl::StatusOr<std::filesystem::path> GetWritableRoot();

}  // namespace Dive
