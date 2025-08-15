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

#include <filesystem>
#include <future>
#include <iostream>
#include <ostream>
#include <string>
#include <system_error>
#include <thread>

#include "absl/flags/flag.h"
#include "absl/flags/internal/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "android_application.h"
#include "command_utils.h"
#include "constants.h"
#include "device_mgr.h"
#include "network/tcp_client.h"
#include "absl/strings/str_cat.h"

using namespace std::chrono_literals;

enum class Command
{
    kNone,
    kGfxrCapture,
    kGfxrReplay,
    kProfileReplay,
    kListDevice,
    kListPackage,
    kRunPackage,
    kRunAndCapture,
    kCleanup,
};

bool AbslParseFlag(absl::string_view text, Command* command, std::string* error)
{
    if (text == "list_device")
    {
        *command = Command::kListDevice;
        return true;
    }
    if (text == "list_package")
    {
        *command = Command::kListPackage;
        return true;
    }
    if (text == "run")
    {
        *command = Command::kRunPackage;
        return true;
    }
    if (text == "capture")
    {
        *command = Command::kRunAndCapture;
        return true;
    }
    if (text == "cleanup")
    {
        *command = Command::kCleanup;
        return true;
    }
    if (text == "gfxr_capture")
    {
        *command = Command::kGfxrCapture;
        return true;
    }
    if (text == "gfxr_replay")
    {
        *command = Command::kGfxrReplay;
        return true;
    }
    if (text == "profile_replay")
    {
        *command = Command::kProfileReplay;
        return true;
    }
    if (text.empty())
    {
        *command = Command::kNone;
        return true;
    }
    *error = "unknown value for enumeration";
    return false;
}

std::string AbslUnparseFlag(Command command)
{
    switch (command)
    {
    case Command::kNone:
        return "";
    case Command::kListDevice:
        return "list_device";
    case Command::kGfxrCapture:
        return "gfxr_capture";
    case Command::kGfxrReplay:
        return "gfxr_replay";
    case Command::kProfileReplay:
        return "profile_replay";
    case Command::kListPackage:
        return "list_package";
    case Command::kRunPackage:
        return "run";
    case Command::kRunAndCapture:
        return "capture";
    case Command::kCleanup:
        return "cleanup";

    default:
        return absl::StrCat(command);
    }
}

ABSL_FLAG(Command,
          command,
          Command::kNone,
          "list of actions: \n\tlist_device \n\tgfxr_capture \n\tgfxr_replay "
          "\n\tprofile_replay \n\tlist_package \n\trun \n\tcapture \n\tcleanup");
ABSL_FLAG(
std::string,
device,
"",
"Device serial. If not specified and only one device is plugged in then that device is used.");
ABSL_FLAG(std::string, package, "", "Package on the device");
ABSL_FLAG(std::string, vulkan_command, "", "the command for vulkan cli application to run");
ABSL_FLAG(std::string, vulkan_command_args, "", "the arguments for vulkan cli application to run");
ABSL_FLAG(std::string,
          type,
          "openxr",
          "application type: \n\t`openxr` for OpenXR applications(apk) \n\t `vulkan` for Vulkan "
          "applications(apk)"
          "\n\t`vulkan_cli` for command line Vulkan application.");
ABSL_FLAG(
std::string,
download_dir,
".",
"specify the directory path on the host to download the capture, default to current directory.");

ABSL_FLAG(std::string,
          device_architecture,
          "",
          "specify the device architecture to capture with gfxr (arm64-v8, armeabi-v7a, x86, or "
          "x86_64). If not specified, the default is the architecture of --device.");
ABSL_FLAG(std::string,
          gfxr_capture_file_dir,
          "gfxr_capture",
          "specify the name of the directory for the gfxr capture. If not specified, the default "
          "file name is gfxr_capture.");

