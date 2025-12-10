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

#include "dive/common/status.h"

#include <string>

#include "absl/debugging/stacktrace.h"
#include "absl/debugging/symbolize.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/cord.h"
#include "absl/strings/str_format.h"

namespace Dive
{

constexpr int kMaxDepth = 64;
constexpr int kMaxSymbolLength = 1024;

std::string StackTraceString(int skip_count)
{
    std::array<void*, kMaxDepth> frames{};
    // skip_count + 1 to ignore this function's own frame
    int depth = absl::GetStackTrace(frames.data(), frames.size(), skip_count + 1);

    std::string                        stack;
    std::array<char, kMaxSymbolLength> buf{};
    for (int i = 0; i < depth; ++i)
    {
        const char* symbol = "(unknown)";
        if (absl::Symbolize(frames[i], buf.data(), buf.size()))
        {
            symbol = buf.data();
        }
        absl::StrAppendFormat(&stack, "@%p %s\n", frames[i], symbol);
    }
    return stack;
}

std::string GetStackTrace(const absl::Status& status)
{
    if (auto stack_trace = status.GetPayload(Dive::kStackTraceKey); stack_trace.has_value())
    {
        return absl::StrCat("Logical stack trace:\n", stack_trace.value());
    }
    return std::string("No stack trace found.");
}

}  // namespace Dive
