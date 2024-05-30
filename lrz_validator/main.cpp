/*
 Copyright 2024 Google LLC

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
#include <vector>

#include "dive_core/data_core.h"
#include "pm4_info.h"

int main(int argc, char** argv)
{
    Pm4InfoInit();
    if ((argc != 2) && (argc != 3))
    {
        std::cout
        << "You need to call: lrz_validator <input_file_name.rd> <output_file_name.txt>(optional)";
        return 0;
    }
    char*             input_file_name = argv[1];
    Dive::LogCompound log_compound;
    {
        std::unique_ptr<Dive::DataCore> data_core = std::make_unique<Dive::DataCore>(&log_compound);
        Dive::CaptureData::LoadResult   load_res = data_core->LoadCaptureData(input_file_name);
        if (load_res != Dive::CaptureData::LoadResult::kSuccess)
        {
            std::cout << "Loading capture " << argv[1] << " failed!";
            return 0;
        }
        std::string output_file_name = "lrz_validation_result.txt";
        if (argc == 3)
        {
            output_file_name = argv[2];
        }

        bool res = data_core->ValidateLRZ(output_file_name);
        if (!res)
        {
            std::cout << "Validate " << argv[1] << " failed!";
            return 0;
        }
    }

    return 1;
}
