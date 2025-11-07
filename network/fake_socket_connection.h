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

#include <deque>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <algorithm>

#include "socket_connection.h"

namespace Network
{

class FakeSocketConnection : public SocketConnection
{
public:
    FakeSocketConnection() :
        SocketConnection(kInvalidSocketValue)
    {
    }

    void PairWith(FakeSocketConnection* peer)
    {
        m_peer = peer;
        peer->m_peer = this;
        m_is_open = true;
        peer->m_is_open = true;
    }

    FakeSocketConnection* GetPeer() const { return m_peer; }

    absl::Status Connect(const std::string& host, int port) override
    {
        if (m_peer && m_is_open)
        {
            return absl::OkStatus();
        }
        return absl::UnavailableError("Fake connection not paired.");
    }

    absl::Status Send(const uint8_t* data, size_t size) override
    {
        if (!m_is_open || !m_peer)
        {
            return absl::AbortedError("Connection closed");
        }
        {
            std::lock_guard<std::mutex> lock(m_peer->m_mutex);
            m_peer->m_recv_buffer.insert(m_peer->m_recv_buffer.end(), data, data + size);
        }
        m_peer->m_cv.notify_one();
        return absl::OkStatus();
    }

    absl::StatusOr<size_t> Recv(uint8_t* data, size_t size, int timeout_ms) override
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        bool                         data_available = m_cv.wait_for(lock,
                                            std::chrono::milliseconds(
                                            timeout_ms == kNoTimeout ? 5000 : timeout_ms),
                                            [this]() {
                                                return !m_recv_buffer.empty() || !m_is_open;
                                            });

        if (!m_is_open && m_recv_buffer.empty())
        {
            return absl::OutOfRangeError("Connection closed by peer");
        }
        if (!data_available && m_recv_buffer.empty())
        {
            return absl::DeadlineExceededError("Fake recv timeout");
        }

        size_t to_read = std::min(size, m_recv_buffer.size());
        std::copy_n(m_recv_buffer.begin(), to_read, data);
        m_recv_buffer.erase(m_recv_buffer.begin(), m_recv_buffer.begin() + to_read);
        return to_read;
    }

    void Close() override
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_is_open = false;
        }
        m_cv.notify_all();
        if (m_peer)
        {
            std::lock_guard<std::mutex> peer_lock(m_peer->m_mutex);
            m_peer->m_is_open = false;
            m_peer->m_cv.notify_all();
        }
    }

    bool IsOpen() const override { return m_is_open; }

private:
    FakeSocketConnection*   m_peer = nullptr;
    bool                    m_is_open = false;
    std::deque<uint8_t>     m_recv_buffer;
    std::mutex              m_mutex;
    std::condition_variable m_cv;
};

}  // namespace Network