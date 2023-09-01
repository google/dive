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

#pragma once

#include "cli.h"
#include "dive_core/capture_data.h"
#include "dive_core/command_hierarchy.h"

namespace Dive
{
class DataCore;
namespace cli
{

typedef Dive::CaptureData::LoadResult LoadResult;

LoadResult PrintCaptureFileBlocks(std::ostream &out, const char *file_name);

void PrintNodes(std::ostream                 &out,
                const Dive::CommandHierarchy *command_hierarchy_ptr,
                const Dive::Topology         &topology,
                uint64_t                      node_index,
                bool                          verbose);

bool ParseCapture(const char                              *filename,
                  std::unique_ptr<Dive::CaptureData>      *out_capture_data,
                  std::unique_ptr<Dive::CommandHierarchy> *out_command_hierarchy);

void PrintHang(std::ostream         &out,
               const char           *dir,
               const Dive::DataCore &data,
               bool                  color,
               bool                  summary);

enum class TopologyName
{
    kTopologyUnknown,
    kTopologyEngine,
    kTopologySubmit,
    kTopologyEvent
};

int PrintTopology(const char *filename, TopologyName topology, bool verbose);

//--------------------------------------------------------------------------------------------------
// Miscellaneous
const char *GetOpCodeStringSafe(uint32_t op_code);

//--------------------------------------------------------------------------------------------------

LoadResult  ReadCaptureDataHeader(const char *file_name, Dive::CaptureDataHeader *header);
const char *CaptureTypeToString(Dive::CaptureDataHeader::CaptureType type);

}  // namespace cli
}  // namespace Dive
