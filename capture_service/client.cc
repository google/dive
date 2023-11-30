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

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "client.h"
#include "constants.h"
#include "dive_service.grpc.pb.h"
#include "dive_service.pb.h"
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

absl::StatusOr<std::string> DiveClient::RunCommand(const std::string& command)
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

absl::StatusOr<int64_t> DiveClient::GetTraceFileSize(const std::string& file_on_server)
{
    ClientContext       context;
    FileMetaDataRequest file_meta_request;
    FileMetaDataReply   file_meta_reply;
    file_meta_request.set_name(file_on_server);
    Status status = m_stub->GetTraceFileMetaData(&context, file_meta_request, &file_meta_reply);
    if (status.ok())
    {
        std::cout << "file " << file_meta_request.name() << ", size: " << file_meta_reply.size()
                  << std::endl;
        return file_meta_reply.size();
    }
    else
    {
        std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        return absl::UnavailableError(status.error_message());
    }
}

absl::Status DiveClient::DownloadFile(const std::string&           file_on_server,
                                      const std::string&           save_path,
                                      std::function<void(int64_t)> progress_callback)
{
    DownLoadRequest request;
    request.set_name(file_on_server);

    ClientContext context;

    std::ofstream                                    fout(save_path, std::ios::binary);
    int64_t                                          total_read = 0;
    FileContent                                      file_content;
    std::unique_ptr<grpc::ClientReader<FileContent>> reader(
    m_stub->DownloadFile(&context, request));
    char buff[kDownLoadFileChunkSize];
    while (reader->Read(&file_content))
    {
        fout << file_content.content();
        total_read += file_content.content().size();
        file_content.clear_content();
        if (progress_callback)
        {
            progress_callback(total_read);
        }
    }
    fout.close();

    Status status = reader->Finish();
    if (!status.ok())
    {
        return absl::UnavailableError(status.error_message());
    }
    return absl::OkStatus();
}

}  // namespace Dive