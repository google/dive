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

// Implementing a class to store GFXR file metadata is necessary to support these changes:
// - Assemble a modified GFXR file quickly with data chunks from the original file and from stored
// modifications

#ifndef GFXRECON_DECODE_DIVE_BLOCK_DATA_H
#define GFXRECON_DECODE_DIVE_BLOCK_DATA_H

#include "third_party/gfxreconstruct/framework/util/defines.h"

#include <string>
#include <vector>

static constexpr size_t kDiveBlockBufferSize = 4096;

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

class DiveBlockData
{
    struct BlockBytesLocation
    {
        uint64_t offset = 0;
        uint64_t size = 0;
    };

    struct NewBlockMetadata
    {
        bool     is_original = false;
        uint32_t id = 0;
    };

public:
    // Add info for the next block in the original GFXR file
    bool AddOriginalBlock(size_t index, uint64_t offset);

    // Calculate block sizes, drop the file-end block and lock the map
    bool FinalizeOriginalBlocksMapSizes();
    bool IsOriginalBlocksMapLocked() { return original_blocks_map_locked_; }

    // Write modified GFXR file at the specified path
    bool WriteGFXRFile(const std::string& original_file_path, const std::string& new_file_path);

private:
    // Determine the order of the blocks for the new file
    bool UpdateNewBlockOrder();

    // Copy a block from one file to another
    bool CopyBlockBetweenFiles(uint64_t bytes_left_to_copy, FILE* original_fd, FILE* new_fd);

    // Info for the blocks in the original GFXR file
    std::vector<BlockBytesLocation> original_blocks_map_ = {};  // Starting block index of 0
    uint64_t                        original_header_size_bytes_ = 0;
    bool                            original_blocks_map_locked_ = false;

    // Used in writing the new file with modifications
    std::vector<NewBlockMetadata> new_blocks_order_ = {};
    char                          block_buffer_[kDiveBlockBufferSize] = {};
};

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)

#endif  // GFXRECON_DECODE_DIVE_BLOCK_DATA_H