ABSL_FLAG(
int,
trigger_capture_after,
5,
"specify how long in seconds the capture be triggered after the application starts when running "
"with the `capture` command. If not specified, it will be triggered after 5 seconds.");
ABSL_FLAG(std::string,
          gfxr_replay_file_path,
          "",
          "specify the on-device path of the gfxr capture to replay.");
ABSL_FLAG(std::string, gfxr_replay_flags, "", "specify flags to pass to gfxr replay.");

ABSL_FLAG(bool, dump_pm4, false, "dump pm4 for gfxr replay");

ABSL_FLAG(std::vector<std::string>,
          metrics,
          {},
          "comma-separated list of metrics to profile for profile_replay command.");

void PrintUsage()
{
    std::cout << absl::ProgramUsageMessage() << std::endl;
}

bool ListDevice(const Dive::DeviceManager& mgr)
{
    auto list = mgr.ListDevice();
    if (list.empty())
    {
        std::cout << "No device connected" << std::endl;
        return false;
    }
    int index = 0;
    std::cout << "Devices: " << std::endl;
    for (const auto& device : list)
    {
        std::cout << ++index << " : " << device.GetDisplayName() << std::endl;
    }

    return true;
}

bool ListPackage(Dive::DeviceManager& mgr, const std::string& device_serial)
{
    auto dev_ret = mgr.SelectDevice(device_serial);
    if (!dev_ret.ok())
    {
        std::cout << "Failed to select device: " << dev_ret.status().message() << std::endl;
        return false;
    }
    auto device = *dev_ret;
    auto ret = device->SetupDevice();
    if (!ret.ok())
    {
        std::cout << "Failed to setup device, error: " << ret.message() << std::endl;
        return false;
    }

    auto result = device->ListPackage();
    if (!result.ok())
    {
        std::cout << "Failed to list packages, error: " << ret.message() << std::endl;
        return false;
    }
    std::vector<std::string> packages = *result;
    int                      index = 0;
    std::cout << "Packages: " << std::endl;
    for (const auto& package : packages)
    {
        std::cout << ++index << " : " << package << std::endl;
    }

    return true;
}

bool RunPackage(Dive::DeviceManager& mgr,
                const std::string&   serial,
                const std::string&   app_type,
                const std::string&   package,
                const std::string&   command,
                const std::string&   command_args,
                const std::string&   device_architecture,
                const std::string&   gfxr_capture_directory,
                bool                 is_gfxr_capture)
{
    if (serial.empty() || (package.empty() && command.empty()))
    {
        std::cout << "Missing required options." << std::endl;
        PrintUsage();
        return false;
    }
    auto dev_ret = mgr.SelectDevice(serial);

    if (!dev_ret.ok())
    {
        std::cout << "Failed to select device " << dev_ret.status().message() << std::endl;
        return false;
    }
    auto dev = *dev_ret;
    dev->EnableGfxr(is_gfxr_capture);
    auto ret = dev->SetupDevice();
    if (!ret.ok())
    {
        std::cout << "Failed to setup device, error: " << ret.message() << std::endl;
        return false;
    }

    if (app_type == "openxr")
    {
        ret = dev->SetupApp(package,
                            Dive::ApplicationType::OPENXR_APK,
                            command_args,
                            device_architecture,
                            gfxr_capture_directory);
    }
    else if (app_type == "vulkan")
    {
        ret = dev->SetupApp(package,
                            Dive::ApplicationType::VULKAN_APK,
                            command_args,
                            device_architecture,
                            gfxr_capture_directory);
    }
    else if (app_type == "vulkan_cli")
    {
        ret = dev->SetupApp(command, command_args, Dive::ApplicationType::VULKAN_CLI);
    }
    else
    {
        PrintUsage();
        return false;
    }
    if (!ret.ok())
    {
        std::cout << "Failed to setup app, error: " << ret.message() << std::endl;
        return false;
    }

    ret = dev->StartApp();
    if (!ret.ok())
    {
        std::cout << "Failed to start app, error: " << ret.message() << std::endl;
    }

    return ret.ok();
}

