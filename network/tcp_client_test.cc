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

#include "tcp_client.h"
#include "fake_socket_connection.h"
#include "messages.h"

#include <gtest/gtest.h>
#include <thread>
#include <filesystem>
#include <fstream>

namespace Network
{
namespace
{

class FakeServer
{
public:
    using RequestHandler = std::function<void(FakeSocketConnection*,
                                              std::unique_ptr<ISerializable>)>;

    void Start(std::shared_ptr<FakeSocketConnection> conn)
    {
        m_conn = conn;
        m_running = true;
        m_thread = std::thread(&FakeServer::RunLoop, this);
    }

    void Stop()
    {
        m_running = false;
        if (m_conn)
        {
            m_conn->Close();
        }
        if (m_thread.joinable())
        {
            m_thread.join();
        }
    }

    void SetHandler(RequestHandler handler)
    {
        std::lock_guard<std::mutex> lock(m_handler_mutex);
        m_handler = std::move(handler);
    }

private:
    void RunLoop()
    {
        while (m_running && m_conn && m_conn->IsOpen())
        {
            auto msg = ReceiveSocketMessage(m_conn.get());
            if (!msg.ok())
            {
                if (absl::IsDeadlineExceeded(msg.status()))
                {
                    continue;
                }
                break;
            }

            auto type = (*msg)->GetMessageType();
            if (type == MessageType::HANDSHAKE_REQUEST)
            {
                HandshakeResponse resp;
                resp.SetMajorVersion(1);
                resp.SetMinorVersion(0);
                if (!SendSocketMessage(m_conn.get(), resp).ok())
                {
                    break;
                }
            }
            else if (type == MessageType::PING_MESSAGE)
            {
                PongMessage resp;
                if (!SendSocketMessage(m_conn.get(), resp).ok())
                {
                    break;
                }
            }
            else
            {
                std::lock_guard<std::mutex> lock(m_handler_mutex);
                if (m_handler)
                {
                    m_handler(m_conn.get(), std::move(*msg));
                }
            }
        }
    }

    std::shared_ptr<FakeSocketConnection> m_conn;
    std::atomic<bool>                     m_running{ false };
    std::thread                           m_thread;
    RequestHandler                        m_handler;
    std::mutex                            m_handler_mutex;
};

struct TestContext
{
    std::shared_ptr<FakeSocketConnection> server_socket;
    FakeServer                            server;
    std::unique_ptr<TcpClient>            client;

    TestContext()
    {
        server_socket = std::make_shared<FakeSocketConnection>();

        client = std::make_unique<TcpClient>([this]() {
            auto connection = std::make_unique<FakeSocketConnection>();
            connection->PairWith(server_socket.get());
            return connection;
        });

        server.Start(server_socket);
        EXPECT_TRUE(client->Connect("fake_host", 0).ok());
    }

    ~TestContext()
    {
        client->Disconnect();
        server.Stop();
    }
};

TEST(TcpClientFakeTest, GetCaptureFileSizeSuccess)
{
    TestContext      ctx;
    constexpr size_t kExpectedSize = 12345;
    MessageType      received_type = MessageType::UNKNOWN;

    ctx.server.SetHandler([&](FakeSocketConnection* conn, std::unique_ptr<ISerializable> req) {
        received_type = req->GetMessageType();

        FileSizeResponse resp;
        resp.SetFound(true);
        resp.SetFileSizeStr(std::to_string(kExpectedSize));
        (void)SendSocketMessage(conn, resp);
    });

    auto result = ctx.client->GetCaptureFileSize("/remote/file");

    ASSERT_EQ(received_type, MessageType::FILE_SIZE_REQUEST);
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(*result, kExpectedSize);
}

TEST(TcpClientFakeTest, GetCaptureFileSizeNotFound)
{
    TestContext ctx;

    ctx.server.SetHandler([](FakeSocketConnection* conn, std::unique_ptr<ISerializable> req) {
        FileSizeResponse resp;
        resp.SetFound(false);
        resp.SetErrorReason("File does not exist");
        (void)SendSocketMessage(conn, resp);
    });

    auto result = ctx.client->GetCaptureFileSize("/remote/missing");

    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.status().code(), absl::StatusCode::kNotFound);
    EXPECT_TRUE(std::string(result.status().message()).find("File does not exist") !=
                std::string::npos);
}

TEST(TcpClientFakeTest, GetCaptureFileSizeInvalidResponse)
{
    TestContext ctx;

    ctx.server.SetHandler([](FakeSocketConnection* conn, std::unique_ptr<ISerializable> req) {
        FileSizeResponse resp;
        resp.SetFound(true);
        resp.SetFileSizeStr("not a number");
        (void)SendSocketMessage(conn, resp);
    });

    auto result = ctx.client->GetCaptureFileSize("/remote/file");
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.status().code(), absl::StatusCode::kInvalidArgument);
}

