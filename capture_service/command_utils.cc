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

#include "command_utils.h"

#include <cstdio>
#include <cstring>
#include "absl/flags/flag.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_format.h"
#include "log.h"

ABSL_FLAG(bool, force_output, false, "whether force to print out the command and its output.");

namespace Dive
{
absl::StatusOr<std::string> RunCommand(const std::string &command, bool quiet)
{
    std::string output;
    std::string err_msg;
    std::string cmd_str = command + " 2>&1";  // Get both stdout and stderr;
    FILE       *pipe = popen(cmd_str.c_str(), "r");
    bool        log_output = !quiet || absl::GetFlag(FLAGS_force_output);
    if (!pipe)
    {
        err_msg = "Popen call failed\n";
        LOGE("%s\n", err_msg.c_str());
        return absl::InternalError(err_msg);
    }

    char buf[128];
    while (fgets(buf, 128, pipe) != nullptr)
    {
        output += std::string(buf);
    }
    if (log_output)
    {
        LOGI("Command: %s\n Output: %s\n", command.c_str(), output.c_str());
    }
    output = absl::StripAsciiWhitespace(output);
    int ret = pclose(pipe);
    if (ret != 0)
    {
        std::string
        err_msg = absl::StrFormat("Command `%s` failed with return code %d, stderr: %s \n",
                                  command,
                                  ret,
                                  output);
        if (log_output)
        {
            LOGE("%s", err_msg.c_str());
        }
        return absl::InternalError(err_msg);
    }

    return output;
}

}  // namespace Dive