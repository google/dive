/*
Copyright 2023 Google Inc.

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

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <future>
#include <iostream>
#include <ostream>
#include <string>
#include <system_error>
#include <thread>

#include "absl/base/no_destructor.h"
#include "absl/cleanup/cleanup.h"
#include "absl/flags/flag.h"
#include "absl/flags/internal/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/flags/usage_config.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "android_application.h"
#include "constants.h"
#include "device_mgr.h"
#include "dive/common/app_types.h"
#include "dive/common/macros.h"
#include "dive/common/status.h"
#include "dive/os/command_utils.h"
#include "network/tcp_client.h"
#include "utils/version_info.h"

namespace
{

using ::Dive::AdbSession;
using ::Dive::AndroidDevice;
using ::Dive::DeviceInfo;
using ::Dive::DeviceManager;

struct GlobalOptions
{
    std::string serial;
    std::string package;
    std::string vulkan_command;
    std::string vulkan_command_args;
    Dive::AppType app_type;
    std::string download_dir;
    std::string gfxr_capture_file_dir;
    int trigger_capture_after;

    Dive::GfxrReplaySettings replay_settings;
};

struct CommandContext
{
    Dive::DeviceManager& mgr;
    const GlobalOptions& options;
};

enum class Command
{
    kNone,
    kListDevice,
    kListPackage,
    kRunPackage,
    kPm4Capture,
    kGfxrCapture,
    kGfxrReplay,
    kCleanup,
};

using ValidatorFunc = absl::Status (*)(const GlobalOptions&);
using ExecutorFunc = absl::Status (*)(const CommandContext&);

// Command definition.
struct CommandDef
{
    Command cmd;
    const char* name;
    const char* description;
    absl::Status (*validator)(const GlobalOptions&);
    absl::Status (*executor)(const CommandContext&);
};

// Forward declaration of command executors.
absl::Status CmdListDevice(const CommandContext& ctx);
absl::Status CmdListPackage(const CommandContext& ctx);
absl::Status CmdRunPackage(const CommandContext& ctx);
absl::Status CmdPm4Capture(const CommandContext& ctx);
absl::Status CmdGfxrCapture(const CommandContext& ctx);
absl::Status CmdGfxrReplay(const CommandContext& ctx);
absl::Status CmdCleanup(const CommandContext& ctx);

absl::Status ValidateAlwaysOk(const GlobalOptions&) { return absl::OkStatus(); }

absl::Status ValidateCleanup(const GlobalOptions& o)
{
    return o.package.empty() ? Dive::InvalidArgumentError("Missing --package") : absl::OkStatus();
}

// Helper to validate options common to run/capture commands.
absl::Status ValidateRunOptions(const GlobalOptions& options)
{
    if (options.package.empty() && options.vulkan_command.empty())
    {
        return Dive::InvalidArgumentError("Missing required flag: --package or --vulkan_command");
    }

    if (!options.package.empty() && !options.vulkan_command.empty())
    {
        return Dive::InvalidArgumentError("Cannot use both --package and --vulkan_command");
    }

    if (!options.vulkan_command.empty() && options.app_type != Dive::AppType::kVulkanCLI_Non_OpenXR)
    {
        return Dive::InvalidArgumentError(
            "Cannot use --vulkan_command with --type other than vulkan_cli_non_openxr");
    }

    if (options.app_type == Dive::AppType::kGLES_OpenXR && options.vulkan_command == "gfxr_capture")
    {
        return Dive::InvalidArgumentError(
            "GFXR capture is not supported for GLES OpenXR applications.");
    }

    return absl::OkStatus();
}

// Helper to validate options for GFXR replay.
absl::Status ValidateGfxrReplayOptions(const GlobalOptions& options)
{
    if (options.replay_settings.remote_capture_path.empty())
    {
        return Dive::InvalidArgumentError("Missing required flag: --gfxr_replay_file_path");
    }
    if (!absl::EndsWith(options.replay_settings.remote_capture_path, ".gfxr"))
    {
        return Dive::InvalidArgumentError(absl::StrCat("Invalid --gfxr_replay_file_path '",
                                                       options.replay_settings.remote_capture_path,
                                                       "'. File must have a .gfxr extension."));
    }
    return absl::OkStatus();
}

constexpr std::array<CommandDef, 7> kCommandDefs = {{
    {Command::kListDevice, "list_device", "List connected Android devices.", ValidateAlwaysOk,
     CmdListDevice},
    {Command::kListPackage, "list_package", "List installable packages on the selected device.",
     ValidateAlwaysOk, CmdListPackage},
    {Command::kRunPackage, "run", "Run an app for manual testing or external capture.",
     ValidateRunOptions, CmdRunPackage},
    {Command::kPm4Capture, "pm4_capture", "Run an app and trigger a PM4 capture after a delay.",
     ValidateRunOptions, CmdPm4Capture},
    {Command::kGfxrCapture, "gfxr_capture", "Run an app and enable GFXR capture via key-press.",
     ValidateRunOptions, CmdGfxrCapture},
    {Command::kGfxrReplay, "gfxr_replay", "Deploy and run a GFXR replay.",
     ValidateGfxrReplayOptions, CmdGfxrReplay},
    {Command::kCleanup, "cleanup", "Clean up app-specific settings on the device.", ValidateCleanup,
     CmdCleanup},
}};

// Generates the help string dynamically for ABSL_FLAG.
std::string GenerateCommandFlagHelp()
{
    std::string usage = "Available commands:\n";
    for (const auto& def : kCommandDefs)
    {
        absl::StrAppend(&usage, absl::StrFormat("\t%-25s : %s\n", def.name, def.description));
    }
    return usage;
}

// Generates the help string dynamically for ABSL_FLAG.
std::string GenerateAppTypeFlagHelp()
{
    std::string usage = "Available application types:\n";
    for (const auto& info : Dive::kAppTypeInfos)
    {
        absl::StrAppend(&usage, absl::StrFormat("\t%-25s : %s\n", info.cli_name, info.description));
    }
    return usage;
}

// Waits for user input before exiting.
absl::Status WaitForExitConfirmation()
{
    std::cout << "Press any key+enter to exit" << std::endl;
    std::string input;
    if (std::getline(std::cin, input))
    {
        std::cout << "Exiting..." << std::endl;
    }
    return absl::OkStatus();
}

// Makes a human-readable string representation of the list of devices. Assumes `devices` is not
// empty.
std::string GetPrintableDeviceList(const std::vector<DeviceInfo>& devices)
{
    return absl::StrCat("Available devices:\n\t",
                        absl::StrJoin(devices.cbegin(), devices.cend(), "\n\t",
                                      [](std::string* out, const DeviceInfo& info) {
                                          absl::StrAppend(out, info.m_serial);
                                      }));
}

// Picks a suitable serial from a list of devices. Assumes `devices` is not empty.
absl::StatusOr<std::string> AutoSelectSerial(const std::vector<DeviceInfo>& devices)
{
    if (devices.size() == 1)
    {
        const std::string& serial = devices.front().m_serial;
        std::cout << "Using single connected device: " << serial << std::endl;
        return serial;
    }

    return Dive::InvalidArgumentError(
        absl::StrCat("Multiple devices connected. Specify --device [serial].\n",
                     GetPrintableDeviceList(devices)));
}

// Returns a valid serial based on what the user provided in `--device serial`.  Assumes `devices`
// is not empty.
absl::StatusOr<std::string> ValidateSerial(const std::vector<DeviceInfo>& devices,
                                           std::string_view serial)
{
    if (serial.empty())
    {
        return AutoSelectSerial(devices);
    }

    if (std::none_of(devices.cbegin(), devices.cend(),
                     [&serial](const DeviceInfo& info) { return info.m_serial == serial; }))
    {
        return Dive::InvalidArgumentError(absl::StrCat(
            "Device with serial '", serial, "' not found.\n", GetPrintableDeviceList(devices)));
    }

    return std::string(serial);
}

// Selects and sets up the target device based on the serial flag. Assumes `serial` is a connected
// device.
absl::Status InitializeDevice(Dive::DeviceManager& mgr, const std::string& serial)
{
    auto device = mgr.SelectDevice(serial);
    if (!device.ok())
    {
        return device.status();
    }

    auto ret = (*device)->SetupDevice();
    if (!ret.ok())
    {
        return Dive::StatusWithContext(ret, "Failed to setup device");
    }
    return absl::OkStatus();
}

// Internal helper to run a package on the device.
absl::Status InternalRunPackage(const CommandContext& ctx, bool enable_gfxr)
{
    auto* device = ctx.mgr.GetDevice();
    if (device == nullptr)
    {
        return Dive::FailedPreconditionError(
            "No device selected. Did you provide --device serial?");
    }
    device->EnableGfxr(enable_gfxr);

    absl::Status ret;

    switch (ctx.options.app_type)
    {
        case Dive::AppType::kVulkan_OpenXR:
            ret = device->SetupApp(ctx.options.package, Dive::ApplicationType::OPENXR_APK,
                                   ctx.options.vulkan_command_args,
                                   ctx.options.gfxr_capture_file_dir);
            break;
        case Dive::AppType::kVulkan_Non_OpenXR:
            ret = device->SetupApp(ctx.options.package, Dive::ApplicationType::VULKAN_APK,
                                   ctx.options.vulkan_command_args,
                                   ctx.options.gfxr_capture_file_dir);
            break;
        case Dive::AppType::kVulkanCLI_Non_OpenXR:
            ret = device->SetupApp(ctx.options.vulkan_command, ctx.options.vulkan_command_args,
                                   Dive::ApplicationType::VULKAN_CLI,
                                   ctx.options.gfxr_capture_file_dir);
            break;
        case Dive::AppType::kGLES_OpenXR:
            ret = device->SetupApp(ctx.options.package, Dive::ApplicationType::OPENXR_APK,
                                   ctx.options.vulkan_command_args,
                                   ctx.options.gfxr_capture_file_dir);
            break;
        case Dive::AppType::kGLES_Non_OpenXR:
            ret = device->SetupApp(ctx.options.package, Dive::ApplicationType::GLES_APK,
                                   ctx.options.vulkan_command_args,
                                   ctx.options.gfxr_capture_file_dir);
            break;
        default:
            return Dive::InvalidArgumentError("Unknown application type.");
    }

    if (!ret.ok())
    {
        return Dive::StatusWithContext(ret, "Setup failed");
    }

    ret = device->StartApp();
    if (!ret.ok())
    {
        return Dive::StatusWithContext(ret, "Start app failed");
    }
    return absl::OkStatus();
}

// Triggers a capture on the device and downloads the resulting file.
absl::Status TriggerPm4Capture(Dive::DeviceManager& mgr, const std::string& download_dir)
{
    if (mgr.GetDevice() == nullptr)
    {
        return Dive::FailedPreconditionError("No device selected, can't capture.");
    }

    Network::TcpClient client;
    const std::string host = "127.0.0.1";
    int port = mgr.GetDevice()->Port();

    absl::Status status = client.Connect(host, port);
    if (!status.ok())
    {
        return Dive::StatusWithContext(status, "Connection failed");
    }

    absl::StatusOr<std::string> capture_file_path = client.StartPm4Capture();
    if (!capture_file_path.ok())
    {
        return Dive::StatusWithContext(capture_file_path.status(), "StartPm4Capture failed");
    }

    std::filesystem::path target_download_dir(download_dir);
    if (!std::filesystem::is_directory(target_download_dir))
    {
        return Dive::InvalidArgumentError("Invalid download directory: " +
                                          target_download_dir.string());
    }

    std::filesystem::path p(*capture_file_path);
    std::string download_file_path = (target_download_dir / p.filename()).string();

    status = client.DownloadFileFromServer(*capture_file_path, download_file_path);
    if (!status.ok())
    {
        return status;
    }

    std::cout << "Capture saved at " << download_file_path << std::endl;
    return absl::OkStatus();
}

// Checks if the capture directory on the device is currently done.
bool IsCaptureFinished(const AdbSession& adb, const std::string& gfxr_capture_directory)
{
    std::string on_device_capture_directory =
        absl::StrCat(Dive::kDeviceCapturePath, "/", gfxr_capture_directory);
    std::string command = "shell lsof " + on_device_capture_directory;
    absl::StatusOr<std::string> output = adb.RunAndGetResult(command);

    if (!output.ok())
    {
        std::cout << "Error checking directory: " << output.status().message() << std::endl;
    }

    std::stringstream ss(output->c_str());
    std::string line;
    int line_count = 0;
    while (std::getline(ss, line))
    {
        line_count++;
    }

    return line_count <= 1;
}

// Renames the screenshot file locally to match the GFXR capture file name.
absl::Status RenameScreenshotFile(const std::filesystem::path& full_target_download_dir,
                                  const std::filesystem::path& gfxr_capture_file_name)
{
    const std::filesystem::path old_screenshot_file_path =
        full_target_download_dir / Dive::kCaptureScreenshotFile;

    // Ensure the file to rename actually exists.
    if (!std::filesystem::exists(old_screenshot_file_path))
    {
        return Dive::NotFoundError(absl::StrCat("Could not find the expected screenshot file: ",
                                                old_screenshot_file_path.string()));
    }

    // Derive the base name from the GFXR file.
    std::string base_name = gfxr_capture_file_name.stem().string();

    // Define the new, final path of the screenshot.
    const std::filesystem::path new_screenshot_file_path =
        full_target_download_dir / absl::StrCat(base_name, ".png");

    std::cout << "Renaming screenshot from " << old_screenshot_file_path.string() << " to "
              << new_screenshot_file_path.string() << std::endl;

    try
    {
        // Avoid renaming if the names are accidentally the same
        if (old_screenshot_file_path != new_screenshot_file_path)
        {
            std::filesystem::rename(old_screenshot_file_path, new_screenshot_file_path);
        }
    }
    catch (const std::exception& e)
    {
        return Dive::InternalError("Failed to rename screenshot file locally: " +
                                   std::string(e.what()));
    }

    return absl::OkStatus();
}

// Retrieves the GFXR capture file name from a list of files in a directory.
absl::StatusOr<std::filesystem::path> GetGfxrCaptureFileName(
    const std::filesystem::path& full_target_download_dir,
    const std::vector<std::string>& file_list)
{
    for (const std::string& filename : file_list)
    {
        std::string trimmed_filename(absl::StripAsciiWhitespace(filename));
        if (absl::EndsWith(trimmed_filename, ".gfxr"))
        {
            return full_target_download_dir / trimmed_filename;
        }
    }
    return Dive::NotFoundError("No file with '.gfxr' extension found in the list.");
}

// Retrieves a GFXR capture from the device and downloads it.
absl::Status RetrieveGfxrCapture(const AdbSession& adb, const GlobalOptions& options)
{
    const std::string& gfxr_capture_directory = options.gfxr_capture_file_dir;
    std::filesystem::path download_dir = options.download_dir;

    // Need to explicitly use forward slash so that this works on Windows targetting Android
    std::string on_device_capture_directory =
        absl::StrCat(Dive::kDeviceCapturePath, "/", gfxr_capture_directory);

    std::cout << "Retrieving capture..." << std::endl;

    // Retrieve the list of files in the capture directory on the device.
    std::string command = absl::StrFormat("shell ls %s", on_device_capture_directory);
    absl::StatusOr<std::string> output = adb.RunAndGetResult(command);
    if (!output.ok())
    {
        return Dive::InternalError("Error getting capture_file name: " +
                                   std::string(output.status().message()));
    }

    std::vector<std::string> file_list =
        absl::StrSplit(std::string(output->data()), '\n', absl::SkipEmpty());

    if (file_list.empty())
    {
        return Dive::NotFoundError("Error, captures not present on device at: " +
                                   on_device_capture_directory);
    }

    // Find name for new local target directory
    std::filesystem::path full_target_download_dir = download_dir / gfxr_capture_directory;
    bool local_target_dir_exists = std::filesystem::exists(full_target_download_dir);
    int suffix = 0;
    while (local_target_dir_exists)
    {
        // Append numerical suffix to make a fresh dir
        full_target_download_dir =
            download_dir / absl::StrFormat("%s_%s", gfxr_capture_directory, std::to_string(suffix));
        suffix++;
        local_target_dir_exists = std::filesystem::exists(full_target_download_dir);
    }

    command = absl::StrFormat(R"(pull "%s" "%s")", on_device_capture_directory,
                              full_target_download_dir.string());
    output = adb.RunAndGetResult(command);
    if (!output.ok())
    {
        return Dive::InternalError("Error pulling files: " +
                                   std::string(output.status().message()));
    }

    auto gfxr_capture_file = GetGfxrCaptureFileName(full_target_download_dir, file_list);

    if (!gfxr_capture_file.ok())
    {
        return gfxr_capture_file.status();
    }

    if (absl::Status ret = RenameScreenshotFile(full_target_download_dir, *gfxr_capture_file);
        !ret.ok())
    {
        std::cout << "Warning: Error renaming screenshot: " << ret.message() << std::endl;
    }

    std::cout << "Capture sucessfully saved at " << full_target_download_dir << std::endl;
    return absl::OkStatus();
}

// Requests from TriggerGfxrCapture that the calling code should honor when determining if multiple
// captures should be taken.
enum class GfxrCaptureControlFlow
{
    // Don't take any more GFXR captures.
    kStop,
    // Proceed to take another GFXR capture.
    kContinue,
};

// Triggers a GFXR capture and screenshot on the device.
absl::StatusOr<GfxrCaptureControlFlow> TriggerGfxrCapture(const AndroidDevice& device,
                                                          const GlobalOptions& options)
{
    const AdbSession& adb = device.Adb();
    const std::string& gfxr_capture_file_dir = options.gfxr_capture_file_dir;

    // GFXR relies on capture_android_trigger changing from false to true as the condition for
    // beginning the capture. Thus, ensure the prop starts as false.
    if (absl::Status status = adb.Run("shell setprop debug.gfxrecon.capture_android_trigger false");
        !status.ok())
    {
        return Dive::InternalError(
            absl::StrCat("Error stopping gfxr runtime capture: ", status.message()));
    }

    // Ask the user what to do.
    std::cout << "Press key g+enter to trigger a capture. Press any other key+enter to stop the "
                 "application.\n";
    std::string input;
    while (std::getline(std::cin, input))
    {
        if (input == "g")
        {
            break;
        }

        std::cout << "Exiting...\n";
        return GfxrCaptureControlFlow::kStop;
    }
    if (!std::cin.good())
    {
        return GfxrCaptureControlFlow::kStop;
    }

    // Trigger capture
    if (absl::Status status = adb.Run("shell setprop debug.gfxrecon.capture_android_trigger true");
        !status.ok())
    {
        return Dive::InternalError(
            absl::StrCat("Error starting gfxr runtime capture: ", status.message()));
    }

    if (absl::Status status = device.TriggerScreenCapture(gfxr_capture_file_dir); !status.ok())
    {
        return Dive::InternalError(
            absl::StrCat("Error creating capture screenshot: ", status.message()));
    }

    // Ask the user what to do.
    std::cout << "Press key g+enter to retrieve the capture.\n";
    while (std::getline(std::cin, input))
    {
        if (input == "g")
        {
            break;
        }

        std::cout << "Press key g+enter to retrieve the capture.\n";
    }
    if (!std::cin.good())
    {
        return GfxrCaptureControlFlow::kStop;
    }

    // Wait for capture to finish.
    std::cout << "Waiting for the current capture to complete...\n";
    while (!IsCaptureFinished(adb, gfxr_capture_file_dir))
    {
        absl::SleepFor(absl::Seconds(1));
    }

    // Retrieve the capture. If this fails, we print an error but don't exit the tool, allowing
    // the user to try again.
    if (absl::Status status = RetrieveGfxrCapture(adb, options); !status.ok())
    {
        std::cout << "Failed to retrieve capture: " << status.message() << '\n';
    }
    else
    {
        std::cout << "Capture complete.\n";
    }

    return GfxrCaptureControlFlow::kContinue;
}

absl::Status CmdListDevice(const CommandContext& ctx)
{
    auto list = ctx.mgr.ListDevice();
    if (list.empty())
    {
        std::cout << "No device connected." << std::endl;
        return absl::OkStatus();
    }
    std::cout << "Devices: " << std::endl;
    for (const auto& device : list)
    {
        std::cout << "\t" << device.GetDisplayName() << std::endl;
    }
    return absl::OkStatus();
}

absl::Status CmdListPackage(const CommandContext& ctx)
{
    auto* device = ctx.mgr.GetDevice();
    auto ret = device->ListPackage();
    if (!ret.ok())
    {
        return ret.status();
    }
    std::cout << "Packages: " << std::endl;
    for (const auto& pkg : *ret)
    {
        std::cout << "\t" << pkg << std::endl;
    }
    return absl::OkStatus();
}

absl::Status CmdRunPackage(const CommandContext& ctx)
{
    absl::Status status = InternalRunPackage(ctx, false);
    if (!status.ok())
    {
        return status;
    }
    return WaitForExitConfirmation();
}

absl::Status CmdPm4Capture(const CommandContext& ctx)
{
    absl::Status status = InternalRunPackage(ctx, false);
    if (!status.ok())
    {
        return status;
    }

    std::cout << "Waiting " << ctx.options.trigger_capture_after << " seconds...\n";
    std::this_thread::sleep_for(std::chrono::seconds(ctx.options.trigger_capture_after));

    status = TriggerPm4Capture(ctx.mgr, ctx.options.download_dir);
    if (!status.ok())
    {
        return Dive::StatusWithContext(status, "Triggering PM4 capture failed");
    }
    return WaitForExitConfirmation();
}

absl::Status CmdGfxrCapture(const CommandContext& context)
{
    RETURN_IF_ERROR(InternalRunPackage(context, /*enable_gfxr=*/true));

    // Only delete the on device capture directory when the application is closed. If deleted too
    // early then capture will fail.
    absl::Cleanup cleanup_device_files = [&context] {
        std::string on_device_capture_directory =
            absl::StrCat(Dive::kDeviceCapturePath, "/", context.options.gfxr_capture_file_dir);
        context.mgr.GetDevice()
            ->Adb()
            .Run(absl::StrFormat("shell rm -rf %s", on_device_capture_directory))
            .IgnoreError();
    };

    while (true)
    {
        absl::StatusOr<GfxrCaptureControlFlow> control_flow =
            TriggerGfxrCapture(*context.mgr.GetDevice(), context.options);
        if (!control_flow.status().ok())
        {
            return control_flow.status();
        }

        switch (*control_flow)
        {
            case GfxrCaptureControlFlow::kStop:
                return absl::OkStatus();
            case GfxrCaptureControlFlow::kContinue:
                // Nothing to do
                break;
        }
    }

    return absl::OkStatus();
}

