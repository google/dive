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

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <memory>

#include "messages.h"

namespace Network
{

enum class ClientStatus
{
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    CONNECTION_FAILED
};

class TcpClient
{
public:
    TcpClient();
    ~TcpClient();

    // Connects to the server and performs the handshake.
    // Returns absl::OkStatus() on success, or an error status on failure.
    absl::Status Connect(const std::string& host, int port);

    // Disconnects from the server.
    void Disconnect();

    // Returns true if the client is in a fully connected and operational state.
    bool IsConnected() const;

    // Requests the server to start a PM4 capture.
    // On success, returns a string identifier (capture file path on the server).
    // On failure, returns a status.
    absl::StatusOr<std::string> StartPm4Capture();

    // Downloads a file from the server to a local path.
    absl::Status DownloadFileFromServer(const std::string& remote_file_path,
                                        const std::string& local_save_path);

private:
    // Performs a ping-pong check with the server.
    absl::Status PingServer();

    // Performs a handshake with the server.
    absl::Status PerformHandshake();

    // Starts the keep-alive checking.
    absl::Status StartKeepAlive();

    // Pings periodically to the Server.
    void KeepAliveLoop();

    // Stops keep-alive.
    void StopKeepAlive();

    ClientStatus GetClientStatus() const;
    void         SetClientStatus(ClientStatus status);
    absl::Status SetStatusAndReturnError(ClientStatus status, const absl::Status& error_status);

    std::unique_ptr<SocketConnection> m_connection;
    std::mutex                        m_connection_mutex;
    ClientStatus                      m_status;
    mutable std::mutex                m_status_mutex;

    // KeepAlive is used to check the connection with the server periodically via a ping-pong
    // mechanism.
    struct KeepAlive
    {
        std::thread             thread;
        std::atomic<bool>       running;
        std::mutex              mutex;
        std::condition_variable cv;
        uint32_t                interval_sec;
    } m_keep_alive;
};

}  // namespace Network