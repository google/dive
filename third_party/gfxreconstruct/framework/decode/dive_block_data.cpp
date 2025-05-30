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

#include "decode/dive_block_data.h"
#include "util/logging.h"

#include <fstream>
#include <memory>

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

bool WriterBlockVisitor::Visit(DiveSingleGfxrBlockInOriginalFile& block)
{
    util::platform::FileSeek(original_file_ptr_, block.offset_, util::platform::FileSeekSet);
    int32_t bytes_left_to_copy = block.size_;
    while (bytes_left_to_copy > 0)
    {
        int32_t bytes_to_copy = bytes_left_to_copy;
        if (bytes_left_to_copy > kDiveBlockBufferSize)
        {
            bytes_to_copy = kDiveBlockBufferSize;
        }
        if (!util::platform::FileRead(copy_buffer_, bytes_to_copy, original_file_ptr_))
        {
            GFXRECON_LOG_ERROR("Copying original block, could not read from original file");
            return false;
        }
        if (!util::platform::FileWrite(copy_buffer_, bytes_to_copy, new_file_ptr_))
        {
            GFXRECON_LOG_ERROR("Copying original block, could not write to new file");
            return false;
        }
        bytes_left_to_copy -= bytes_to_copy;
        if (bytes_left_to_copy < 0)
        {
            GFXRECON_LOG_ERROR("Copying original block, miscalculation of bytes_left_to_copy: %d", bytes_left_to_copy);
            return false;
        }
    }
    return true;
}

bool WriterBlockVisitor::Visit(DiveSingleGfxrBlockInBuffer& block)
{
    if (!util::platform::FileWrite(copy_buffer_, block.size_in_bytes_, new_file_ptr_))
    {
        GFXRECON_LOG_ERROR("Writing modified block, could not write to new file");
        return false;
    }
    return true;
}

bool DiveBlockData::AddOriginalBlock(size_t index, uint64_t offset)
{
    if (original_blocks_map_locked_)
    {
        GFXRECON_LOG_ERROR("Cannot add more original blocks to a locked DiveBlockData object");
        return false;
    }

    if (index != original_blocks_map_.size())
    {
        GFXRECON_LOG_ERROR(
            "Unexpected block id mismatch with index: %d, expected index: %d", index, original_blocks_map_.size());
        return false;
    }

    original_blocks_map_.emplace_back(std::make_shared<DiveSingleGfxrBlockInOriginalFile>(offset));

    return true;
}

bool DiveBlockData::FinalizeOriginalBlocksMapSizes()
{
    if (original_blocks_map_locked_)
    {
        return true;
    }

    if (original_blocks_map_.empty())
    {
        GFXRECON_LOG_ERROR("Original block map is empty");
        return false;
    }

    // Calculating block size for header (before block id 0)
    original_header_block_.offset_ = 0;
    original_header_block_.size_   = original_blocks_map_[0]->offset_;

    // Calculating block sizes
    for (size_t i = 0; i < original_blocks_map_.size() - 1; i++)
    {
        uint64_t size = original_blocks_map_[i + 1]->offset_ - original_blocks_map_[i]->offset_;
        if (size > kDiveBlockBufferSize)
        {
            GFXRECON_LOG_WARNING("Original block with id (%d) is larger than kDiveBlockBufferSize: %d > %d, will cause "
                                 "more copy operations when writing new file",
                                 i,
                                 size,
                                 kDiveBlockBufferSize);
        }

        original_blocks_map_[i]->size_ = size;
    }

    // The file processor calls AddOriginalBlock() even at the very end of the GFXR file, so this last block has a size
    // of 0 and its offset is equal to the file size. The info was used in the calculation of the size of the
    // penultimate block and now the last block needs to be trimmed.
    original_blocks_map_.pop_back();

    original_blocks_map_locked_ = true;
    return true;
}

