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

#include "log.h"
#include <stdarg.h>
#include <iostream>
#include "common.h"
#if defined(WIN32)
#    include <windows.h>
#endif

namespace Dive
{

// =================================================================================================
// LogNull
// =================================================================================================
LogNull& LogNull::GetInstance()
{
    static LogNull log_null;
    return log_null;
}

// =================================================================================================
// LogConsole
// =================================================================================================
void LogConsole::Log(const LogEntry& entry)
{
    std::ostringstream oss;

    switch (entry.m_type)
    {
    case LogType::kInfo:
        oss << "INFO";
        break;
    case LogType::kWarning:
        oss << "WARNING";
        break;
    case LogType::kError:
        oss << "ERROR";
        break;
    default:
        DIVE_ASSERT(false);
    }
    switch (entry.m_cat)
    {
    case LogCategory::kParsing:
        oss << "(Parsing): ";
        break;
    case LogCategory::kPerformance:
        oss << "(Performance): ";
        break;
    default:
        DIVE_ASSERT(false);
    }

    oss << entry.m_file << "(" << entry.m_line << "): " << entry.m_short_desc;
    if (!entry.m_long_desc.empty())
        oss << " - " << entry.m_long_desc;
    oss << std::endl;

#if defined(WIN32)
    OutputDebugString(oss.str().c_str());
#else
    std::cout << oss.str();
#endif
}

// =================================================================================================
// LogRecord
// =================================================================================================
void LogRecord::Reset()
{
    m_log_entries.clear();
}

//--------------------------------------------------------------------------------------------------
void LogRecord::Log(const LogEntry& entry)
{
    m_log_entries.push_back(entry);
}

//--------------------------------------------------------------------------------------------------
size_t LogRecord::GetNumEntries() const
{
    return m_log_entries.size();
}

//--------------------------------------------------------------------------------------------------
const LogRecord::LogEntry& LogRecord::GetEntry(uint32_t index) const
{
    return m_log_entries[index];
}

// =================================================================================================
// DeferredLog
// =================================================================================================
void DeferredLog::LogEntriesTo(LogAssociation association, uint32_t id, ILog& other) const
{
    for (auto e : m_log_entries)
    {
        e.m_ref = CrossRef(association, id);
        other.Log(e);
    }
}

// =================================================================================================
// LogCompound
// =================================================================================================
void LogCompound::Reset()
{
    for (size_t i = 0; i < m_logs.size(); ++i)
    {
        m_logs[i]->Reset();
    }
}

//--------------------------------------------------------------------------------------------------
void LogCompound::AddLog(ILog* log_ptr)
{
    m_logs.push_back(log_ptr);
}

//--------------------------------------------------------------------------------------------------
void LogCompound::Log(const LogEntry& entry)
{
    for (size_t i = 0; i < m_logs.size(); ++i)
    {
        m_logs[i]->Log(entry);
    }
}

// =================================================================================================
// Stream manipulators
// =================================================================================================
std::ostream& detailed(std::ostream& out)
{
    return out;
}

// =================================================================================================
// LogEntryBuilder
// =================================================================================================

LogEntryBuilder::LogEntryBuilder(ILog&       log,
                                 LogType     type,
                                 LogCategory cat,
                                 CrossRef    ref,
                                 const char* file,
                                 int         line) :
    m_log(log)
{
    m_entry.m_type = type;
    m_entry.m_cat = cat;
    m_entry.m_ref = ref;
    m_entry.m_code = LogCode::kUnspecified;
    m_entry.m_file = file;
    m_entry.m_line = line;
}

LogEntryBuilder::LogEntryBuilder(ILog&          log,
                                 LogType        type,
                                 LogCategory    cat,
                                 LogAssociation association,
                                 uint32_t       id,
                                 const char*    file,
                                 int            line) :
    LogEntryBuilder(log, type, cat, CrossRef(association, id), file, line)
{
}

//--------------------------------------------------------------------------------------------------
LogEntryBuilder::~LogEntryBuilder()
{
    // If the "short desc" has already been filled, then what's left in the m_buf is the "long desc"
    if (m_entry.m_short_desc.empty())
        m_entry.m_short_desc = m_buf.str();
    else
        m_entry.m_long_desc = m_buf.str();
    m_log.Log(m_entry);
}

//--------------------------------------------------------------------------------------------------
// Logic for the various stream manipulators
LogEntryBuilder& LogEntryBuilder::operator<<(std::ostream& (*manip)(std::ostream&))
{
    if (manip == detailed)
    {
        // Switch to filling out m_long_desc after first flushing out to m_short_desc
        DIVE_ASSERT(m_entry.m_short_desc.empty());
        m_entry.m_short_desc = m_buf.str();
        m_buf = std::ostringstream();
    }
    m_buf << manip;
    return *this;
}

//--------------------------------------------------------------------------------------------------
// For the Code() manipulator
LogEntryBuilder& LogEntryBuilder::operator<<(const code& c)
{
    m_entry.m_code = c.GetCode();
    return *this;
}

}  // namespace Dive
