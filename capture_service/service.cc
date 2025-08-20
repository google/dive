/*
Copyright 2023 Google Inc.

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

#include "service.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include "constants.h"
#include "common/log.h"
#include "trace_mgr.h"

namespace Dive
{

absl::Status SendPong(Network::SocketConnection *client_conn)
{
    Network::PongMessage response;
    return Network::SendMessage(client_conn, response);
}

absl::Status Handshake(Network::HandshakeRequest *request, Network::SocketConnection *client_conn)
{
    Network::HandshakeResponse response;
    response.SetMajorVersion(request->GetMajorVersion());
    response.SetMinorVersion(request->GetMinorVersion());
    return Network::SendMessage(client_conn, response);
}

absl::Status StartPm4Capture(Network::SocketConnection *client_conn)
{
    GetTraceMgr().TriggerTrace();
    GetTraceMgr().WaitForTraceDone();
    std::string capture_file_path = GetTraceMgr().GetTraceFilePath();

    Network::Pm4CaptureResponse response;
    response.SetString(capture_file_path);
    return Network::SendMessage(client_conn, response);
}

absl::Status DownloadFile(Network::DownloadFileRequest *request,
                          Network::SocketConnection    *client_conn)
{
    Network::DownloadFileResponse response;
    std::string                   file_path = request->GetString();

    std::error_code ec;
    auto            file_size = std::filesystem::file_size(file_path, ec);
    if (!ec)
    {
        response.SetFound(true);
        response.SetFilePath(file_path);
        response.SetFileSizeStr(std::to_string(file_size));
    }
    else
    {
        response.SetFound(false);
        response.SetErrorReason(ec.message());
    }

    auto status = Network::SendMessage(client_conn, response);
    if (!status.ok())
    {
        return status;
    }
    if (!response.GetFound())
    {
        return absl::NotFoundError(response.GetErrorReason());
    }
    return client_conn->SendFile(file_path);
}

absl::Status GetFileSize(Network::FileSizeRequest *request, Network::SocketConnection *client_conn)
{
    Network::FileSizeResponse response;
    std::string               file_path = request->GetString();

    std::error_code ec;
    auto            file_size = std::filesystem::file_size(file_path, ec);
    if (!ec)
    {
        response.SetFound(true);
        response.SetFileSizeStr(std::to_string(file_size));
    }
    else
    {
        response.SetFound(false);
        response.SetErrorReason(ec.message());
    }
    return Network::SendMessage(client_conn, response);
}

void ServerMessageHandler::OnConnect()
{
    LOGI("ServerMessageHandler: onConnect()");
}

void ServerMessageHandler::OnDisconnect()
{
    LOGI("ServerMessageHandler: onDisconnect()");
}

void ServerMessageHandler::HandleMessage(std::unique_ptr<Network::ISerializable> message,
                                         Network::SocketConnection              *client_conn)
{
    if (!message)
    {
        LOGI("Message is null.");
        return;
    }
    if (!client_conn)
    {
        LOGI("Client connection is null.");
        return;
    }

    switch (message->GetMessageType())
    {
    case Network::MessageType::PING_MESSAGE:
    {
        LOGI("Message received: Ping");
        auto status = SendPong(client_conn);
        if (!status.ok())
        {
            LOGI("Send pong failed: %.*s", (int)status.message().length(), status.message().data());
        }
        break;
    }
    case Network::MessageType::HANDSHAKE_REQUEST:
    {
        LOGI("Message received: HandShakeRequest");
        auto *request = dynamic_cast<Network::HandshakeRequest *>(message.get());
        if (request)
        {
            auto status = Handshake(request, client_conn);
            if (!status.ok())
            {
                LOGI("Handshake failed: %.*s",
                     (int)status.message().length(),
                     status.message().data());
            }
        }
        else
        {
            LOGI("HandShakeRequest message is null.");
        }
        break;
    }
    case Network::MessageType::PM4_CAPTURE_REQUEST:
    {
        LOGI("Message received: Pm4CaptureRequest");
        auto status = StartPm4Capture(client_conn);
        if (!status.ok())
        {
            LOGI("StartPm4Capture failed: %.*s",
                 (int)status.message().length(),
                 status.message().data());
        }
        break;
    }
    case Network::MessageType::DOWNLOAD_FILE_REQUEST:
    {
        LOGI("Message received: DownloadFileRequest");
        auto *request = dynamic_cast<Network::DownloadFileRequest *>(message.get());
        if (request)
        {
            auto status = DownloadFile(request, client_conn);
            if (!status.ok())
            {
                LOGI("DownloadFile failed: %.*s",
                     (int)status.message().length(),
                     status.message().data());
            }
        }
        else
        {
            LOGI("DownloadFileRequest message is null.");
        }
        break;
    }
    case Network::MessageType::FILE_SIZE_REQUEST:
    {
        LOGI("Message received: FileSizeRequest");
        auto *request = dynamic_cast<Network::FileSizeRequest *>(message.get());
        if (request)
        {
            auto status = GetFileSize(request, client_conn);
            if (!status.ok())
            {
                LOGI("GetFileSize failed: %.*s",
                     (int)status.message().length(),
                     status.message().data());
            }
        }
        else
        {
            LOGI("FileSizeRequest message is null.");
        }
        break;
    }
    default:
    {
        LOGW("Message type %d unhandled.", (int)message->GetMessageType());
        break;
    }
    }
}

void RunServer()
{
    // We use a Unix (local) domain socket in an abstract namespace rather than an internet domain.
    // It avoids the need to grant INTERNET permission to the target application.
    // Also, no file-based permissions apply since it is in an abstract namespace.
    auto server = std::make_unique<Network::UnixDomainServer>(
    std::make_unique<ServerMessageHandler>());

    std::string server_address = kUnixAbstractPath;
    auto        status = server->Start(server_address);
    if (!status.ok())
    {
        LOGW("Error starting the server: %.*s",
             static_cast<int>(status.message().length()),
             status.message().data());
    }
    LOGI("Server listening on %s", server_address.c_str());
    server->Wait();
}

int ServerMain()
{
    RunServer();
    return 0;
}

}  // namespace Dive