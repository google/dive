/*
Copyright 2025 Google Inc.

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

// Implementing a custom file processor is necessary to support these changes:
// - Loop a single frame for N times, or infinitely

#ifndef GFXRECON_DECODE_DIVE_FILE_PROCESSOR_H
#define GFXRECON_DECODE_DIVE_FILE_PROCESSOR_H

#include "decode/file_processor.h"

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

class DiveFileProcessor : public FileProcessor
{
  public:
    void SetLoopSingleFrameCount(uint64_t loop_single_frame_count);

  protected:
    bool ProcessFrameMarker(const format::BlockHeader& block_header,
                            format::MarkerType         marker_type,
                            bool&                      should_break) override;

    bool ProcessStateMarker(const format::BlockHeader& block_header, format::MarkerType marker_type) override;

  private:
    // Application will terminate after the single frame has been looped loop_single_frame_count_ times.
    // If 0, application will loop infinitely.
    uint64_t loop_single_frame_count_{ 0 };

    // Capture file offset of the marker that indicates the end of resources setup.
    int64_t state_end_marker_file_offset_{ 0 };
};

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)

#endif // GFXRECON_DECODE_DIVE_FILE_PROCESSOR_H
