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

#include "absl/base/log_severity.h"
#include "absl/log/log.h"
#include "constants.h"
#include "dive/utils/device_resources_constants.h"
#include "network/message_utils.h"
#include "trace_mgr.h"

namespace Dive
{

absl::Status StartPm4Capture(Network::SocketConnection* client_conn)
{
    GetTraceMgr().TriggerTrace();
    GetTraceMgr().WaitForTraceDone();
    std::string capture_file_path = GetTraceMgr().GetTraceFilePath();

    Network::Pm4CaptureResponse response;
    response.SetString(capture_file_path);
    return Network::SendSocketMessage(client_conn, response);
}

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
        case Network::MessageType::PM4_CAPTURE_REQUEST:
        {
            LOG(INFO) << "Message received: Pm4CaptureRequest";
            if (absl::Status status = StartPm4Capture(client_conn); !status.ok())
            {
                LOG(ERROR) << "StartPm4Capture failed: " << status.message();
            }
            return;
        }
        default:
        {
            Network::BaseMessageHandler::HandleMessage(std::move(message), client_conn);
            return;
        }
    }
}

int RunServer()
{
    // We use a Unix (local) domain socket in an abstract namespace rather than an internet domain.
    // It avoids the need to grant INTERNET permission to the target application.
    // Also, no file-based permissions apply since it is in an abstract namespace.
    auto server =
        std::make_unique<Network::UnixDomainServer>(std::make_unique<ServerMessageHandler>());

    std::string server_address = Dive::DeviceResourcesConstants::kUnixAbstractPath;
    auto status = server->Start(server_address);
    if (!status.ok())
    {
        LOG(ERROR) << "Could not start the server: " << status.message();
        return 1;
    }
    LOG(INFO) << "Server listening on: " << server_address;
    server->Wait();
    return 0;
}

int ServerMain() { return RunServer(); }

}  // namespace Dive
