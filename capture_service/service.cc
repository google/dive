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

#include "service.h"

#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"
#include "trace_mgr.h"
#include "command_utils.h"
#include "grpcpp/grpcpp.h"
#include "grpcpp/health_check_service_interface.h"
#include "log.h"

ABSL_FLAG(uint16_t, port, 19999, "Server port for the service");

namespace Dive
{

grpc::Status DiveServiceImpl::StartTrace(grpc::ServerContext  *context,
                                           const TraceRequest *request,
                                           TraceReply         *reply)
{
    GetTraceMgr()->TriggerTrace();
    GetTraceMgr()->WaitForTraceDone();
    reply->set_trace_file_path(GetTraceMgr()->GetTraceFilePath());
    return grpc::Status::OK;
}

grpc::Status DiveServiceImpl::TestConnection(grpc::ServerContext *context,
                                             const TestRequest   *request,
                                             TestReply           *reply)
{
    reply->set_message("Hello" + request->message());
    LOGD("TestConnection request received \n");
    return grpc::Status::OK;
}

grpc::Status DiveServiceImpl::RunCommand(grpc::ServerContext     *context,
                                         const RunCommandRequest *request,
                                         RunCommandReply         *reply)
{
    LOGD("Request command %s", request->command().c_str());
    auto out = ::Dive::RunCommand(request->command());
    reply->set_output(out);

    return grpc::Status::OK;
}

void RunServer(uint16_t port)
{
    std::string     server_address = absl::StrFormat("0.0.0.0:%d", port);
    DiveServiceImpl service;

    grpc::EnableDefaultHealthCheckService(true);
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    LOGI("Server listening on %s", server_address.c_str());
    server->Wait();
}

int server_main()
{
    RunServer(absl::GetFlag(FLAGS_port));
    return 0;
}

}  // namespace Dive