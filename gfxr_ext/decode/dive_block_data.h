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

#include "util/defines.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

static constexpr size_t kDiveBlockBufferSize = 4096;

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

class DiveOriginalBlock;
class DiveModificationBlock;

// Abstract class representing a visitor for IDiveBlock objects
class BlockVisitor
{
public:
    virtual bool Visit(const DiveOriginalBlock& block) = 0;
    virtual bool Visit(const DiveModificationBlock& block) = 0;
};

// A visitor that tracks the order of visited blocks for testing
class TestBlockVisitor : public BlockVisitor
{
public:
    bool                     Visit(const DiveOriginalBlock& block) override;
    bool                     Visit(const DiveModificationBlock& block) override;
    std::vector<std::string> GetTraversedPathString();

private:
    std::vector<std::string> traversed_ = {};
};

// A visitor that writes out a IDiveBlock into a provided file new_file_ptr_
class WriterBlockVisitor : public BlockVisitor
{
public:
    WriterBlockVisitor(FILE* original_file_ptr, FILE* new_file_ptr) :
        original_file_ptr_(original_file_ptr),
        new_file_ptr_(new_file_ptr)
    {
    }
    ~WriterBlockVisitor() {}
    bool Visit(const DiveOriginalBlock& block) override;
    bool Visit(const DiveModificationBlock& block) override;

private:
    FILE* original_file_ptr_ = nullptr;
    FILE* new_file_ptr_ = nullptr;
    char  copy_buffer_[kDiveBlockBufferSize] = {};
};

// Abstract class representing a single binary block encoded in .gfxr format
class IDiveBlock
{
public:
    IDiveBlock() {}
    virtual ~IDiveBlock() {}
    virtual bool Accept(BlockVisitor& visitor) const = 0;
};

// Representing a gfxr-encoded block with data stored in the original file
class DiveOriginalBlock : public IDiveBlock
{
public:
    DiveOriginalBlock() {}
    DiveOriginalBlock(uint64_t offset) :
        offset_(offset)
    {
    }
    virtual ~DiveOriginalBlock() {}
    bool Accept(BlockVisitor& visitor) const override { return visitor.Visit(*this); }

    uint64_t offset_ = 0;
    uint64_t size_ = 0;
};

// Representing a gfxr-encoded block with data stored in a buffer
// If blob_ptr is undefined, that is interpreted as an empty block
// blob_ptr_ is expected to point at a non-empty vector at the time of traversal
class DiveModificationBlock : public IDiveBlock
{
public:
    DiveModificationBlock() {}
    DiveModificationBlock(std::shared_ptr<std::vector<char>> blob_ptr) :
        blob_ptr_(blob_ptr)
    {
    }
    virtual ~DiveModificationBlock() {}
    bool Accept(BlockVisitor& visitor) const override { return visitor.Visit(*this); }

    std::shared_ptr<std::vector<char>> blob_ptr_ = nullptr;
};

class DiveBlockData
{
public:
    // Add info for the next block in the original GFXR file
    bool AddOriginalBlock(size_t index, uint64_t offset);

    // Calculate block sizes, drop the file-end block and lock the map
    bool FinalizeOriginalBlocksMapSizes();
    bool IsOriginalBlocksMapLocked() const { return original_blocks_map_locked_; }

    // Add or edit modifications
    bool ModificationExists(uint32_t primary_id, int32_t secondary_id) const;
    bool AddModification(uint32_t                           primary_id,
                         int32_t                            secondary_id,
                         std::shared_ptr<std::vector<char>> blob_ptr);
    bool RemoveModification(uint32_t primary_id, int32_t secondary_id);
    void ClearAllModifications() { modifications_map_.clear(); }

    // Write modified GFXR file at the specified path
    bool TraverseBlocks(BlockVisitor& visitor) const;
    bool WriteGFXRFile(const std::string& original_file_path,
                       const std::string& new_file_path) const;

private:
    // Info for the blocks in the original GFXR file
    std::vector<std::shared_ptr<DiveOriginalBlock>>
                      original_blocks_map_ = {};  // Starting block index of 0
    DiveOriginalBlock original_header_block_ = {};
    bool              original_blocks_map_locked_ = false;

    // Info for modifications
    //
    // The first key (uint32_t) is the primary_id which is the original_id. Valid values:
    // [0...original_blocks_map_.size()-1]
    //
    // The second key (int32_t) is the secondary_id which represents the position of
    // this modified block relative to the primary_id block, with negative values coming before the
    // original block and positive values after. A secondary_id of 0 represents a modification
    // overwriting the original block, and only these modifications are allowed to have a value of
    // nullptr.
    //
    // Each modification has an unique pair of primary_id and secondary_id.
    std::map<uint32_t, std::map<int32_t, std::shared_ptr<IDiveBlock>>> modifications_map_ = {};
};

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)

#endif  // GFXRECON_DECODE_DIVE_BLOCK_DATA_H