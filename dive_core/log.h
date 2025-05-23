/*
 Copyright 2021 Google LLC

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
#include <stdint.h>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "cross_ref.h"

namespace Dive
{

//--------------------------------------------------------------------------------------------------
enum class LogType
{
    kInfo,
    kWarning,
    kError
};

//--------------------------------------------------------------------------------------------------
enum class LogCategory
{
    kGeneric,
    kParsing,
    kPerformance,
};

//--------------------------------------------------------------------------------------------------
typedef CrossRefType LogAssociation;

//--------------------------------------------------------------------------------------------------
// For specific warnings/errors, to help with filtering in the future.
// Please follow a numering scheme. For example, all barriers can be of the form 0xC###.
enum class LogCode : uint16_t
{
    kUnspecified = 0,
    kMergeBarriers = 0xC001,
    kDrawIssues = 0xD001,
    kClearIssues = 0xD002,
};

//--------------------------------------------------------------------------------------------------
class ILog
{
public:
    struct LogEntry
    {
        LogType     m_type;
        LogCategory m_cat;
        LogCode     m_code;
        CrossRef    m_ref;
        const char* m_file;
        int         m_line;
        std::string m_short_desc;
        std::string m_long_desc;
    };
    virtual void Reset() {}
    virtual void Log(const LogEntry& entry) = 0;
};

//--------------------------------------------------------------------------------------------------
// Dummy log (if no logging desired)
class LogNull : public ILog
{
public:
    virtual void    Log(const LogEntry& entry) override {}
    static LogNull& GetInstance();
};

//--------------------------------------------------------------------------------------------------
// Log to the console
class LogConsole : public ILog
{
public:
    virtual void Log(const LogEntry& entry) override;
};

//--------------------------------------------------------------------------------------------------
// Log to internal record for later access
class LogRecord : public ILog
{
public:
    virtual void Reset() override;
    virtual void Log(const LogEntry& entry) override;

    size_t          GetNumEntries() const;
    const LogEntry& GetEntry(uint32_t index) const;

protected:
    std::vector<LogEntry> m_log_entries;
};

class DeferredLog : public LogRecord
{
public:
    void LogEntriesTo(LogAssociation association, uint32_t id, ILog& other) const;
};

//--------------------------------------------------------------------------------------------------
// A log that outputs to multiple logging streams
class LogCompound : public ILog
{
public:
    virtual void Reset() override;
    void         AddLog(ILog* log_ptr);
    virtual void Log(const LogEntry& entry) override;

private:
    std::vector<ILog*> m_logs;
};

//--------------------------------------------------------------------------------------------------
// Various stream manipulators used with LogEntryBuilder

// Used to indicate string following this are for the "detailed" descriptor
std::ostream& detailed(std::ostream& out);

// Used to indicate the code for this log entry
class code
{
public:
    code(LogCode c) :
        m_code(c)
    {
    }
    LogCode GetCode() const { return m_code; }

private:
    LogCode m_code;
};

//--------------------------------------------------------------------------------------------------
// Builder allows << operators to be used to construct a log entry
class LogEntryBuilder
{
public:
    LogEntryBuilder(ILog&       log,
                    LogType     type,
                    LogCategory cat,
                    CrossRef    ref,
                    const char* file,
                    int         line);
    LogEntryBuilder(ILog&          log,
                    LogType        type,
                    LogCategory    cat,
                    LogAssociation association,
                    uint32_t       id,  // Ignored if (association == kNone)
                    const char*    file,
                    int            line);
    ~LogEntryBuilder();

    template<typename T> LogEntryBuilder& operator<<(const T& val)
    {
        m_buf << val;
        return *this;
    }

    LogEntryBuilder& operator<<(std::ostream& (*manip)(std::ostream&));
    LogEntryBuilder& operator<<(const code& c);

private:
    ILog&              m_log;
    ILog::LogEntry     m_entry;
    std::ostringstream m_buf;
};

class LogEntryBuilderNull
{
public:
    template<typename T> LogEntryBuilderNull& operator<<(const T& val) { return *this; }
    inline LogEntryBuilderNull& operator<<(std::ostream& (*manip)(std::ostream&)) { return *this; }
    inline LogEntryBuilderNull& operator<<(const code& c) { return *this; }
};

//--------------------------------------------------------------------------------------------------
// Macros
#ifndef NDEBUG

// The "internal" warnings/errors are compiled out in RELEASE
#    define LOG_INTERNAL_WARNING(log_stream, cat) \
        LogEntryBuilder(log_stream,               \
                        LogType::kWarning,        \
                        cat,                      \
                        LogAssociation::kNone,    \
                        UINT32_MAX,               \
                        __FILE__,                 \
                        __LINE__)

#    define LOG_INTERNAL_ERROR(log_stream, cat) \
        LogEntryBuilder(log_stream,             \
                        LogType::kError,        \
                        cat,                    \
                        LogAssociation::kNone,  \
                        UINT32_MAX,             \
                        __FILE__,               \
                        __LINE__)
#else

#    define LOG_INTERNAL_WARNING(...) LogEntryBuilderNull()
#    define LOG_INTERNAL_ERROR(...) LogEntryBuilderNull()

#endif

// The "Public" warnings/errors are for those that need to be visible to users
#define LOG_PUBLIC_INFO(log_stream, cat)               \
    Dive::LogEntryBuilder(log_stream,                  \
                          Dive::LogType::kInfo,        \
                          cat,                         \
                          Dive::LogAssociation::kNone, \
                          UINT32_MAX,                  \
                          __FILE__,                    \
                          __LINE__)

#define LOG_PUBLIC_WARNING(log_stream, cat) \
    LogEntryBuilder(log_stream,             \
                    LogType::kWarning,      \
                    cat,                    \
                    LogAssociation::kNone,  \
                    UINT32_MAX,             \
                    __FILE__,               \
                    __LINE__)

#define LOG_PUBLIC_ERROR(log_stream, cat)  \
    LogEntryBuilder(log_stream,            \
                    LogType::kError,       \
                    cat,                   \
                    LogAssociation::kNone, \
                    UINT32_MAX,            \
                    __FILE__,              \
                    __LINE__)

// Warnings/Errors associated with a specific Event
#define LOG_PUBLIC_EVENT_WARNING(log_stream, cat, id) \
    LogEntryBuilder(log_stream,                       \
                    LogType::kWarning,                \
                    cat,                              \
                    LogAssociation::kEvent,           \
                    id,                               \
                    __FILE__,                         \
                    __LINE__)
#define LOG_PUBLIC_EVENT_ERROR(log_stream, cat, id) \
    LogEntryBuilder(log_stream,                     \
                    LogType::kError,                \
                    cat,                            \
                    LogAssociation::kEvent,         \
                    id,                             \
                    __FILE__,                       \
                    __LINE__)

// Warnings/Errors associated with a specific Barrier
#define LOG_PUBLIC_BARRIER_WARNING(log_stream, cat, id) \
    LogEntryBuilder(log_stream,                         \
                    LogType::kWarning,                  \
                    cat,                                \
                    LogAssociation::kBarrier,           \
                    id,                                 \
                    __FILE__,                           \
                    __LINE__)
#define LOG_PUBLIC_BARRIER_ERROR(log_stream, cat, id) \
    LogEntryBuilder(log_stream,                       \
                    LogType::kError,                  \
                    cat,                              \
                    LogAssociation::kBarrier,         \
                    id,                               \
                    __FILE__,                         \
                    __LINE__)

#define LOG_GFR_WARNING(log_stream, cat, id)                 \
    LogEntryBuilder(log_stream,                              \
                    LogType::kWarning,                       \
                    cat,                                     \
                    CrossRef(LogAssociation::kGFRIndex, id), \
                    __FILE__,                                \
                    __LINE__)

}  // namespace Dive
