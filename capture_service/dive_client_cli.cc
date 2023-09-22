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
#include <iostream>
#include <ostream>
#include <thread>

#include "absl/flags/flag.h"
#include "absl/flags/internal/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
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

    default: return absl::StrCat(command);
    }
}

ABSL_FLAG(Command,
          command,
          Command::kNone,
          "list of actions: \n list_device\n list_package\n run\n capture\n");
ABSL_FLAG(std::string, target, "localhost:19999", "Server address");
ABSL_FLAG(std::string, device, "", "Device serial");
ABSL_FLAG(std::string, package, "", "Package on the device");
ABSL_FLAG(std::string, activity, "", "Activity to run for the package");

void print_usage(const char* app)
{
    std::cout << "Usage: \n";
    std::cout << "\t" << app << "device_serial package_name [activity]" << std::endl;
}

bool list_device(Dive::DeviceManager& mgr)
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

bool list_package(Dive::DeviceManager& mgr, const std::string device_serial)
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

bool run_package(Dive::DeviceManager& mgr, const std::string& package, const std::string& activity)
{
    std::string serial = absl::GetFlag(FLAGS_device);

    if (serial.empty() || package.empty())
    {
        std::cout << "Please run with --device [serial] --package [package]" << std::endl;
        return false;
    }
    auto dev = mgr.SelectDevice(serial);
    dev->SetupApp(package);

    std::string target_activity = dev->GetMainActivity(package);
    if (!activity.empty())
    {
        if (target_activity != activity)
        {
            std::cout << "detected activity is " << target_activity << ", specified is " << activity
                      << std::endl;
            target_activity = activity;
        }
    }

    dev->StartApp(package, target_activity);
    return true;
}

bool run_and_capture(Dive::DeviceManager& mgr,
                     const std::string&   package,
                     const std::string&   activity)
{

    std::string target_str = absl::GetFlag(FLAGS_target);

    run_package(mgr, package, activity);

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

int main(int argc, char** argv)
{
    absl::SetProgramUsageMessage("Run app with --help for more details");
    absl::ParseCommandLine(argc, argv);
    std::string target_str = absl::GetFlag(FLAGS_target);
    Command     cmd = absl::GetFlag(FLAGS_command);
    if (cmd == Command::kNone)
    {
        std::cout << "Run " << argv[0] << " with --command " << std::endl;
        std::cout << absl::ProgramUsageMessage() << std::endl;
    }
    std::string serial = absl::GetFlag(FLAGS_device);
    std::string package = absl::GetFlag(FLAGS_package);
    std::string activity = absl::GetFlag(FLAGS_activity);

    Dive::DeviceManager mgr;

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
        run_package(mgr, package, activity);
        break;
    }

    case Command::kRunAndCapture:
    {
        run_and_capture(mgr, package, activity);
        break;
    }
    case Command::kNone:
    {
        print_usage(argv[0]);
        break;
    }
    }

    auto list = mgr.ListDevice();
    if (list.empty())
    {
        std::cout << "No device connected" << std::endl;
        return 0;
    }
    std::cout << "Press Enter to exit" << std::endl;
    std::string input;
    if (std::getline(std::cin, input))
    {
        std::cout << "exiting..." << std::endl;
    }
}
