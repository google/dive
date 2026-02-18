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

#pragma once

#include "messages.h"
#include "socket_connection.h"

namespace Network
{

absl::Status SendPong(Network::SocketConnection* client_conn);

absl::Status Handshake(Network::HandshakeRequest* request, Network::SocketConnection* client_conn);

absl::Status DownloadFile(Network::DownloadFileRequest* request,
                          Network::SocketConnection* client_conn);

absl::Status GetFileSize(Network::FileSizeRequest* request, Network::SocketConnection* client_conn);

absl::Status RemoveFile(Network::RemoveFileRequest* request,
                        Network::SocketConnection* client_conn);

}  // namespace Network
