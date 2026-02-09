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

#include "crashpad_client.h"

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "absl/base/no_destructor.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "base/files/file_path.h"
#include "client/crash_report_database.h"
#include "client/crashpad_client.h"
#include "client/settings.h"
#include "crashpad_utils.h"
#include "dive/os/command_utils.h"
#include "dive/utils/version_info.h"

namespace Dive
{

namespace
{

constexpr char kDbDirectory[] = "crash_database";
constexpr char kMetricsDirectory[] = "crash_metrics";
constexpr char kCrashReportUrl[] = "https://clients2.google.com/cr/report";
constexpr char kFormat[] = "minidump";
constexpr char kNoRateLimitFlag[] = "--no-rate-limit";
constexpr int kMaxCrashpadVersionLength = 30;

}  // namespace

absl::Status InitializeCrashpad()
{
    auto writable_root = GetWritableRoot();
    if (!writable_root.ok())
    {
        return writable_root.status();
    }

    std::filesystem::path database_path = *writable_root / kDbDirectory;
    std::filesystem::path metrics_path = *writable_root / kMetricsDirectory;

    std::error_code ec;
    std::filesystem::create_directories(database_path, ec);
    if (ec)
    {
        return absl::InternalError("Could not create DataBase directory: " + ec.message());
    }
    std::filesystem::create_directories(metrics_path, ec);
    if (ec)
    {
        return absl::InternalError("Could not create Metrics directory: " + ec.message());
    }

    // Crashpad requires explicit user consent or a programmatic override to upload
    // reports. We enable uploads here to ensure the client can transmit crash
    // data to the remote collection server.
    std::unique_ptr<crashpad::CrashReportDatabase> database =
        crashpad::CrashReportDatabase::Initialize(base::FilePath(database_path.native()));
    if (database && database->GetSettings())
    {
        database->GetSettings()->SetUploadsEnabled(true);
    }
    else
    {
        return absl::InternalError("Failed to initialize Crashpad database.");
    }

    std::string version = Dive::GetHostShortVersionString();
    if (version.size() > kMaxCrashpadVersionLength)
    {
        return absl::InternalError(absl::StrCat("Crashpad version string '", version, "' (",
                                                version.size(), ") is too long, max is ",
                                                kMaxCrashpadVersionLength));
    }

    std::map<std::string, std::string> annotations = {
        {"product", kProductName}, {"format", kFormat}, {"version", version}};

    std::vector<std::string> arguments = {kNoRateLimitFlag};

    absl::StatusOr<std::filesystem::path> exe_dir = Dive::GetExecutableDirectory();
    if (!exe_dir.ok())
    {
        return exe_dir.status();
    }
    std::filesystem::path handler_path = *exe_dir / GetHandlerBinaryName();
    if (!std::filesystem::exists(handler_path))
    {
        return absl::NotFoundError(
            absl::StrCat("Crashpad handler not found at: ", handler_path.string()));
    }

    static absl::NoDestructor<crashpad::CrashpadClient> client;

    bool success = client->StartHandler(
        base::FilePath(handler_path.native()), base::FilePath(database_path.native()),
        base::FilePath(metrics_path.native()), kCrashReportUrl, annotations, arguments,
        /*restartable=*/true,
        /*asynchronous_start=*/false);

    if (!success)
    {
        return absl::InternalError("Failed to start Crashpad handler.");
    }
    std::cout << "Crashpad initialized successfully." << std::endl;
    return absl::OkStatus();
}

}  // namespace Dive
