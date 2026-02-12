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

#include <fstream>

#include "capture_service/constants.h"
#include "capture_service/remote_files.h"
#include "dive_block_data.h"
#include "dive_pm4_capture.h"
#include "dive_renderdoc.h"
#include "util/logging.h"
#include "util/platform.h"

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

namespace
{

bool ShouldCreateRenderDocCapture()
{
    std::string property = util::platform::GetEnv(Dive::kReplayCreateRenderDocCapture);
    // Be a little more generous with the values accepted to avoid frustrating or confusing the
    // user.
    return property == "true" || property == "1";
}

}  // namespace

void DiveFileProcessor::SetLoopSingleFrameCount(uint64_t loop_single_frame_count)
{
    loop_single_frame_count_ = loop_single_frame_count;
    GFXRECON_LOG_INFO("Setting DiveFileProcessor::loop_single_frame_count_: %d",
                      loop_single_frame_count);
}

void DiveFileProcessor::SetDiveBlockData(std::shared_ptr<DiveBlockData> p_block_data)
{
    dive_block_data_ = p_block_data;

    // When populating DiveBlockData we want to run through the entire file.
    run_without_decoders_ = true;
}

bool DiveFileProcessor::WriteFile(const std::string& name, const std::string& content)
{
    std::string new_file_path = absolute_path_ + "/" + name;

    FILE* fd = nullptr;
    int result = util::platform::FileOpen(&fd, new_file_path.c_str(), "wb");
    if (result || fd == nullptr)
    {
        GFXRECON_LOG_ERROR("Failed to open file %s, exit code: %d", new_file_path.c_str(), result);
        return false;
    }

    bool res = util::platform::FilePuts(content.c_str(), fd);
    if (!res)
    {
        GFXRECON_LOG_ERROR("Could not write file: %s", new_file_path.c_str());
    }

    GFXRECON_LOG_INFO("Wrote file: %s", new_file_path.c_str());

    result = util::platform::FileClose(fd);
    if (result)
    {
        GFXRECON_LOG_ERROR("Failed to close file %s, exit code: %d", new_file_path.c_str(), result);
        return false;
    }

    return true;
}

bool DiveFileProcessor::ProcessFrameDelimiter(const FrameEndMarkerArgs& end_frame)
{
    // current_frame_number_ increments during single frame looping despite the frame number staying
    // the same. To avoid triggering an assert in FileProcessor::ProcessFrameDelimiter (and to match
    // previous behavior), set current_frame_number_ to the expected value.
    //
    // TODO: b/481393648 - Set current_frame_number_ to 0 instead after investigation
    auto loop_current_frame_number = current_frame_number_;
    current_frame_number_ = end_frame.frame_number - GetFirstFrame();
    bool is_frame_delimiter = FileProcessor::ProcessFrameDelimiter(end_frame);
    current_frame_number_ = loop_current_frame_number;

#if defined(__ANDROID__)
    if (DivePM4Capture::GetInstance().IsPM4CaptureEnabled())
    {
        DivePM4Capture::GetInstance().TryStopCapture();
    }
#endif

    // At the last frame in the capture file, determine whether to jump back to the state end
    // marker, or terminate replay if the loop count has been reached
    bool finite_looping = loop_single_frame_count_ > 0;
    // Reaching the Frame End marker means that we've completed one loop. current_frame_number_
    // will be incremented by 1 each loop, but that is handled by FileProcessor after we return.
    uint64_t loops_done = current_frame_number_ + 1;
    if (finite_looping && (loops_done >= loop_single_frame_count_))
    {
        if (ShouldCreateRenderDocCapture())
        {
            // Finalize the RenderDoc capture after all loops have completed. While the intended
            // use case is to capture only 1 frame (which can be accomplished by setting
            // loop_single_frame_count_), I don't see any reason to prevent the user from
            // capturing all loops if they really want to.
            if (const RENDERDOC_API_1_0_0* renderdoc = GetRenderDocApi(); renderdoc != nullptr)
            {
                if (renderdoc->EndFrameCapture(/*device=*/nullptr, /*wndHandle=*/nullptr) != 1)
                {
                    GFXRECON_LOG_WARNING(
                        "EndFrameCapture failed, RenderDoc .rdc capture likely not created!");
                }
            }
            else
            {
                GFXRECON_LOG_WARNING("GetRenderDocApi failed. Could not end RenderDoc capture!");
            }
        }

        GFXRECON_LOG_INFO("Looped %d frames, terminating replay asap", loop_single_frame_count_);
        // The act of not seeking should cause replay to hit EOF and stop (assuming there is only
        // one frame in the capture file)
        return is_frame_delimiter;
    }

    // The block index is printed by --pbi-all. It helps correlate problematic blocks with manual
    // inspection of capture file. Reset it to the loop point to make debugging easier.
    block_index_ = state_end_marker_block_index_;

    std::shared_ptr<FileInputStream> gfxr_file = gfxr_file_.lock();
    GFXRECON_ASSERT(gfxr_file);
    SeekActiveFile(gfxr_file, state_end_marker_file_offset_, util::platform::FileSeekSet);

    return is_frame_delimiter;
}

