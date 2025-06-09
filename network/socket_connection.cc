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

#ifdef WIN32
#    define _WINSOCK_DEPRECATED_NO_WARNINGS
#    define NOMINMAX
#    include <ws2tcpip.h>
#    pragma comment(lib, "Ws2_32.lib")
using ssize_t = SSIZE_T;
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
#ifdef WIN32
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

absl::StatusOr<std::unique_ptr<SocketConnection>> SocketConnection::Create(
SocketType initial_socket_value)
{
    if (!NetworkInitializer::Instance().IsInitialized())
    {
        return absl::InternalError("Create: Network failed to initialize.");
    }
    return std::unique_ptr<SocketConnection>(new SocketConnection(initial_socket_value));
}

SocketConnection::~SocketConnection()
{
    Close();
}

SocketConnection::SocketConnection(SocketType initial_socket_value) :
    m_socket(initial_socket_value),
    m_is_listening(false),
    m_accept_timout_ms(kAcceptTimeout)
{
}

absl::Status SocketConnection::BindAndListenOnUnixDomain(const std::string& server_address)
{
#ifdef WIN32
    return absl::UnimplementedError(
    "BindAndListenOnUnixDomain: This POSIX server method is not supported/implemented on Windows.");
#else
    if (IsOpen())
    {
        Close();
    }
    m_socket = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_socket == kInvalidSocketValue)
    {
        return absl::InternalError(
        absl::StrCat("BindAndListenOnUnixDomain: socket() creation failed: ", strerror(errno)));
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
        auto status = absl::InternalError(
        absl::StrCat("BindAndListenOnUnixDomain: bind() failed: ", strerror(errno)));
        Close();
        return status;
    }
    ret = ::listen(m_socket, SOMAXCONN);
    if (ret < 0)
    {
        auto status = absl::InternalError(
        absl::StrCat("BindAndListenOnUnixDomain: listen() failed: ", strerror(errno)));
        Close();
        return status;
    }
    m_is_listening = true;
    return absl::OkStatus();
#endif
}

absl::StatusOr<std::unique_ptr<SocketConnection>> SocketConnection::Accept()
{
#ifdef WIN32
    return absl::UnimplementedError(
    "Accept: This POSIX server method is not supported/implemented on Windows.");
#else
    if (!IsOpen() || !m_is_listening)
    {
        return absl::FailedPreconditionError("Accept: Socket not created or not listening.");
    }
    pollfd pfd;
    pfd.fd = m_socket;
    pfd.events = POLLIN;
    pfd.revents = 0;
    int ret = poll(&pfd, 1, m_accept_timout_ms);
    if (ret < 0)
    {
        return absl::InternalError(absl::StrCat("Accept: poll() failed: ", strerror(errno)));
    }
    if (ret == 0)
    {
        return absl::DeadlineExceededError("Accept: Timeout waiting for connection.");
    }
    if (!(pfd.revents & POLLIN))
    {
        return absl::InternalError("Accept: poll() returned without POLLIN or known error event.");
    }

    SocketType new_socket = (SocketType)::accept(m_socket, nullptr, nullptr);
    if (new_socket == kInvalidSocketValue)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return absl::UnavailableError("Accept: accept() would block.");
        }
        return absl::InternalError(
        absl::StrCat("Accept: accept() system call failed: ", strerror(errno)));
    }
    return std::unique_ptr<SocketConnection>(new SocketConnection(new_socket));
#endif
}

absl::Status SocketConnection::Connect(const std::string& host, int port)
{
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
        return absl::UnavailableError(
        absl::StrCat("Connect: getaddrinfo failed: ", gai_strerror(ret)));
    }

    std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> si_guard(server_info, freeaddrinfo);
    absl::Status                                       last_attempt_status;
    for (addrinfo* p = server_info; p != nullptr; p = p->ai_next)
    {
        m_socket = (SocketType)::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (m_socket == kInvalidSocketValue)
        {
            last_attempt_status = absl::InternalError(
            absl::StrCat("Connect: socket() creation failed: ", strerror(errno)));
            continue;
        }
        if (::connect(m_socket, p->ai_addr, (socklen_t)p->ai_addrlen) == -1)
        {
            last_attempt_status = absl::UnavailableError(
            absl::StrCat("Connect: connect() system call failed: ", strerror(errno)));
            Close();
            continue;
        }
        last_attempt_status = absl::OkStatus();
        break;
    }
    if (!last_attempt_status.ok())
    {
        return last_attempt_status;
    }
    m_is_listening = false;
    return absl::OkStatus();
}

