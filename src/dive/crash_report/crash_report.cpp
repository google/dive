/*
 Copyright 2026 Google LLC

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

#include "crash_report.h"

#include "absl/log/log.h"
#include "absl_crash_handler.h"
#include "dive_crashpad/crashpad_client.h"

namespace Dive
{
namespace
{

absl::Status InitializeAbseilCrashReporting(const char* argv, const std::string& product_name)
{
    LOG(INFO) << "[Crash Report] Initializing Absl Crash Handler.";
    absl::InitializeSymbolizer(argv);
    AbslCrashHandler::Initialize(argv);

    absl::FailureSignalHandlerOptions options;
    options.writerfn = AbslCrashHandler::Writer;
    absl::InstallFailureSignalHandler(options);
    LOG(INFO) << "[Crash Report] Absl Crash Handler initialized successfully.";
    return absl::OkStatus();
}

}  // namespace

absl::Status InitializeCrashReporting(const char* argv, const std::string& product_name)
{
    auto status = InitializeCrashpad(product_name);
    if (!absl::IsUnavailable(status))
    {
        return status;
    }
    return InitializeAbseilCrashReporting(argv, product_name);
}

}  // namespace Dive
