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

#pragma once

#include "dive_service.grpc.pb.h"

namespace Dive
{

class DiveServiceImpl final : public DiveService::Service
{
    grpc::Status StartTrace(grpc::ServerContext *context,
                            const TraceRequest  *request,
                            TraceReply          *reply) override;

    grpc::Status TestConnection(grpc::ServerContext *context,
                                const TestRequest   *request,
                                TestReply           *reply) override;

    grpc::Status RunCommand(grpc::ServerContext     *context,
                            const RunCommandRequest *request,
                            RunCommandReply         *reply) override;

    grpc::Status GetTraceFileMetaData(grpc::ServerContext       *context,
                                      const FileMetaDataRequest *request,
                                      FileMetaDataReply         *response) override;

    grpc::Status DownloadFile(grpc::ServerContext             *context,
                              const DownLoadRequest           *request,
                              grpc::ServerWriter<FileContent> *writer) override;
};

}  // namespace Dive