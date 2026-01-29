/*
Copyright 2026 Google Inc.

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

#include "dive_core/gfxr_capture_data.h"

#include <fstream>
#include <iostream>

#include "absl/functional/any_invocable.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "gfxr_ext/decode/dive_block_data.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "third_party/gfxreconstruct/framework/format/format_util.h"

namespace Dive
{
namespace
{

using gfxrecon::decode::BlockVisitor;
using gfxrecon::decode::DiveBlockData;
using gfxrecon::decode::DiveModificationBlock;
using gfxrecon::decode::DiveOriginalBlock;
using gfxrecon::format::BlockHeader;
using gfxrecon::format::BlockType;

// NOTE: This is brittle since it needs to be updated every time there's a new BlockType
bool IsValidBlockType(BlockType block_type)
{
    return block_type == BlockType::kUnknownBlock || block_type == BlockType::kFrameMarkerBlock ||
           block_type == BlockType::kStateMarkerBlock || block_type == BlockType::kMetaDataBlock ||
           block_type == BlockType::kFunctionCallBlock || block_type == BlockType::kAnnotation ||
           block_type == BlockType::kMethodCallBlock ||
           block_type == BlockType::kCompressedMetaDataBlock ||
           block_type == BlockType::kCompressedFunctionCallBlock ||
           block_type == BlockType::kCompressedMethodCallBlock;
}

absl::StatusOr<BlockHeader> ReadBlockHeader(std::ifstream& gfxr_file)
{
    BlockHeader header = {.size = 0, .type = BlockType::kUnknownBlock};
    // NOTE: This is brittle since it relies on internal implementation details of the file format
    gfxr_file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (gfxr_file.fail())
    {
        return absl::InternalError("Could not read header");
    }
    return header;
}

class LambdaBlockVisitor : public BlockVisitor
{
 public:
    using OriginalBlockVisitor = absl::AnyInvocable<bool(const DiveOriginalBlock&)>;
    using ModificationBlockVisitor = absl::AnyInvocable<bool(const DiveModificationBlock&)>;

    explicit LambdaBlockVisitor(OriginalBlockVisitor handle_original_block,
                                ModificationBlockVisitor handle_modification_block)
        : handle_original_block_(std::move(handle_original_block)),
          handle_modification_block_(std::move(handle_modification_block))
    {
    }

    bool Visit(const DiveOriginalBlock& block) override { return handle_original_block_(block); }
    bool Visit(const DiveModificationBlock& block) override
    {
        return handle_modification_block_(block);
    }

 private:
    OriginalBlockVisitor handle_original_block_;
    ModificationBlockVisitor handle_modification_block_;
};

struct DiveBlockDataCounts
{
    int original_count = 0;
    int modified_count = 0;
    bool operator==(const DiveBlockDataCounts&) const = default;
};

DiveBlockDataCounts CountBlocks(const DiveBlockData& block_data)
{
    DiveBlockDataCounts counts{};
    LambdaBlockVisitor block_counter(
        [&counts](const DiveOriginalBlock& /*block*/) {
            ++counts.original_count;
            return true;
        },
        [&counts](const DiveModificationBlock& /*block*/) {
            ++counts.modified_count;
            return true;
        });
    // Ignore return since the visitor never return false
    block_data.TraverseBlocks(block_counter);
    return counts;
}

TEST(GfxrCaptureDataTest, LoadsAllBlocksIntoMemory)
{
    GfxrCaptureData capture_data;
    ASSERT_EQ(capture_data.LoadCaptureFile(TEST_DATA_DIR
                                           "/com.google.bigwheels.project_sample_01_triangle.debug_"
                                           "trim_trigger_20250718T132545.gfxr"),
              CaptureData::LoadResult::kSuccess);
    ASSERT_EQ(CountBlocks(*capture_data.GetMutableGfxrData()),
              (DiveBlockDataCounts{.original_count = 231, .modified_count = 0}));
}

TEST(GfxrCaptureDataTest, LastBlockIsProperlySized)
{
    GfxrCaptureData capture_data;
    ASSERT_EQ(capture_data.LoadCaptureFile(TEST_DATA_DIR
                                           "/com.google.bigwheels.project_sample_01_triangle.debug_"
                                           "trim_trigger_20250718T132545.gfxr"),
              CaptureData::LoadResult::kSuccess);

    const DiveOriginalBlock* last_block = nullptr;
    LambdaBlockVisitor find_last_original_block(
        [&last_block](const DiveOriginalBlock& block) {
            last_block = &block;
            return true;
        },
        [](const DiveModificationBlock& /*block*/) {
            ADD_FAILURE() << "Unexpected DiveModificationBlock";
            return false;
        });
    ASSERT_TRUE(capture_data.GetMutableGfxrData()->TraverseBlocks(find_last_original_block));

    ASSERT_NE(last_block, nullptr);
    ASSERT_EQ(last_block->size_, sizeof(gfxrecon::format::Marker));
}

TEST(GfxrCaptureDataTest, AllLoadedBlocksAreValid)
{
    constexpr const char* kTestFile = TEST_DATA_DIR
        "/com.google.bigwheels.project_sample_01_triangle.debug_"
        "trim_trigger_20250718T132545.gfxr";
    GfxrCaptureData capture_data;
    ASSERT_EQ(capture_data.LoadCaptureFile(kTestFile), CaptureData::LoadResult::kSuccess);

    std::ifstream gfxr_file(kTestFile, std::ios_base::binary);
    ASSERT_TRUE(gfxr_file.good());

    LambdaBlockVisitor block_validator(
        [&gfxr_file](const DiveOriginalBlock& block) {
            gfxr_file.seekg(block.offset_, std::ios_base::beg);
            absl::StatusOr<BlockHeader> header = ReadBlockHeader(gfxr_file);
            if (!header.ok())
            {
                ADD_FAILURE() << header.status().message();
                return false;
            }

            EXPECT_TRUE(IsValidBlockType(header->type));

            return true;
        },
        [&gfxr_file](const DiveModificationBlock& block) {
            ADD_FAILURE() << "Unexpected DiveModificationBlock";
            return false;
        });

    ASSERT_TRUE(capture_data.GetMutableGfxrData()->TraverseBlocks(block_validator));
}

}  // namespace
}  // namespace Dive
