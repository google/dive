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

#include <cstdint>
#include <thread>

namespace DiveLayer
{

inline uintptr_t DataKey(const void *object)
{
    return (uintptr_t)(*(void **)object);
}

bool IsLibwrapLoaded();

class ServerRunner
{
public:
    ServerRunner();
    ~ServerRunner();

private:
    bool        is_libwrap_loaded;
    std::thread server_thread;
};

ServerRunner &GetServerRunner();

}  // namespace DiveLayer
