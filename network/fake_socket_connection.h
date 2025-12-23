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

class FakeSocketConnection : public ISocketConnection
{
public:
    FakeSocketConnection() {}

    ~FakeSocketConnection() override
    {
        Close();
        Unpair();
    }

    void PairWith(FakeSocketConnection* peer)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_peer = peer;
        if (peer)
        {
            std::lock_guard<std::mutex> peer_lock(peer->m_mutex);
            peer->m_peer = this;
        }
    }

    FakeSocketConnection* GetPeer() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_peer;
    }

    absl::Status BindAndListenOnUnixDomain(const std::string& addr) override
    {
        return absl::UnimplementedError("Not implemented for FakeSocketConnection.");
    }

    absl::StatusOr<std::unique_ptr<ISocketConnection>> Accept() override
    {
        return absl::UnimplementedError("Not implemented for FakeSocketConnection.");
    }

    absl::Status Connect(const std::string& host, int port) override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_peer && m_is_open)
        {
            return absl::OkStatus();
        }
        return absl::UnavailableError("Fake connection not paired or closed.");
    }

    absl::Status Send(const uint8_t* data, size_t size) override
    {
        FakeSocketConnection* peer_ptr = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_is_open || !m_peer)
            {
                return absl::AbortedError("Connection closed");
            }
            peer_ptr = m_peer;
        }

        {
            std::lock_guard<std::mutex> lock(peer_ptr->m_mutex);
            if (!peer_ptr->m_is_open)
            {
                return absl::AbortedError("Connection closed by peer");
            }
            peer_ptr->m_recv_buffer.insert(peer_ptr->m_recv_buffer.end(), data, data + size);
        }

        peer_ptr->m_cv.notify_one();
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

        if (!m_recv_buffer.empty())
        {
            size_t to_read = std::min(size, m_recv_buffer.size());
            std::copy_n(m_recv_buffer.begin(), to_read, data);
            m_recv_buffer.erase(m_recv_buffer.begin(), m_recv_buffer.begin() + to_read);
            return to_read;
        }

        if (!m_is_open)
        {
            return absl::OutOfRangeError("Connection closed");
        }

        return absl::DeadlineExceededError("Fake recv timeout");
    }

    void Close() override
    {
        FakeSocketConnection* peer_ptr = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            peer_ptr = m_peer;
        }

        // Lock both safely to avoid deadlock if both close at once.
        if (peer_ptr)
        {
            std::unique_lock<std::mutex> lock1(m_mutex, std::defer_lock);
            std::unique_lock<std::mutex> lock2(peer_ptr->m_mutex, std::defer_lock);
            std::lock(lock1, lock2);

            m_is_open = false;
            if (peer_ptr->m_peer == this)
            {
                peer_ptr->m_cv.notify_all();
            }
        }
        else
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_is_open = false;
        }
        m_cv.notify_all();
    }

    bool IsOpen() const override
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_is_open;
    }

private:
    void Unpair()
    {
        FakeSocketConnection* peer_ptr = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            peer_ptr = m_peer;
            m_peer = nullptr;
        }

        if (peer_ptr)
        {
            std::lock_guard<std::mutex> lock(peer_ptr->m_mutex);
            if (peer_ptr->m_peer == this)
            {
                peer_ptr->m_peer = nullptr;
            }
        }
    }

    FakeSocketConnection*   m_peer = nullptr;
    bool                    m_is_open = true;
    std::deque<uint8_t>     m_recv_buffer;
    mutable std::mutex      m_mutex;
    std::condition_variable m_cv;
};

}  // namespace Network