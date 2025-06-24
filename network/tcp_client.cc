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

#include <chrono>

#include "tcp_client.h"

namespace
{
constexpr uint32_t kKeepAliveIntervalSec = 5;
constexpr uint32_t kPingTimeoutMs = 5000;
constexpr uint32_t kHandshakeMajorVersion = 1;
constexpr uint32_t kHandshakeMinorVersion = 0;
}  // namespace

namespace Network
{

TcpClient::TcpClient() :
    m_status(ClientStatus::DISCONNECTED)
{
    m_keep_alive.running = false;
    m_keep_alive.interval_sec = kKeepAliveIntervalSec;
}

TcpClient::~TcpClient()
{
    Disconnect();
}

absl::Status TcpClient::Connect(const std::string& host, int port)
{
    if (GetClientStatus() == ClientStatus::CONNECTED ||
        GetClientStatus() == ClientStatus::CONNECTING)
    {
        return absl::AlreadyExistsError(
        "Connect: Client is already connected or in the process of connecting.");
    }

    StopKeepAlive();
    m_connection.reset();

    SetClientStatus(ClientStatus::CONNECTING);
    auto connection_or = SocketConnection::Create();
    if (!connection_or.ok())
    {
        return SetStatusAndReturnError(ClientStatus::CONNECTION_FAILED,
                                       absl::Status(connection_or.status().code(),
                                                    absl::StrCat("Connect: ",
                                                                 connection_or.status()
                                                                 .message())));
    }
    auto new_connection = std::move(connection_or.value());
    auto conn_status = new_connection->Connect(host, port);
    if (!conn_status.ok())
    {
        return SetStatusAndReturnError(ClientStatus::CONNECTION_FAILED,
                                       absl::Status(conn_status.code(),
                                                    absl::StrCat("Connect: Connect fail: ",
                                                                 conn_status.message())));
    }

    m_connection = std::unique_ptr<SocketConnection, SocketConnectionDeleter>(
    new_connection.release());
    SetClientStatus(ClientStatus::CONNECTED);

    std::cout << "Client: Connected & handshaking." << std::endl;
    auto handshake_status = PerformHandshake();
    if (!handshake_status.ok())
    {
        m_connection.reset();
        return SetStatusAndReturnError(ClientStatus::CONNECTION_FAILED,
                                       absl::Status(handshake_status.code(),
                                                    absl::StrCat("Connect: Handshake fail: ",
                                                                 handshake_status.message())));
    }

    std::cout << "Client: Connected & StartKeepAlive." << std::endl;
    auto keep_alive_status = StartKeepAlive();
    if (!keep_alive_status.ok())
    {
        if (absl::IsAlreadyExists(keep_alive_status))
        {
            std::cout << keep_alive_status.message() << std::endl;
        }
        else
        {
            m_connection.reset();
            return SetStatusAndReturnError(ClientStatus::CONNECTION_FAILED,
                                           absl::Status(keep_alive_status.code(),
                                                        absl::StrCat("Connect: KeepAlive fail: ",
                                                                     keep_alive_status.message())));
        }
    }
    std::cout << "Client: Succesfully connected." << std::endl;
    return absl::OkStatus();
}

void TcpClient::Disconnect()
{
    StopKeepAlive();
    m_connection.reset();
    SetClientStatus(ClientStatus::DISCONNECTED);
    std::cout << "Client: Disconnected." << std::endl;
}

bool TcpClient::IsConnected() const
{
    return GetClientStatus() == ClientStatus::CONNECTED && m_connection && m_connection->IsOpen();
}

absl::StatusOr<std::string> TcpClient::StartPm4Capture()
{
    std::lock_guard<std::mutex> lock(m_connection_mutex);
    if (!IsConnected())
    {
        return absl::FailedPreconditionError("StartPm4Capture: Client is not connected.");
    }

    Pm4CaptureRequest pm4_request;
    std::cout << "Client: StartPm4Capture request." << std::endl;
    auto send_status = SendMessage(m_connection.get(), pm4_request);
    if (!send_status.ok())
    {
        return SetStatusAndReturnError(ClientStatus::CONNECTION_FAILED,
                                       absl::
                                       Status(send_status.code(),
                                              absl::StrCat("StartPm4Capture: SendMessage fail: ",
                                                           send_status.message())));
    }

    auto receive_or = ReceiveMessage(m_connection.get());
    if (!receive_or.ok())
    {
        return SetStatusAndReturnError(ClientStatus::CONNECTION_FAILED,
                                       absl::
                                       Status(receive_or.status().code(),
                                              absl::StrCat("StartPm4Capture: ReceiveMessage fail: ",
                                                           receive_or.status().message())));
    }

    auto response = std::move(receive_or.value());
    if (response->GetMessageType() != MessageType::PM4_CAPTURE_RESPONSE)
    {
        return absl::FailedPreconditionError(
        absl::StrCat("StartPm4Capture: Unexpected message type in Capture response "
                     "(Expected: ",
                     MessageType::PM4_CAPTURE_RESPONSE,
                     ", Got: ",
                     response->GetMessageType(),
                     ")."));
    }

    auto* pm4_response = dynamic_cast<Pm4CaptureResponse*>(response.get());
    if (!pm4_response)
    {
        return absl::InternalError(
        "StartPm4Capture: Failed to cast received message to Pm4CaptureResponse.");
    }

    std::cout << "Client: StartPm4Capture response OK (remote_file_path: "
              << pm4_response->GetString() << ")." << std::endl;
    return pm4_response->GetString();
}

absl::Status TcpClient::DownloadFileFromServer(const std::string& remote_file_path,
                                               const std::string& local_save_path)
{
    std::lock_guard<std::mutex> lock(m_connection_mutex);
    if (!IsConnected())
    {
        return absl::FailedPreconditionError("DownloadFileFromServer: Client is not connected.");
    }

    DownloadFileRequest download_request;
    download_request.SetString(remote_file_path);

    std::cout << "Client: Requesting to download file from server '" << remote_file_path << "' to '"
              << local_save_path << "'." << std::endl;
    auto send_status = SendMessage(m_connection.get(), download_request);
    if (!send_status.ok())
    {
        return SetStatusAndReturnError(ClientStatus::CONNECTION_FAILED,
                                       absl::Status(send_status.code(),
                                                    absl::StrCat("DownloadFileFromServer: "
                                                                 "SendMessage fail: ",
                                                                 send_status.message())));
    }

    auto receive_or = ReceiveMessage(m_connection.get());
    if (!receive_or.ok())
    {
        return SetStatusAndReturnError(ClientStatus::CONNECTION_FAILED,
                                       absl::Status(receive_or.status().code(),
                                                    absl::StrCat("DownloadFileFromServer: "
                                                                 "ReceiveMessage fail: ",
                                                                 receive_or.status().message())));
    }

    auto response = std::move(receive_or.value());
    if (response->GetMessageType() != MessageType::DOWNLOAD_FILE_RESPONSE)
    {
        return absl::FailedPreconditionError(
        absl::StrCat("DownloadFileFromServer: Unexpected message type in Download response "
                     "(Expected: ",
                     MessageType::DOWNLOAD_FILE_RESPONSE,
                     ", Got: ",
                     response->GetMessageType(),
                     ")."));
    }

    auto* download_response = dynamic_cast<DownloadFileResponse*>(response.get());
    if (!download_response)
    {
        return absl::InternalError(
        "DownloadFileFromServer: Failed to cast received message to DownloadFileResponse.");
    }

    if (!download_response->GetFound())
    {
        return absl::NotFoundError(
        absl::StrCat("DownloadFileFromServer: Server could not provide file. Reason: ",
                     download_response->GetErrorReason()));
    }

    std::cout << "Client: Server offering file (size = " << download_response->GetFileSizeStr()
              << " bytes). Starting download." << std::endl;
    size_t file_size = 0;
    try
    {
        file_size = std::stoull(download_response->GetFileSizeStr());
    }
    catch (const std::exception& e)
    {
        return absl::InvalidArgumentError(
        absl::StrCat("DownloadFileFromServer: Invalid file size from server: '",
                     download_response->GetFileSizeStr(),
                     "'. Message error: ",
                     e.what()));
    }

    auto recv_status = m_connection->ReceiveFile(local_save_path, file_size);
    if (!recv_status.ok())
    {
        return SetStatusAndReturnError(ClientStatus::CONNECTION_FAILED,
                                       absl::Status(recv_status.code(),
                                                    absl::StrCat("DownloadFileFromServer: "
                                                                 "ReceiveFile fail: ",
                                                                 recv_status.message())));
    }

    std::cout << "Client: File from server '" << download_request.GetString()
              << "' downloaded successfully to '" << local_save_path << "'." << std::endl;
    return absl::OkStatus();
}

absl::Status TcpClient::PingServer()
{
    std::lock_guard<std::mutex> lock(m_connection_mutex);
    if (!IsConnected())
    {
        return absl::FailedPreconditionError(absl::StrCat("PingServer: Client is not connected."));
    }

    PingMessage ping_request;
    std::cout << "Client: Send PING." << std::endl;
    auto send_status = SendMessage(m_connection.get(), ping_request);
    if (!send_status.ok())
    {
        return SetStatusAndReturnError(ClientStatus::CONNECTION_FAILED,
                                       absl::Status(send_status.code(),
                                                    absl::StrCat("PingServer: SendMessage fail: ",
                                                                 send_status.message())));
    }

    auto receive_or = ReceiveMessage(m_connection.get(), kPingTimeoutMs);
    if (!receive_or.ok())
    {
        return SetStatusAndReturnError(ClientStatus::CONNECTION_FAILED,
                                       absl::
                                       Status(receive_or.status().code(),
                                              absl::StrCat("PingServer: ReceiveMessage fail: ",
                                                           receive_or.status().message())));
    }

    auto response = std::move(receive_or.value());
    if (response->GetMessageType() != MessageType::PONG_MESSAGE)
    {
        return absl::FailedPreconditionError(
        absl::StrCat("PingServer: Unexpected message type in Ping response "
                     "(Expected: ",
                     MessageType::PONG_MESSAGE,
                     ", Got: ",
                     response->GetMessageType(),
                     ")."));
    }

    auto* pong_response = dynamic_cast<PongMessage*>(response.get());
    if (!pong_response)
    {
        return absl::InternalError("PingServer: Failed to cast received message to PongMessage.");
    }
    std::cout << "Client: Ping successful." << std::endl;
    return absl::OkStatus();
}

absl::Status TcpClient::PerformHandshake()
{
    std::lock_guard<std::mutex> lock(m_connection_mutex);
    if (!IsConnected())
    {
        return absl::FailedPreconditionError(
        absl::StrCat("PerformHandshake: Client is not connected."));
    }

    HandShakeRequest hs_request;
    hs_request.SetMajorVersion(kHandshakeMajorVersion);
    hs_request.SetMinorVersion(kHandshakeMinorVersion);
    std::cout << "Client: Sending Handshake (Client v" << hs_request.GetMajorVersion() << "."
              << hs_request.GetMinorVersion() << ")" << std::endl;

    auto send_status = SendMessage(m_connection.get(), hs_request);
    if (!send_status.ok())
    {
        return SetStatusAndReturnError(ClientStatus::CONNECTION_FAILED,
                                       absl::
                                       Status(send_status.code(),
                                              absl::StrCat("PerformHandshake: SendMessage fail: ",
                                                           send_status.message())));
    }

    auto receive_or = ReceiveMessage(m_connection.get());
    if (!receive_or.ok())
    {
        return SetStatusAndReturnError(ClientStatus::CONNECTION_FAILED,
                                       absl::Status(receive_or.status().code(),
                                                    absl::StrCat("PerformHandshake: ReceiveMessage "
                                                                 "fail: ",
                                                                 receive_or.status().message())));
    }

    auto response = std::move(receive_or.value());
    if (response->GetMessageType() != MessageType::HANDSHAKE_RESPONSE)
    {
        return absl::FailedPreconditionError(
        absl::StrCat("PerformHandshake: Unexpected message type in Handshake response "
                     "(Expected: ",
                     MessageType::HANDSHAKE_RESPONSE,
                     ", Got: ",
                     response->GetMessageType(),
                     ")."));
    }

    auto* hs_response = dynamic_cast<HandShakeResponse*>(response.get());
    if (!hs_response)
    {
        return absl::InternalError(
        "PerformHandshake: Failed to cast received message to HandShakeResponse.");
    }

    std::cout << "Client: Server Handshake (Server v" << hs_response->GetMajorVersion() << "."
              << hs_response->GetMinorVersion() << ")" << std::endl;

    if (hs_response->GetMajorVersion() != hs_request.GetMajorVersion() ||
        hs_response->GetMinorVersion() != hs_request.GetMinorVersion())
    {
        return absl::FailedPreconditionError(
        absl::StrCat("PerformHandshake: Handshake version mismatch. Server is v",
                     hs_response->GetMajorVersion(),
                     ".",
                     hs_response->GetMinorVersion(),
                     " Client requires v",
                     hs_request.GetMajorVersion(),
                     ".",
                     hs_request.GetMinorVersion()));
    }
    std::cout << "Client: Handshake versions compatible." << std::endl;
    return absl::OkStatus();
}

absl::Status TcpClient::StartKeepAlive()
{
    if (m_keep_alive.running.load())
    {
        return absl::AlreadyExistsError("StartKeepAlive: Keep-alive is already running.");
    }
    if (!IsConnected())
    {
        return absl::FailedPreconditionError("StartKeepAlive: Client is not connected.");
    }

    m_keep_alive.running.store(true);
    m_keep_alive.thread = std::thread(&TcpClient::KeepAliveLoop, this);

    std::cout << "Client: Keep-alive thread started. Interval: " << m_keep_alive.interval_sec
              << " s, Ping Timeout: " << kPingTimeoutMs << " ms." << std::endl;
    return absl::OkStatus();
}

void TcpClient::KeepAliveLoop()
{
    while (m_keep_alive.running.load())
    {
        std::unique_lock<std::mutex> lk(m_keep_alive.mutex);
        if (m_keep_alive.cv.wait_for(lk, std::chrono::seconds(m_keep_alive.interval_sec), [this] {
                return !m_keep_alive.running.load();
            }))
        {
            std::cout << "KeepAliveLoop: Client Keep-Alive Thread: Stop signaled via CV."
                      << std::endl;
            break;
        }
        lk.unlock();

        if (IsConnected())
        {
            auto ping_status = PingServer();
            if (!ping_status.ok())
            {
                std::cout << "KeepAliveLoop: Ping failed. Reason: " << ping_status.message()
                          << std::endl;
                SetClientStatus(ClientStatus::CONNECTION_FAILED);
                m_keep_alive.running.store(false);
            }
        }
        else
        {
            std::cout << "KeepAliveLoop: Client not connected. Stopping keep-alive." << std::endl;
            m_keep_alive.running.store(false);
        }
    }
}

void TcpClient::StopKeepAlive()
{
    m_keep_alive.running.store(false);
    m_keep_alive.cv.notify_one();
    if (m_keep_alive.thread.joinable())
    {
        m_keep_alive.thread.join();
    }
}

ClientStatus TcpClient::GetClientStatus() const
{
    std::lock_guard<std::mutex> lock(m_status_mutex);
    return m_status;
}

void TcpClient::SetClientStatus(ClientStatus status)
{
    std::lock_guard<std::mutex> lock(m_status_mutex);
    m_status = status;
}

absl::Status TcpClient::SetStatusAndReturnError(ClientStatus        status,
                                                const absl::Status& error_status)
{
    SetClientStatus(status);
    return error_status;
}

}  // namespace Network