TEST(TcpClientFakeTest, DownloadFileFromServerSuccess)
{
    TestContext ctx;
    std::string file_content = "Mock file content data";
    std::string local_path = (std::filesystem::temp_directory_path() / "test_download.tmp")
                             .string();

    MessageType received_type = MessageType::UNKNOWN;

    ctx.server.SetHandler([&](FakeSocketConnection* conn, std::unique_ptr<ISerializable> req) {
        received_type = req->GetMessageType();

        DownloadFileResponse resp;
        resp.SetFound(true);
        resp.SetFileSizeStr(std::to_string(file_content.size()));
        if (SendSocketMessage(conn, resp).ok())
        {
            (void)conn->Send(reinterpret_cast<const uint8_t*>(file_content.data()),
                             file_content.size());
        }
    });

    auto status = ctx.client->DownloadFileFromServer("/remote/file", local_path);

    ASSERT_EQ(received_type, MessageType::DOWNLOAD_FILE_REQUEST);
    ASSERT_TRUE(status.ok()) << status.message();

    std::ifstream ifs(local_path, std::ios::binary);
    ASSERT_TRUE(ifs.good());
    std::string read_content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
    EXPECT_EQ(read_content, file_content);

    ifs.close();
    std::filesystem::remove(local_path);
}

TEST(TcpClientFakeTest, DownloadFileFromServerNotFound)
{
    TestContext ctx;
    ctx.server.SetHandler([](FakeSocketConnection* conn, std::unique_ptr<ISerializable> req) {
        DownloadFileResponse resp;
        resp.SetFound(false);
        resp.SetErrorReason("Restricted access");
        (void)SendSocketMessage(conn, resp);
    });

    auto status = ctx.client->DownloadFileFromServer("/remote/secret", "dont_care.tmp");
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.code(), absl::StatusCode::kNotFound);
    EXPECT_TRUE(std::string(status.message()).find("Restricted access") != std::string::npos);
}

TEST(TcpClientFakeTest, StartPm4CaptureSuccess)
{
    TestContext ctx;
    MessageType received_type = MessageType::UNKNOWN;

    ctx.server.SetHandler([&](FakeSocketConnection* conn, std::unique_ptr<ISerializable> req) {
        received_type = req->GetMessageType();
        Pm4CaptureResponse resp;
        resp.SetString("/var/log/capture.pm4");
        (void)SendSocketMessage(conn, resp);
    });

    auto result = ctx.client->StartPm4Capture();

    ASSERT_EQ(received_type, MessageType::PM4_CAPTURE_REQUEST);
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(*result, "/var/log/capture.pm4");
}

TEST(TcpClientFakeTest, DisconnectAndReconnect)
{
    TestContext ctx;
    EXPECT_TRUE(ctx.client->IsConnected());

    ctx.client->Disconnect();
    EXPECT_FALSE(ctx.client->IsConnected());

    ctx.server.Stop();
    ctx.server_socket = std::make_shared<FakeSocketConnection>();
    ctx.server.Start(ctx.server_socket);

    auto status = ctx.client->Connect("fake_host", 0);
    EXPECT_TRUE(status.ok()) << status.message();
    EXPECT_TRUE(ctx.client->IsConnected());
}

}  // namespace
}  // namespace Network