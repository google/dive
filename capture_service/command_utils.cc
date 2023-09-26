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

#include <chrono>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <thread>
#include "absl/strings/ascii.h"
#include "log.h"

namespace Dive
{

#if defined(WIN32)
// TODO(renfeng): figure out how to run exe on Windows
std::string RunCommand(const std::string &command)
{
    return "";
}
#else
CommandResult RunCommand(const std::string &command, bool quiet)
{
    CommandResult result;
    std::string   cmd_str = command + " 2>&1";  // Get both stdout and stderr;
    FILE         *pipe = popen(cmd_str.c_str(), "r");
    if (!pipe)
    {
        LOGE("Popen call failed\n");
        return result;
    }

    char        buf[128];
    std::string output;
    while (fgets(buf, 128, pipe) != nullptr)
    {
        output += std::string(buf);
    }
    if (!quiet)
    {
        LOGD("Command: %s\n Output: %s\n", command.c_str(), output.c_str());
    }
    output = absl::StripAsciiWhitespace(output);
    result.m_ret = pclose(pipe);
    if (result.Ok())
    {
        result.m_output = std::move(output);
    }
    else
    {
        if (!quiet)
        {
            LOGE("Command `%s` failed with return code %d, stderr: %s \n",
                 command.c_str(),
                 result.m_ret,
                 output.c_str());
        }

        result.m_err = std::move(output);
    }

    return result;
}
#endif

CommandResult AdbSession::Run(const std::string &command, bool quiet) const
{
    return RunCommand("adb -s " + m_serial + " " + command, quiet);
}
}  // namespace Dive