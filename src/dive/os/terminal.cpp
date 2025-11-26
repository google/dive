/*
 Copyright 2025 Google LLC

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

#include "dive/os/terminal.h"

#include <cstdio>
#include <ios>

#ifdef _WIN32
#    include <Windows.h>
#    include <fcntl.h>
#    include <io.h>
#endif

namespace Dive
{

void AttachToTerminalOutputIfAvailable()
{
#ifdef _WIN32
    if (_fileno(stderr) >= 0)
    {
        // Note: -2 is explicitly stderr is not open.
        return;
    }

    if (auto handle = GetStdHandle(STD_ERROR_HANDLE); handle != nullptr)
    {
        // Mismatched windows/c-runtime state, or GetStdHandle failed.
        return;
    }

    if (auto hwnd = GetConsoleWindow(); hwnd != nullptr)
    {
        // Already attached to console.
        // Not sure why we don't have stderr.
        return;
    }

    if (!AttachConsole(ATTACH_PARENT_PROCESS))
    {
        // Not invoked from console.
        // We might want to setup a stderr file later.
        return;
    }

    // Route stdout and stderr to console.
    std::freopen("CONOUT$", "wt", stderr);
    std::freopen("CONOUT$", "wt", stdout);
    std::ios::sync_with_stdio();

    // Redirect absl raw log, which use file handle 2.
    constexpr int kAbslRawLogFd = 2;
    if (_fileno(stderr) != kAbslRawLogFd)
    {
        auto console_handle = GetStdHandle(STD_ERROR_HANDLE);
        if (console_handle == nullptr || console_handle == INVALID_HANDLE_VALUE)
        {
            return;
        }

        int fd = _open_osfhandle(reinterpret_cast<intptr_t>(console_handle), _O_TEXT);
        if (fd == -1)
        {
            return;
        }
        _dup2(fd, kAbslRawLogFd);
        _close(fd);
    }
#endif
}

}  // namespace Dive
