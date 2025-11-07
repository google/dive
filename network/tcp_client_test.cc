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
            auto msg = ReceiveSocketMessage(m_conn.get(), 100);
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

class TcpClientFakeTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_client_fake = new FakeSocketConnection();
        m_server_fake = std::make_shared<FakeSocketConnection>();
        m_client_fake->PairWith(m_server_fake.get());

        // Inject the pre-created fake into the client
        m_client = std::make_unique<TcpClient>(
        [this]() { return std::unique_ptr<SocketConnection>(m_client_fake); });

        m_fake_server.Start(m_server_fake);
        ASSERT_TRUE(m_client->Connect("fake_host", 0).ok());
    }

    void TearDown() override
    {
        m_client->Disconnect();
        m_fake_server.Stop();
    }

    std::unique_ptr<TcpClient>            m_client;
    FakeServer                            m_fake_server;
    FakeSocketConnection*                 m_client_fake;
    std::shared_ptr<FakeSocketConnection> m_server_fake;
};

TEST_F(TcpClientFakeTest, GetCaptureFileSizeSuccess)
{
    const size_t kExpectedSize = 12345;
    m_fake_server.SetHandler(
    [kExpectedSize](FakeSocketConnection* conn, std::unique_ptr<ISerializable> req) {
        ASSERT_EQ(req->GetMessageType(), MessageType::FILE_SIZE_REQUEST);
        FileSizeResponse resp;
        resp.SetFound(true);
        resp.SetFileSizeStr(std::to_string(kExpectedSize));
        ASSERT_TRUE(SendSocketMessage(conn, resp).ok());
    });

    auto result = m_client->GetCaptureFileSize("/remote/file");
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(*result, kExpectedSize);
}

TEST_F(TcpClientFakeTest, GetCaptureFileSizeNotFound)
{
    m_fake_server.SetHandler([](FakeSocketConnection* conn, std::unique_ptr<ISerializable> req) {
        FileSizeResponse resp;
        resp.SetFound(false);
        resp.SetErrorReason("File does not exist");
        ASSERT_TRUE(SendSocketMessage(conn, resp).ok());
    });

    auto result = m_client->GetCaptureFileSize("/remote/missing");
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.status().code(), absl::StatusCode::kNotFound);
    EXPECT_TRUE(std::string(result.status().message()).find("File does not exist") !=
                std::string::npos);
}

TEST_F(TcpClientFakeTest, GetCaptureFileSizeInvalidResponse)
{
    // Server sends non-numeric size
    m_fake_server.SetHandler([](FakeSocketConnection* conn, std::unique_ptr<ISerializable> req) {
        FileSizeResponse resp;
        resp.SetFound(true);
        resp.SetFileSizeStr("not a number");
        ASSERT_TRUE(SendSocketMessage(conn, resp).ok());
    });

    auto result = m_client->GetCaptureFileSize("/remote/file");
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.status().code(), absl::StatusCode::kInvalidArgument);
}

TEST_F(TcpClientFakeTest, DownloadFileFromServerSuccess)
{
    std::string file_content = "Mock file content data";
    std::string local_path = (std::filesystem::temp_directory_path() / "test_download.tmp")
                             .string();

    // Configure server to send response AND file data immediately
    m_fake_server.SetHandler(
    [file_content](FakeSocketConnection* conn, std::unique_ptr<ISerializable> req) {
        ASSERT_EQ(req->GetMessageType(), MessageType::DOWNLOAD_FILE_REQUEST);

        DownloadFileResponse resp;
        resp.SetFound(true);
        resp.SetFileSizeStr(std::to_string(file_content.size()));
        ASSERT_TRUE(SendSocketMessage(conn, resp).ok());

        // Immediately send the "file" raw data through the fake connection
        ASSERT_TRUE(
        conn->Send(reinterpret_cast<const uint8_t*>(file_content.data()), file_content.size())
        .ok());
    });

    auto status = m_client->DownloadFileFromServer("/remote/file", local_path);
    ASSERT_TRUE(status.ok()) << status.message();

    // Verify file was written to disk by TcpClient's base SocketConnection::ReceiveFile
    std::ifstream ifs(local_path, std::ios::binary);
    ASSERT_TRUE(ifs.good());
    std::string read_content((std::istreambuf_iterator<char>(ifs)),
                             std::istreambuf_iterator<char>());
    EXPECT_EQ(read_content, file_content);

    std::filesystem::remove(local_path);
}

TEST_F(TcpClientFakeTest, DownloadFileNotFound)
{
    m_fake_server.SetHandler([](FakeSocketConnection* conn, std::unique_ptr<ISerializable> req) {
        DownloadFileResponse resp;
        resp.SetFound(false);
        resp.SetErrorReason("Restricted access");
        ASSERT_TRUE(SendSocketMessage(conn, resp).ok());
    });

    auto status = m_client->DownloadFileFromServer("/remote/secret", "dont_care.tmp");
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.code(), absl::StatusCode::kNotFound);
    EXPECT_TRUE(std::string(status.message()).find("Restricted access") != std::string::npos);
}

TEST_F(TcpClientFakeTest, StartPm4CaptureSuccess)
{
    m_fake_server.SetHandler([](FakeSocketConnection* conn, std::unique_ptr<ISerializable> req) {
        ASSERT_EQ(req->GetMessageType(), MessageType::PM4_CAPTURE_REQUEST);
        Pm4CaptureResponse resp;
        resp.SetString("/var/log/capture.pm4");
        ASSERT_TRUE(SendSocketMessage(conn, resp).ok());
    });

    auto result = m_client->StartPm4Capture();
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(*result, "/var/log/capture.pm4");
}

TEST_F(TcpClientFakeTest, DisconnectStopsKeepAlive)
{
    EXPECT_TRUE(m_client->IsConnected());
    m_client->Disconnect();
    EXPECT_FALSE(m_client->IsConnected());
    // Wait a bit to ensure no crashes from background threads
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

}  // namespace
}  // namespace Network