absl::Status SocketConnection::Send(const uint8_t* data, size_t size)
{
    if (!IsOpen() || m_is_listening)
    {
        return absl::FailedPreconditionError(
        "Send: Socket is invalid or operation not supported on a listening socket.");
    }
    if (size == 0)
    {
        return absl::OkStatus();
    }

    size_t total_sent = 0;
    while (total_sent < size)
    {
        ssize_t sent;
#ifdef WIN32
        sent = ::send(static_cast<SOCKET>(m_socket),
                      (const char*)data + total_sent,
                      (int)(size - total_sent),
                      0);
#else
        sent = ::send(m_socket, data + total_sent, size - total_sent, MSG_NOSIGNAL);
#endif
        if (sent == -1)
        {
            int e = 0;
#ifdef WIN32
            e = WSAGetLastError();
            if (e == WSAEWOULDBLOCK)
            {
                return absl::UnavailableError("Send: Operation would block.");
            }
            else if (e == WSAECONNRESET || e == WSAECONNABORTED || e == WSAESHUTDOWN)
            {
                Close();
                return absl::AbortedError("Send: Connection reset by peer.");
            }
            else
            {
                return absl::InternalError(
                absl::StrCat("Send: send() failed with WinSock error: ", e));
            }
#else
            e = errno;
            if (e == EAGAIN || e == EWOULDBLOCK)
            {
                return absl::UnavailableError("Send: Operation would block.");
            }
            else if (e == EPIPE || e == ECONNRESET)
            {
                Close();
                return absl::AbortedError("Send: Connection reset by peer (EPIPE/ECONNRESET).");
            }
            else
            {
                return absl::InternalError(absl::StrCat("Send: send() failed: ", strerror(e)));
            }
#endif
        }

        if (sent == 0)
        {
            return absl::AbortedError("Send: Peer has closed the connection.");
        }

        total_sent += static_cast<size_t>(sent);
    }

    return absl::OkStatus();
}

absl::StatusOr<size_t> SocketConnection::Recv(uint8_t* data, size_t size, int timeout_ms)
{
    if (!IsOpen() || m_is_listening)
    {
        return absl::FailedPreconditionError(
        "Recv: Socket is invalid or operation not supported on a listening socket.");
    }
    if (size == 0)
    {
        return 0;
    }

    size_t total_received = 0;
    while (total_received < size)
    {
        // Wait for data to be available using poll/select with timeout.
#ifdef WIN32
        TIMEVAL tv;
        fd_set  read_fds;
        FD_ZERO(&read_fds);
        FD_SET(static_cast<SOCKET>(m_socket), &read_fds);

        if (timeout_ms >= 0)
        {
            tv.tv_sec = timeout_ms / 1000;
            tv.tv_usec = (timeout_ms % 1000) * 1000;
        }
        int activity = select(0, &read_fds, nullptr, nullptr, (timeout_ms < 0) ? nullptr : &tv);
        if (activity == SOCKET_ERROR)
        {
            return absl::InternalError(
            absl::StrCat("Recv: select() failed with WinSock error: ", WSAGetLastError()));
        }
        if (activity == 0)
        {
            return absl::DeadlineExceededError("Recv: Timed out waiting for data.");
        }
#else
        struct pollfd pfd;
        pfd.fd = m_socket;
        pfd.events = POLLIN;
        pfd.revents = 0;

        int ret = poll(&pfd, 1, timeout_ms);
        if (ret < 0)
        {
            return absl::InternalError(absl::StrCat("Recv: poll() failed: ", strerror(errno)));
        }
        if (ret == 0)
        {
            return absl::DeadlineExceededError("Recv: Timeout waiting for data.");
        }
        if (!(pfd.revents & POLLIN))
        {
            return absl::InternalError("Recv: poll() returned an error event on the socket.");
        }
#endif

        // Data is available to perform the actual recv.
        ssize_t received;
#ifdef WIN32
        received = ::recv(static_cast<SOCKET>(m_socket),
                          reinterpret_cast<char*>(data),
                          static_cast<int>(size),
                          0);
#else
        received = ::recv(m_socket, data, size, 0);
#endif
        if (received == -1)
        {
#ifdef WIN32
            int wsa_err = WSAGetLastError();
            if (wsa_err == WSAEWOULDBLOCK)
            {
                return absl::UnavailableError("Recv: Operation would block.");
            }
            else if (wsa_err == WSAECONNRESET || wsa_err == WSAECONNABORTED ||
                     wsa_err == WSAESHUTDOWN)
            {
                Close();
                return absl::AbortedError("Recv: Connection reset by peer.");
            }
            else
            {
                return absl::InternalError(
                absl::StrCat("Recv: recv() failed with WinSock error: ", wsa_err));
            }
#else
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return absl::UnavailableError("Recv: Operation would block.");
            }
            else if (errno == ECONNRESET)
            {
                Close();
                return absl::AbortedError("Recv: Connection reset by peer.");
            }
            else
            {
                return absl::InternalError(
                absl::StrCat("Recv: recv() system call failed: ", strerror(errno)));
            }
#endif
        }
        if (received == 0)
        {
            return absl::OutOfRangeError("Recv: Connection gracefully closed by peer.");
        }

        total_received += static_cast<size_t>(received);
    }

    return total_received;
}

