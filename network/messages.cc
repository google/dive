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

#include "messages.h"

constexpr uint32_t kMaxPayloadSize = 16 * 1024 * 1024;

namespace Network
{

void WriteUint32ToBuffer(uint32_t value, Buffer& dest)
{
    uint32_t       net_val = htonl(value);
    const uint8_t* p_val = reinterpret_cast<const uint8_t*>(&net_val);
    dest.insert(dest.end(), p_val, p_val + sizeof(uint32_t));
}

void WriteStringToBuffer(const std::string& str, Buffer& dest)
{
    WriteUint32ToBuffer(static_cast<uint32_t>(str.length()), dest);
    dest.insert(dest.end(), str.begin(), str.end());
}

absl::StatusOr<uint32_t> ReadUint32FromBuffer(const Buffer& src, size_t& offset)
{
    if (src.size() < offset + sizeof(uint32_t))
    {
        return absl::InvalidArgumentError("Buffer too small to read a uint32_t.");
    }
    uint32_t net_val;
    std::memcpy(&net_val, src.data() + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    return ntohl(net_val);
}

absl::StatusOr<std::string> ReadStringFromBuffer(const Buffer& src, size_t& offset)
{
    absl::StatusOr<uint32_t> val = ReadUint32FromBuffer(src, offset);
    if (!val.ok())
    {
        return val.status();
    }

    uint32_t len = *val;
    if (src.size() < offset + len)
    {
        return absl::InvalidArgumentError("Buffer too small for declared string length.");
    }
    std::string result(reinterpret_cast<const char*>(src.data() + offset), len);
    offset += len;
    return result;
}

absl::Status HandShakeMessage::Serialize(Buffer& dest) const
{
    dest.clear();
    WriteUint32ToBuffer(m_major_version, dest);
    WriteUint32ToBuffer(m_minor_version, dest);
    return absl::OkStatus();
}

absl::Status HandShakeMessage::Deserialize(const Buffer& src)
{
    size_t                   offset = 0;
    absl::StatusOr<uint32_t> major = ReadUint32FromBuffer(src, offset);
    if (!major.ok())
    {
        return major.status();
    }
    m_major_version = *major;

    absl::StatusOr<uint32_t> minor = ReadUint32FromBuffer(src, offset);
    if (!minor.ok())
    {
        return minor.status();
    }
    m_minor_version = *minor;

    if (offset != src.size())
    {
        return absl::InvalidArgumentError("Handshake message has unexpected trailing data.");
    }
    return absl::OkStatus();
}

absl::Status StringMessage::Serialize(Buffer& dest) const
{
    dest.clear();
    WriteStringToBuffer(m_str, dest);
    return absl::OkStatus();
}

absl::Status StringMessage::Deserialize(const Buffer& src)
{
    size_t                      offset = 0;
    absl::StatusOr<std::string> str = ReadStringFromBuffer(src, offset);
    if (!str.ok())
    {
        return str.status();
    }

    m_str = *std::move(str);
    if (offset != src.size())
    {
        return absl::InvalidArgumentError("String message has unexpected trailing data.");
    }
    return absl::OkStatus();
}

absl::Status DownloadFileResponse::Serialize(Buffer& dest) const
{
    dest.push_back(static_cast<uint8_t>(m_found));
    WriteStringToBuffer(m_error_reason, dest);
    WriteStringToBuffer(m_file_path, dest);
    WriteStringToBuffer(m_file_size_str, dest);

    return absl::OkStatus();
}

absl::Status DownloadFileResponse::Deserialize(const Buffer& src)
{
    size_t offset = 0;

    // Deserialize the 'found' boolean.
    if (src.size() < offset + sizeof(uint8_t))
    {
        return absl::InvalidArgumentError("Buffer too small for 'found' field.");
    }
    m_found = (src[offset] != 0);
    offset += sizeof(uint8_t);

    // Deserialize the strings using the StatusOr-returning helper
    absl::StatusOr<std::string> error_reason = ReadStringFromBuffer(src, offset);
    if (!error_reason.ok())
    {
        return error_reason.status();
    }
    m_error_reason = *std::move(error_reason);

    absl::StatusOr<std::string> file_path = ReadStringFromBuffer(src, offset);
    if (!file_path.ok())
    {
        return file_path.status();
    }
    m_file_path = *std::move(file_path);

    absl::StatusOr<std::string> file_size = ReadStringFromBuffer(src, offset);
    if (!file_size.ok())
    {
        return file_size.status();
    }
    m_file_size_str = *std::move(file_size);

    // Final check for trailing data.
    if (offset != src.size())
    {
        return absl::InvalidArgumentError("Message has unexpected trailing data.");
    }

    return absl::OkStatus();
}

absl::Status FileSizeResponse::Serialize(Buffer& dest) const
{
    dest.push_back(static_cast<uint8_t>(m_found));
    WriteStringToBuffer(m_error_reason, dest);
    WriteStringToBuffer(m_file_size_str, dest);

    return absl::OkStatus();
}

absl::Status FileSizeResponse::Deserialize(const Buffer& src)
{
    size_t offset = 0;

    // Deserialize the 'found' boolean.
    if (src.size() < offset + sizeof(uint8_t))
    {
        return absl::InvalidArgumentError("Buffer too small for 'found' field.");
    }
    m_found = (src[offset] != 0);
    offset += sizeof(uint8_t);

    // Deserialize the strings using the StatusOr-returning helper
    absl::StatusOr<std::string> error_reason = ReadStringFromBuffer(src, offset);
    if (!error_reason.ok())
    {
        return error_reason.status();
    }
    m_error_reason = *std::move(error_reason);

    absl::StatusOr<std::string> file_size = ReadStringFromBuffer(src, offset);
    if (!file_size.ok())
    {
        return file_size.status();
    }
    m_file_size_str = *std::move(file_size);

    // Final check for trailing data.
    if (offset != src.size())
    {
        return absl::InvalidArgumentError("Message has unexpected trailing data.");
    }

    return absl::OkStatus();
}

absl::Status ReceiveBuffer(SocketConnection* conn, uint8_t* buffer, size_t size, int timeout_ms)
{
    if (!conn)
    {
        return absl::InvalidArgumentError("Provided SocketConnection is null.");
    }
    size_t total_received = 0;
    while (total_received < size)
    {
        absl::StatusOr<size_t> received = conn->Recv(buffer + total_received,
                                                     size - total_received,
                                                     timeout_ms);
        if (!received.ok())
        {
            return received.status();
        }
        total_received += *received;
    }
    return absl::OkStatus();
}

absl::Status SendBuffer(SocketConnection* conn, const uint8_t* buffer, size_t size)
{
    if (!conn)
    {
        return absl::InvalidArgumentError("Provided SocketConnection is null.");
    }
    return conn->Send(buffer, size);
}

absl::StatusOr<std::unique_ptr<ISerializable>> ReceiveMessage(SocketConnection* conn,
                                                              int               timeout_ms)
{
    if (!conn)
    {
        return absl::InvalidArgumentError("Provided SocketConnection is null.");
    }

    const size_t header_size = sizeof(uint32_t) * 2;
    uint8_t      header_buffer[header_size];

    // Receive the message header.
    absl::Status status = ReceiveBuffer(conn, header_buffer, header_size, timeout_ms);
    if (!status.ok())
    {
        return status;
    }

    // Parse header.
    uint32_t net_type, net_length;
    std::memcpy(&net_type, header_buffer, sizeof(uint32_t));
    std::memcpy(&net_length, header_buffer + sizeof(uint32_t), sizeof(uint32_t));
    uint32_t type = ntohl(net_type);
    uint32_t payload_length = ntohl(net_length);

    if (payload_length > kMaxPayloadSize)
    {
        conn->Close();
        return absl::InvalidArgumentError(
        absl::StrCat("Payload size ", payload_length, " exceeds limit."));
    }

    // Receive the message payload.
    Buffer payload_buffer(payload_length);
    status = ReceiveBuffer(conn, payload_buffer.data(), payload_length, timeout_ms);
    if (!status.ok())
    {
        return status;
    }

    // Create and deserialize the message object.
    std::unique_ptr<ISerializable> message;
    switch (static_cast<MessageType>(type))
    {
    case MessageType::HANDSHAKE_REQUEST:
        message = std::make_unique<HandShakeRequest>();
        break;
    case MessageType::HANDSHAKE_RESPONSE:
        message = std::make_unique<HandShakeResponse>();
        break;
    case MessageType::PING_MESSAGE:
        message = std::make_unique<PingMessage>();
        break;
    case MessageType::PONG_MESSAGE:
        message = std::make_unique<PongMessage>();
        break;
    case MessageType::PM4_CAPTURE_REQUEST:
        message = std::make_unique<Pm4CaptureRequest>();
        break;
    case MessageType::PM4_CAPTURE_RESPONSE:
        message = std::make_unique<Pm4CaptureResponse>();
        break;
    case MessageType::DOWNLOAD_FILE_REQUEST:
        message = std::make_unique<DownloadFileRequest>();
        break;
    case MessageType::DOWNLOAD_FILE_RESPONSE:
        message = std::make_unique<DownloadFileResponse>();
        break;
    case MessageType::FILE_SIZE_REQUEST:
        message = std::make_unique<FileSizeRequest>();
        break;
    case MessageType::FILE_SIZE_RESPONSE:
        message = std::make_unique<FileSizeResponse>();
        break;
    default:
        conn->Close();
        return absl::InvalidArgumentError(absl::StrCat("Unknown message type: ", type));
    }

    status = message->Deserialize(payload_buffer);
    if (!status.ok())
    {
        conn->Close();
        return status;
    }

    return message;
}

absl::Status SendMessage(SocketConnection* conn, const ISerializable& message)
{
    if (!conn)
    {
        return absl::InvalidArgumentError("Provided SocketConnection is null.");
    }

    // Serialize the message payload.
    Buffer       payload_buffer;
    absl::Status status = message.Serialize(payload_buffer);
    if (!status.ok())
    {
        return status;
    }
    if (payload_buffer.size() > kMaxPayloadSize)
    {
        return absl::InvalidArgumentError("Serialized payload size exceeds limit.");
    }

    // Construct and send the header.
    uint32_t     net_type = htonl(static_cast<uint32_t>(message.GetMessageType()));
    uint32_t     net_payload_length = htonl(static_cast<uint32_t>(payload_buffer.size()));
    const size_t header_size = sizeof(net_type) + sizeof(net_payload_length);
    uint8_t      header_buffer[header_size];
    std::memcpy(header_buffer, &net_type, sizeof(uint32_t));
    std::memcpy(header_buffer + sizeof(uint32_t), &net_payload_length, sizeof(uint32_t));

    status = SendBuffer(conn, header_buffer, header_size);
    if (!status.ok())
    {
        return status;
    }

    // Send the payload.
    status = SendBuffer(conn, payload_buffer.data(), payload_buffer.size());
    if (!status.ok())
    {
        return status;
    }

    return absl::OkStatus();
}

}  // namespace Network
