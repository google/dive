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

#include "platform_net.h"
#include "absl/status/statusor.h"

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
    static absl::StatusOr<std::unique_ptr<SocketConnection>> Create(
    SocketType initial_socket_value = kInvalidSocketValue);

    ~SocketConnection();

    SocketConnection& operator=(const SocketConnection&) = delete;
    SocketConnection(const SocketConnection&) = delete;

    // Server methods.
    absl::Status BindAndListenOnUnixDomain(const std::string& server_address);
    absl::StatusOr<std::unique_ptr<SocketConnection>> Accept();

    // Client method.
    absl::Status Connect(const std::string& host, int port);

    // Data transfer methods.
    absl::Status                Send(const uint8_t* data, size_t size);
    absl::StatusOr<size_t>      Recv(uint8_t* data, size_t size, int timeout_ms = kNoTimeout);
    absl::Status                SendString(const std::string& s);
    absl::StatusOr<std::string> ReceiveString();
    absl::Status                SendFile(const std::string& file_path);
    absl::Status                ReceiveFile(const std::string&          file_path,
                                            size_t                      file_size,
                                            std::function<void(size_t)> progress_callback = nullptr);

    void Close();
    bool IsOpen() const;

private:
    explicit SocketConnection(SocketType initial_socket_value);

    SocketType m_socket;
    bool       m_is_listening;
    int        m_accept_timout_ms;
};

}  // namespace Network