void DiveFileProcessor::ProcessStateEndMarker(const StateEndMarkerArgs& state_end)
{
    FileProcessor::ProcessStateEndMarker(state_end);

    // Store state end marker offset
    std::shared_ptr<FileInputStream> gfxr_file = gfxr_file_.lock();
    GFXRECON_ASSERT(gfxr_file);
    state_end_marker_file_offset_ = gfxr_file->FileTell();
    // The block index is useful while debugging to match the call stack or --pbi output with the
    // capture file. If it's not reset to the loop point then it just keeps counting up.
    state_end_marker_block_index_ = block_index_;
    GFXRECON_LOG_INFO("Stored state end marker offset %d", state_end_marker_file_offset_);
    GFXRECON_LOG_INFO("Single frame number %d", GetFirstFrame());
#if defined(__ANDROID__)
    if (DivePM4Capture::GetInstance().IsPM4CaptureEnabled())
    {
        DivePM4Capture::GetInstance().TryStartCapture();
    }
    // Tell other processes that replay has finished trim state loading.
    // TODO: b/444647876 - Implementation that doesn't use global state (filesystem)
    if (!std::ofstream(Dive::kReplayStateLoadedSignalFile))
    {
        GFXRECON_LOG_INFO(
            "Failed to create a file signaling that trim state loading is "
            "complete. This will impact our ability to gather metrics.");
    }
#endif
    // Don't bother trying to start a capture for infinite replay since it will never finish!
    if (loop_single_frame_count_ > 0 && ShouldCreateRenderDocCapture())
    {
        if (const RENDERDOC_API_1_0_0* renderdoc = GetRenderDocApi(); renderdoc != nullptr)
        {
            renderdoc->SetCaptureFilePathTemplate(
                Dive::GetRenderDocCaptureFilePathTemplate(gfxr_file->GetFilename())
                    .string()
                    .c_str());
            // Let RenderDoc choose the Vulkan context and window handle since we typically only
            // expect one of each.
            renderdoc->StartFrameCapture(/*device=*/nullptr, /*wndHandle=*/nullptr);
        }
        else
        {
            GFXRECON_LOG_DEBUG("GetRenderDocApi failed! Could not start RenderDoc capture.");
        }
    }
}

void DiveFileProcessor::StoreBlockInfo()
{
    std::shared_ptr<FileInputStream> gfxr_file = gfxr_file_.lock();
    if (!gfxr_file)
    {
        // Assuming that the first time StoreBlockInfo() is called, the active file is .gfxr file
        gfxr_file = file_stack_.back().active_file;
        gfxr_file_ = gfxr_file;
        GFXRECON_LOG_INFO("Storing active filename %s", gfxr_file->GetFilename().c_str());
    }

    if (!dive_block_data_)
    {
        return;
    }

    int64_t offset = gfxr_file->FileTell();
    GFXRECON_ASSERT(offset > 0);
    dive_block_data_->AddOriginalBlock(block_index_, static_cast<uint64_t>(offset));
}

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)
