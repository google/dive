/*
Copyright 2025 Google Inc.

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

#ifdef WIN32
#    define _WINSOCK_DEPRECATED_NO_WARNINGS
#    define NOMINMAX
#    include <winsock2.h>
#    include <ws2tcpip.h>
#    pragma comment(lib, "Ws2_32.lib")

// On Windows, a socket is a pointer-sized handle to ensure 32/64-bit compatibility.
using SocketType = uintptr_t;
// The value for an invalid socket on Windows is INVALID_SOCKET (~0).
constexpr SocketType kInvalidSocketValue = ~static_cast<SocketType>(0);

using ssize_t = SSIZE_T;

#    undef SendMessage

#else
#    include <netdb.h>
#    include <poll.h>
#    include <sys/un.h>
#    include <unistd.h>
#    include <netinet/in.h>

// On POSIX systems, a socket is a file descriptor (`int`).
using SocketType = int;
// Functions that return a file descriptor use -1 to indicate an error.
constexpr SocketType kInvalidSocketValue = -1;

#endif