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

#include <vector>
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/ascii.h"
#include "dive/common/log.h"
#ifndef WIN32
#    error "Build this for Win32 platform only"
#endif
#include <windows.h>

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
    HANDLE      hChildStdOutRd = NULL;
    HANDLE      hChildStdOutWr = NULL;
    HANDLE      hChildStdErrRd = NULL;
    HANDLE      hChildStdErrWr = NULL;

    SECURITY_ATTRIBUTES sa;
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&sa, sizeof(sa));
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hChildStdOutRd, &hChildStdOutWr, &sa, 0))
    {
        err_msg = "Create pipe to read stdout failed.";
        LOGE("%s\n", err_msg.c_str());
        return absl::InternalError(err_msg);
    }
    if (!SetHandleInformation(hChildStdOutRd, HANDLE_FLAG_INHERIT, 0))
    {
        err_msg = "SetHandleInformation for stdout failed.";
        LOGE("%s\n", err_msg.c_str());
        return absl::InternalError(err_msg);
    }
    if (!CreatePipe(&hChildStdErrRd, &hChildStdErrWr, &sa, 0))
    {
        err_msg = "CreatePipe for stderr failed.";
        LOGE("%s\n", err_msg.c_str());
        return absl::InternalError(err_msg);
    }
    if (!SetHandleInformation(hChildStdErrRd, HANDLE_FLAG_INHERIT, 0))
    {
        err_msg = "SetHandleInformation for stdout failed.";
        LOGE("%s\n", err_msg.c_str());
        return absl::InternalError(err_msg);
    }

    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hChildStdOutWr;
    si.hStdError = hChildStdErrWr;
    si.wShowWindow = SW_HIDE;

    int                  len = MultiByteToWideChar(CP_UTF8, 0, command.c_str(), -1, NULL, 0);
    std::vector<wchar_t> cmd(len);

    int res = MultiByteToWideChar(CP_UTF8, 0, command.c_str(), -1, cmd.data(), len);
    if (res == 0)
    {
        err_msg = "Failed to convert std::string to utf-8 string.";
        LOGE("%s\n", err_msg.c_str());
        return absl::InternalError(err_msg);
    }

    bool bSuccess = CreateProcessW(NULL,
                                   cmd.data(),  // command line
                                   NULL,        // process security attributes
                                   NULL,        // primary thread security attributes
                                   TRUE,        // handles are inherited
                                   CREATE_UNICODE_ENVIRONMENT,  // creation flags
                                   NULL,                        // use parent's environment
                                   NULL,                        // use parent's current directory
                                   &si,                         // STARTUPINFO pointer
                                   &pi);                        // receives PROCESS_INFORMATION

    if (!bSuccess)
    {
        err_msg = absl::StrFormat("Error create process %d", GetLastError());
        LOGE("%s\n", err_msg.c_str());
        return absl::InternalError(err_msg);
    }
    else
    {

        CloseHandle(hChildStdOutWr);
        CloseHandle(hChildStdErrWr);
    }

    BOOL  success = FALSE;
    char  buf[4096];
    DWORD dwOutputRead, dwErrorRead;

    for (;;)
    {
        success = ReadFile(hChildStdOutRd, buf, sizeof(buf), &dwOutputRead, NULL);
        output += std::string(buf, dwOutputRead);
        if (!success && !dwOutputRead)
            break;
    }
    output = absl::StripAsciiWhitespace(output);

    for (;;)
    {
        success = ReadFile(hChildStdErrRd, buf, sizeof(buf), &dwErrorRead, NULL);
        output += std::string(buf, dwErrorRead);

        if (!success && !dwErrorRead)
            break;
    }
    output = absl::StripAsciiWhitespace(output);

    CloseHandle(hChildStdOutRd);
    CloseHandle(hChildStdErrRd);

    WaitForSingleObject(pi.hProcess, INFINITE);
    int ret = 0;
    GetExitCodeProcess(pi.hProcess, (LPDWORD)&ret);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return LogCommand(command, output, ret);
}

absl::StatusOr<std::filesystem::path> GetExecutableDirectory()
{
    wchar_t buffer[4096];
    DWORD   length = GetModuleFileNameW(nullptr, buffer, std::size(buffer));
    if (length > 0)
    {
        return std::filesystem::path(buffer).parent_path();
    }
    return absl::InternalError("Failed to get executable directory.");
}

}  // namespace Dive