absl::Status CmdGfxrReplay(const CommandContext& ctx)
{
    absl::Status status = ctx.mgr.DeployReplayApk(ctx.options.serial);
    if (!status.ok())
    {
        return Dive::StatusWithContext(status, "Failed to deploy replay apk");
    }

    status = ctx.mgr.RunReplayApk(ctx.options.replay_settings);
    if (!status.ok())
    {
        return Dive::StatusWithContext(status, "Failed to run replay apk");
    }
    return absl::OkStatus();
}

absl::Status CmdCleanup(const CommandContext& ctx)
{
    return ctx.mgr.CleanupPackageProperties(ctx.options.package);
}

// Overload for parsing the Command enum from command line flags.
bool AbslParseFlag(absl::string_view text, Command* command, std::string* error)
{
    if (text.empty())
    {
        *command = Command::kNone;
        return true;
    }
    for (const auto& def : kCommandDefs)
    {
        if (text == def.name)
        {
            *command = def.cmd;
            return true;
        }
    }
    *error = absl::StrCat("\n" + GenerateCommandFlagHelp());
    return false;
}

// Overload for converting the Command enum back to a string.
std::string AbslUnparseFlag(Command command)
{
    if (command == Command::kNone) return "";
    for (const auto& def : kCommandDefs)
    {
        if (def.cmd == command) return def.name;
    }
    return "unknown";
}

}  // namespace

