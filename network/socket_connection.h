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

class ISocketConnection
{
public:
    virtual ~ISocketConnection() = default;

    ISocketConnection& operator=(const ISocketConnection&) = delete;
    ISocketConnection(const ISocketConnection&) = delete;

    // Server methods.
    virtual absl::Status BindAndListenOnUnixDomain(const std::string& server_address) = 0;
    virtual absl::StatusOr<std::unique_ptr<ISocketConnection>> Accept() = 0;

    // Client method.
    virtual absl::Status Connect(const std::string& host, int port) = 0;

    // Data transfer methods.
    virtual absl::Status           Send(const uint8_t* data, size_t size) = 0;
    virtual absl::StatusOr<size_t> Recv(uint8_t* data,
                                        size_t   size,
                                        int      timeout_ms = kNoTimeout) = 0;

    virtual void Close() = 0;
    virtual bool IsOpen() const = 0;

    absl::Status                SendString(const std::string& s);
    absl::StatusOr<std::string> ReceiveString();
    absl::Status                SendFile(const std::string& file_path);
    absl::Status                ReceiveFile(const std::string&          file_path,
                                            size_t                      file_size,
                                            std::function<void(size_t)> progress_callback = nullptr);

protected:
    ISocketConnection() = default;
};

class SocketConnection : public ISocketConnection
{
public:
    static absl::StatusOr<std::unique_ptr<SocketConnection>> Create(
    SocketType initial_socket_value = kInvalidSocketValue);

    explicit SocketConnection(SocketType initial_socket_value);
    ~SocketConnection() override;

    absl::Status BindAndListenOnUnixDomain(const std::string& server_address) override;
    absl::StatusOr<std::unique_ptr<ISocketConnection>> Accept() override;

    absl::Status Connect(const std::string& host, int port) override;

    absl::Status           Send(const uint8_t* data, size_t size) override;
    absl::StatusOr<size_t> Recv(uint8_t* data, size_t size, int timeout_ms = kNoTimeout) override;

    void Close() override;
    bool IsOpen() const override;

private:
    SocketType m_socket;
    bool       m_is_listening;
    int        m_accept_timout_ms;
};

}  // namespace Network