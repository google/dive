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

const std::string kTestServerAddress = "test_server_socket";

// The tests assume the client and server are running on a PC (Linux), as we are not using
// an Android device. However, this creates a limitation: we cannot directly test the TcpClient
// against the UnixDomainServer because they use incompatible communication protocols. The TcpClient
// uses TCP/IP sockets, while the UnixDomainServer uses Unix domain sockets (UDS). To test the
// UnixDomainServer, we will start an instance of it and use a basic, raw socket client to verify
// its behavior, such as handling connections and basic messages.
class UnixDomainServerTest : public ::testing::Test
{
protected:
    void SetUp() override { server = std::make_unique<UnixDomainServer>(); }

    void TearDown() override
    {
        if (server)
        {
            server->Stop();
        }
    }

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
        strncpy(addr.sun_path + 1, kTestServerAddress.c_str(), kTestServerAddress.size());
        if (connect(client_socket,
                    (sockaddr*)&addr,
                    offsetof(sockaddr_un, sun_path) + 1 + kTestServerAddress.size()) < 0)
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

    std::unique_ptr<UnixDomainServer> server;
};

TEST_F(UnixDomainServerTest, StartAndStop)
{
    ASSERT_TRUE(server->Start(kTestServerAddress).ok());
    // Give the server enough time to start.
    std::this_thread::sleep_for(std::chrono::seconds(1));
    server->Stop();
    // Should return immediately.
    server->Wait();
}

TEST_F(UnixDomainServerTest, ClientConnectAndDisconnect)
{
    ASSERT_TRUE(server->Start(kTestServerAddress).ok());
    absl::StatusOr<std::unique_ptr<SocketConnection>> client_conn = ConnectClient();
    ASSERT_TRUE(client_conn.ok());
    EXPECT_TRUE((*client_conn)->IsOpen());
    (*client_conn)->Close();
    EXPECT_FALSE((*client_conn)->IsOpen());
}

TEST_F(UnixDomainServerTest, HandShakeSuccess)
{
    ASSERT_TRUE(server->Start(kTestServerAddress).ok());
    absl::StatusOr<std::unique_ptr<SocketConnection>> client_conn = ConnectClient();
    ASSERT_TRUE(client_conn.ok());

    // Client sends request.
    HandshakeRequest request;
    request.SetMajorVersion(5);
    request.SetMinorVersion(2);
    ASSERT_TRUE(SendMessage((*client_conn).get(), request).ok());

    // Client receives response.
    absl::StatusOr<std::unique_ptr<ISerializable>> response_msg = ReceiveMessage(
    (*client_conn).get());
    ASSERT_TRUE(response_msg.ok());
    ASSERT_NE(*response_msg, nullptr);

    // Verify response.
    ASSERT_EQ((*response_msg)->GetMessageType(), MessageType::HANDSHAKE_RESPONSE);
    auto* response = dynamic_cast<HandshakeResponse*>((*response_msg).get());
    ASSERT_NE(response, nullptr);
    EXPECT_EQ(response->GetMajorVersion(), request.GetMajorVersion());
    EXPECT_EQ(response->GetMinorVersion(), request.GetMinorVersion());
}

TEST_F(UnixDomainServerTest, HandShakeFails)
{
    ASSERT_TRUE(server->Start(kTestServerAddress).ok());
    absl::StatusOr<std::unique_ptr<SocketConnection>> client_conn = ConnectClient();
    ASSERT_TRUE(client_conn.ok());

    // Client sends ping message rather than HandShakeRequest.
    PingMessage request;
    ASSERT_TRUE(SendMessage((*client_conn).get(), request).ok());

    // Client receives pong message rather than HandShakeResponse.
    absl::StatusOr<std::unique_ptr<ISerializable>> response_msg = ReceiveMessage(
    (*client_conn).get());
    ASSERT_TRUE(response_msg.ok());
    ASSERT_NE(*response_msg, nullptr);
    ASSERT_NE((*response_msg)->GetMessageType(), MessageType::HANDSHAKE_RESPONSE);
}

TEST_F(UnixDomainServerTest, PingPongSuccess)
{
    ASSERT_TRUE(server->Start(kTestServerAddress).ok());
    auto client_conn = ConnectClient();
    ASSERT_TRUE(client_conn.ok());

    // Client sends ping.
    PingMessage ping;
    ASSERT_TRUE(SendMessage((*client_conn).get(), ping).ok());

    // Client receives pong.
    auto response_msg = ReceiveMessage((*client_conn).get());
    ASSERT_TRUE(response_msg.ok());
    ASSERT_NE(*response_msg, nullptr);
    ASSERT_EQ((*response_msg)->GetMessageType(), MessageType::PONG_MESSAGE);
}

TEST_F(UnixDomainServerTest, PingPongFails)
{
    ASSERT_TRUE(server->Start(kTestServerAddress).ok());
    auto client_conn = ConnectClient();
    ASSERT_TRUE(client_conn.ok());

    // Client sends HandShakeRequest rather than ping message.
    HandshakeRequest ping;
    ASSERT_TRUE(SendMessage((*client_conn).get(), ping).ok());

    // Client receives HandShakeResponse rather than pong message.
    auto response_msg = ReceiveMessage((*client_conn).get());
    ASSERT_TRUE(response_msg.ok());
    ASSERT_NE(*response_msg, nullptr);
    ASSERT_NE((*response_msg)->GetMessageType(), MessageType::PONG_MESSAGE);
}

}  // namespace
}  // namespace Network