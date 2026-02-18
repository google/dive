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

#pragma once

#include <fcntl.h>
#include <sys/stat.h>

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#include "absl/debugging/failure_signal_handler.h"
#include "absl/debugging/symbolize.h"

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

namespace Dive
{

class CrashHandler
{
 public:
    static void Initialize(const char* argv0)
    {
        // 1. Generate Timestamp: yyyyMMdd-HHmmss
        auto now = std::chrono::system_clock::now();
        std::time_t in_time_t = std::chrono::system_clock::to_time_t(now);

        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d-%H%M%S");
        std::string filename = "dive-" + ss.str() + ".log.txt";

        // 2. Determine Executable Directory
        // std::filesystem::absolute handles resolving relative paths like "./dive"
        std::filesystem::path exe_path_fs(argv0);
        std::filesystem::path exe_dir = std::filesystem::absolute(exe_path_fs).parent_path();
        std::filesystem::path exe_full_path = exe_dir / filename;

        // 3. Determine Temp Directory
        std::error_code ec;
        std::filesystem::path temp_dir = std::filesystem::temp_directory_path(ec);
        if (ec)
        {
            // Fallback if system temp dir cannot be found
#if defined(_WIN32)
            temp_dir = std::filesystem::path("C:/Windows/Temp");
#else
            temp_dir = std::filesystem::path("/tmp");
#endif
        }
        std::filesystem::path temp_full_path = temp_dir / filename;

        // 4. Store paths safely in char arrays
        SafeStrCopy(m_primary_path, exe_full_path.string().c_str());
        SafeStrCopy(m_fallback_path, temp_full_path.string().c_str());

        std::cout << "Absl crash handler initialized" << std::endl;
        std::cout << "  1. Primary Log Path:  " << exe_full_path.string() << std::endl;
        std::cout << "  2. Fallback Log Path: " << temp_full_path.string() << std::endl;
    }

    static void Writer(const char* data)
    {
        if (data == nullptr)
        {
            return;
        }

        // Avoid strlen for crash handler to be safe
        uint32_t len = 0;
        while (data[len] != '\0')
        {
            len++;
        }

        if (m_fd == kInvalidFd)
        {
            m_fd = SysOpen(m_primary_path);

            if (m_fd == kInvalidFd)
            {
                m_fd = SysOpen(m_fallback_path);
            }
        }

        if (m_fd != kInvalidFd)
        {
            SysWrite(m_fd, data, len);
        }
    }

 private:
    static constexpr int kInvalidFd = -1;
    static constexpr int kMaxPath = 2048;

    inline static int m_fd = kInvalidFd;

    // Use char array to avoid potential allocation within the crash handler
    inline static char m_primary_path[kMaxPath] = {0};
    inline static char m_fallback_path[kMaxPath] = {0};

    template <size_t N>
    static void SafeStrCopy(char (&dest)[N], const char* src)
    {
        if (!src)
        {
            return;
        }

        size_t i = 0;
        for (; i < N - 1 && src[i] != '\0'; ++i)
        {
            dest[i] = src[i];
        }
        dest[i] = '\0';
    }

    static int SysOpen(const char* path)
    {
#if defined(_WIN32)
        constexpr int flags = _O_CREAT | _O_TRUNC | _O_WRONLY | _O_TEXT;
        constexpr int mode = _S_IREAD | _S_IWRITE;
        return _open(path, flags, mode);
#else
        constexpr int flags = O_CREAT | O_TRUNC | O_WRONLY;
        // 0: Indicates this is an octal number
        // 6: (Owner):  Read (4) + Write (2) = Read/Write
        // 6: (Group):  Read (4) + Write (2) = Read/Write
        // 4: (Others): Read (4) = Read Only
        constexpr int mode = 0664;
        return open(path, flags, mode);
#endif
    }

    static void SysWrite(int fd, const char* data, uint32_t len)
    {
#if defined(_WIN32)
        _write(fd, data, len);
#else
        [[maybe_unused]] ssize_t res = write(fd, data, len);
#endif
    }
};

inline void InitializeAbslHandler(const char* argv0)
{
    absl::InitializeSymbolizer(argv0);
    CrashHandler::Initialize(argv0);

    absl::FailureSignalHandlerOptions options;
    options.writerfn = CrashHandler::Writer;
    absl::InstallFailureSignalHandler(options);
}

}  // namespace Dive
