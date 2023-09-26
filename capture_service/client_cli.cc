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

    std::string reply = client.RequestStartTrace();
    std::cout << "Capture is save on device at: " << reply << std::endl;

    return 0;
}
