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

#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "client.h"
#include "dive_service.grpc.pb.h"
#include "grpcpp/grpcpp.h"

ABSL_FLAG(std::string, target, "localhost:19999", "Server address");

int main(int argc, char **argv)
{
    absl::ParseCommandLine(argc, argv);
    std::string      target_str = absl::GetFlag(FLAGS_target);
    Dive::DiveClient client(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
    auto             test_res = client.TestConnection();
    if (test_res.ok())
    {
        std::cout << "Test connection succeed: " << *test_res << std::endl;
    }
    else
    {
        std::cerr << "Test connection failed: " << test_res.status().message() << std::endl;
        return -1;
    }

    auto reply = client.RequestStartTrace();
    if (!reply.ok())
    {
        std::cout << "RequestStartTrace failed " << reply.status().message() << std::endl;
        return -1;
    }
    std::cout << "Capture is save on device at: " << *reply << std::endl;
    auto res = client.DownloadFile(*reply, "capture.rd");
    if (!res.ok())
    {
        std::cout << "Download file failed " << res.message();
        return -1;
    }
    return 0;
}
