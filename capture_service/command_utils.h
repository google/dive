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
// Logs the command and the result of a command line application.
// Returns the output of the command if it finished successfully, or error status otherwise
absl::StatusOr<std::string> LogCommand(const std::string &command,
                                       const std::string &output,
                                       int                ret);

// Runs a command line application.
// Returns the output of the command if it finished successfully, or error status otherwise
absl::StatusOr<std::string> RunCommand(const std::string &command);

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
    inline absl::Status Run(const std::string &command) const
    {
        return RunCommand("adb -s " + m_serial + " " + command).status();
    }

    // RunAndGetResult runs the commands and returns the output of the command if it finished
    // successfully, or error status otherwise
    inline absl::StatusOr<std::string> RunAndGetResult(const std::string &command) const
    {
        return RunCommand("adb -s " + m_serial + " " + command);
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
