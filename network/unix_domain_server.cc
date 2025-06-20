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
#include "log.h"

namespace Network
{

DefaultMessageHandler::DefaultMessageHandler() {}

void DefaultMessageHandler::OnConnect()
{
    LOGI("DefaultMessageHandler::OnConnect()");
}

void DefaultMessageHandler::HandleMessage(std::unique_ptr<ISerializable> message,
                                          SocketConnection*              client_conn)
{
    if (!message)
    {
        LOGI("DefaultMessageHandler::HandleMessage: Null message.");
        return;
    }
    if (!client_conn)
    {
        LOGI("DefaultMessageHandler::HandleMessage: Null client connection.");
        return;
    }

    switch (static_cast<MessageType>(message->GetMessageType()))
    {
    case MessageType::HANDSHAKE_REQUEST:
    {
        auto* request = dynamic_cast<HandShakeRequest*>(message.get());
        if (request)
        {
            HandShakeResponse response;
            response.SetMajorVersion(request->GetMajorVersion());
            response.SetMinorVersion(request->GetMinorVersion());
            auto status = SendMessage(client_conn, response);
            if (!status.ok())
            {
                LOGI("DefaultMessageHandler::HandleMessage: SendMessage fail: %s",
                     status.message());
            }
        }
        else
        {
            LOGI("DefaultMessageHandler::HandleMessage: Handshake request is null");
        }
        break;
    }
    case MessageType::PING_MESSAGE:
    {
        auto* request = dynamic_cast<PingMessage*>(message.get());
        if (request)
        {
            PongMessage response;
            auto        status = SendMessage(client_conn, response);
            if (!status.ok())
            {
                LOGI("DefaultMessageHandler::HandleMessage: SendMessage fail: %s",
                     status.message());
            }
        }
        else
        {
            LOGI("DefaultMessageHandler::HandleMessage: Ping request is null");
        }
        break;
    }
    default:
    {
        LOGI("DefaultMessageHandler::HandleMessage: Unknown message type.");
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
    if (m_server_thread.joinable())
    {
        m_server_thread.join();
    }

    auto connection_or = SocketConnection::Create();
    if (!connection_or.ok())
    {
        return absl::Status(connection_or.status().code(),
                            absl::StrCat("Start: ", connection_or.status().message()));
    }
    auto new_connection = std::move(connection_or.value());
    auto conn_status = new_connection->BindAndListenOnUnixDomain(server_address);
    if (!conn_status.ok())
    {
        return absl::Status(conn_status.code(), absl::StrCat("Start: ", conn_status.message()));
    }

    m_listen_connection = std::unique_ptr<SocketConnection, SocketConnectionDeleter>(
    new_connection.release());
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
        if (!m_client_connection)
        {
            if (!m_listen_connection || !m_listen_connection->IsOpen())
            {
                if (m_is_running.load())
                {
                    LOGI(
                    "AcceptAndHandleClientLoop: Listen socket is closed unexpectedly. Stopping.");
                }
                else
                {
                    LOGI("AcceptAndHandleClientLoop: Listen socket closed for shutdown.");
                }
                break;
            }

            auto acc_connection_or = m_listen_connection->Accept();
            if (!acc_connection_or.ok())
            {
                if (!m_is_running.load())
                {
                    LOGI("AcceptAndHandleClientLoop: Accept: Exiting loop due to shutdown.");
                    break;
                }
                LOGI("AcceptAndHandleClientLoop: Error accepting new client: %s",
                     acc_connection_or.status().message());
                continue;
            }

            auto acc_connection = std::move(acc_connection_or.value());
            {
                std::lock_guard<std::mutex> lk(m_client_mutex);
                m_client_connection = std::unique_ptr<SocketConnection, SocketConnectionDeleter>(
                acc_connection.release());
            }
            LOGI("AcceptAndHandleClientLoop: New client accepted.");
            m_handler->OnConnect();
        }

        if (m_client_connection)
        {
            if (!m_client_connection->IsOpen())
            {
                LOGI("AcceptAndHandleClientLoop: Client connection is closed.");
                m_handler->OnDisconnect();
                ResetClientConnection();
                continue;
            }

            auto recv_message_or = ReceiveMessage(m_client_connection.get());
            if (!recv_message_or.ok())
            {
                if (!m_is_running.load())
                {
                    LOGI(
                    "AcceptAndHandleClientLoop: ReceiveMessage: Exiting loop due to shutdown.");
                    m_handler->OnDisconnect();
                    break;
                }

                LOGI("AcceptAndHandleClientLoop: ReceiveMessage failed: %s",
                     recv_message_or.status().message());
                m_handler->OnDisconnect();
                ResetClientConnection();
            }
            else
            {
                m_handler->HandleMessage(std::move(recv_message_or.value()),
                                         m_client_connection.get());
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