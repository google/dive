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

#include "unix_domain_server.h"

#include "common/log.h"
#include "absl/strings/str_cat.h"

namespace Network
{

DefaultMessageHandler::DefaultMessageHandler() {}

void DefaultMessageHandler::OnConnect()
{
    LOGI("DefaultMessageHandler::OnConnect()");
}

void DefaultMessageHandler::HandleMessage(std::unique_ptr<ISerializable> message,
                                          ISocketConnection*             client_conn)
{
    if (!message)
    {
        LOGW("DefaultMessageHandler::HandleMessage: Null message.");
        return;
    }
    if (!client_conn)
    {
        LOGW("DefaultMessageHandler::HandleMessage: Null client connection.");
        return;
    }

    switch (message->GetMessageType())
    {
    case MessageType::HANDSHAKE_REQUEST:
    {
        auto* request = dynamic_cast<HandshakeRequest*>(message.get());
        if (request)
        {
            HandshakeResponse response;
            response.SetMajorVersion(request->GetMajorVersion());
            response.SetMinorVersion(request->GetMinorVersion());
            auto status = SendSocketMessage(client_conn, response);
            if (!status.ok())
            {
                LOGW("DefaultMessageHandler::HandleMessage: SendSocketMessage fail: %.*s",
                     static_cast<int>(status.message().length()),
                     status.message().data());
            }
        }
        else
        {
            LOGW("DefaultMessageHandler::HandleMessage: Handshake request is null");
        }
        break;
    }
    case MessageType::PING_MESSAGE:
    {
        auto* request = dynamic_cast<PingMessage*>(message.get());
        if (request)
        {
            PongMessage response;
            auto        status = SendSocketMessage(client_conn, response);
            if (!status.ok())
            {
                LOGW("DefaultMessageHandler::HandleMessage: SendSocketMessage fail: %.*s",
                     static_cast<int>(status.message().length()),
                     status.message().data());
            }
        }
        else
        {
            LOGW("DefaultMessageHandler::HandleMessage: Ping request is null");
        }
        break;
    }
    default:
    {
        LOGW("DefaultMessageHandler::HandleMessage: Unknown message type = %d",
             static_cast<uint32_t>(message->GetMessageType()));
    }
    }
}

void DefaultMessageHandler::OnDisconnect()
{
    LOGI("DefaultMessageHandler::OnDisconnect()");
}

UnixDomainServer::UnixDomainServer(std::unique_ptr<IMessageHandler> handler) :
    m_handler(std::move(handler)),
    m_is_running(false)
{
}

UnixDomainServer::~UnixDomainServer()
{
    Stop();
}

absl::Status UnixDomainServer::Start(const std::string& server_address)
{
    if (m_is_running.load())
    {
        return absl::AlreadyExistsError("Start: Server is already running.");
    }

    auto connection = SocketConnection::Create();
    if (!connection.ok())
    {
        return absl::Status(connection.status().code(),
                            absl::StrCat("Start: Failed to create socket: ",
                                         connection.status().message()));
    }
    auto conn_status = (*connection)->BindAndListenOnUnixDomain(server_address);
    if (!conn_status.ok())
    {
        return absl::Status(conn_status.code(),
                            absl::StrCat("Start: Failed to bind and listen socket: ",
                                         conn_status.message()));
    }

    m_listen_connection = *std::move(connection);
    m_is_running.store(true);
    m_server_thread = std::thread(&UnixDomainServer::AcceptAndHandleClientLoop, this);
    return absl::OkStatus();
}

void UnixDomainServer::Wait()
{
    std::unique_lock<std::mutex> lock(m_wait_mutex);
    m_wait_cv.wait(lock, [this] { return !m_is_running.load(); });
}

void UnixDomainServer::Stop()
{
    m_is_running.store(false);
    m_listen_connection.reset();
    ResetClientConnection();
    if (m_server_thread.joinable())
    {
        m_server_thread.join();
    }

    m_wait_cv.notify_one();
    LOGI("UnixDomainServer: Stopped completely.");
}

void UnixDomainServer::AcceptAndHandleClientLoop()
{
    while (m_is_running.load())
    {
        // We only accept one client connection at a time.
        if (!m_client_connection)
        {
            if (!m_listen_connection || !m_listen_connection->IsOpen())
            {
                LOGI(m_is_running.load() ?
                     "AcceptAndHandleClientLoop: Listen socket closed unexpectedly. Stopping." :
                     "AcceptAndHandleClientLoop: Listen socket closed for shutdown.");
                break;
            }

            auto acc_connection = m_listen_connection->Accept();
            if (!acc_connection.ok())
            {
                if (!m_is_running.load())
                {
                    LOGI("AcceptAndHandleClientLoop: Accept: Exiting loop due to shutdown.");
                    break;
                }
                LOGI("AcceptAndHandleClientLoop: Error accepting new client: %.*s",
                     static_cast<int>(acc_connection.status().message().length()),
                     acc_connection.status().message().data());
                continue;
            }

            {
                std::lock_guard<std::mutex> lk(m_client_mutex);
                m_client_connection = *std::move(acc_connection);
            }
            LOGI("AcceptAndHandleClientLoop: New client accepted.");
            m_handler->OnConnect();
        }

        // Having accepted the only client connection, we're now ready to receive messages from it.
        if (m_client_connection)
        {
            if (!m_client_connection->IsOpen())
            {
                LOGI("AcceptAndHandleClientLoop: Client connection is closed.");
                m_handler->OnDisconnect();
                ResetClientConnection();
                continue;
            }

            auto recv_message = ReceiveSocketMessage(m_client_connection.get());
            if (!recv_message.ok())
            {
                if (!m_is_running.load())
                {
                    LOGI("AcceptAndHandleClientLoop: ReceiveSocketMessage: Exiting loop due to "
                         "shutdown.");
                    m_handler->OnDisconnect();
                    break;
                }

                LOGI("AcceptAndHandleClientLoop: ReceiveSocketMessage failed: %.*s",
                     static_cast<int>(recv_message.status().message().length()),
                     recv_message.status().message().data());
                m_handler->OnDisconnect();
                ResetClientConnection();
            }
            else
            {
                m_handler->HandleMessage(*std::move(recv_message), m_client_connection.get());
            }
        }
    }

    LOGI("AcceptAndHandleClientLoop: Exiting loop.");
    m_is_running.store(false);
    m_wait_cv.notify_one();
}

void UnixDomainServer::ResetClientConnection()
{
    std::lock_guard<std::mutex> lk(m_client_mutex);
    m_client_connection.reset();
}

}  // namespace Network