// Abseil flags parsing uses ADL. AppType and GfxrReplayOptions are in the Dive namespace so
// AbslParseFlag and AbslUnparseFlag need to be as well.
namespace Dive
{

// Overload for parsing the AppType enum from command line flags.
bool AbslParseFlag(absl::string_view text, Dive::AppType* type, std::string* error)
{
    for (const auto& info : Dive::kAppTypeInfos)
    {
        if (text == info.cli_name)
        {
            *type = info.type;
            return true;
        }
    }
    *error = absl::StrCat("\n" + GenerateAppTypeFlagHelp());
    return false;
}

// Overload for converting the AppType enum back to a string.
std::string AbslUnparseFlag(Dive::AppType type)
{
    for (const auto& info : Dive::kAppTypeInfos)
    {
        if (info.type == type)
        {
            return std::string(info.cli_name);
        }
    }
    return absl::StrCat(static_cast<int>(type));
}

bool AbslParseFlag(absl::string_view text, GfxrReplayOptions* run_type, std::string* error)
{
    if (text == "normal")
    {
        *run_type = GfxrReplayOptions::kNormal;
        return true;
    }
    if (text == "pm4_dump")
    {
        *run_type = GfxrReplayOptions::kPm4Dump;
        return true;
    }
    if (text == "perf_counters")
    {
        *run_type = GfxrReplayOptions::kPerfCounters;
        return true;
    }
    if (text == "gpu_timing")
    {
        *run_type = GfxrReplayOptions::kGpuTiming;
        return true;
    }
    if (text == "renderdoc")
    {
        *run_type = GfxrReplayOptions::kRenderDoc;
        return true;
    }
    *error = "unknown value for enumeration";
    return false;
}

std::string AbslUnparseFlag(GfxrReplayOptions run_type)
{
    switch (run_type)
    {
        case GfxrReplayOptions::kNormal:
            return "normal";
        case GfxrReplayOptions::kPm4Dump:
            return "pm4_dump";
        case GfxrReplayOptions::kPerfCounters:
            return "perf_counters";
        case GfxrReplayOptions::kGpuTiming:
            return "gpu_timing";
        case GfxrReplayOptions::kRenderDoc:
            return "renderdoc";

        default:
            return absl::StrCat(run_type);
    }
}

}  // namespace Dive

