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

#include <mutex>
#include <vector>

#include "absl/base/no_destructor.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_format.h"
#include "dive/log/log_utils.h"
#include "dive/os/command_utils.h"

#ifndef WIN32
#error "Build this for Win32 platform only"
#endif
#include <windows.h>

namespace Dive
{

// Global mutex to serialize handle inheritance in Windows.
// This prevents concurrent RunCommand calls from accidentally
// inheriting each other's open pipe handles.
static absl::NoDestructor<std::mutex> process_mutex;

absl::StatusOr<std::string> RunCommand(const std::string& command)
{
    LogCommand(command);

    std::string output;
    std::string err_msg;
    HANDLE hChildStdOutRd = NULL;
    HANDLE hChildStdOutWr = NULL;
    HANDLE hChildStdErrRd = NULL;
    HANDLE hChildStdErrWr = NULL;

    SECURITY_ATTRIBUTES sa;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&sa, sizeof(sa));
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    int len = MultiByteToWideChar(CP_UTF8, 0, command.c_str(), -1, NULL, 0);
    std::vector<wchar_t> cmd(len);

    int res = MultiByteToWideChar(CP_UTF8, 0, command.c_str(), -1, cmd.data(), len);
    if (res == 0)
    {
        err_msg = "Failed to convert std::string to utf-8 string.";
        return absl::InternalError(err_msg);
    }

    bool bSuccess = false;

    {
        // We use a global mutex to serialize pipe creation and process launching. Because
        // CreateProcessW is called with bInheritHandles = TRUE, it inherits all inheritable handles
        // currently open in the entire parent process. If multiple threads create pipes
        // concurrently, a long-running child process can accidentally inherit a sibling thread's
        // pipe. This prevents the OS from sending an EOF signal when the sibling process dies,
        // causing ReadFile to hang indefinitely and freeze the host UI. The mutex prevents this
        // handle leak.
        std::lock_guard<std::mutex> lock(*process_mutex);

        if (!CreatePipe(&hChildStdOutRd, &hChildStdOutWr, &sa, 0))
        {
            err_msg = "Create pipe to read stdout failed.";
            return absl::InternalError(err_msg);
        }
        if (!SetHandleInformation(hChildStdOutRd, HANDLE_FLAG_INHERIT, 0))
        {
            CloseHandle(hChildStdOutRd);
            CloseHandle(hChildStdOutWr);
            err_msg = "SetHandleInformation for stdout failed.";
            return absl::InternalError(err_msg);
        }
        if (!CreatePipe(&hChildStdErrRd, &hChildStdErrWr, &sa, 0))
        {
            CloseHandle(hChildStdOutRd);
            CloseHandle(hChildStdOutWr);
            err_msg = "CreatePipe for stderr failed.";
            return absl::InternalError(err_msg);
        }
        if (!SetHandleInformation(hChildStdErrRd, HANDLE_FLAG_INHERIT, 0))
        {
            CloseHandle(hChildStdOutRd);
            CloseHandle(hChildStdOutWr);
            CloseHandle(hChildStdErrRd);
            CloseHandle(hChildStdErrWr);
            err_msg = "SetHandleInformation for stderr failed.";
            return absl::InternalError(err_msg);
        }

        si.cb = sizeof(si);
        si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        si.hStdOutput = hChildStdOutWr;
        si.hStdError = hChildStdErrWr;
        si.wShowWindow = SW_HIDE;

        bSuccess = CreateProcessW(NULL,
                                  cmd.data(),                  // command line
                                  NULL,                        // process security attributes
                                  NULL,                        // primary thread security attributes
                                  TRUE,                        // handles are inherited
                                  CREATE_UNICODE_ENVIRONMENT,  // creation flags
                                  NULL,                        // use parent's environment
                                  NULL,                        // use parent's current directory
                                  &si,                         // STARTUPINFO pointer
                                  &pi);                        // receives PROCESS_INFORMATION

        if (bSuccess)
        {
            // Close the write ends in the parent process IMMEDIATELY after
            // the child is created. This ensures the child is the only process
            // holding the write handles, and prevents other threads from inheriting them.
            CloseHandle(hChildStdOutWr);
            CloseHandle(hChildStdErrWr);
        }
    }

    if (!bSuccess)
    {
        // If CreateProcessW failed, the pipes were successfully created but
        // will never be used. Close the remaining handles to prevent a leak.
        CloseHandle(hChildStdOutRd);
        CloseHandle(hChildStdOutWr);
        CloseHandle(hChildStdErrRd);
        CloseHandle(hChildStdErrWr);

        err_msg = absl::StrFormat("Error create process %d", GetLastError());
        return absl::InternalError(err_msg);
    }

    BOOL success = FALSE;
    char buf[4096];
    DWORD dwOutputRead, dwErrorRead;

    for (;;)
    {
        success = ReadFile(hChildStdOutRd, buf, sizeof(buf), &dwOutputRead, NULL);
        output += std::string(buf, dwOutputRead);
        if (!success && !dwOutputRead) break;
    }
    output = absl::StripAsciiWhitespace(output);

    for (;;)
    {
        success = ReadFile(hChildStdErrRd, buf, sizeof(buf), &dwErrorRead, NULL);
        output += std::string(buf, dwErrorRead);

        if (!success && !dwErrorRead) break;
    }
    output = absl::StripAsciiWhitespace(output);

    CloseHandle(hChildStdOutRd);
    CloseHandle(hChildStdErrRd);

    WaitForSingleObject(pi.hProcess, INFINITE);
    int ret = 0;
    GetExitCodeProcess(pi.hProcess, (LPDWORD)&ret);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return LogCommandAndReturnOutput(command, output, ret);
}

absl::StatusOr<std::filesystem::path> GetExecutableDirectory()
{
    wchar_t buffer[4096];
    DWORD length = GetModuleFileNameW(nullptr, buffer, std::size(buffer));
    if (length > 0)
    {
        return std::filesystem::path(buffer).parent_path();
    }
    return absl::InternalError("Failed to get executable directory.");
}

}  // namespace Dive
