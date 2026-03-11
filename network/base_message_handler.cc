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

#include "base_message_handler.h"

#include <memory>

#include "absl/log/log.h"
#include "message_handler.h"
#include "message_utils.h"
#include "serializable.h"
#include "socket_connection.h"

namespace Network
{

void BaseMessageHandler::OnConnect() { LOG(INFO) << "BaseMessageHandler: onConnect()"; }

void BaseMessageHandler::OnDisconnect() { LOG(INFO) << "BaseMessageHandler: onDisconnect()"; }

void BaseMessageHandler::HandleMessage(std::unique_ptr<Network::ISerializable> message,
                                       Network::SocketConnection* client_conn)
{
    if (!message)
    {
        LOG(ERROR) << "Message is null.";
        return;
    }
    if (!client_conn)
    {
        LOG(ERROR) << "Client connection is null.";
        return;
    }

    switch (message->GetMessageType())
    {
        case Network::MessageType::PING_MESSAGE:
        {
            LOG(INFO) << "Message received: Ping";
            if (absl::Status status = Network::SendPong(client_conn); !status.ok())
            {
                LOG(ERROR) << "Send pong failed: " << status.message();
            }
            return;
        }
        case Network::MessageType::HANDSHAKE_REQUEST:
        {
            LOG(INFO) << "Message received: HandShakeRequest";
            auto* request = static_cast<Network::HandshakeRequest*>(message.get());

            if (absl::Status status = Network::Handshake(request, client_conn); !status.ok())
            {
                LOG(ERROR) << "Handshake failed: " << status.message();
                return;
            }
            return;
        }
        case Network::MessageType::DOWNLOAD_FILE_REQUEST:
        {
            LOG(INFO) << "Message received: DownloadFileRequest";
            auto* request = static_cast<Network::DownloadFileRequest*>(message.get());

            if (absl::Status status = Network::DownloadFile(request, client_conn); !status.ok())
            {
                LOG(ERROR) << "DownloadFile failed: " << status.message();
                return;
            }
            return;
        }
        case Network::MessageType::FILE_SIZE_REQUEST:
        {
            LOG(INFO) << "Message received: FileSizeRequest";
            auto* request = static_cast<Network::FileSizeRequest*>(message.get());

            if (absl::Status status = Network::GetFileSize(request, client_conn); !status.ok())
            {
                LOG(ERROR) << "GetFileSize failed:" << status.message();
                return;
            }
            return;
        }
        case Network::MessageType::REMOVE_FILE_REQUEST:
        {
            LOG(INFO) << "Message received: RemoveFileRequest";
            auto* request = static_cast<Network::RemoveFileRequest*>(message.get());

            if (absl::Status status = Network::RemoveFile(request, client_conn); !status.ok())
            {
                LOG(ERROR) << "RemoveFile failed: " << status.message();
                return;
            }
            return;
        }
        default:
        {
            LOG(WARNING) << "Message type unhandled, type: "
                         << static_cast<uint32_t>(message->GetMessageType());
            return;
        }
    }
}

}  // namespace Network
