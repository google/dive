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
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "client.h"
#include "dive_service.grpc.pb.h"
#include "grpcpp/grpcpp.h"

namespace Dive
{
using grpc::ClientContext;
using grpc::Status;

absl::StatusOr<std::string> DiveClient::RequestStartTrace()
{
    TraceRequest  request;
    TraceReply    reply;
    ClientContext context;
    Status        status = m_stub->StartTrace(&context, request, &reply);

    if (status.ok())
    {
        return reply.trace_file_path();
    }
    else
    {
        std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        return absl::UnavailableError(status.error_message());
    }
}

absl::StatusOr<std::string> DiveClient::TestConnection()
{
    TestRequest   request;
    TestReply     reply;
    ClientContext context;
    request.set_message("Test connection request");
    Status status = m_stub->TestConnection(&context, request, &reply);

    if (status.ok())
    {
        return reply.message();
    }
    else
    {
        std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        return absl::UnavailableError(status.error_message());
    }
}

absl::StatusOr<std::string> DiveClient::RunCommand(const std::string &command)
{
    RunCommandRequest request;
    RunCommandReply   reply;
    ClientContext     context;
    request.set_command(command);
    Status status = m_stub->RunCommand(&context, request, &reply);

    if (status.ok())
    {
        return reply.output();
    }
    else
    {
        std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        return absl::UnavailableError(status.error_message());
    }
}
}  // namespace Dive