bool TriggerCapture(Dive::DeviceManager& mgr)
{
    if (mgr.GetDevice() == nullptr)
    {
        std::cout << "No device selected, can't capture. Did you provide --device serial?"
                  << std::endl;
        return false;
    }

    std::string        download_dir = absl::GetFlag(FLAGS_download_dir);
    Network::TcpClient client;
    const std::string  host = "127.0.0.1";
    int                port = mgr.GetDevice()->Port();
    absl::Status       status = client.Connect(host, port);
    if (!status.ok())
    {
        std::cout << "Connection failed: " << status.message() << std::endl;
        return false;
    }
    absl::StatusOr<std::string> capture_file_path = client.StartPm4Capture();
    if (!capture_file_path.ok())
    {
        std::cout << capture_file_path.status().message() << std::endl;
        return false;
    }

    std::filesystem::path target_download_dir(download_dir);
    if (!std::filesystem::is_directory(target_download_dir))
    {
        std::cout << "Invalid download directory: " << target_download_dir << std::endl;
        return false;
    }
    std::filesystem::path p(*capture_file_path);
    std::string           download_file_path = (target_download_dir / p.filename()).string();
    status = client.DownloadFileFromServer(*capture_file_path, download_file_path);
    if (!status.ok())
    {
        std::cout << status.message() << std::endl;
        return false;
    }
    std::cout << "Capture saved at " << download_file_path << std::endl;
    return true;
}

absl::Status IsCaptureDirectoryBusy(Dive::DeviceManager& mgr,
                                    const std::string&   gfxr_capture_directory)
{
    std::string                 on_device_capture_directory = absl::StrCat(Dive::kDeviceCapturePath,
                                                           "/",
                                                           gfxr_capture_directory);
    std::string                 command = "shell lsof " + on_device_capture_directory;
    absl::StatusOr<std::string> output = mgr.GetDevice()->Adb().RunAndGetResult(command);

    if (!output.ok())
    {
        std::cout << "Error checking directory: " << output.status().message() << std::endl;
    }

    std::stringstream ss(output->c_str());
    std::string       line;
    int               line_count = 0;
    while (std::getline(ss, line))
    {
        line_count++;
    }

    return line_count <= 1 ? absl::OkStatus() :
                             absl::InternalError("Capture file operation in progress.");
}

bool RetrieveGfxrCapture(Dive::DeviceManager& mgr, const std::string& gfxr_capture_directory)
{
    std::filesystem::path download_dir = absl::GetFlag(FLAGS_download_dir);

    // Need to explicitly use forward slash so that this works on Windows targetting Android
    std::string on_device_capture_directory = absl::StrCat(Dive::kDeviceCapturePath,
                                                           "/",
                                                           gfxr_capture_directory);

    std::cout << "Retrieving capture..." << std::endl;

    // Retrieve the list of files in the capture directory on the device.
    std::string command = absl::StrFormat("shell ls %s", on_device_capture_directory);
    absl::StatusOr<std::string> output = mgr.GetDevice()->Adb().RunAndGetResult(command);
    if (!output.ok())
    {
        std::cout << "Error getting capture_file name: " << output.status().message() << std::endl;
        return false;
    }

    std::vector<std::string> file_list = absl::StrSplit(std::string(output->data()), '\n');

    // Check if on-device directory is empty
    if ((file_list.empty()) || (file_list[0].empty()))
    {
        std::cout << "Error, captures not present on device at: " << on_device_capture_directory
                  << std::endl;
        return false;
    }

    // Find name for new local target directory
    std::filesystem::path full_target_download_dir = download_dir / gfxr_capture_directory;
    bool local_target_dir_exists = std::filesystem::exists(full_target_download_dir);
    int  suffix = 0;
    while (local_target_dir_exists)
    {
        // Append numerical suffix to make a fresh dir
        full_target_download_dir = download_dir / absl::StrFormat("%s_%s",
                                                                  gfxr_capture_directory,
                                                                  std::to_string(suffix));
        suffix++;
        local_target_dir_exists = std::filesystem::exists(full_target_download_dir);
    }

    command = absl::StrFormat("pull %s %s",
                              on_device_capture_directory,
                              full_target_download_dir.string());
    output = mgr.GetDevice()->Adb().RunAndGetResult(command);
    if (!output.ok())
    {
        std::cout << "Error pulling files: " << output.status().message() << std::endl;
        return false;
    }

    std::cout << "Capture sucessfully saved at " << full_target_download_dir << std::endl;
    return true;
}

