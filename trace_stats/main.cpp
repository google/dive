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
#include <iostream>
#include <optional>
#include <vector>
#include <sstream>
#include <numeric>
#include <set>
#include <array>

#include "dive_core/data_core.h"
#include "trace_stats.h"
#include "pm4_info.h"

int main(int argc, char **argv)
{
    Pm4InfoInit();

    // Handle args
    if ((argc != 2) && (argc != 3))
    {
        std::cout << "You need to call: lrz_validator <input_file_name.rd> "
                     "<output_details_file_name.txt>(optional)";
        return 0;
    }
    char *input_file_name = argv[1];

    std::string output_file_name = "";
    if (argc == 3)
    {
        output_file_name = argv[2];
    }

    // Load capture
    std::unique_ptr<Dive::DataCore> data_core = std::make_unique<Dive::DataCore>();
    Dive::CaptureData::LoadResult   load_res = data_core->LoadPm4CaptureData(input_file_name);
    if (load_res != Dive::CaptureData::LoadResult::kSuccess)
    {
        std::cout << "Loading capture \"" << input_file_name << "\" failed!";
        return 0;
    }
    std::cout << "Capture file \"" << input_file_name << "\" is loaded!\n";

    // Create meta data
    if (!data_core->CreatePm4MetaData())
    {
        std::cout << "Failed to create meta data!";
        return 0;
    }
    std::cout << "Gathering Stats...\n";

    std::ostream *ostream = &std::cout;
    std::ofstream ofstream;
    if (!output_file_name.empty())
    {
        std::cout << "Output details to \"" << output_file_name << "\"" << std::endl;
        ofstream.open(output_file_name);
        ostream = &ofstream;
    }

    // Gather Stats...
    const Dive::CaptureMetadata &meta_data = data_core->GetCaptureMetadata();
    Dive::CaptureStats           capture_stats;
    Dive::TraceStats             trace_stats;
    trace_stats.GatherTraceStats(meta_data, capture_stats);
    trace_stats.PrintTraceStats(capture_stats, *ostream);

    return 1;
}
