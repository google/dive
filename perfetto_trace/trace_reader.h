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

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "gpu_slice_data.h"
#include "include/perfetto/trace_processor/trace_processor.h"

namespace Dive
{
namespace ptp = ::perfetto::trace_processor;

class TraceReader
{
public:
    TraceReader(const std::string& trace_file_path);
    bool        LoadTraceFile();
    bool        LoadTraceFileFromBuffer(const std::vector<uint8_t>& data);
    SurfaceData PopulatePerfettoTraceData();

private:
    std::string                          m_trace_file_path;
    std::unique_ptr<ptp::TraceProcessor> m_trace_processor;
};

}  // namespace Dive
