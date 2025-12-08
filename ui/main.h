/*
 Copyright 2025 Google LLC

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

#include <string>

#include "ui/impl_pointer.h"

class DiveUIMain
{
public:
    struct TestOptions
    {
        std::string output;  // Output directory path
        std::string prefix;  // Filename prefix for test
        std::string scenario;
    };
    struct Impl;

    DiveUIMain(const DiveUIMain&) = delete;
    DiveUIMain(DiveUIMain&&) = delete;
    DiveUIMain& operator=(const DiveUIMain&) = delete;
    DiveUIMain& operator=(DiveUIMain&&) = delete;

    DiveUIMain(int argc, char** argv);
    ~DiveUIMain();

    void SetOptions(const TestOptions& options);

    int Run();

private:
    ImplPointer<Impl> m_impl;
};
