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

#include "socket_connection.h"
#include <fstream>
#include <string>
#include <vector>

#ifdef _WIN32
#    define _WINSOCK_DEPRECATED_NO_WARNINGS
#    define NOMINMAX
#    include <ws2tcpip.h>
#    pragma comment(lib, "Ws2_32.lib")
typedef SSIZE_T ssize_t;
#else
#    include <netdb.h>
#    include <poll.h>
#    include <sys/un.h>
#    include <unistd.h>
#endif

namespace Network
{

NetworkInitializer::NetworkInitializer() :
    m_initialized(false)
{
#ifdef _WIN32
    WSADATA wd;
    int     res = WSAStartup(MAKEWORD(2, 2), &wd);
    if (res == 0)
    {
        m_initialized = true;
        // Register WSACleanup to be called automatically on program exit
        std::atexit([]() { WSACleanup(); });
    }
    else
    {
        m_initialized = false;
    }
#else
    m_initialized = true;
#endif
}

const NetworkInitializer& NetworkInitializer::Instance()
{
    static NetworkInitializer singleton;
    return singleton;
}

bool NetworkInitializer::IsInitialized() const
{
    return m_initialized;
}

SocketConnection::SocketConnection() :
    SocketConnection(kInvalidSocketValue)
{
}

SocketConnection::~SocketConnection()
{
    Close();
}

SocketConnection::SocketConnection(SocketType initial_socket_value) :
    m_socket(initial_socket_value),
    m_is_listening(false),
    m_accept_timout_ms(kAcceptTimeout),
    m_recv_timeout_ms(kNoTimeout)
{
    if (!NetworkInitializer::Instance().IsInitialized())
    {
        m_last_error_msg = "CRITICAL: Network subsystem failed to initialize.";
    }
}

bool SocketConnection::BindAndListenOnUnixDomain(const std::string& server_address,
                                                 std::error_code&   ec)
{
#ifdef _WIN32
    ec = std::make_error_code(std::errc::operation_not_permitted);
    SetError(ec,
             "BindAndListenOnUnixDomain: This POSIX server method is not supported/implemented on "
             "Windows.");
    return false;
#else
    ec.clear();
    m_last_error_msg.clear();
    if (IsOpen())
    {
        Close();
    }
    m_socket = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_socket == kInvalidSocketValue)
    {
        SetPlatformError("BindAndListenOnUnixDomain: socket() creation failed.");
        ec = std::make_error_code(std::errc::io_error);
        return false;
    }

    /// Bind and listen on a Unix (Local) Domain with abstract namespace.
    sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    // first char is '\0'
    addr.sun_path[0] = '\0';
    strncpy(addr.sun_path + 1, server_address.c_str(), server_address.size() + 1);

    int ret = ::bind(m_socket,
                     (sockaddr*)&addr,
                     (socklen_t)(offsetof(sockaddr_un, sun_path) + 1 + server_address.size()));
    if (ret < 0)
    {
        SetPlatformError("BindAndListenOnUnixDomain: bind() failed");
        ec = std::make_error_code(std::errc::io_error);
        Close();
        return false;
    }
    ret = ::listen(m_socket, SOMAXCONN);
    if (ret < 0)
    {
        SetPlatformError("BindAndListenOnUnixDomain: listen() failed");
        ec = std::make_error_code(std::errc::io_error);
        Close();
        return false;
    }
    m_is_listening = true;
    m_last_error_msg.clear();
    return true;
#endif
}

std::unique_ptr<SocketConnection> SocketConnection::Accept(std::error_code& ec)
{
#ifdef _WIN32
    ec = std::make_error_code(std::errc::operation_not_permitted);
    SetError(ec, "Accept: This POSIX server method is not supported/implemented on Windows.");
    return nullptr;
#else
    ec.clear();
    m_last_error_msg.clear();
    if (!IsOpen() || !m_is_listening)
    {
        ec = std::make_error_code(std::errc::operation_not_permitted);
        SetError(ec, "Accept: Socket not created or not listening.");
        return nullptr;
    }
    pollfd pfd;
    pfd.fd = m_socket;
    pfd.events = POLLIN;
    pfd.revents = 0;
    int ret = poll(&pfd, 1, m_accept_timout_ms);
    if (ret < 0)
    {
        SetPlatformError("Accept: poll() failed.");
        ec = std::make_error_code(std::errc::io_error);
        return nullptr;
    }
    if (ret == 0)
    {
        ec = std::make_error_code(std::errc::timed_out);
        SetError(ec, "Accept: Timeout waiting for connection.");
        return nullptr;
    }
    if (!(pfd.revents & POLLIN))
    {
        ec = std::make_error_code(std::errc::io_error);
        SetError(ec, "Accept: poll() returned without POLLIN or known error event.");
        return nullptr;
    }

    SocketType new_socket = (SocketType)::accept(m_socket, nullptr, nullptr);
    if (new_socket == kInvalidSocketValue)
    {
        SetPlatformError("Accept: accept() system call failed.");
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            ec = std::make_error_code(std::errc::operation_would_block);
        }
        if (!ec)
        {
            ec = std::make_error_code(std::errc::io_error);
        }
        return nullptr;
    }
    return std::make_unique<SocketConnection>(new_socket);
