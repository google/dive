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
#include "constants.h"
#include "device_mgr.h"

using namespace std::chrono_literals;

enum class Command
{
    kNone,
    kGfxrCapture,
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
    case Command::kGfxrCapture: return "gfxr_capture";
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
std::string,
device_architecture,
"x86",
"specify the device architecture to capture with gfxr (arm64-v8, armeabi-v7a, x86, or x86_64). If not specified, the default is x86.");
ABSL_FLAG(std::string,
          gfxr_capture_file_dir,
          "gfxr_capture",
          "specify the name of the directory for the gfxr capture. If not specified, the default "
          "file name is gfxr_capture.");
ABSL_FLAG(int,
          frame,
          -1,
          "specify which frame to capture with gfxr. If not specified, the default is -1.");
ABSL_FLAG(
std::string,
frame_range,
"",
"specify the range of frames to capture with gfxr. If not specified, the default is frame "
".");

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
                 const std::string&   command_args,
                 const std::string&   device_architecture,
                 const std::string&   gfxr_capture_directory,
                 const std::string&   gfxr_capture_frames,
                 bool                 is_gfxr_capture)
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
                            "",
                            device_architecture,
                            gfxr_capture_directory,
                            gfxr_capture_frames);
    }
    else if (app_type == "vulkan")
    {
        ret = dev->SetupApp(package,
                            Dive::ApplicationType::VULKAN_APK,
                            "",
                            device_architecture,
                            gfxr_capture_directory,
                            gfxr_capture_frames);
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
    auto ret = mgr.GetDevice()->RetrieveTrace(*trace_file_path,
                                              target_download_path.generic_string(),
                                              false);
    if (ret.ok())
        std::cout << "Capture saved at " << target_download_path << std::endl;
    else
        std::cout << "Failed to retrieve capture file" << std::endl;

    return ret.ok();
}

bool retrieve_gfxr_capture(Dive::DeviceManager& mgr, const std::string& gfxr_capture_directory)
{
    std::string           target_str = absl::GetFlag(FLAGS_target);
    std::string           download_path = absl::GetFlag(FLAGS_download_path);
    std::filesystem::path target_download_path(download_path);
    if (!std::filesystem::exists(target_download_path))
    {
        std::error_code ec;
        if (!std::filesystem::create_directories(target_download_path, ec))
        {
            std::cout << "error creating directory: " << ec << std::endl;
        }
    }
    std::string capture_directory = Dive::kGfxrCaptureDirectory + gfxr_capture_directory;
    auto        ret = mgr.GetDevice()->RetrieveTrace(capture_directory,
                                              target_download_path.generic_string(),
                                              true);
    if (ret.ok())
        std::cout << "GFXR capture directory saved at " << target_download_path << std::endl;
    else
        std::cout << "Failed to retrieve capture directory" << std::endl;
    return ret.ok();
}

bool run_and_capture(Dive::DeviceManager& mgr,
                     const std::string&   app_type,
                     const std::string&   package,
                     const std::string&   command,
                     const std::string&   command_args,
                     const std::string&   device_architecture,
                     const std::string&   gfxr_capture_directory,
                     const std::string&   gfxr_capture_frames,
                     const bool           is_gfxr_capture)
{

    std::string target_str = absl::GetFlag(FLAGS_target);
    run_package(mgr,
                app_type,
                package,
                command,
                command_args,
                device_architecture,
                gfxr_capture_directory,
                gfxr_capture_frames,
                is_gfxr_capture);
    int time_to_wait_in_seconds = absl::GetFlag(FLAGS_trigger_capture_after);
    std::cout << "wait for " << time_to_wait_in_seconds << " seconds" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(time_to_wait_in_seconds));

    if (is_gfxr_capture)
    {
        retrieve_gfxr_capture(mgr, gfxr_capture_directory);
    }
    else
    {
        trigger_capture(mgr);
    }

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
    std::string device_architecture = absl::GetFlag(FLAGS_device_architecture);
    std::string gfxr_capture_file_dir = absl::GetFlag(FLAGS_gfxr_capture_file_dir);
    std::string frame_range = absl::GetFlag(FLAGS_frame_range);
    int         frame = absl::GetFlag(FLAGS_frame);

    Dive::DeviceManager mgr;
    auto                list = mgr.ListDevice();
    if (list.empty())
    {
        std::cout << "No device connected" << std::endl;
        return 0;
    }

    switch (cmd)
    {
    case Command::kGfxrCapture:
    {
        std::string gfxr_frame_range;
        if (frame != -1 && frame_range != "")
        {
            std::cout << "Please specify either a single frame or a range of frames to capture "
                         "with GFXR not both."
                      << std::endl;
            break;
        }
        else if (frame != -1)
        {
            gfxr_frame_range = std::to_string(frame);
        }
        else if (frame_range != "")
        {
            gfxr_frame_range = frame_range;
        }
        else
        {
            std::cout
            << "Please specify either a single frame or a range of frames to capture with GFXR."
            << std::endl;
            break;
        }

        run_and_capture(mgr,
                        app_type,
                        package,
                        vulkan_command,
                        vulkan_command_args,
                        device_architecture,
                        gfxr_capture_file_dir,
                        gfxr_frame_range,
                        true);
        break;
    }
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
        if (run_package(mgr,
                        app_type,
                        package,
                        vulkan_command,
                        vulkan_command_args,
                        "",
                        "",
                        "",
                        false))
        {
            process_input(mgr);
        }

        break;
    }

    case Command::kRunAndCapture:
    {
        run_and_capture(mgr,
                        app_type,
                        package,
                        vulkan_command,
                        vulkan_command_args,
                        "",
                        "",
                        "",
                        false);
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