ABSL_FLAG(Command, command, Command::kNone, GenerateCommandFlagHelp());

ABSL_FLAG(
    std::string, device, "",
    "The serial number of the target Android device. "
    "If unspecified and only one device is connected, that device will be used automatically.");

ABSL_FLAG(std::string, package, "",
          "The Android package name of the target application (e.g., com.example.myapp).");

ABSL_FLAG(std::string, vulkan_command, "",
          "The executable name or path for Vulkan CLI applications (used when "
          "--type=vulkan_cli_non_openxr).");

ABSL_FLAG(std::string, vulkan_command_args, "", "Arguments to pass to the Vulkan CLI application.");

ABSL_FLAG(Dive::AppType, type, Dive::AppType::kVulkan_OpenXR, GenerateAppTypeFlagHelp());

ABSL_FLAG(std::string, download_dir, ".",
          "The local host directory where captured files will be saved. Defaults to the current "
          "directory.");

ABSL_FLAG(std::string, gfxr_capture_file_dir, "gfxr_capture",
          absl::StrCat("The name of the temporary subdirectory on the Android device (under '",
                       Dive::kDeviceCapturePath,
                       "') where the GFXR capture is stored. This subdirectory name is mirrored on "
                       "the host within ",
                       "'--download_dir'."));

ABSL_FLAG(int, trigger_capture_after, 5,
          "The delay in seconds before automatically triggering a capture (only used with "
          "'pm4_capture').");

