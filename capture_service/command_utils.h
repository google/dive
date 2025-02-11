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

#include <string>
#include <thread>
#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace Dive
{
// StatusOr Macros simplified from protobuf/stubs/status_macros.h
#define RETURN_IF_ERROR(expr)               \
    do                                      \
    {                                       \
        const absl::Status status = (expr); \
        if (!status.ok())                   \
            return status;                  \
    } while (0)

template<typename T> absl::Status DoAssignOrReturn(T &lhs, absl::StatusOr<T> result)
{
    if (result.ok())
    {
        lhs = *result;
    }
    return result.status();
}

#define STATUS_MACROS_CONCAT_NAME_INNER(x, y) x##y
#define STATUS_MACROS_CONCAT_NAME(x, y) STATUS_MACROS_CONCAT_NAME_INNER(x, y)

#define ASSIGN_OR_RETURN_IMPL(status, lhs, rexpr)         \
    absl::Status status = Dive::DoAssignOrReturn(lhs, (rexpr)); \
    if (!status.ok())                                     \
        return status;

#define ASSIGN_OR_RETURN(lhs, rexpr) \
    ASSIGN_OR_RETURN_IMPL(STATUS_MACROS_CONCAT_NAME(_status_or_value, __COUNTER__), lhs, rexpr);

// Runs a command line application.
// Returns the output of the command if it finished successfully, or error status otherwise
absl::StatusOr<std::string> RunCommand(const std::string &command, bool quiet = false);

class AdbSession
{
public:
    AdbSession() = default;
    AdbSession(const std::string &serial) :
        m_serial(serial)
    {
    }
    ~AdbSession()
    {
        for (auto &t : m_background_threads)
        {
            if (t.joinable())
            {
                t.join();
            }
        }
    }

    // Run runs the commands and returns the status of that commands.
    inline absl::Status Run(const std::string &command, bool quiet = true) const
    {
        return RunCommand("adb -s " + m_serial + " " + command, quiet).status();
    }

    // RunAndGetResult runs the commands and returns the output of the command if it finished
    // successfully, or error status otherwise
    inline absl::StatusOr<std::string> RunAndGetResult(const std::string &command,
                                                       bool               quiet = true) const
    {
        return RunCommand("adb -s " + m_serial + " " + command, quiet);
    }

    inline absl::Status RunCommandBackground(const std::string &command)
    {
        std::string full_command = "adb -s " + m_serial + " " + command;
        auto        worker = [full_command]() { RunCommand(full_command).IgnoreError(); };
        m_background_threads.emplace_back(std::thread(worker));
        return absl::OkStatus();
    }

private:
    std::string              m_serial;
    std::vector<std::thread> m_background_threads;
};
}  // namespace Dive
