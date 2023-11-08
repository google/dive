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
#include "absl/strings/ascii.h"
#include "log.h"
#ifndef WIN32
#    error "Build this for Win32 platform only"
#endif
#include <windows.h>

namespace Dive
{

CommandResult RunCommand(const std::string &command, bool quiet)
{
    CommandResult result;
    HANDLE        hChildStdOutRd = NULL;
    HANDLE        hChildStdOutWr = NULL;
    HANDLE        hChildStdErrRd = NULL;
    HANDLE        hChildStdErrWr = NULL;

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
        LOGE("Create pipe to read stdout failed.");
        return result;
    }
    if (!SetHandleInformation(hChildStdOutRd, HANDLE_FLAG_INHERIT, 0))
    {
        LOGE("SetHandleInformation for stdout failed.");
        return result;
    }
    if (!CreatePipe(&hChildStdErrRd, &hChildStdErrWr, &sa, 0))
    {
        LOGE("CreatePipe for stderr failed");
        return result;
    }
    if (!SetHandleInformation(hChildStdErrRd, HANDLE_FLAG_INHERIT, 0))
    {
        LOGE("SetHandleInformation for stdout failed");
        return result;
    }

    si.cb = sizeof(si);
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdOutput = hChildStdOutWr;
    si.hStdError = hChildStdErrWr;

    int                  len = MultiByteToWideChar(CP_UTF8, 0, command.c_str(), -1, NULL, 0);
    std::vector<wchar_t> cmd(len);

    int res = MultiByteToWideChar(CP_UTF8, 0, command.c_str(), -1, cmd.data(), len);
    if (res == 0)
    {
        LOGE("Failed to convert std::string to utf-8 string");
        return result;
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
        LOGE("Error create process %d", GetLastError());
        return result;
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
        result.m_output += std::string(buf, dwOutputRead);
        if (!success && !dwOutputRead)
            break;
    }
    result.m_output = absl::StripAsciiWhitespace(result.m_output);

    for (;;)
    {
        success = ReadFile(hChildStdErrRd, buf, sizeof(buf), &dwErrorRead, NULL);
        result.m_err += std::string(buf, dwErrorRead);

        if (!success && !dwErrorRead)
            break;
    }
    result.m_err = absl::StripAsciiWhitespace(result.m_err);

    CloseHandle(hChildStdOutRd);
    CloseHandle(hChildStdErrRd);

    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, (LPDWORD)&result.m_ret);
    LOGD("result->m_ret is %d\n", result.m_ret);
    if (!quiet)
    {
        LOGI("Command: %s\n Output: %s\n", command.c_str(), result.m_output.c_str());
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (!result.Ok() && !quiet)
    {

        LOGE("Command `%s` failed with return code %d, stderr: %s \n",
             command.c_str(),
             result.m_ret,
             result.m_output.c_str());
    }
    return result;
}

}  // namespace Dive