ABSL_FLAG(std::string, gfxr_replay_file_path, "",
          "The full path to the .gfxr capture file located on the Android device to be replayed.");

ABSL_FLAG(std::string, gfxr_replay_flags, "",
          "Additional command-line flags to pass directly to the GFXR replay tool.");

ABSL_FLAG(std::vector<std::string>, metrics, {},
          "A comma-separated list of metrics to profile. "
          "Only used when --gfxr_replay_run_type is set to 'perf_counters'.");

ABSL_FLAG(Dive::GfxrReplayOptions, gfxr_replay_run_type, Dive::GfxrReplayOptions::kNormal,
          "The analysis mode to perform during replay:\n"
          "\tnormal       : Standard replay with no analysis.\n"
          "\tpm4_dump     : Capture all PM4 packets.\n"
          "\tperf_counters: Collect performance metrics (requires --metrics).\n"
          "\tgpu_timing   : Collect GPU timing data.\n"
          "\trenderdoc    : Create a RenderDoc capture from the replay.");

ABSL_FLAG(bool, validation_layer, false,
          "If true, runs the GFXR replay with the Vulkan Validation Layer enabled.");
ABSL_FLAG(bool, wait_for_debugger, false,
          "Tell GFXR replay app to wait for a debugger before continuing to replay");