bool DiveBlockData::AddModification(uint32_t                primary_id,
                                    int32_t                 secondary_id,
                                    std::shared_ptr<char[]> blob_ptr,
                                    uint64_t                blob_size)
{
    if (!original_blocks_map_locked_)
    {
        GFXRECON_LOG_ERROR("Cannot make modifications before original file is processed");
        return false;
    }

    if (primary_id >= original_blocks_map_.size())
    {
        GFXRECON_LOG_ERROR("Primary index (%d) is out of bounds, largest original block id: %d",
                           primary_id,
                           original_blocks_map_.size() - 1);
        return false;
    }

    // The only time an empty blob is used is to indicate a deletion modficiation of the original block
    if (secondary_id != 0)
    {
        if ((blob_size == 0) || (blob_ptr == nullptr))
            GFXRECON_LOG_ERROR("Invalid blob provided for modification at: (%d, %d)", primary_id, secondary_id);
        return false;
    }

    if ((modifications_map_.count(primary_id) > 0) && (modifications_map_[primary_id].count(secondary_id) > 0))
    {
        GFXRECON_LOG_ERROR(
            "Modified block already exists, please remove or clear first: (%d, %d)", primary_id, secondary_id);
        return false;
    }

    auto new_block_ptr = std::make_shared<DiveSingleGfxrBlockInBuffer>(blob_ptr, blob_size);

    modifications_map_[primary_id][secondary_id] = std::move(new_block_ptr);

    return true;
}

bool DiveBlockData::RemoveModification(uint32_t primary_id, int32_t secondary_id)
{
    if ((modifications_map_.count(primary_id) > 0) && (modifications_map_[primary_id].count(secondary_id) > 0))
    {
        modifications_map_[primary_id].erase(secondary_id);
        return false;
    }

    GFXRECON_LOG_ERROR("No modified block at: (%d, %d), cannot remove", primary_id, secondary_id);
    return false;
}

bool DiveBlockData::WriteGFXRFile(const std::string& original_file_path, const std::string& new_file_path)
{
    if (!original_blocks_map_locked_)
    {
        GFXRECON_LOG_ERROR("DiveBlockData original map must be finished before writing new file");
        return false;
    }

    FILE* original_fd;
    int   result = util::platform::FileOpen(&original_fd, original_file_path.c_str(), "rb");
    if (result || original_fd == nullptr)
    {
        GFXRECON_LOG_ERROR("Failed to open file %s", original_file_path.c_str());
        return false;
    }

    FILE* new_fd;
    result = util::platform::FileOpen(&new_fd, new_file_path.c_str(), "wb");
    if (result || new_fd == nullptr)
    {
        GFXRECON_LOG_ERROR("Failed to open file %s", new_file_path.c_str());
        return false;
    }

    WriterBlockVisitor writer = { original_fd, new_fd };

    // Copy the original header
    if (!original_header_block_.Accept(writer))
    {
        GFXRECON_LOG_ERROR("Could not copy header");
        return false;
    }

    // Go through block-by-block in order of primary_id
    for (uint32_t primary_id = 0; primary_id < original_blocks_map_.size(); primary_id++)
    {
        std::map<int32_t, std::shared_ptr<DiveSingleBlock>> blocks_to_write = {};
        if (modifications_map_.count(primary_id) > 0)
        {
            // Copy all the modifications relating to the original block with primary_id
            blocks_to_write = modifications_map_[primary_id];
        }

        // If there is no modification of the original block, insert a pointer to the original block into the map of
        // blocks ordered by secondary_id
        if (blocks_to_write.count(0) == 0)
        {
            blocks_to_write[0] = original_blocks_map_[primary_id];
        }

        // For a given primary_id, go through block-by-block in order of secondary_id
        for (auto it = blocks_to_write.begin(); it != blocks_to_write.end(); ++it)
        {
            int32_t secondary_id = it->first;

            auto current_block = it->second;

            if (!current_block->Accept(writer))
            {
                GFXRECON_LOG_ERROR("Couldn't write block with ids (%d, %d)", primary_id, secondary_id);
                return false;
            }
        }
    }

    if (util::platform::FileClose(original_fd))
    {
        GFXRECON_LOG_ERROR("Failed to close file %s", original_file_path.c_str());
        return false;
    }

    if (util::platform::FileClose(new_fd))
    {
        GFXRECON_LOG_ERROR("Failed to close file %s", new_file_path.c_str());
        return false;
    }

    GFXRECON_LOG_INFO("Wrote new gfxr file: %s", new_file_path.c_str());
    return true;
}

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)