absl::Status SocketConnection::SendString(const std::string& s)
{
    // Include null terminator.
    const auto len = s.length() + 1;
    return Send(reinterpret_cast<const uint8_t*>(s.c_str()), len);
}

absl::StatusOr<std::string> SocketConnection::ReceiveString()
{
    std::string received_string;
    char        c = 0;
    while (true)
    {
        absl::StatusOr<size_t> ret = Recv(reinterpret_cast<uint8_t*>(&c), 1);
        if (!ret.ok())
        {
            if (absl::IsOutOfRange(ret.status()))
            {
                return absl::DataLossError(
                "Connection closed before a null terminator was received.");
            }
            return ret.status();
        }
        if (c == 0)
        {
            return received_string;
        }
        received_string.push_back(c);
    }
}

absl::Status SocketConnection::SendFile(const std::string& file_path)
{
    std::ifstream file_stream(file_path, std::ios::binary | std::ios::ate);
    if (!file_stream)
    {
        return absl::NotFoundError(absl::StrCat("SendFile: Failed to open file '", file_path, "'"));
    }
    std::streamsize file_size = file_stream.tellg();
    if (file_size < 0)
    {
        file_stream.close();
        return absl::InternalError(
        absl::StrCat("SendFile: Failed to determine size of file '", file_path, "'"));
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
            file_stream.close();
            return absl::InternalError(
            absl::StrCat("SendFile: Failed to read chunk from file '", file_path, "'"));
        }
        size_t current_read = static_cast<size_t>(file_stream.gcount());
        if (current_read == 0)
        {
            file_stream.close();
            return absl::DataLossError(absl::StrCat("SendFile: File size mismatch. Read 0 bytes "
                                                    "before reaching expected end of file '",
                                                    file_path,
                                                    "'"));
        }
        absl::Status ret = this->Send(reinterpret_cast<uint8_t*>(buffer.data()), current_read);
        if (!ret.ok())
        {
            return absl::Status(ret.code(),
                                absl::StrCat("SendFile: Failed to send chunk for file '",
                                             file_path,
                                             "': ",
                                             ret.message()));
        }
        total_sent += static_cast<std::streamsize>(current_read);
    }
    file_stream.close();
    return absl::OkStatus();
}

absl::Status SocketConnection::ReceiveFile(const std::string& file_path, size_t file_size)
{
    std::ofstream file_stream(file_path, std::ios::binary | std::ios::trunc);
    if (!file_stream)
    {
        return absl::PermissionDeniedError(
        absl::StrCat("ReceiveFile: Failed to open file '", file_path, "' for writing."));
    }
    const size_t         CHUNK_SIZE = 4096;
    std::vector<uint8_t> buffer(CHUNK_SIZE);
    size_t               total_received = 0;
    while (total_received < file_size)
    {
        size_t to_receive = std::min(CHUNK_SIZE, file_size - total_received);
        auto   ret = this->Recv(buffer.data(), to_receive);
        if (!ret.ok())
        {
            file_stream.close();
            return absl::Status(ret.status().code(),
                                absl::StrCat("ReceiveFile: Failed to receive chunk for '",
                                             file_path,
                                             "': ",
                                             ret.status().message()));
        }
        size_t current_received = ret.value();
        if (!file_stream.write(reinterpret_cast<char*>(buffer.data()), current_received))
        {
            file_stream.close();
            return absl::InternalError(
            absl::StrCat("ReceiveFile: Failed to write to file '", file_path, "'"));
        }
        total_received += current_received;
    }
    file_stream.close();
    return absl::OkStatus();
}

void SocketConnection::Close()
{
    if (m_socket != kInvalidSocketValue)
    {
#ifdef WIN32
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

}  // namespace Network