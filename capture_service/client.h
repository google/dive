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

#include <memory>

#include "absl/status/statusor.h"
#include "dive_service.grpc.pb.h"
#include "grpcpp/grpcpp.h"

namespace Dive
{

class DiveClient
{
public:
    DiveClient(std::shared_ptr<grpc::Channel> channel) : m_stub(DiveService::NewStub(channel)) {}

    absl::StatusOr<std::string> RequestStartTrace();

    absl::StatusOr<std::string> TestConnection();

    absl::StatusOr<std::string> RunCommand(const std::string& command);
    absl::StatusOr<int64_t>     GetTraceFileSize(const std::string& file_on_server);
    absl::Status                DownloadFile(const std::string&           file_on_server,
                                             const std::string&           save_path,
                                             std::function<void(int64_t)> progress_callback = nullptr);

private:
    std::unique_ptr<DiveService::Stub> m_stub;
};
}  // namespace Dive