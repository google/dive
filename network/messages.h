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

#include "serializable.h"
#include "socket_connection.h"

namespace Network
{

// Helper to write a uint32_t to a buffer.
void WriteUint32ToBuffer(uint32_t value, Buffer& dest);

// Helper to write a string (length + data) to the buffer.
void WriteStringToBuffer(const std::string& str, Buffer& dest);

// Helper to read a uint32_t from a buffer.
absl::StatusOr<uint32_t> ReadUint32FromBuffer(const Buffer& src, size_t& offset);

// Helper to read a string (length + data) from the buffer.
absl::StatusOr<std::string> ReadStringFromBuffer(const Buffer& src, size_t& offset);

enum class MessageType : uint32_t
{
    HANDSHAKE_REQUEST = 1,
    HANDSHAKE_RESPONSE = 2,
    PING_MESSAGE = 3,
    PONG_MESSAGE = 4,
    PM4_CAPTURE_REQUEST = 5,
    PM4_CAPTURE_RESPONSE = 6,
    DOWNLOAD_FILE_REQUEST = 7,
    DOWNLOAD_FILE_RESPONSE = 8
};

class HandShakeMessage : public ISerializable
{
public:
    absl::Status Serialize(Buffer& dest) const override;
    absl::Status Deserialize(const Buffer& src) override;

    uint32_t GetMajorVersion() const { return m_major_version; }
    uint32_t GetMinorVersion() const { return m_minor_version; }
    void     SetMajorVersion(uint32_t major) { m_major_version = major; }
    void     SetMinorVersion(uint32_t minor) { m_minor_version = minor; }

private:
    uint32_t m_major_version;
    uint32_t m_minor_version;
};

class HandShakeRequest : public HandShakeMessage
{
public:
    MessageType GetMessageType() const override { return m_type; }

private:
    const MessageType m_type = MessageType::HANDSHAKE_REQUEST;
};

class HandShakeResponse : public HandShakeMessage
{
public:
    MessageType GetMessageType() const override { return m_type; }

private:
    const MessageType m_type = MessageType::HANDSHAKE_RESPONSE;
};

class EmptyMessage : public ISerializable
{
public:
    absl::Status Serialize(Buffer& dest) const override { return absl::OkStatus(); }
    absl::Status Deserialize(const Buffer& src) override { return absl::OkStatus(); }
};

class Pm4CaptureRequest : public EmptyMessage
{
public:
    MessageType GetMessageType() const override { return m_type; }

private:
    const MessageType m_type = MessageType::PM4_CAPTURE_REQUEST;
};

class StringMessage : public ISerializable
{
public:
    absl::Status Serialize(Buffer& dest) const override;
    absl::Status Deserialize(const Buffer& src) override;

    const std::string& GetString() const { return m_str; }
    void               SetString(std::string str) { m_str = std::move(str); }

private:
    std::string m_str;
};

class Pm4CaptureResponse : public StringMessage
{
public:
    MessageType GetMessageType() const override { return m_type; }

private:
    const MessageType m_type = MessageType::PM4_CAPTURE_RESPONSE;
};

class PingMessage : public EmptyMessage
{
public:
    MessageType GetMessageType() const override { return m_type; }

private:
    const MessageType m_type = MessageType::PING_MESSAGE;
};

class PongMessage : public EmptyMessage
{
public:
    MessageType GetMessageType() const override { return m_type; }

private:
    const MessageType m_type = MessageType::PONG_MESSAGE;
};

class DownloadFileRequest : public StringMessage
{
public:
    MessageType GetMessageType() const override { return m_type; }

private:
    const MessageType m_type = MessageType::DOWNLOAD_FILE_REQUEST;
};

class DownloadFileResponse : public ISerializable
{
public:
    MessageType  GetMessageType() const override { return m_type; }
    absl::Status Serialize(Buffer& dest) const override;
    absl::Status Deserialize(const Buffer& src) override;

    bool GetFound() const { return m_found; }
    void SetFound(bool found) { m_found = found; }

    const std::string& GetErrorReason() const { return m_error_reason; }
    void SetErrorReason(std::string error_reason) { m_error_reason = std::move(error_reason); }

    const std::string& GetFilePath() const { return m_file_path; }
    void               SetFilePath(std::string file_path) { m_file_path = std::move(file_path); }

    const std::string& GetFileSizeStr() const { return m_file_size_str; }
    void SetFileSizeStr(std::string file_size_str) { m_file_size_str = std::move(file_size_str); }

private:
    // Flag indicating whether the requested file was found on the server.
    bool m_found;
    // A description of the error if the download failed. Empty if successful.
    std::string m_error_reason;
    // The local path where the downloaded file has been saved on the server.
    // It can be the same as the requested file path from client.
    std::string m_file_path;
    // A string representation of the downloaded file's size.
    // It avoids to use uint64_t which requires custom implementation for htonll/ntohll.
    std::string m_file_size_str;

    const MessageType m_type = MessageType::DOWNLOAD_FILE_RESPONSE;
};

// Message Helper Functions (TLV Framing).

// Helper to receive an exact number of bytes.
absl::Status ReceiveBuffer(SocketConnection* conn,
                           uint8_t*          buffer,
                           size_t            size,
                           int               timeout_ms = kNoTimeout);

// Helper to send an exact number of bytes.
absl::Status SendBuffer(SocketConnection* conn, const uint8_t* buffer, size_t size);

// Returns a fully-formed message or an error status.
absl::StatusOr<std::unique_ptr<ISerializable>> ReceiveMessage(SocketConnection* conn,
                                                              int timeout_ms = kNoTimeout);

// Sends a full message (header + payload).
absl::Status SendMessage(SocketConnection* conn, const ISerializable& message);

}  // namespace Network