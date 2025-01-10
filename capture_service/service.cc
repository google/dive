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

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/strings/str_format.h"
#include "command_utils.h"
#include "constants.h"
#include "grpcpp/grpcpp.h"
#include "grpcpp/health_check_service_interface.h"
#include "log.h"
#include "trace_mgr.h"

ABSL_FLAG(uint16_t, port, 19999, "Server port for the service");

namespace Dive
{

grpc::Status DiveServiceImpl::StartTrace(grpc::ServerContext *context,
                                         const TraceRequest  *request,
                                         TraceReply          *reply)
{
    GetTraceMgr().TriggerTrace();
    GetTraceMgr().WaitForTraceDone();
    reply->set_trace_file_path(GetTraceMgr().GetTraceFilePath());
    return grpc::Status::OK;
}

grpc::Status DiveServiceImpl::TestConnection(grpc::ServerContext *context,
                                             const TestRequest   *request,
                                             TestReply           *reply)
{
    reply->set_message(request->message() + " received.");
    LOGD("TestConnection request received \n");
    return grpc::Status::OK;
}

grpc::Status DiveServiceImpl::RunCommand(grpc::ServerContext     *context,
                                         const RunCommandRequest *request,
                                         RunCommandReply         *reply)
{
    LOGD("Request command %s", request->command().c_str());
    auto result = ::Dive::RunCommand(request->command());
    if (result.ok())
    {
        reply->set_output(*result);
    }

    return grpc::Status::OK;
}

grpc::Status DiveServiceImpl::GetTraceFileMetaData(grpc::ServerContext       *context,
                                                   const FileMetaDataRequest *request,
                                                   FileMetaDataReply         *response)
{
    std::string target_file = request->name();
    std::cout << "request get metadata for file " << target_file << std::endl;

    response->set_name(target_file);

    if (!std::filesystem::exists(target_file))
    {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "");
    }

    int64_t file_size = std::filesystem::file_size(target_file);
    response->set_size(file_size);

    return grpc::Status::OK;
}

grpc::Status DiveServiceImpl::DownloadFile(grpc::ServerContext             *context,
                                           const DownLoadRequest           *request,
                                           grpc::ServerWriter<FileContent> *writer)
{
    std::string target_file = request->name();
    std::cout << "request to download file " << target_file << std::endl;

    if (!std::filesystem::exists(target_file))
    {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "");
    }
    int64_t file_size = std::filesystem::file_size(target_file);

    FileContent file_content;
    int64_t     total_read = 0;
    int64_t     cur_read = 0;

    std::ifstream fin(target_file, std::ios::binary);
    char          buff[kDownLoadFileChunkSize];
    while (!fin.eof())
    {
        file_content.clear_content();
        cur_read = fin.read(buff, kDownLoadFileChunkSize).gcount();
        total_read += cur_read;
        std::cout << "read " << cur_read << std::endl;
        file_content.set_content(std::string(buff, cur_read));
        writer->Write(file_content);
        if (cur_read != kDownLoadFileChunkSize)
            break;
    }
    std::cout << "Read done, file size " << file_size << ", actually send " << total_read
              << std::endl;
    fin.close();

    if (total_read != file_size)
    {
        std::cout << "file size " << file_size << ", actually send " << total_read << std::endl;
        return grpc::Status(grpc::StatusCode::INTERNAL, "");
    }

    return grpc::Status::OK;
}

std::unique_ptr<grpc::Server> &GetServer()
{
    static std::unique_ptr<grpc::Server> server = nullptr;
    return server;
}

void StopServer()
{
    auto &server = GetServer();
    if (server)
    {
        LOGI("StopServer at service.cc");
        server->Shutdown();
        server = nullptr;
    }
}

void RunServer(uint16_t port)
{
    LOGI("port is %d\n", port);
    std::string     server_address = absl::StrFormat("0.0.0.0:%d", port);
    DiveServiceImpl service;

    grpc::EnableDefaultHealthCheckService(true);
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

    builder.RegisterService(&service);
    auto &server = GetServer();
    server = builder.BuildAndStart();
    LOGI("Server listening on %s", server_address.c_str());
    server->Wait();
}

int ServerMain()
{
    RunServer(absl::GetFlag(FLAGS_port));
    return 0;
}

}  // namespace Dive