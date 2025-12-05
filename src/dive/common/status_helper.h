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

#include <string>
#include <string_view>

#include "absl/debugging/stacktrace.h"
#include "absl/debugging/symbolize.h"
#include "absl/status/status.h"
#include "absl/strings/cord.h"
#include "absl/strings/str_format.h"

namespace Dive
{

constexpr const char* kStackTraceKey = "dive.logical_stack_trace";
constexpr int         kMaxDepth = 64;
constexpr int         kMaxSymbolLength = 1024;

inline std::string StackTraceString(int skip_count = 0)
{
    void* frames[kMaxDepth];
    // skip_count + 1 to ignore this function's own frame
    int depth = absl::GetStackTrace(frames, kMaxDepth, skip_count + 1);

    std::string stack;
    char        buf[kMaxSymbolLength];
    for (int i = 0; i < depth; ++i)
    {
        const char* symbol = "(unknown)";
        if (absl::Symbolize(frames[i], buf, sizeof(buf)))
        {
            symbol = buf;
        }
        absl::StrAppendFormat(&stack, "@%p %s\n", frames[i], symbol);
    }
    return stack;
}

inline void AddStackTrace(absl::Status& status, int skip_count = 0)
{
    status.SetPayload(kStackTraceKey, absl::Cord(StackTraceString(skip_count + 1)));
}

inline absl::Status InvalidArgumentError(std::string_view message)
{
    absl::Status status(absl::StatusCode::kInvalidArgument, message);
    AddStackTrace(status, /*skip_count = */ 1);
    return status;
}

inline absl::Status NotFoundError(std::string_view message)
{
    absl::Status status(absl::StatusCode::kNotFound, message);
    AddStackTrace(status, /*skip_count = */ 1);
    return status;
}

inline absl::Status FailedPreconditionError(std::string_view message)
{
    absl::Status status(absl::StatusCode::kFailedPrecondition, message);
    AddStackTrace(status, /*skip_count = */ 1);
    return status;
}

inline absl::Status UnavailableError(std::string_view message)
{
    absl::Status status(absl::StatusCode::kUnavailable, message);
    AddStackTrace(status, /*skip_count = */ 1);
    return status;
}

inline absl::Status AlreadyExistsError(std::string_view message)
{
    absl::Status status(absl::StatusCode::kAlreadyExists, message);
    AddStackTrace(status, /*skip_count = */ 1);
    return status;
}

inline absl::Status AbortedError(std::string_view message)
{
    absl::Status status(absl::StatusCode::kAborted, message);
    AddStackTrace(status, /*skip_count = */ 1);
    return status;
}

inline absl::Status CancelledError(std::string_view message)
{
    absl::Status status(absl::StatusCode::kCancelled, message);
    AddStackTrace(status, /*skip_count = */ 1);
    return status;
}

inline absl::Status DataLossError(std::string_view message)
{
    absl::Status status(absl::StatusCode::kDataLoss, message);
    AddStackTrace(status, /*skip_count = */ 1);
    return status;
}

inline absl::Status DeadlineExceededError(std::string_view message)
{
    absl::Status status(absl::StatusCode::kDeadlineExceeded, message);
    AddStackTrace(status, /*skip_count = */ 1);
    return status;
}

inline absl::Status InternalError(std::string_view message)
{
    absl::Status status(absl::StatusCode::kInternal, message);
    AddStackTrace(status, /*skip_count = */ 1);
    return status;
}

inline absl::Status OutOfRangeError(std::string_view message)
{
    absl::Status status(absl::StatusCode::kOutOfRange, message);
    AddStackTrace(status, /*skip_count = */ 1);
    return status;
}

inline absl::Status PermissionDeniedError(std::string_view message)
{
    absl::Status status(absl::StatusCode::kPermissionDenied, message);
    AddStackTrace(status, /*skip_count = */ 1);
    return status;
}

inline absl::Status StatusWithContext(const absl::Status& status, std::string_view context)
{
    absl::Status new_status = absl::Status(status.code(),
                                           absl::StrCat(context, ": ", status.message()));
    new_status.SetPayload(kStackTraceKey, status.GetPayload(kStackTraceKey).value_or(absl::Cord()));
    return new_status;
}

}  // namespace Dive
