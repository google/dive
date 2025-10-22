/*
 Copyright 2025 Google LLC

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

#include <atomic>
#include <memory>

#pragma once

namespace Dive
{

// Usage:
//
// Async operations:
//     absl::Status LongOperation(const Dive::Context& context)
//     {
//         if (context.Cancelled())
//         {
//             return absl::Status(absl::StatusCode::kCancelled, "cancelled");
//         }
//         ...
//     }
// CLI:
//     LongOperation(Dive::Context::Background());
// UI:
//     Dive::SimpleContext op_context;
//     void Start() // qt slot
//     {
//         op_context = Dive::SimpleContext::Create();
//         run_async([context = op_context](){LongOperation(context)});
//     }
//     void Stop()
//     {
//         if(!op_context.IsNull())
//         {
//             op_context->Cancel();
//         }
//     }
class Context;

// Wrapper for context token.
// It's an alias to std::shared_ptr since we need to be able to hold the underlying
// token in multiple thread.
template<typename ContextT> class ContextHolder
{
    template<typename> friend class ContextHolder;
    using ImplType = std::shared_ptr<ContextT>;

public:
    using ContextType = ContextT;

    ContextHolder() = default;
    explicit ContextHolder(ImplType impl) :
        m_impl(impl)
    {
    }

    ContextHolder(const ContextHolder&) = default;
    ContextHolder(ContextHolder&&) = default;
    template<typename ContextT2>
    ContextHolder(const ContextHolder<ContextT2>& other) :
        m_impl(other.m_impl)
    {
    }
    template<typename ContextT2>
    ContextHolder(ContextHolder<ContextT2>&& other) :
        m_impl(std::move(other.m_impl))
    {
    }

    ContextHolder& operator=(const ContextHolder&) = default;
    ContextHolder& operator=(ContextHolder&&) = default;

    template<typename ContextT2> ContextHolder& operator=(const ContextHolder<ContextT2>& other)
    {
        m_impl = other.m_impl;
    }
    template<typename ContextT2> ContextHolder& operator=(ContextHolder<ContextT2>&& other)
    {
        m_impl = std::move(other.m_impl);
    }

    bool         IsNull() const { return m_impl == nullptr; }
    ContextType* Get() const { return m_impl.get(); }
    ContextType* operator->() const { return m_impl.get(); }
    ContextType& operator*() const { return *m_impl; }
    // Avoid possible confusion with !Cancelled()
    explicit operator bool() const = delete;

    bool Cancelled() const { return m_impl ? m_impl->Cancelled() : false; }

    template<typename... Args> static ContextHolder Create(Args&&... args)
    {
        return ContextHolder(ImplType(new ContextType(std::forward<Args>(args)...)));
    }

private:
    ImplType m_impl;
};

// ContextToken is the interface type for context implementation.
class ContextToken
{
public:
    virtual ~ContextToken() = default;

    ContextToken(const ContextToken&) = delete;
    ContextToken(ContextToken&&) = delete;
    ContextToken& operator=(const ContextToken&) = delete;
    ContextToken& operator=(ContextToken&&) = delete;

    virtual bool Cancelled() const { return false; }

protected:
    ContextToken() = default;
};

// A cancelable context implementation.
class SimpleContextToken : public ContextToken
{
public:
    bool Cancelled() const override { return m_cancelled.load(); }
    void Cancel() { m_cancelled.store(true); }

private:
    std::atomic_bool m_cancelled = false;
};

// Context to be passed to long operations.
// Note: Prefer const Dive::Context& since it does not copy shared_ptr.
class Context : public ContextHolder<ContextToken>
{
public:
    using ContextHolder::ContextHolder;
    static Context Background() { return Context(); }
};

using SimpleContext = ContextHolder<SimpleContextToken>;
}  // namespace Dive
