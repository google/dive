/*
Copyright 2024 Google Inc.

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

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

namespace Dive
{

class Task
{
public:
    using Function = std::function<void()>;

    Task() = default;

    // Disable copy and assign;
    Task(const Task& other) = delete;
    Task(const Function& function) = delete;
    Task& operator=(const Task& other) = delete;
    Task& operator=(const Function& f) = delete;

    Task(Task&& other) :
        m_func(std::move(other.m_func))
    {
    }
    Task(Function&& f) :
        m_func(std::move(f))
    {
    }

    Task& operator=(Task&& other)
    {
        m_func = std::move(other.m_func);
        return *this;
    }

    Task& operator=(Function&& f)
    {
        m_func = std::move(f);
        return *this;
    }

    // operator()() runs the task.
    void operator()() const { m_func(); }

private:
    Function m_func;
};

class TaskRunner
{
public:
    TaskRunner()
    {
        m_worker_thread = std::thread([this]() {
            while (!this->m_shutdown)
            {
                this->WaitForWork();
            }
        });
    }

    ~TaskRunner()
    {
        NotifyShutdown();
        if (m_worker_thread.joinable())
        {
            m_worker_thread.join();
        }
    }

    template<typename Function> inline void Schedule(Function&& f)
    {
        Enqueue(Task(std::forward<Function>(f)));
    }

private:
    void Enqueue(Task&& task)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_task_queue.push_back(std::move(task));
        m_cond.notify_one();
    }

    void NotifyShutdown()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_shutdown = true;
        m_cond.notify_one();
    }

    void WaitForWork()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait(lock, [&]() { return !m_task_queue.empty() || m_shutdown; });
        if (m_shutdown)
        {
            return;
        }
        auto work = std::move(m_task_queue.front());
        m_task_queue.pop_front();
        lock.unlock();

        work();
    }

    std::mutex              m_mutex;
    std::condition_variable m_cond;
    std::deque<Task>        m_task_queue;
    bool                    m_shutdown = false;
    std::thread             m_worker_thread;
};
}  // namespace Dive