#endif
}

bool SocketConnection::Connect(const std::string& host, int port, std::error_code& ec)
{
    ec.clear();
    m_last_error_msg.clear();
    if (IsOpen())
    {
        Close();
    }
    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo*   server_info = nullptr;
    std::string port_str = std::to_string(port);

    int ret = getaddrinfo(host.c_str(), port_str.c_str(), &hints, &server_info);
    if (ret)
    {
        m_last_error_msg = "Connect: getaddrinfo: " + std::string(gai_strerror(ret));
        ec = std::make_error_code(std::errc::network_unreachable);
        if (server_info)
        {
            freeaddrinfo(server_info);
        }
        return false;
    }

    std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> si_guard(server_info, freeaddrinfo);
    std::string                                        most_recent_err;
    for (addrinfo* p = server_info; p != nullptr; p = p->ai_next)
    {
        m_socket = (SocketType)::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (m_socket == kInvalidSocketValue)
        {
            SetPlatformError("Connect: socket() creation failed.");
            most_recent_err = m_last_error_msg;
            continue;
        }
        if (::connect(m_socket, p->ai_addr, (socklen_t)p->ai_addrlen) == -1)
        {
            SetPlatformError("Connect: connect() failed.");
            most_recent_err = m_last_error_msg;
            Close();
            continue;
        }
        break;
    }
    if (m_socket == kInvalidSocketValue)
    {
        m_last_error_msg = most_recent_err.empty() ? "Connect: All attempts failed." :
                                                     most_recent_err;
        ec = std::make_error_code(std::errc::connection_refused);
        return false;
    }
    m_is_listening = false;
    m_last_error_msg.clear();
    return true;
}

size_t SocketConnection::Send(const uint8_t* data, size_t size, std::error_code& ec)
{
    ec.clear();
    m_last_error_msg.clear();
    if (!IsOpen() || m_is_listening)
    {
        ec = std::make_error_code(std::errc::not_connected);
        SetError(ec, "Send: Socket is invalid or operation not supported on a listening socket.");
        return 0;
    }
    if (size == 0)
    {
        return 0;
    }

    size_t total_sent = 0;
    while (total_sent < size)
    {
        ssize_t sent;
#ifdef _WIN32
        sent = ::send(static_cast<SOCKET>(m_socket),
                      (const char*)data + total_sent,
                      (int)(size - total_sent),
                      0);
#else
        sent = ::send(m_socket, data + total_sent, size - total_sent, MSG_NOSIGNAL);
#endif
        if (sent == -1)
        {
            SetPlatformError("Send: send() system call failed.");
            int e = 0;
#ifdef _WIN32
            e = WSAGetLastError();
            if (e == WSAEWOULDBLOCK)
            {
                ec = std::make_error_code(std::errc::operation_would_block);
            }
            else if (e == WSAECONNRESET || e == WSAECONNABORTED || e == WSAESHUTDOWN)
            {
                ec = std::make_error_code(std::errc::connection_reset);
                Close();
            }
            else
            {
                ec = std::make_error_code(std::errc::io_error);
            }
#else
            e = errno;
            if (e == EAGAIN || e == EWOULDBLOCK)
            {
                ec = std::make_error_code(std::errc::operation_would_block);
            }
            else if (e == EPIPE || e == ECONNRESET)
            {
                ec = std::make_error_code(std::errc::connection_reset);
                Close();
            }
            else
            {
                ec = std::make_error_code(std::errc::io_error);
            }
#endif
            return total_sent;
        }

        if (sent == 0)
        {
            if (total_sent < size)
            {
                ec = std::make_error_code(std::errc::connection_aborted);
                SetError(ec, "Send: Peer has closed the connection (send returned 0).");
                Close();
            }
            return total_sent;
        }

        total_sent += static_cast<size_t>(sent);
    }

    return total_sent;
}

