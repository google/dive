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

// Implementing a custom file processor for Dive

#include "dive_file_processor.h"

#include "third_party/gfxreconstruct/framework/util/logging.h"

#include "dive_block_data.h"

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

// TODO GH #1195: frame numbering should be 1-based.
const uint32_t kFirstFrame = 0;

void DiveFileProcessor::SetLoopSingleFrameCount(uint64_t loop_single_frame_count)
{
    loop_single_frame_count_ = loop_single_frame_count;
}

void DiveFileProcessor::SetDiveBlockData(std::shared_ptr<DiveBlockData> p_block_data)
{
    dive_block_data_ = p_block_data;

    // When populating DiveBlockData we want to run through the entire file.
    run_without_decoders_ = true;
}

bool DiveFileProcessor::ProcessFrameMarker(const format::BlockHeader& block_header,
                                           format::MarkerType         marker_type,
                                           bool&                      should_break)
{
    // Read the rest of the frame marker data. Currently frame markers are not dispatched to
    // decoders.
    uint64_t frame_number = 0;
    bool     success = ReadBytes(&frame_number, sizeof(frame_number));

    if (success)
    {
        // Validate frame end marker's frame number matches first_frame_ when
        // capture_uses_frame_markers_ is true.
        GFXRECON_ASSERT((marker_type != format::kEndMarker) || (!UsesFrameMarkers()) ||
                        (frame_number == GetFirstFrame()));

        for (auto decoder : decoders_)
        {
            if (marker_type == format::kEndMarker)
            {
                decoder->DispatchFrameEndMarker(frame_number);
            }
            else
            {
                GFXRECON_LOG_WARNING("Skipping unrecognized frame marker with type %u",
                                     marker_type);
            }
        }
    }
    else
    {
        HandleBlockReadError(kErrorReadingBlockData, "Failed to read frame marker data");
    }

    // Break from loop on frame delimiter.
    if (IsFrameDelimiter(block_header.type, marker_type))
    {
        // If the capture file contains frame markers, it will have a frame marker for every
        // frame-ending API call such as vkQueuePresentKHR. If this is the first frame marker
        // encountered, reset the frame count and ignore frame-ending API calls in
        // IsFrameDelimiter(format::ApiCallId call_id).
        if (!UsesFrameMarkers())
        {
            SetUsesFrameMarkers(true);
            current_frame_number_ = kFirstFrame;
        }

        // Make sure to increment the frame number on the way out.
        ++current_frame_number_;
        ++block_index_;
        should_break = true;

        // At the last frame in the capture file, determine whether to jump back to the state end
        // marker, or terminate replay if the loop count has been reached
        if ((loop_single_frame_count_ > 0) && (current_frame_number_ >= loop_single_frame_count_))
        {
            GFXRECON_LOG_INFO("Looped %d frames, terminating replay asap", current_frame_number_);
            return success;
        }
        SeekActiveFile(state_end_marker_file_offset_, util::platform::FileSeekSet);
        should_break = false;
    }
    return success;
}

bool DiveFileProcessor::ProcessStateMarker(const format::BlockHeader& block_header,
                                           format::MarkerType         marker_type)
{
    bool success = FileProcessor::ProcessStateMarker(block_header, marker_type);

    if ((success) && (marker_type == format::kEndMarker))
    {
        // Store state end marker offset
        state_end_marker_file_offset_ = TellActiveFile();
        GFXRECON_LOG_INFO("Stored state end marker offset %d", state_end_marker_file_offset_);
        GFXRECON_LOG_INFO("Single frame number %d", GetFirstFrame());
    }

    return success;
}

void DiveFileProcessor::StoreBlockInfo()
{
    if (!dive_block_data_)
    {
        return;
    }

    int64_t offset = TellActiveFile();
    GFXRECON_ASSERT(offset > 0);
    dive_block_data_->AddOriginalBlock(block_index_, static_cast<uint64_t>(offset));
}

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)