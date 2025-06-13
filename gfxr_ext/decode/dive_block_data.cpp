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

#include <fstream>

#include "dive_block_data.h"

#include "third_party/gfxreconstruct/framework/util/logging.h"

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

bool DiveBlockData::AddOriginalBlock(size_t index, uint64_t offset)
{
    if (original_blocks_map_locked_)
    {
        GFXRECON_LOG_ERROR("Cannot add more original blocks to a locked DiveBlockData object");
        return false;
    }

    if (index != original_blocks_map_.size())
    {
        GFXRECON_LOG_ERROR("Unexpected block id mismatch with index: %d, expected index: %d",
                           index,
                           original_blocks_map_.size());
        return false;
    }

    original_blocks_map_.push_back(BlockBytesLocation{ offset, 0 });

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

    // Calculating block sizes
    original_header_size_bytes_ = original_blocks_map_[0].offset;
    for (size_t i = 0; i < original_blocks_map_.size() - 1; i++)
    {
        uint64_t size = original_blocks_map_[i + 1].offset - original_blocks_map_[i].offset;
        original_blocks_map_[i].size = size;
    }

    // The file processor calls AddOriginalBlock() even at the very end of the GFXR file, so this
    // last block has a size of 0 and its offset is equal to the file size. The info was used in the
    // calculation of the size of the penultimate block and now the last block needs to be trimmed.
    original_blocks_map_.pop_back();

    original_blocks_map_locked_ = true;
    return true;
}

bool DiveBlockData::UpdateNewBlockOrder()
{
    new_blocks_order_.clear();

    // TODO(chenangela): Insert modified blocks in the correct order, return false if an arrangement
    // can't be found
    for (uint32_t primary_index = 0; primary_index < original_blocks_map_.size(); primary_index++)
    {
        NewBlockMetadata block;
        block.is_original = true;
        block.id = primary_index;
        new_blocks_order_.push_back(block);
    }

    return true;
}

bool DiveBlockData::CopyBlockBetweenFiles(uint64_t bytes_left_to_copy,
                                          FILE*    original_fd,
                                          FILE*    new_fd)
{
    while (bytes_left_to_copy > 0)
    {
        uint64_t bytes_to_copy = bytes_left_to_copy;
        if (bytes_left_to_copy > kDiveBlockBufferSize)
        {
            bytes_to_copy = kDiveBlockBufferSize;
        }
        util::platform::FileRead(block_buffer_, bytes_to_copy, original_fd);
        util::platform::FileWrite(block_buffer_, bytes_to_copy, new_fd);

        if (bytes_left_to_copy < bytes_to_copy)
        {
            GFXRECON_LOG_ERROR("Miscalculation of bytes_left_to_copy: %d", bytes_left_to_copy);
            return false;
        }
        bytes_left_to_copy -= bytes_to_copy;
    }
    return true;
}

bool DiveBlockData::WriteGFXRFile(const std::string& original_file_path,
                                  const std::string& new_file_path)
{
    if (!original_blocks_map_locked_)
    {
        GFXRECON_LOG_ERROR("DiveBlockData original map must be finished before writing new file");
        return false;
    }

    bool res = UpdateNewBlockOrder();
    if (!res)
    {
        GFXRECON_LOG_ERROR("Cannot determine new blocks order");
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

    // Track block size relative to buffer size and recommend a larger buffer if required
    uint64_t max_block_size = original_header_size_bytes_;

    // Copy the original header
    if (!CopyBlockBetweenFiles(original_header_size_bytes_, original_fd, new_fd))
    {
        GFXRECON_LOG_ERROR("Could not copy header");
        return false;
    }

    // Write block by block
    for (size_t i = 0; i < new_blocks_order_.size(); i++)
    {
        NewBlockMetadata current_block_metadata = new_blocks_order_[i];

        if (current_block_metadata.is_original)
        {
            BlockBytesLocation current_block_location = original_blocks_map_.at(
            current_block_metadata.id);

            if (current_block_location.size > max_block_size)
            {
                max_block_size = current_block_location.size;
            }

            util::platform::FileSeek(original_fd,
                                     current_block_location.offset,
                                     util::platform::FileSeekSet);

            if (!CopyBlockBetweenFiles(current_block_location.size, original_fd, new_fd))
            {
                GFXRECON_LOG_ERROR("Could not copy original block id: %d",
                                   current_block_metadata.id);
                return false;
            }
        }
    }

    if (max_block_size > kDiveBlockBufferSize)
    {
        GFXRECON_LOG_WARNING("kDiveBlockBufferSize (%d) is too small to accommodate max block size "
                             "(%d) and will cause "
                             "more copy operations in CopyBlockBetweenFiles",
                             kDiveBlockBufferSize,
                             max_block_size);
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