/*
Copyright 2026 Google Inc.

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

#include "dive/log/android_log_sink.h"

#include <android/log.h>

#include <string>

#include "absl/base/log_severity.h"
#include "absl/log/check.h"
#include "absl/log/globals.h"
#include "absl/log/log_sink.h"
#include "absl/status/status.h"

namespace Dive
{
namespace
{

int AndroidLogLevel(const absl::LogEntry& entry)
{
    switch (entry.log_severity())
    {
        case absl::LogSeverity::kFatal:
            return ANDROID_LOG_FATAL;
        case absl::LogSeverity::kError:
            return ANDROID_LOG_ERROR;
        case absl::LogSeverity::kWarning:
            return ANDROID_LOG_WARN;
        default:
            if (entry.verbosity() >= 2) return ANDROID_LOG_VERBOSE;
            if (entry.verbosity() == 1) return ANDROID_LOG_DEBUG;
            return ANDROID_LOG_INFO;
    }
}

}  // namespace

void AndroidLogSink::Send(const absl::LogEntry& entry)
{
    const int level = AndroidLogLevel(entry);
    __android_log_write(level, m_android_tag.c_str(),
                        entry.text_message_with_prefix_and_newline_c_str());
    if (entry.log_severity() == absl::LogSeverity::kFatal)
        __android_log_write(ANDROID_LOG_FATAL, m_android_tag.c_str(), "terminating.\n");
}

}  // namespace Dive
