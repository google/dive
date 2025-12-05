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

#include "unix_domain_server.h"

#include <gtest/gtest.h>

namespace Network
{
namespace
{

constexpr char kTestServerAddress[] = "test_server_socket";

// The tests assume the client and server are running on a PC (Linux), as we are not using
// an Android device. However, this creates a limitation: we cannot directly test the TcpClient
// against the UnixDomainServer because they use incompatible communication protocols. The TcpClient
// uses TCP/IP sockets, while the UnixDomainServer uses Unix domain sockets (UDS). To test the
// UnixDomainServer, we will start an instance of it and use a basic, raw socket client to verify
// its behavior, such as handling connections and basic messages.

// Creates a raw socket client and connects to the Unix Domain Server.
absl::StatusOr<std::unique_ptr<SocketConnection>> ConnectClient()
{
    SocketType client_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        return absl::InternalError("Client: socket creation failed.");
    }
    /// Connects to an Unix (Local) Domain with abstract namespace.
    sockaddr_un addr = {};
    addr.sun_family = AF_UNIX;
    // first char is '\0'
    addr.sun_path[0] = '\0';
    strncpy(addr.sun_path + 1, kTestServerAddress, std::size(kTestServerAddress));
    if (connect(client_socket,
                (sockaddr*)&addr,
                offsetof(sockaddr_un, sun_path) + std::size(kTestServerAddress)) < 0)
    {
        close(client_socket);
        return absl::UnavailableError("Client: connect failed.");
    }

    absl::StatusOr<std::unique_ptr<SocketConnection>> connection = SocketConnection::Create(
    client_socket);
    if (!connection.ok())
    {
        return connection.status();
    }
    return *std::move(connection);
}

TEST(UnixDomainServerTest, StartAndStop)
{
    auto server = std::make_unique<UnixDomainServer>();
    ASSERT_TRUE(server->Start(kTestServerAddress).ok());
    server->Stop();
    // Should return immediately.
    server->Wait();
}

TEST(UnixDomainServerTest, ClientConnectAndDisconnect)
{
    auto server = std::make_unique<UnixDomainServer>();
    ASSERT_TRUE(server->Start(kTestServerAddress).ok());

    absl::StatusOr<std::unique_ptr<SocketConnection>> client_conn = ConnectClient();
    ASSERT_TRUE(client_conn.ok());
    EXPECT_TRUE((*client_conn)->IsOpen());
    (*client_conn)->Close();
    EXPECT_FALSE((*client_conn)->IsOpen());

    server->Stop();
}

TEST(UnixDomainServerTest, HandShakeSuccess)
{
    auto server = std::make_unique<UnixDomainServer>();
    ASSERT_TRUE(server->Start(kTestServerAddress).ok());

    absl::StatusOr<std::unique_ptr<SocketConnection>> client_conn = ConnectClient();
    ASSERT_TRUE(client_conn.ok());

    // Client sends request.
    HandshakeRequest request;
    request.SetMajorVersion(5);
    request.SetMinorVersion(2);
    ASSERT_TRUE(SendSocketMessage((*client_conn).get(), request).ok());

    // Client receives response.
    absl::StatusOr<std::unique_ptr<ISerializable>> response_msg = ReceiveSocketMessage(
    (*client_conn).get());
    ASSERT_TRUE(response_msg.ok());
    ASSERT_NE(*response_msg, nullptr);

    // Verify response.
    ASSERT_EQ((*response_msg)->GetMessageType(), MessageType::HANDSHAKE_RESPONSE);
    auto* response = dynamic_cast<HandshakeResponse*>((*response_msg).get());
    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->GetMajorVersion(), request.GetMajorVersion());
    EXPECT_EQ(response->GetMinorVersion(), request.GetMinorVersion());

    server->Stop();
}

TEST(UnixDomainServerTest, HandShakeFails)
{
    auto server = std::make_unique<UnixDomainServer>();
    ASSERT_TRUE(server->Start(kTestServerAddress).ok());

    absl::StatusOr<std::unique_ptr<SocketConnection>> client_conn = ConnectClient();
    ASSERT_TRUE(client_conn.ok());

    // Client sends a HANDSHAKE_REQUEST header, but claims a payload length of 0.
    // The server expects 8 bytes (2 uint32) for a handshake. Deserialization should fail,
    // causing the server to close the connection.

    uint32_t type = htonl(static_cast<uint32_t>(MessageType::HANDSHAKE_REQUEST));
    // Invalid length (Too small)
    uint32_t len = htonl(0);

    std::vector<uint8_t> buffer;
    buffer.insert(buffer.end(), (uint8_t*)&type, (uint8_t*)&type + sizeof(type));
    buffer.insert(buffer.end(), (uint8_t*)&len, (uint8_t*)&len + sizeof(len));

    // Send the malformed message.
    ASSERT_TRUE((*client_conn)->Send(buffer.data(), buffer.size()).ok());

    // The server should detect the error and close the connection.
    // ReceiveSocketMessage should return an error.
    absl::StatusOr<std::unique_ptr<ISerializable>> response_msg = ReceiveSocketMessage(
    (*client_conn).get());

    EXPECT_FALSE(response_msg.ok());

    server->Stop();
}

TEST(UnixDomainServerTest, PingPongSuccess)
{
    auto server = std::make_unique<UnixDomainServer>();
    ASSERT_TRUE(server->Start(kTestServerAddress).ok());

    absl::StatusOr<std::unique_ptr<SocketConnection>> client_conn = ConnectClient();
    ASSERT_TRUE(client_conn.ok());

    // Client sends ping.
    PingMessage ping;
    ASSERT_TRUE(SendSocketMessage((*client_conn).get(), ping).ok());

    // Client receives pong.
    absl::StatusOr<std::unique_ptr<ISerializable>> response_msg = ReceiveSocketMessage(
    (*client_conn).get());
    ASSERT_TRUE(response_msg.ok());
    ASSERT_NE(*response_msg, nullptr);
    ASSERT_EQ((*response_msg)->GetMessageType(), MessageType::PONG_MESSAGE);

    server->Stop();
}

TEST(UnixDomainServerTest, PingPongFails)
{
    auto server = std::make_unique<UnixDomainServer>();
    ASSERT_TRUE(server->Start(kTestServerAddress).ok());

    absl::StatusOr<std::unique_ptr<SocketConnection>> client_conn = ConnectClient();
    ASSERT_TRUE(client_conn.ok());

    // Client sends a PING_MESSAGE header but declares an invalid payload size.
    // We choose a size larger than kMaxPayloadSize (16MB) to trigger the security check
    // in ReceiveSocketMessage on the server side immediately.

    const uint32_t kTooBigSize = 32 * 1024 * 1024;
    uint32_t       type = htonl(static_cast<uint32_t>(MessageType::PING_MESSAGE));
    uint32_t       len = htonl(kTooBigSize);

    std::vector<uint8_t> buffer;
    buffer.insert(buffer.end(), (uint8_t*)&type, (uint8_t*)&type + sizeof(type));
    buffer.insert(buffer.end(), (uint8_t*)&len, (uint8_t*)&len + sizeof(len));

    // Send the malicious header.
    ASSERT_TRUE((*client_conn)->Send(buffer.data(), buffer.size()).ok());

    // The server should detect the excessive size in the header and close the connection.
    absl::StatusOr<std::unique_ptr<ISerializable>> response_msg = ReceiveSocketMessage(
    (*client_conn).get());

    EXPECT_FALSE(response_msg.ok());

    server->Stop();
}

}  // namespace
}  // namespace Network