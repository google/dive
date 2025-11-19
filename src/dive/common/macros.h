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
#include <thread>

#include "absl/status/status.h"
#include "absl/status/statusor.h"

// StatusOr Macros simplified from protobuf/stubs/status_macros.h
#define RETURN_IF_ERROR(expr)               \
    do                                      \
    {                                       \
        const absl::Status status = (expr); \
        if (!status.ok())                   \
            return status;                  \
    } while (0)

template<typename T> absl::Status DoAssignOrReturn(T &lhs, absl::StatusOr<T> result)
{
    if (result.ok())
    {
        lhs = *result;
    }
    return result.status();
}

#define STATUS_MACROS_CONCAT_NAME_INNER(x, y) x##y
#define STATUS_MACROS_CONCAT_NAME(x, y) STATUS_MACROS_CONCAT_NAME_INNER(x, y)

#define ASSIGN_OR_RETURN_IMPL(status, lhs, rexpr)         \
    absl::Status status = DoAssignOrReturn(lhs, (rexpr)); \
    if (!status.ok())                                     \
        return status;

#define ASSIGN_OR_RETURN(lhs, rexpr) \
    ASSIGN_OR_RETURN_IMPL(STATUS_MACROS_CONCAT_NAME(_status_or_value, __COUNTER__), lhs, rexpr);
