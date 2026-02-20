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

#include "message_utils.h"

#include <filesystem>

#include "dive/common/status.h"

namespace Network
{

absl::Status SendPong(Network::SocketConnection* client_conn)
{
    Network::PongMessage response;
    return Network::SendSocketMessage(client_conn, response);
}

absl::Status Handshake(Network::HandshakeRequest* request, Network::SocketConnection* client_conn)
{
    Network::HandshakeResponse response;
    response.SetMajorVersion(request->GetMajorVersion());
    response.SetMinorVersion(request->GetMinorVersion());
    return Network::SendSocketMessage(client_conn, response);
}

absl::Status DownloadFile(Network::DownloadFileRequest* request,
                          Network::SocketConnection* client_conn)
{
    Network::DownloadFileResponse response;
    std::string file_path = request->GetString();
    std::error_code ec;

    if (std::filesystem::is_regular_file(file_path, ec))
    {
        if (auto file_size = std::filesystem::file_size(file_path, ec); !ec)
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
    }
    else
    {
        response.SetFound(false);
        response.SetErrorReason(ec ? ec.message() : "Path does not exist or is not a regular file");
    }

    if (auto status = Network::SendSocketMessage(client_conn, response); !status.ok())
    {
        return Dive::StatusWithContext(status, "DownloadFile");
    }

    if (!response.GetFound())
    {
        return Dive::NotFoundError(response.GetErrorReason());
    }

    return client_conn->SendFile(file_path);
}

absl::Status GetFileSize(Network::FileSizeRequest* request, Network::SocketConnection* client_conn)
{
    Network::FileSizeResponse response;
    std::string file_path = request->GetString();
    std::error_code ec;

    if (std::filesystem::is_regular_file(file_path, ec))
    {
        if (auto file_size = std::filesystem::file_size(file_path, ec); !ec)
        {
            response.SetFound(true);
            response.SetFileSizeStr(std::to_string(file_size));
        }
        else
        {
            response.SetFound(false);
            response.SetErrorReason(ec.message());
        }
    }
    else
    {
        response.SetFound(false);
        response.SetErrorReason(ec ? ec.message() : "Path does not exist or is not a regular file");
    }

    return Network::SendSocketMessage(client_conn, response);
}

absl::Status RemoveFile(Network::RemoveFileRequest* request, Network::SocketConnection* client_conn)
{
    Network::RemoveFileResponse response;
    std::string file_path = request->GetString();
    std::error_code ec;

    if (std::filesystem::remove(file_path, ec))
    {
        response.SetSuccess(true);
    }
    else
    {
        response.SetSuccess(false);
        response.SetErrorReason(ec ? ec.message() : "Unknown error");
    }

    return Network::SendSocketMessage(client_conn, response);
}

}  // namespace Network
