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
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_format.h"
#include "common/log.h"

namespace Dive
{

absl::StatusOr<std::string> LogCommand(const std::string &command,
                                       const std::string &output,
                                       int                ret)
{
    // Always log command and output for debug builds
    LOGD("> %s\n", command.c_str());
    LOGD("%s\n", output.c_str());

    if (ret != 0)
    {
        auto err_msg = absl::StrFormat("Command `%s` failed with return code %d, error: %s\n",
                                       command,
                                       ret,
                                       output);
        // Always log error
        LOGE("ERROR: %s\n", err_msg.c_str());
        return absl::UnknownError(err_msg);
    }
    return output;
}

absl::StatusOr<std::string> RunCommand(const std::string &command)
{
    std::string output;
    std::string err_msg;
    std::string cmd_str = command + " 2>&1";  // Get both stdout and stderr;
    FILE       *pipe = popen(cmd_str.c_str(), "r");
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
    output = absl::StripAsciiWhitespace(output);
    int ret = pclose(pipe);

    return LogCommand(command, output, ret);
}

absl::StatusOr<std::filesystem::path> GetExecutableDirectory()
{
    char    buffer[PATH_MAX];
    ssize_t length = readlink("/proc/self/exe", buffer, PATH_MAX);
    if (length > 0)
    {
        buffer[length] = '\0';
        return std::filesystem::path(buffer).parent_path();
    }
    return absl::InternalError("Failed to get executable directory.");
}

}  // namespace Dive