size_t SocketConnection::Recv(uint8_t* data, size_t size, std::error_code& ec)
{
    ec.clear();
    m_last_error_msg.clear();
    if (!IsOpen() || m_is_listening)
    {
        ec = std::make_error_code(std::errc::not_connected);
        SetError(ec, "Recv: Socket is invalid or operation not supported on a listening socket.");
        return 0;
    }
    if (size == 0)
    {
        return 0;
    }

    // Wait for data to be available using poll/select with timeout.
#ifdef _WIN32
    TIMEVAL tv;
    fd_set  read_fds;
    FD_ZERO(&read_fds);
    FD_SET(static_cast<SOCKET>(m_socket), &read_fds);

    if (m_recv_timeout_ms >= 0)
    {
        tv.tv_sec = m_recv_timeout_ms / 1000;
        tv.tv_usec = (m_recv_timeout_ms % 1000) * 1000;
    }
    int activity = select(0, &read_fds, nullptr, nullptr, (m_recv_timeout_ms < 0) ? nullptr : &tv);
    if (activity == SOCKET_ERROR)
    {
        SetPlatformError("Recv: select failed during recv.");
        ec = std::make_error_code(std::errc::io_error);
        return 0;
    }
    if (activity == 0)
    {
        ec = std::make_error_code(std::errc::timed_out);
        SetError(ec, "Recv: timed out waiting for data.");
        return 0;
    }
#else
    struct pollfd pfd;
    pfd.fd = m_socket;
    pfd.events = POLLIN;
    pfd.revents = 0;

    int ret = poll(&pfd, 1, m_recv_timeout_ms);
    if (ret < 0)
    {
        SetPlatformError("Recv: poll() failed.");
        ec = std::make_error_code(std::errc::io_error);
        return 0;
    }
    if (ret == 0)
    {
        ec = std::make_error_code(std::errc::timed_out);
        SetError(ec, "Recv: Timeout waiting for data.");
        return 0;
    }
    if (!(pfd.revents & POLLIN))
    {
        ec = std::make_error_code(std::errc::io_error);
        SetError(ec, "Recv: poll() returned without POLLIN or known error event.");
        return 0;
    }
#endif

    // Data is available to perform the actual recv.
    ssize_t total_received;
#ifdef _WIN32
    total_received = ::recv(static_cast<SOCKET>(m_socket),
                            reinterpret_cast<char*>(data),
                            static_cast<int>(size),
                            0);
#else
    total_received = ::recv(m_socket, data, size, 0);
#endif

    if (total_received == -1)
    {
        SetPlatformError("Recv: recv() system call failed.");
#ifdef _WIN32
        int wsa_err = WSAGetLastError();
        if (wsa_err == WSAEWOULDBLOCK)
        {
            ec = std::make_error_code(std::errc::operation_would_block);
        }
        else if (wsa_err == WSAECONNRESET || wsa_err == WSAECONNABORTED || wsa_err == WSAESHUTDOWN)
        {
            ec = std::make_error_code(std::errc::connection_reset);
            Close();
        }
        else
        {
            ec = std::make_error_code(std::errc::io_error);
        }
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            ec = std::make_error_code(std::errc::operation_would_block);
        }
        else if (errno == ECONNRESET)
        {
            ec = std::make_error_code(std::errc::connection_reset);
            Close();
        }
        else
        {
            ec = std::make_error_code(std::errc::io_error);
        }
#endif
        return 0;
    }
    if (total_received == 0)
    {
        ec = std::make_error_code(std::errc::connection_aborted);
        SetError(ec, "Recv: Connection Closed by peer during recv (0 bytes received).");
        return 0;
    }
    return static_cast<size_t>(total_received);
}

bool SocketConnection::SendString(const std::string& s, std::error_code& ec)
{
    // Include null terminator.
    const auto len = s.length() + 1;
    return Send(reinterpret_cast<const uint8_t*>(s.c_str()), len, ec) == len;
}

bool SocketConnection::ReceiveString(std::string& s, std::error_code& ec)
{
    char c = 0;
    s.clear();
    ec.clear();
    while (true)
    {
        size_t received = Recv(reinterpret_cast<uint8_t*>(&c), 1, ec);
        if (ec || received != 1)
        {
            if (!ec && received == 0)
            {
                ec = std::make_error_code(std::errc::connection_aborted);
            }
            s.clear();
            return false;
        }
        if (c == 0)
        {
            return true;
        }
        s.push_back(c);
    }
}

