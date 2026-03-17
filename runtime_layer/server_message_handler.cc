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
#include "network/drawcall_filter_config.h"
#include "network/message_utils.h"
#include "vk_rt_layer_impl.h"

namespace DiveLayer
{

extern DiveRuntimeLayer sDiveRuntimeLayer;

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
        case Network::MessageType::DRAWCALL_FILTER_CONFIG_REQUEST:
        {
            LOG(INFO) << "Message received: DrawcallFilterConfigRequest";
            auto* request = static_cast<Network::DrawcallFilterConfigRequest*>(message.get());
            Network::DrawcallFilterConfig config;
            config.target_vertex_count = request->GetVertexCount();
            config.target_index_count = request->GetIndexCount();
            config.target_instance_count = request->GetInstanceCount();
            config.filter_by_vertex_count = request->GetFilterByVertexCount();
            config.filter_by_index_count = request->GetFilterByIndexCount();
            config.filter_by_instance_count = request->GetFilterByInstanceCount();

            sDiveRuntimeLayer.EnqueueFrameBoundaryTask(
                [config]() { sDiveRuntimeLayer.UpdateFilterConfig(config); });

            Network::DrawcallFilterConfigResponse response;
            if (absl::Status status = Network::SendSocketMessage(client_conn, response);
                !status.ok())
            {
                LOG(ERROR) << "Send DrawcallFilterConfigResponse failed: " << status.message();
                return;
            }
            return;
        }
        default:
        {
            Network::BaseMessageHandler::HandleMessage(std::move(message), client_conn);
            break;
        }
    }
}

}  // namespace DiveLayer