void TriggerGfxrCapture(Dive::DeviceManager& mgr,
                        const std::string&   package,
                        const std::string&   gfxr_capture_directory)
{
    std::cout
    << "Press key g+enter to trigger a capture and g+enter to retrieve the capture. Press "
       "any other key+enter to stop the application. Note that this may impact your "
       "capture file if the capture has not been completed. \n";
    std::string
    capture_complete_message = "Capture complete. Press key g+enter to trigger another capture or "
                               "any other key+enter to stop the application.";

    std::string  input;
    bool         is_capturing = false;
    absl::Status ret;
    while (std::getline(std::cin, input))
    {
        if (input == "g")
        {
            if (is_capturing)
            {
                ret = IsCaptureDirectoryBusy(mgr, gfxr_capture_directory);
                while (!ret.ok())
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    std::cout << "GFXR capture in progress, please wait for current capture to "
                                 "complete before starting another."
                              << std::endl;
                }
                ret = mgr.GetDevice()->Adb().Run(
                "shell setprop debug.gfxrecon.capture_android_trigger false");
                if (!ret.ok())
                {
                    std::cout << "There was an error stopping the gfxr runtime capture."
                              << std::endl;
                    return;
                }

                // Retrieve the capture and asset file at the end of the capture.
                RetrieveGfxrCapture(mgr, gfxr_capture_directory);
                is_capturing = false;
                std::cout << capture_complete_message << std::endl;
            }
            else
            {
                ret = mgr.GetDevice()->Adb().Run(
                "shell setprop debug.gfxrecon.capture_android_trigger true");

                if (!ret.ok())
                {
                    std::cout << "There was an error starting the gfxr runtime capture."
                              << std::endl;
                    return;
                }
                is_capturing = true;
                std::cout << "Capture started. Press g+enter to retrieve the capture." << std::endl;
            }
        }
        else
        {
            if (is_capturing)
            {
                std::string warning_message = "GFXR capture in progress, please wait for capture "
                                              "to complete before stopping the application.";

                std::cout << warning_message << std::endl;
                ret = IsCaptureDirectoryBusy(mgr, gfxr_capture_directory);
                while (!ret.ok())
                {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    std::cout << warning_message << std::endl;
                }
                ret = mgr.GetDevice()->Adb().Run(
                "shell setprop debug.gfxrecon.capture_android_trigger false");
                if (!ret.ok())
                {
                    std::cout << "There was an error stopping the gfxr runtime capture."
                              << std::endl;
                    return;
                }

                // Retrieve the capture and asset file at the end of the capture.
                RetrieveGfxrCapture(mgr, gfxr_capture_directory);
                is_capturing = false;
                std::cout << capture_complete_message << std::endl;
            }
            else
            {
                std::cout << "Exiting..." << std::endl;
                break;
            }
        }
    }

    // Only delete the on device capture directory when the application is closed.
    std::string on_device_capture_directory = absl::StrCat(Dive::kDeviceCapturePath,
                                                           "/",
                                                           gfxr_capture_directory);
    ret = mgr.GetDevice()->Adb().Run(
    absl::StrFormat("shell rm -rf %s", on_device_capture_directory));
}