bool SocketConnection::SendFile(const std::string& file_path, std::error_code& ec)
{
    ec.clear();
    m_last_error_msg.clear();
    std::ifstream file_stream(file_path, std::ios::binary | std::ios::ate);
    if (!file_stream)
    {
        ec = std::make_error_code(std::errc::no_such_file_or_directory);
        SetError(ec, "SendFile: Failed to open file '" + file_path + "'");
        return false;
    }
    std::streamsize file_size = file_stream.tellg();
    if (file_size < 0)
    {
        ec = std::make_error_code(std::errc::io_error);
        SetError(ec, "SendFile: Failed to determine size of file '" + file_path + "'");
        file_stream.close();
        return false;
    }

    file_stream.seekg(0);
    const size_t      CHUNK_SIZE = 4096;
    std::vector<char> buffer(CHUNK_SIZE);
    std::streamsize   total_sent = 0;
    while (total_sent < file_size)
    {
        std::streamsize to_read = std::min(static_cast<std::streamsize>(CHUNK_SIZE),
                                           file_size - total_sent);
        if (!file_stream.read(buffer.data(), to_read))
        {
            ec = std::make_error_code(std::errc::io_error);
            SetError(ec, "SendFile: Failed to read from file '" + file_path + "'");
            file_stream.close();
            return false;
        }
        size_t current_read = static_cast<size_t>(file_stream.gcount());
        if (current_read == 0)
        {
            ec = std::make_error_code(std::errc::io_error);
            SetError(ec, "SendFile: Read 0 bytes from file '" + file_path + "'");
            file_stream.close();
            return false;
        }
        size_t sent = this->Send((uint8_t*)buffer.data(), current_read, ec);
        if (ec || sent != current_read)
        {
            if (!ec)
            {
                ec = std::make_error_code(std::errc::io_error);
            }
            SetError(ec, "sendFile: Send chunk '" + file_path + "'");
            file_stream.close();
            return false;
        }
        total_sent += static_cast<std::streamsize>(sent);
    }
    file_stream.close();
    return true;
}

bool SocketConnection::ReceiveFile(const std::string& file_path,
                                   size_t             file_size,
                                   std::error_code&   ec)
{
    ec.clear();
    m_last_error_msg.clear();
    std::ofstream file_stream(file_path, std::ios::binary | std::ios::trunc);
    if (!file_stream)
    {
        ec = std::make_error_code(std::errc::permission_denied);
        SetError(ec, "ReceiveFile: Failed to open file '" + file_path + "'");
        return false;
    }
    const size_t         CHUNK_SIZE = 4096;
    std::vector<uint8_t> buffer(CHUNK_SIZE);
    size_t               total_received = 0;
    while (total_received < file_size)
    {
        size_t to_receive = std::min(CHUNK_SIZE, file_size - total_received);
        size_t current_received = this->Recv(buffer.data(), to_receive, ec);
        if (ec || current_received == 0)
        {
            if (!ec && current_received == 0)
            {
                ec = std::make_error_code(std::errc::connection_aborted);
            }
            SetError(ec, "ReceiveFile: Recv chunk '" + file_path + "'");
            file_stream.close();
            return false;
        }
        if (!file_stream.write(reinterpret_cast<char*>(buffer.data()), current_received))
        {
            ec = std::make_error_code(std::errc::io_error);
            SetError(ec, "ReceiveFile: Failed to write to file '" + file_path + "'");
            file_stream.close();
            return false;
        }
        total_received += current_received;
    }
    file_stream.close();
    return true;
}

void SocketConnection::Close()
{
    if (m_socket != kInvalidSocketValue)
    {
#ifdef _WIN32
        ::shutdown(static_cast<SOCKET>(m_socket), SD_BOTH);
        ::closesocket(static_cast<SOCKET>(m_socket));
#else
        ::shutdown(m_socket, SHUT_RDWR);
        ::close(m_socket);
#endif
        m_socket = kInvalidSocketValue;
        m_is_listening = false;
    }
}

bool SocketConnection::IsOpen() const
{
    return m_socket != kInvalidSocketValue;
}

std::string SocketConnection::GetLastErrorMsg()
{
    return m_last_error_msg.empty() ? "No error" : m_last_error_msg;
}

void SocketConnection::SetError(const std::error_code& ec, const std::string& context)
{
    if (ec)
    {
        m_last_error_msg = (!context.empty() ? context + ": " : "") + ec.message() +
                           " (OS:" + std::to_string(ec.value()) + ")";
    }
    else
    {
        m_last_error_msg.clear();
    }
}

void SocketConnection::SetPlatformError(const std::string& context)
{
    int err = 0;
#ifdef _WIN32
    err = WSAGetLastError();
#else
    err = errno;
#endif
    SetError(std::error_code(err, std::system_category()), context);
}

}  // namespace Network