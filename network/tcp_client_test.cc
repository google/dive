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
#include "unix_domain_server.h"

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

namespace Network
{
namespace
{

constexpr std::string_view kTestHost = "127.0.0.1";
constexpr int              kTestPort = 54321;
constexpr std::string_view kTestFileContent = "This is a test file for download.";

std::string GetServerSideFileName()
{
    return (std::filesystem::temp_directory_path() / "test_server_file.tmp").string();
}

std::string GetClientSideFileName()
{
    return (std::filesystem::temp_directory_path() / "test_client_file.tmp").string();
}

// The tests assume the client and server are running on a PC (Linux), as we are not using
// an Android device. However, this creates a limitation: we cannot directly test the TcpClient
// against the UnixDomainServer because they use incompatible communication protocols. The TcpClient
// uses TCP/IP sockets, while the UnixDomainServer uses Unix domain sockets (UDS). To test the
// TcpClient, we create a minimal TCP server.
class TestTcpServer
{
public:
    TestTcpServer() :
        m_is_running(false)
    {
    }

    ~TestTcpServer() { Stop(); }

    absl::Status Start(int port)
    {
        auto conn = SocketConnection::Create();
        if (!conn.ok())
        {
            return conn.status();
        }
        m_listen_connection = *std::move(conn);

        // This is a simplified, test-only bind/listen for TCP on any address.
        sockaddr_in server_addr = {};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        SocketType server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0)
        {
            return absl::InternalError("Test server socket creation failed.");
        }

        // To avoid "address already in use" errors in tests.
        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
        {
            close(server_socket);
            return absl::InternalError("Test server bind failed.");
        }
        if (listen(server_socket, 1) < 0)
        {
            close(server_socket);
            return absl::InternalError("Test server listen failed.");
        }

        auto new_conn = SocketConnection::Create(server_socket);
        m_listen_connection = *std::move(new_conn);
        m_listen_connection->SetIsListening(true);

        m_is_running.store(true);
        m_server_thread = std::thread(&TestTcpServer::AcceptLoop, this);
        return absl::OkStatus();
    }

    void Stop()
    {
        if (m_is_running.load())
        {
            m_is_running.store(false);
            m_listen_connection->Close();
            if (m_server_thread.joinable())
            {
                m_server_thread.join();
            }
        }
    }

private:
    // Simplified version to accept only one client.
    void AcceptLoop()
    {
        while (m_is_running.load())
        {
            auto client_conn = m_listen_connection->Accept();
            if (!client_conn.ok())
            {
                if (!m_is_running.load())
                {
                    // Exiting loop due to shutdown.
                    break;
                }
                // Accept can time out or has an internal error, which is fine.
                std::cout << client_conn.status().message() << std::endl;
                continue;
            }
            HandleClient(*std::move(client_conn));
        }
    }

    void HandleClient(std::unique_ptr<SocketConnection> client_conn)
    {
        DefaultMessageHandler handler;
        handler.OnConnect();
        while (client_conn->IsOpen())
        {
            auto msg = ReceiveMessage(client_conn.get());
            if (!msg.ok())
            {
                // Client disconnected or error.
                break;
            }
            // Add custom handlers for file operations.
            if ((*msg)->GetMessageType() == MessageType::PM4_CAPTURE_REQUEST)
            {
                Pm4CaptureResponse response;
                response.SetString(GetServerSideFileName());
                ASSERT_TRUE(SendMessage(client_conn.get(), response).ok());
            }
            else if ((*msg)->GetMessageType() == MessageType::FILE_SIZE_REQUEST)
            {
                auto*            req = dynamic_cast<FileSizeRequest*>((*msg).get());
                FileSizeResponse response;
                if (req->GetString() == GetServerSideFileName() &&
                    std::filesystem::exists(GetServerSideFileName()))
                {
                    response.SetFound(true);
                    response.SetFileSizeStr(std::to_string(kTestFileContent.size()));
                }
                else
                {
                    response.SetFound(false);
                    response.SetErrorReason("File not found.");
                }
                ASSERT_TRUE(SendMessage(client_conn.get(), response).ok());
            }
            else if ((*msg)->GetMessageType() == MessageType::DOWNLOAD_FILE_REQUEST)
            {
                auto*                req = dynamic_cast<DownloadFileRequest*>((*msg).get());
                DownloadFileResponse response;
                if (req->GetString() == GetServerSideFileName() &&
                    std::filesystem::exists(GetServerSideFileName()))
                {
                    response.SetFound(true);
                    response.SetFileSizeStr(std::to_string(kTestFileContent.size()));
                    ASSERT_TRUE(SendMessage(client_conn.get(), response).ok());
                    ASSERT_TRUE(client_conn->SendFile(GetServerSideFileName()).ok());
                }
                else
                {
                    response.SetFound(false);
                    response.SetErrorReason("File not found.");
                    ASSERT_TRUE(SendMessage(client_conn.get(), response).ok());
                }
            }
            else
            {
                handler.HandleMessage(*std::move(msg), client_conn.get());
            }
        }
        handler.OnDisconnect();
    }

    std::unique_ptr<SocketConnection> m_listen_connection;
    std::thread                       m_server_thread;
    std::atomic<bool>                 m_is_running;
};

class TcpClientTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create a dummy file for the server to serve.
        std::ofstream ofs(GetServerSideFileName());
        ofs << kTestFileContent;
        ofs.close();
        ASSERT_TRUE(server.Start(kTestPort).ok());
        ASSERT_TRUE(client.Connect(std::string(kTestHost), kTestPort).ok());
    }

    void TearDown() override
    {
        client.Disconnect();
        server.Stop();
        std::filesystem::remove(GetServerSideFileName());
        std::filesystem::remove(GetClientSideFileName());
    }

    TestTcpServer server;
    TcpClient     client;
};

TEST_F(TcpClientTest, IsConnected)
{
    EXPECT_TRUE(client.IsConnected());
    client.Disconnect();
    EXPECT_FALSE(client.IsConnected());
}

TEST_F(TcpClientTest, ConnectionFails)
{
    TcpClient new_client;
    // Connecting to a port where no one is listening.
    auto status = new_client.Connect(std::string(kTestHost), kTestPort + 1);
    EXPECT_FALSE(status.ok());
}

TEST_F(TcpClientTest, StartPm4Capture)
{
    auto result = client.StartPm4Capture();
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(*result, GetServerSideFileName());
}

TEST_F(TcpClientTest, GetCaptureFileSize)
{
    auto result = client.GetCaptureFileSize(GetServerSideFileName());
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(*result, kTestFileContent.size());
}

TEST_F(TcpClientTest, GetCaptureFileSizeNotFound)
{
    auto result = client.GetCaptureFileSize("nonexistent.file");
    ASSERT_FALSE(result.ok());
    EXPECT_EQ(result.status().code(), absl::StatusCode::kNotFound);
}

TEST_F(TcpClientTest, DownloadFileFromServer)
{
    auto status = client.DownloadFileFromServer(GetServerSideFileName(), GetClientSideFileName());
    ASSERT_TRUE(status.ok());

    std::ifstream ifs(GetClientSideFileName());
    ASSERT_TRUE(ifs.good());
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    EXPECT_EQ(content, kTestFileContent);
}

TEST_F(TcpClientTest, DownloadFileNotFound)
{
    auto status = client.DownloadFileFromServer("nonexistent.file", GetClientSideFileName());
    ASSERT_FALSE(status.ok());
    EXPECT_EQ(status.code(), absl::StatusCode::kNotFound);
}

}  // namespace
}  // namespace Network