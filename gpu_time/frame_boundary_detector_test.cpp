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

#include "frame_boundary_detector.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace Dive
{

namespace
{

// Mock Vulkan Handles for testing
#define MOCK_HANDLE(type, name, val) \
    const type name = reinterpret_cast<type>(static_cast<uintptr_t>(val));

MOCK_HANDLE(VkCommandPool, MOCK_POOL_1, 0x100);
MOCK_HANDLE(VkCommandPool, MOCK_POOL_2, 0x200);

MOCK_HANDLE(VkCommandBuffer, MOCK_CMD_1, 0x101);
MOCK_HANDLE(VkCommandBuffer, MOCK_CMD_2, 0x102);
MOCK_HANDLE(VkCommandBuffer, MOCK_CMD_SECONDARY, 0x103);

class FrameBoundaryDetectorTest : public ::testing::Test
{
 protected:
    FrameBoundaryDetector detector;

    // Helper to quickly allocate a command buffer in the detector
    void AllocateCmd(VkCommandBuffer cmd, VkCommandPool pool,
                     VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY)
    {
        VkCommandBufferAllocateInfo alloc_info = {};
        alloc_info.commandPool = pool;
        alloc_info.level = level;
        alloc_info.commandBufferCount = 1;
        detector.OnAllocateCommandBuffers(&alloc_info, &cmd);
    }

    // Helper to create a valid VR boundary label
    VkDebugUtilsLabelEXT GetValidBoundaryLabel()
    {
        VkDebugUtilsLabelEXT label = {};
        label.pLabelName = FrameBoundaryDetector::kVulkanVrFrameDelimiterString;
        return label;
    }
};

TEST_F(FrameBoundaryDetectorTest, TracksPrimaryCommandBuffers)
{
    AllocateCmd(MOCK_CMD_1, MOCK_POOL_1);

    // It should be tracked, but initially NOT a boundary
    EXPECT_FALSE(detector.IsFrameBoundary(MOCK_CMD_1));

    // We can verify it is tracked by successfully marking it
    auto label = GetValidBoundaryLabel();
    EXPECT_TRUE(detector.MarkBoundary(MOCK_CMD_1, &label).success);
    EXPECT_TRUE(detector.IsFrameBoundary(MOCK_CMD_1));
}

TEST_F(FrameBoundaryDetectorTest, IgnoresSecondaryCommandBuffers)
{
    AllocateCmd(MOCK_CMD_SECONDARY, MOCK_POOL_1, VK_COMMAND_BUFFER_LEVEL_SECONDARY);

    // It should NOT be tracked. Marking it should fail.
    auto label = GetValidBoundaryLabel();
    auto status = detector.MarkBoundary(MOCK_CMD_SECONDARY, &label);

    EXPECT_FALSE(status.success);
    EXPECT_THAT(status.message, ::testing::HasSubstr("not in the cmd cache"));
}

TEST_F(FrameBoundaryDetectorTest, FreeingRemovesCommandBuffer)
{
    AllocateCmd(MOCK_CMD_1, MOCK_POOL_1);

    // Free it
    detector.OnFreeCommandBuffers(1, &MOCK_CMD_1);

    // Marking it should now fail because it was removed from the cache
    auto label = GetValidBoundaryLabel();
    EXPECT_FALSE(detector.MarkBoundary(MOCK_CMD_1, &label).success);
}

TEST_F(FrameBoundaryDetectorTest, MarkBoundaryWithNullPointerFails)
{
    AllocateCmd(MOCK_CMD_1, MOCK_POOL_1);

    auto status = detector.MarkBoundary(MOCK_CMD_1, nullptr);
    EXPECT_FALSE(status.success);

    VkDebugUtilsLabelEXT null_name_label = {};
    null_name_label.pLabelName = nullptr;
    status = detector.MarkBoundary(MOCK_CMD_1, &null_name_label);
    EXPECT_FALSE(status.success);
}

TEST_F(FrameBoundaryDetectorTest, MarkBoundaryIgnoresStandardLabels)
{
    AllocateCmd(MOCK_CMD_1, MOCK_POOL_1);

    VkDebugUtilsLabelEXT standard_label = {};
    standard_label.pLabelName = "JustARegularDrawcall";

    // It should return success (no error), but NOT set the boundary flag
    auto status = detector.MarkBoundary(MOCK_CMD_1, &standard_label);
    EXPECT_TRUE(status.success);
    EXPECT_FALSE(detector.IsFrameBoundary(MOCK_CMD_1));
}

TEST_F(FrameBoundaryDetectorTest, ResetCommandBufferClearsFlag)
{
    AllocateCmd(MOCK_CMD_1, MOCK_POOL_1);
    auto label = GetValidBoundaryLabel();
    detector.MarkBoundary(MOCK_CMD_1, &label);

    EXPECT_TRUE(detector.IsFrameBoundary(MOCK_CMD_1));

    // Resetting should clear the boundary flag
    detector.OnResetCommandBuffer(MOCK_CMD_1);
    EXPECT_FALSE(detector.IsFrameBoundary(MOCK_CMD_1));
}

TEST_F(FrameBoundaryDetectorTest, ResetCommandPoolClearsFlagsForPoolOnly)
{
    AllocateCmd(MOCK_CMD_1, MOCK_POOL_1);
    AllocateCmd(MOCK_CMD_2, MOCK_POOL_2);

    auto label = GetValidBoundaryLabel();
    detector.MarkBoundary(MOCK_CMD_1, &label);
    detector.MarkBoundary(MOCK_CMD_2, &label);

    EXPECT_TRUE(detector.IsFrameBoundary(MOCK_CMD_1));
    EXPECT_TRUE(detector.IsFrameBoundary(MOCK_CMD_2));

    // Reset POOL_1 only
    detector.OnResetCommandPool(MOCK_POOL_1);

    // CMD_1 (Pool 1) should be cleared, CMD_2 (Pool 2) should remain untouched
    EXPECT_FALSE(detector.IsFrameBoundary(MOCK_CMD_1));
    EXPECT_TRUE(detector.IsFrameBoundary(MOCK_CMD_2));
}

TEST_F(FrameBoundaryDetectorTest, ContainsAndClearBoundaryFlags)
{
    AllocateCmd(MOCK_CMD_1, MOCK_POOL_1);
    AllocateCmd(MOCK_CMD_2, MOCK_POOL_1);

    auto label = GetValidBoundaryLabel();
    // Only CMD_2 has the boundary
    detector.MarkBoundary(MOCK_CMD_2, &label);

    VkCommandBuffer submit_cmds[] = {MOCK_CMD_1, MOCK_CMD_2};
    VkSubmitInfo submit_info = {};
    submit_info.commandBufferCount = 2;
    submit_info.pCommandBuffers = submit_cmds;

    // Check ContainsFrameBoundary (Read-Only)
    EXPECT_TRUE(detector.ContainsFrameBoundary(1, &submit_info));

    // Flag should still be there after checking
    EXPECT_TRUE(detector.IsFrameBoundary(MOCK_CMD_2));

    // Consume the boundaries (Write)
    detector.ClearBoundaryFlags(1, &submit_info);

    // Verify it was consumed
    EXPECT_FALSE(detector.ContainsFrameBoundary(1, &submit_info));
    EXPECT_FALSE(detector.IsFrameBoundary(MOCK_CMD_2));
}

TEST_F(FrameBoundaryDetectorTest, ContainsBoundarySafelyHandlesNulls)
{
    // Pass nullptrs, should safely return false without crashing
    EXPECT_FALSE(detector.ContainsFrameBoundary(1, nullptr));

    VkSubmitInfo submit_info = {};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = nullptr;
    EXPECT_FALSE(detector.ContainsFrameBoundary(1, &submit_info));
}

}  // namespace

}  // namespace Dive
