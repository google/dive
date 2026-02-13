/*
Copyright 2026 Google Inc.

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

#include <cstdio>
#include <string>

#include "absl/cleanup/cleanup.h"
#include "absl/status/statusor.h"

namespace Dive
{

// TODO b/464042788 - Try making a helper function for the prefix/function logging with absl
// cleanup?

// TODO b/464042788 - Look into possibility of using C++20 <source_location> library with the
// current compilers we are using for Dive project

// Log command with a ">" prefix for easy skimming of the log
void LogCommand(const std::string& command);

// Logs the result of a command line application.
// Returns the output of the command if it finished successfully, or error status otherwise
absl::StatusOr<std::string> LogCommandAndReturnOutput(const std::string& command,
                                                      const std::string& output, int ret);

}  // namespace Dive