bool RunAndCapture(Dive::DeviceManager& mgr,
                   const std::string&   serial,
                   const std::string&   app_type,
                   const std::string&   package,
                   const std::string&   command,
                   const std::string&   command_args,
                   const std::string&   device_architecture,
                   const std::string&   gfxr_capture_directory,
                   const bool           is_gfxr_capture)
{
    if (!RunPackage(mgr,
                    serial,
                    app_type,
                    package,
                    command,
                    command_args,
                    device_architecture,
                    gfxr_capture_directory,
                    is_gfxr_capture))
    {
        return false;
    }

    if (is_gfxr_capture)
    {
        TriggerGfxrCapture(mgr, package, gfxr_capture_directory);
    }
    else
    {
        int time_to_wait_in_seconds = absl::GetFlag(FLAGS_trigger_capture_after);
        std::cout << "wait for " << time_to_wait_in_seconds << " seconds" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(time_to_wait_in_seconds));

        TriggerCapture(mgr);

        std::cout << "Press other key+enter to exit" << std::endl;
        std::string input;
        if (std::getline(std::cin, input))
        {
            std::cout << "Exiting..." << std::endl;
        }
    }

    return true;
}

bool CleanUpAppAndDevice(Dive::DeviceManager& mgr,
                         const std::string&   serial,
                         const std::string&   package)
{
    if (serial.empty())
    {
        std::cout << "Please run with `--device [serial]` and `--package [package]` options."
                  << std::endl;
        PrintUsage();
        return false;
    }

    if (package.empty())
    {
        std::cout << "Package not provided. You run run with `--package [package]` options to "
                     "clean up package specific settings.";
    }

    if (mgr.GetDevice() == nullptr)
    {
        if (absl::StatusOr<Dive::AndroidDevice*> device = mgr.SelectDevice(serial); !device.ok())
        {
            std::cout << "Failed to select device: " << device.status() << std::endl;
            return false;
        }
    }

    return mgr.Cleanup(serial, package).ok();
}

bool ProcessInput(Dive::DeviceManager& mgr)
{
    std::cout << "Press key t+enter to trigger a capture. \nPress any other key+enter to exit.";

    std::string input;
    while (std::getline(std::cin, input))
    {
        if (input == "t")
        {
            std::cout << "key t pressed " << std::endl;
            TriggerCapture(mgr);
        }
        else
        {
            break;
        }
        std::cout << "Press key t+enter to trigger a capture. \nPress other key+enter to exit.";
    }

    return true;
}

bool DeployGfxrReplay(Dive::DeviceManager& mgr, const std::string& device_serial)
{
    auto dev_ret = mgr.SelectDevice(device_serial);

    if (!dev_ret.ok())
    {
        std::cout << "Failed to select device " << dev_ret.status().message() << std::endl;
        return false;
    }

    auto dev = *dev_ret;
    auto ret = dev->SetupDevice();
    if (!ret.ok())
    {
        std::cout << "Failed to setup device, error: " << ret.message() << std::endl;
        return false;
    }
    // Deploying install/gfxr-replay.apk
    ret = mgr.DeployReplayApk(device_serial);
    if (!ret.ok())
    {
        return false;
    }

    return true;
}

bool DeployAndRunGfxrReplay(Dive::DeviceManager& mgr,
                            const std::string&   device_serial,
                            const std::string    gfxr_replay_capture,
                            const std::string    gfxr_replay_flags)
{
    bool        dump_pm4 = absl::GetFlag(FLAGS_dump_pm4);
    std::string capture_download_dir = absl::GetFlag(FLAGS_download_dir);

    if (!DeployGfxrReplay(mgr, device_serial))
    {
        return false;
    }
    // Running replay for on-device capture
    auto ret = mgr.RunReplayApk(gfxr_replay_capture,
                                gfxr_replay_flags,
                                dump_pm4,
                                capture_download_dir);
    return ret.ok();
}

