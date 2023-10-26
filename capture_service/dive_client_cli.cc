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

#include <chrono>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <string>
#include <thread>

#include "absl/flags/flag.h"
#include "absl/flags/internal/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "android_application.h"
#include "client.h"
#include "command_utils.h"
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
ABSL_FLAG(int,
          type,
          0,
          "application type: \n\t0 for OpenXR applications \n\t 1 for Vulkan applications");

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
        std::cout << ++index << " : " << device << std::endl;
    }

    return true;
}

bool list_package(Dive::DeviceManager& mgr, const std::string& device_serial)
{
    auto device = mgr.SelectDevice(device_serial);
    auto packages = device->ListPackage();

    int index = 0;
    std::cout << "Packages: " << std::endl;
    for (const auto& package : packages)
    {
        std::cout << ++index << " : " << package << std::endl;
    }

    return true;
}

bool run_package(Dive::DeviceManager& mgr, const std::string& package, const int app_type)
{
    std::string serial = absl::GetFlag(FLAGS_device);

    if (serial.empty() || package.empty())
    {
        print_usage();
        return false;
    }
    auto dev = mgr.SelectDevice(serial);
    if (app_type == 0)
        dev->SetupApp(package, Dive::ApplicationType::OPENXR);
    else if (app_type == 1)
        dev->SetupApp(package, Dive::ApplicationType::VULKAN);

    dev->StartApp();
    return true;
}

bool run_and_capture(Dive::DeviceManager& mgr, const std::string& package)
{

    std::string target_str = absl::GetFlag(FLAGS_target);
    int         app_type = absl::GetFlag(FLAGS_type);
    run_package(mgr, package, app_type);

    Dive::DiveClient client(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    std::this_thread::sleep_for(2000ms);
    std::cout << "TestConnection reply: " << client.TestConnection() << std::endl;

    std::string trace_file_path = client.RequestStartTrace();
    std::cout << "Trigger capture: " << trace_file_path << std::endl;
    std::this_thread::sleep_for(5s);

    mgr.GetDevice()->RetrieveTraceFile(trace_file_path, ".");

    std::this_thread::sleep_for(2000ms);
    std::cout << "TestConnection reply: " << client.TestConnection() << std::endl;

    return true;
}

bool clean_up_app_and_device(Dive::DeviceManager& mgr, const std::string& package)
{
    std::string serial = absl::GetFlag(FLAGS_device);

    if (serial.empty() || package.empty())
    {
        std::cout << "Please run with `--device [serial]` and `--package [package]` options."
                  << std::endl;
        print_usage();
        return false;
    }

    mgr.Cleanup(serial, package);
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
    int         app_type = absl::GetFlag(FLAGS_type);

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
        run_package(mgr, package, app_type);
        break;
    }

    case Command::kRunAndCapture:
    {
        run_and_capture(mgr, package);
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

    std::cout << "Press Enter to exit" << std::endl;
    std::string input;
    if (std::getline(std::cin, input))
    {
        std::cout << "Exiting..." << std::endl;
    }
}
