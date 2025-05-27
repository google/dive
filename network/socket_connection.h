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

#include <memory>
#include <system_error>

#ifdef _WIN32
typedef uintptr_t    SocketType;
constexpr SocketType kInvalidSocketValue = ~static_cast<SocketType>(0);
#else
typedef int          SocketType;
constexpr SocketType kInvalidSocketValue = -1;
#endif

constexpr int kNoTimeout = -1;
constexpr int kAcceptTimeout = 2000;

namespace Network
{

class NetworkInitializer
{
public:
    NetworkInitializer();
    NetworkInitializer& operator=(const NetworkInitializer&) = delete;
    NetworkInitializer(const NetworkInitializer&) = delete;

    static const NetworkInitializer& Instance();
    bool                             IsInitialized() const;

private:
    bool m_initialized;
};

class SocketConnection
{
public:
    SocketConnection();
    ~SocketConnection();

    explicit SocketConnection(SocketType initial_socket);

    SocketConnection& operator=(const SocketConnection&) = delete;
    SocketConnection(const SocketConnection&) = delete;

    // Server methods.
    bool BindAndListenOnUnixDomain(const std::string& server_address, std::error_code& ec);
    std::unique_ptr<SocketConnection> Accept(std::error_code& ec);

    // Client method.
    bool Connect(const std::string& host, int port, std::error_code& ec);

    // Data transfer methods.
    size_t Send(const uint8_t* data, size_t size, std::error_code& ec);
    size_t Recv(uint8_t* data, size_t size, std::error_code& ec);
    bool   SendString(const std::string& s, std::error_code& ec);
    bool   ReceiveString(std::string& s, std::error_code& ec);
    bool   SendFile(const std::string& file_path, std::error_code& ec);
    bool   ReceiveFile(const std::string& file_path, size_t file_size, std::error_code& ec);

    void        Close();
    bool        IsOpen() const;
    std::string GetLastErrorMsg();

private:
    void SetError(const std::error_code& ec, const std::string& context = "");
    void SetPlatformError(const std::string& context = "");

    SocketType  m_socket;
    bool        m_is_listening;
    int         m_accept_timout_ms;
    int         m_recv_timeout_ms;
    std::string m_last_error_msg;
};

}  // namespace Network