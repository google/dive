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
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "messages.h"
#include "message_handler.h"

namespace Network
{

// Default message handler if user doesn't provide/implement one.
class DefaultMessageHandler : public IMessageHandler
{
public:
    DefaultMessageHandler();
    void OnConnect() override;
    void HandleMessage(std::unique_ptr<ISerializable> message,
                       SocketConnection*              client_conn) override;
    void OnDisconnect() override;
};

// The UnixDomainServer is designed to run two threads. One thread accepts and handles a single
// client connection, while the main thread starts the server and waits for the first thread to
// finish or for an unexpected error to occur that necessitates stopping the server.
// The server is designed to accept only one client connection at a time.
class UnixDomainServer
{
public:
    // Constructs the server, taking ownership of the provided IMessageHandler.
    explicit UnixDomainServer(
    std::unique_ptr<IMessageHandler> handler = std::make_unique<DefaultMessageHandler>());

    // Stops the server and cleans up all resources.
    ~UnixDomainServer();

    // Starts the server to listen on a Unix Domain.
    absl::Status Start(const std::string& server_address);

    // Blocks the calling thread until the server stops.
    void Wait();

    // Gracefully stops the server thread and closes connections.
    void Stop();

private:
    // The primary run loop for the server's worker thread.
    void AcceptAndHandleClientLoop();

    // Resets the client connection to prepare for a new client.
    void ResetClientConnection();

    // Server connection.
    std::unique_ptr<SocketConnection> m_listen_connection;
    // The only one client connection at a time.
    std::unique_ptr<SocketConnection> m_client_connection;
    // The thread that accepts and handles the only one client connection at a time.
    std::thread m_server_thread;

    std::unique_ptr<IMessageHandler> m_handler;
    std::atomic<bool>                m_is_running;
    std::mutex                       m_client_mutex;
    std::mutex                       m_wait_mutex;
    std::condition_variable          m_wait_cv;
};
}  // namespace Network