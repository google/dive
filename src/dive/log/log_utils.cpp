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

#include "dive/log/log_utils.h"

#include <iostream>
#include <string>

#include "absl/cleanup/cleanup.h"
#include "absl/log/log.h"
#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"

namespace Dive
{

// TODO b/464042788 - Try passing down the appropriate file to logger with
// LOG().AtLocation(location.file_name(), location.line())

void LogCommand(const std::string& command)
{
    // Log command first, in case the command hangs
    LOG(INFO) << "> " << command;
}

absl::StatusOr<std::string> LogCommandAndReturnOutput(const std::string& command,
                                                      const std::string& output, int ret)
{
    // Always log output
    LOG(INFO) << output;

    if (ret != 0)
    {
        return absl::UnknownError(absl::StrFormat(
            "Command `%s` failed with return code %d, error: %s\n", command, ret, output));
    }
    return output;
}

}  // namespace Dive
