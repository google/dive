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

#include "dive_service.grpc.pb.h"
#include "grpcpp/grpcpp.h"

namespace Dive
{

class DiveClient
{
public:
    DiveClient(std::shared_ptr<grpc::Channel> channel) :
        m_stub(DiveService::NewStub(channel))
    {
    }

    std::string RequestStartTrace();

    std::string TestConnection();

    std::string RunCommand(const std::string &command);

private:
    std::unique_ptr<DiveService::Stub> m_stub;
};
}  // namespace Dive