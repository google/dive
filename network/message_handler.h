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
#include "serializable.h"
#include "socket_connection.h"

namespace Network
{

class IMessageHandler
{
public:
    virtual ~IMessageHandler() = default;

    // Callback for when a new client connects.
    virtual void OnConnect() = 0;

    // Processes a message received from the client.
    virtual void HandleMessage(std::unique_ptr<ISerializable> message,
                               SocketConnection*              client_conn) = 0;

    // Callback for when a client disconnects.
    virtual void OnDisconnect() = 0;
};

}  // namespace Network