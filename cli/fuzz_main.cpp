/*
 Copyright 2020 Google LLC

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

#include "dive_core/capture_data.h"
#include "dive_core/command_hierarchy.h"

//--------------------------------------------------------------------------------------------------
class membuf : public std::basic_streambuf<char>
{
public:
    membuf(const uint8_t *p, size_t l) { setg((char *)p, (char *)p, (char *)p + l); }
};

class memstream : public std::istream
{
public:
    memstream(const uint8_t *p, size_t l) :
        std::istream(&_buffer),
        _buffer(p, l)
    {
        rdbuf(&_buffer);
    }

private:
    membuf _buffer;
};

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    memstream capture_file(data, size);

    Dive::LogConsole                   log;
    std::unique_ptr<Dive::CaptureData> capture_data_ptr = std::make_unique<Dive::CaptureData>(&log);
    Dive::CaptureData::LoadResult      result = capture_data_ptr->LoadCaptureFile(capture_file);

    if (result != Dive::CaptureData::LoadResult::kSuccess)
    {
        return 1;
    }

    return 0;
}

#ifdef DIVE_FUZZ_LOADER
// Simple program to help debug Fuzz failures.
int main(int argc, char *argv[])
{
    auto f = fopen(argv[1], "rb");
    fseek(f, SEEK_END, 0);
    size_t size = ftell(f);
    fseek(f, SEEK_SET, 0);
    uint8_t *data = new uint8_t[size];
    fread(data, 1, size, f);
    fclose(f);

    auto res = LLVMFuzzerTestOneInput(data, size);

    delete[] data;
    return res;
}
#endif
