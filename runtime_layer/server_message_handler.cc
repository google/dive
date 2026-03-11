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

#include "absl/log/log.h"
#include "network/message_utils.h"

namespace DiveLayer
{

void ServerMessageHandler::HandleMessage(std::unique_ptr<Network::ISerializable> message,
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
        default:
        {
            Network::BaseMessageHandler::HandleMessage(std::move(message), client_conn);
            break;
        }
    }
}

}  // namespace DiveLayer
