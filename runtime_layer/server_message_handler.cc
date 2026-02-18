/*
Copyright 2026 Google Inc.

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

#include "server_message_handler.h"

#include "common/log.h"
#include "network/message_utils.h"

namespace DiveLayer
{

void ServerMessageHandler::OnConnect() { LOGI("ServerMessageHandler: onConnect()"); }

void ServerMessageHandler::OnDisconnect() { LOGI("ServerMessageHandler: onDisconnect()"); }

void ServerMessageHandler::HandleMessage(std::unique_ptr<Network::ISerializable> message,
                                         Network::SocketConnection* client_conn)
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
            auto status = Network::SendPong(client_conn);
            if (!status.ok())
            {
                LOGI("Send pong failed: %.*s", (int)status.message().length(),
                     status.message().data());
            }
            break;
        }
        case Network::MessageType::HANDSHAKE_REQUEST:
        {
            LOGI("Message received: HandShakeRequest");
            auto* request = dynamic_cast<Network::HandshakeRequest*>(message.get());
            if (request)
            {
                auto status = Network::Handshake(request, client_conn);
                if (!status.ok())
                {
                    LOGI("Handshake failed: %.*s", (int)status.message().length(),
                         status.message().data());
                }
            }
            else
            {
                LOGI("HandShakeRequest message is null.");
            }
            break;
        }
        case Network::MessageType::DOWNLOAD_FILE_REQUEST:
        {
            LOGI("Message received: DownloadFileRequest");
            auto* request = dynamic_cast<Network::DownloadFileRequest*>(message.get());
            if (request)
            {
                auto status = Network::DownloadFile(request, client_conn);
                if (!status.ok())
                {
                    LOGI("DownloadFile failed: %.*s", (int)status.message().length(),
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
            auto* request = dynamic_cast<Network::FileSizeRequest*>(message.get());
            if (request)
            {
                auto status = Network::GetFileSize(request, client_conn);
                if (!status.ok())
                {
                    LOGI("GetFileSize failed: %.*s", (int)status.message().length(),
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

}  // namespace DiveLayer