int main(int argc, char** argv)
{
    absl::FlagsUsageConfig flags_usage_config;
    flags_usage_config.version_string = Dive::GetCompleteVersionString;
    absl::SetFlagsUsageConfig(flags_usage_config);
    absl::SetProgramUsageMessage("Dive Tool CLI. Use --help for details.");
    absl::ParseCommandLine(argc, argv);

    GlobalOptions opts{
        .serial = absl::GetFlag(FLAGS_device),
        .package = absl::GetFlag(FLAGS_package),
        .vulkan_command = absl::GetFlag(FLAGS_vulkan_command),
        .vulkan_command_args = absl::GetFlag(FLAGS_vulkan_command_args),
        .app_type = absl::GetFlag(FLAGS_type),
        .download_dir = absl::GetFlag(FLAGS_download_dir),
        .gfxr_capture_file_dir = absl::GetFlag(FLAGS_gfxr_capture_file_dir),
        .trigger_capture_after = absl::GetFlag(FLAGS_trigger_capture_after),
        .replay_settings =
            {
                .remote_capture_path = absl::GetFlag(FLAGS_gfxr_replay_file_path),
                .local_download_dir = absl::GetFlag(FLAGS_download_dir),
                .run_type = absl::GetFlag(FLAGS_gfxr_replay_run_type),
                .replay_flags_str = absl::GetFlag(FLAGS_gfxr_replay_flags),
                .wait_for_debugger = absl::GetFlag(FLAGS_wait_for_debugger),
                .metrics = absl::GetFlag(FLAGS_metrics),
                .use_validation_layer = absl::GetFlag(FLAGS_validation_layer),
            },
    };

    Command cmd = absl::GetFlag(FLAGS_command);
    const CommandDef* selected_def = nullptr;
    for (const auto& def : kCommandDefs)
    {
        if (def.cmd == cmd)
        {
            selected_def = &def;
            break;
        }
    }

    if (cmd == Command::kNone || selected_def == nullptr)
    {
        std::cout << "Error: No valid command specified.\n"
                  << GenerateCommandFlagHelp() << std::endl;
        return EXIT_FAILURE;
    }

    if (absl::Status status = selected_def->validator(opts); !status.ok())
    {
        std::cout << status.message() << std::endl;
        std::cout << Dive::GetStackTrace(status) << std::endl;
        return EXIT_FAILURE;
    }

    Dive::DeviceManager mgr;
    if (cmd != Command::kListDevice)
    {
        std::vector<DeviceInfo> devices = mgr.ListDevice();
        if (devices.empty())
        {
            std::cout << "No Android devices connected." << std::endl;
            return EXIT_FAILURE;
        }

        absl::StatusOr<std::string> validated_serial = ValidateSerial(devices, opts.serial);
        if (!validated_serial.ok())
        {
            std::cout << validated_serial.status().message() << std::endl;
            std::cout << Dive::GetStackTrace(validated_serial.status()) << std::endl;
            return EXIT_FAILURE;
        }
        opts.serial = *validated_serial;

        if (absl::Status status = InitializeDevice(mgr, opts.serial); !status.ok())
        {
            std::cout << status.message() << std::endl;
            std::cout << Dive::GetStackTrace(status) << std::endl;
            return EXIT_FAILURE;
        }
    }

    CommandContext ctx{.mgr = mgr, .options = opts};
    absl::Status ret = selected_def->executor(ctx);
    if (!ret.ok())
    {
        std::cout << "Error executing command '" << selected_def->name << "': " << ret.message()
                  << std::endl;
        std::cout << Dive::GetStackTrace(ret) << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
