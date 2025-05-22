/*
 Copyright 2021 Google LLC

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

#include <sstream>

#ifdef WIN32
#    include <windows.h>
#else
#    include <cstdio>    // for fileno()
#    include <unistd.h>  // for isatty()
#endif

#include "cli.h"
#include "dive_core/common/dive_capture_format.h"

namespace Dive
{
namespace cli
{

void Init() {}

bool IsConsoleOutput()
{
#ifdef WIN32
    DWORD tmp;
    return GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &tmp);
#else
    return isatty(fileno(stdout)) != 0;
#endif
}

const char* RepositoryVersion()
{
#ifdef REPO_SHA1
    return REPO_SHA1;
#else
    return nullptr;
#endif
}

std::string FileFormatVersion()
{
    std::stringstream sst;
    sst << Dive::kCaptureMajorVersion << "." << Dive::kCaptureMinorVersion << "."
        << Dive::kCaptureRevision;
    return sst.str();
}

}  // namespace cli
}  // namespace Dive