bool DeployAndProfilingGfxrReplay(Dive::DeviceManager& mgr,
                                  const std::string&   device_serial,
                                  const std::string    gfxr_replay_capture)
{
    std::string capture_download_dir = absl::GetFlag(FLAGS_download_dir);

    if (!DeployGfxrReplay(mgr, device_serial))
    {
        return false;
    }
    std::vector<std::string> metrics = absl::GetFlag(FLAGS_metrics);

    auto ret = mgr.RunProfilingOnReplay(gfxr_replay_capture, metrics, capture_download_dir);
    return ret.ok();
}

int main(int argc, char** argv)
{
    absl::SetProgramUsageMessage("Run app with --help for more details");
    absl::ParseCommandLine(argc, argv);
    Command     cmd = absl::GetFlag(FLAGS_command);
    std::string serial = absl::GetFlag(FLAGS_device);
    std::string package = absl::GetFlag(FLAGS_package);
    std::string vulkan_command = absl::GetFlag(FLAGS_vulkan_command);
    std::string vulkan_command_args = absl::GetFlag(FLAGS_vulkan_command_args);
    std::string app_type = absl::GetFlag(FLAGS_type);
    std::string device_architecture = absl::GetFlag(FLAGS_device_architecture);
    std::string gfxr_capture_file_dir = absl::GetFlag(FLAGS_gfxr_capture_file_dir);
    std::string gfxr_replay_file_path = absl::GetFlag(FLAGS_gfxr_replay_file_path);
    std::string gfxr_replay_flags = absl::GetFlag(FLAGS_gfxr_replay_flags);

    Dive::DeviceManager mgr;
    auto                list = mgr.ListDevice();
    if (list.empty())
    {
        std::cout << "No device connected" << std::endl;
        return 0;
    }

    if (serial.empty() && list.size() == 1)
    {
        serial = list.front().m_serial;
        std::cout << "--device unspecified, using " << serial << '\n';
    }

    bool res = false;

    switch (cmd)
    {
    case Command::kGfxrCapture:
    {
        RunAndCapture(mgr,
                      serial,
                      app_type,
                      package,
                      vulkan_command,
                      vulkan_command_args,
                      device_architecture,
                      gfxr_capture_file_dir,
                      true);
        break;
    }
    case Command::kGfxrReplay:
    {
        if (gfxr_replay_file_path == "")
        {
            std::cout << "Invalid flags: Must specify --gfxr_replay_file_path" << std::endl;
            break;
        }
        res = DeployAndRunGfxrReplay(mgr, serial, gfxr_replay_file_path, gfxr_replay_flags);
        break;
    }
    case Command::kProfileReplay:
    {
        if (gfxr_replay_file_path == "")
        {
            std::cout << "Invalid flags: Must specify --gfxr_replay_file_path" << std::endl;
            break;
        }
        res = DeployAndProfilingGfxrReplay(mgr, serial, gfxr_replay_file_path);
        break;
    }
    case Command::kListDevice:
    {
        res = ListDevice(mgr);
        break;
    }
    case Command::kListPackage:
    {
        res = ListPackage(mgr, serial);
        break;
    }

    case Command::kRunPackage:
    {
        if (RunPackage(mgr,
                       serial,
                       app_type,
                       package,
                       vulkan_command,
                       vulkan_command_args,
                       "",
                       "",
                       false))
        {
            res = ProcessInput(mgr);
        }

        break;
    }

    case Command::kRunAndCapture:
    {
        res = RunAndCapture(mgr,
                            serial,
                            app_type,
                            package,
                            vulkan_command,
                            vulkan_command_args,
                            "",
                            "",
                            false);
        break;
    }
    case Command::kCleanup:
    {
        res = CleanUpAppAndDevice(mgr, serial, package);
        break;
    }
    case Command::kNone:
    {
        PrintUsage();
        res = true;
        break;
    }
    }

    return res ? 0 : 1;
}
