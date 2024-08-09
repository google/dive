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
#include <iostream>
#include <ostream>
#include <string>
#include <system_error>
#include <thread>

#include "absl/flags/flag.h"
#include "absl/flags/internal/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "android_application.h"
#include "client.h"
#include "device_mgr.h"

using namespace std::chrono_literals;

enum class Command
{
    kNone,
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
    case Command::kNone: return "";
    case Command::kListDevice: return "list_device";
    case Command::kListPackage: return "list_package";
    case Command::kRunPackage: return "run";
    case Command::kRunAndCapture: return "capture";
    case Command::kCleanup: return "cleanup";

    default: return absl::StrCat(command);
    }
}

ABSL_FLAG(Command,
          command,
          Command::kNone,
          "list of actions: \n\tlist_device \n\tlist_package \n\trun \n\tcapture");
ABSL_FLAG(std::string, target, "localhost:19999", "Server address");
ABSL_FLAG(std::string, device, "", "Device serial");
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
download_path,
".",
"specify the full path to download the capture on the host, default to current directory.");

ABSL_FLAG(
int,
trigger_capture_after,
5,
"specify how long in seconds the capture be triggered after the application starts when running "
"with the `capture` command. If not specified, it will be triggered after 5 seconds.");

void print_usage()
{
    std::cout << absl::ProgramUsageMessage() << std::endl;
}

bool list_device(const Dive::DeviceManager& mgr)
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

bool list_package(Dive::DeviceManager& mgr, const std::string& device_serial)
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

bool run_package(Dive::DeviceManager& mgr,
                 const std::string&   app_type,
                 const std::string&   package,
                 const std::string&   command,
                 const std::string&   command_args)
{
    std::string serial = absl::GetFlag(FLAGS_device);

    if (serial.empty() || (package.empty() && command.empty()))
    {
        print_usage();
        return false;
    }
    auto dev_ret = mgr.SelectDevice(serial);

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

    if (app_type == "openxr")
    {
        ret = dev->SetupApp(package, Dive::ApplicationType::OPENXR_APK, "");
    }
    else if (app_type == "vulkan")
    {
        ret = dev->SetupApp(package, Dive::ApplicationType::VULKAN_APK, "");
    }
    else if (app_type == "vulkan_cli")
    {
        ret = dev->SetupApp(command, command_args, Dive::ApplicationType::VULKAN_CLI);
    }
    else
    {
        print_usage();
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

bool trigger_capture(Dive::DeviceManager& mgr)
{
    std::string target_str = absl::GetFlag(FLAGS_target);
    std::string download_path = absl::GetFlag(FLAGS_download_path);
    std::string input;

    Dive::DiveClient client(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    absl::StatusOr<std::string> reply = client.TestConnection();
    if (reply.ok())
        std::cout << *reply << std::endl;
    else
        std::cout << "TestConnection failed with " << reply.status() << std::endl;

    absl::StatusOr<std::string> trace_file_path = client.RequestStartTrace();
    if (trace_file_path.ok())
        std::cout << "Trigger capture: " << *trace_file_path << std::endl;
    else
        std::cout << "Failed to trigger capture: " << trace_file_path.status() << std::endl;

    std::filesystem::path p(*trace_file_path);
    std::filesystem::path target_download_path(download_path);
    if (!std::filesystem::exists(target_download_path))
    {
        std::error_code ec;
        if (!std::filesystem::create_directories(target_download_path, ec))
        {
            std::cout << "error create directory: " << ec << std::endl;
        }
    }
    target_download_path /= p.filename();
    auto ret = mgr.GetDevice()->RetrieveTraceFile(*trace_file_path,
                                                  target_download_path.generic_string());
    if (ret.ok())
        std::cout << "Capture saved at " << target_download_path << std::endl;
    else
        std::cout << "Failed to retrieve capture file" << std::endl;

    return ret.ok();
}

bool run_and_capture(Dive::DeviceManager& mgr,
                     const std::string&   app_type,
                     const std::string&   package,
                     const std::string&   command,
                     const std::string&   command_args)
{

    std::string target_str = absl::GetFlag(FLAGS_target);
    run_package(mgr, app_type, package, command, command_args);
    int time_to_wait_in_seconds = absl::GetFlag(FLAGS_trigger_capture_after);
    std::cout << "wait for " << time_to_wait_in_seconds << " seconds" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(time_to_wait_in_seconds));
    trigger_capture(mgr);

    std::cout << "Press Enter to exit" << std::endl;
    std::string input;
    if (std::getline(std::cin, input))
    {
        std::cout << "Exiting..." << std::endl;
    }

    return true;
}

bool clean_up_app_and_device(Dive::DeviceManager& mgr, const std::string& package)
{
    std::string serial = absl::GetFlag(FLAGS_device);

    if (serial.empty())
    {
        std::cout << "Please run with `--device [serial]` and `--package [package]` options."
                  << std::endl;
        print_usage();
        return false;
    }

    if (package.empty())
    {
        std::cout << "Package not provided. You run run with `--package [package]` options to "
                     "clean up package specific settings.";
    }

    return mgr.Cleanup(serial, package).ok();
}

bool process_input(Dive::DeviceManager& mgr)
{
    std::cout << "Press key t+enter to trigger a capture. \nPress any other key + enter to exit.";

    std::string input;
    while (std::getline(std::cin, input))
    {
        if (input == "t")
        {
            std::cout << "key t pressed " << std::endl;
            trigger_capture(mgr);
        }
        else
        {
            break;
        }
        std::cout << "Press key t+enter to trigger a capture. \nPress enter to exit.";
    }

    return true;
}

int main(int argc, char** argv)
{
    absl::SetProgramUsageMessage("Run app with --help for more details");
    absl::ParseCommandLine(argc, argv);
    std::string target_str = absl::GetFlag(FLAGS_target);
    Command     cmd = absl::GetFlag(FLAGS_command);
    std::string serial = absl::GetFlag(FLAGS_device);
    std::string package = absl::GetFlag(FLAGS_package);
    std::string vulkan_command = absl::GetFlag(FLAGS_vulkan_command);
    std::string vulkan_command_args = absl::GetFlag(FLAGS_vulkan_command_args);
    std::string app_type = absl::GetFlag(FLAGS_type);

    Dive::DeviceManager mgr;
    auto                list = mgr.ListDevice();
    if (list.empty())
    {
        std::cout << "No device connected" << std::endl;
        return 0;
    }

    switch (cmd)
    {
    case Command::kListDevice:
    {
        list_device(mgr);
        break;
    }
    case Command::kListPackage:
    {
        list_package(mgr, serial);
        break;
    }

    case Command::kRunPackage:
    {
        if (run_package(mgr, app_type, package, vulkan_command, vulkan_command_args))
        {
            process_input(mgr);
        }

        break;
    }

    case Command::kRunAndCapture:
    {
        run_and_capture(mgr, app_type, package, vulkan_command, vulkan_command_args);
        break;
    }
    case Command::kCleanup:
    {
        clean_up_app_and_device(mgr, package);
        break;
    }
    case Command::kNone:
    {
        print_usage();
        break;
    }
    }
}
