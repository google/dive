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
// - Assemble a modified GFXR file quickly with data chunks from the original file and from stored modifications

#ifndef GFXRECON_DECODE_DIVE_BLOCK_DATA_H
#define GFXRECON_DECODE_DIVE_BLOCK_DATA_H

#include "util/defines.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

static constexpr size_t kDiveBlockBufferSize = 4096;

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

class DiveSingleGfxrBlockInOriginalFile;
class DiveSingleGfxrBlockInBuffer;

// Abstract class representing a visitor for DiveSingleBlock objects
class BlockVisitor
{
  public:
    virtual bool Visit(DiveSingleGfxrBlockInOriginalFile& block) = 0;
    virtual bool Visit(DiveSingleGfxrBlockInBuffer& block)       = 0;
};

// A visitor that writes out a DiveSingleBlock into a provided file new_file_ptr_
class WriterBlockVisitor : public BlockVisitor
{
  public:
    WriterBlockVisitor(FILE* original_file_ptr, FILE* new_file_ptr) :
        original_file_ptr_(original_file_ptr), new_file_ptr_(new_file_ptr)
    {}
    ~WriterBlockVisitor() {}
    bool Visit(DiveSingleGfxrBlockInOriginalFile& block) override;
    bool Visit(DiveSingleGfxrBlockInBuffer& block) override;

  private:
    FILE* original_file_ptr_                 = nullptr;
    FILE* new_file_ptr_                      = nullptr;
    char  copy_buffer_[kDiveBlockBufferSize] = {};
};

// Abstract class representing a binary block encoded in .gfxr format
class DiveSingleBlock
{
  public:
    DiveSingleBlock() {}
    ~DiveSingleBlock() {}
    virtual bool Accept(BlockVisitor& visitor) = 0;
};

// Representing a gfxr-encoded block's position in the original file
class DiveSingleGfxrBlockInOriginalFile : public DiveSingleBlock
{
  public:
    DiveSingleGfxrBlockInOriginalFile() {}
    DiveSingleGfxrBlockInOriginalFile(uint64_t offset) : offset_(offset) {}
    ~DiveSingleGfxrBlockInOriginalFile() {}
    bool Accept(BlockVisitor& visitor) override { return visitor.Visit(*this); }

    uint64_t offset_ = 0;
    uint64_t size_   = 0;
};

// Representing a gfxr-encoded block in a buffer
// If blob_ptr is undefined, that is interpreted as an empty block
class DiveSingleGfxrBlockInBuffer : public DiveSingleBlock
{
  public:
    DiveSingleGfxrBlockInBuffer() {}
    DiveSingleGfxrBlockInBuffer(std::shared_ptr<char[]> blob_ptr, uint64_t size_in_bytes) :
        blob_ptr_(blob_ptr), size_in_bytes_(size_in_bytes)
    {}
    ~DiveSingleGfxrBlockInBuffer() {}
    bool Accept(BlockVisitor& visitor) override { return visitor.Visit(*this); }

    std::shared_ptr<char[]> blob_ptr_      = nullptr;
    uint64_t                size_in_bytes_ = 0;
};

class DiveBlockData
{
  public:
    // Add info for the next block in the original GFXR file
    bool AddOriginalBlock(size_t index, uint64_t offset);

    // Calculate block sizes, drop the file-end block and lock the map
    bool FinalizeOriginalBlocksMapSizes();
    bool IsOriginalBlocksMapLocked() { return original_blocks_map_locked_; }

    // Add or edit modifications
    bool
    AddModification(uint32_t primary_id, int32_t secondary_id, std::shared_ptr<char[]> blob_ptr, uint64_t blob_size);
    bool RemoveModification(uint32_t primary_id, int32_t secondary_id);
    void ClearAllModifications() { modifications_map_.clear(); }

    // Write modified GFXR file at the specified path
    bool WriteGFXRFile(const std::string& original_file_path, const std::string& new_file_path);

  private:
    // Info for the blocks in the original GFXR file
    std::vector<std::shared_ptr<DiveSingleGfxrBlockInOriginalFile>>
                                      original_blocks_map_        = {}; // Starting block index of 0
    DiveSingleGfxrBlockInOriginalFile original_header_block_      = {};
    bool                              original_blocks_map_locked_ = false;

    // Info for modifications
    //
    // The first key (uint32_t) is the primary_id which is the original_id. Valid values:
    // [0...original_blocks_map_.size()-1]
    //
    // The second key (int32_t) is the secondary_id which represents the position of
    // this modified block relative to the primary_id block, with negative values coming before the original block and
    // positive values after. A secondary_id of 0 represents a modification overwriting the original block.
    //
    // Each modification has an unique pair of primary_id and secondary_id.
    std::map<uint32_t, std::map<int32_t, std::shared_ptr<DiveSingleBlock>>> modifications_map_ = {};
};

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)

#endif // GFXRECON_DECODE_DIVE_BLOCK_DATA_H