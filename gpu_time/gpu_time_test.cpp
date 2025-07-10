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

#include <gtest/gtest.h>
#include "gpu_time.h"

namespace Dive
{

// Since the GPUTime class relies on Vulkan types and function pointers,
// we need to create mocks and stubs for them to use in our tests.
// We are not testing Vulkan itself, just how our class interacts with it.

// --- Mock Vulkan Handles and Functions ---

// Use custom non-zero pointer values for handles to simulate real ones.
// Using reinterpret_cast to create opaque handles from integer values.
#define MOCK_HANDLE(name, type, val) \
    const type name = reinterpret_cast<type>(static_cast<uintptr_t>(val));

MOCK_HANDLE(MOCK_DEVICE, VkDevice, 0x1);
MOCK_HANDLE(MOCK_QUEUE, VkQueue, 0x2);
MOCK_HANDLE(MOCK_COMMAND_POOL, VkCommandPool, 0x3);
MOCK_HANDLE(MOCK_COMMAND_BUFFER_1, VkCommandBuffer, 0x10);
MOCK_HANDLE(MOCK_COMMAND_BUFFER_2, VkCommandBuffer, 0x20);
MOCK_HANDLE(MOCK_QUERY_POOL, VkQueryPool, 0x4);

// Mock implementations of the Vulkan functions that GPUTime calls.
// These functions allow us to control the behavior and return values during tests.

VkResult MockCreateQueryPool(VkDevice                     device,
                             const VkQueryPoolCreateInfo* pCreateInfo,
                             const VkAllocationCallbacks* pAllocator,
                             VkQueryPool*                 pQueryPool)
{
    *pQueryPool = MOCK_QUERY_POOL;
    return VK_SUCCESS;
}

void MockDestroyQueryPool(VkDevice                     device,
                          VkQueryPool                  queryPool,
                          const VkAllocationCallbacks* pAllocator)
{
    // No-op for testing
}

void MockResetQueryPool(VkDevice    device,
                        VkQueryPool queryPool,
                        uint32_t    firstQuery,
                        uint32_t    queryCount)
{
    // No-op for testing
}

void MockCmdWriteTimestamp(VkCommandBuffer         commandBuffer,
                           VkPipelineStageFlagBits pipelineStage,
                           VkQueryPool             queryPool,
                           uint32_t                query)
{
    // No-op for testing
}

VkResult MockGetQueryPoolResults(VkDevice           device,
                                 VkQueryPool        queryPool,
                                 uint32_t           firstQuery,
                                 uint32_t           queryCount,
                                 size_t             dataSize,
                                 void*              pData,
                                 VkDeviceSize       stride,
                                 VkQueryResultFlags flags)
{

    // Simulate returning some timestamp data.
    // Each query result consists of a timestamp (uint64_t) and an availability flag (uint64_t).
    uint64_t* timestamps = static_cast<uint64_t*>(pData);

    // Simulate a 16ms duration for the first command buffer (queries 0 and 1)
    // Timestamp values are in nanoseconds. 16ms = 16,000,000 ns
    timestamps[0] = 1000000000;  // Start time for cmd 1
    timestamps[1] = 1;           // Availability
    timestamps[2] = 1016000000;  // End time for cmd 1
    timestamps[3] = 1;           // Availability

    // Simulate a 32ms duration for the second command buffer (queries 2 and 3)
    // 32ms = 32,000,000 ns
    timestamps[4] = 2000000000;  // Start time for cmd 2
    timestamps[5] = 1;           // Availability
    timestamps[6] = 2032000000;  // End time for cmd 2
    timestamps[7] = 1;           // Availability

    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL MockQueueWaitIdle(VkQueue queue)
{
    // No-op for testing
    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL MockDeviceWaitIdle(VkDevice device)
{
    // No-op for testing
    return VK_SUCCESS;
}

// --- GPUTime Tests ---
// A test fixture is used to set up a GPUTime object for each test.
class GPUTimeTest : public ::testing::Test
{
protected:
    Dive::GPUTime gpuTime;
    const float   MOCK_TIMESTAMP_PERIOD = 1.0f;  // 1 ns per tick for easy calculations

    void SetUp() override
    {
        // This is called before each test.
        // Initialize the GPUTime object with our mock functions.
        auto status = gpuTime.OnCreateDevice(MOCK_DEVICE,
                                             nullptr,
                                             MOCK_TIMESTAMP_PERIOD,
                                             MockCreateQueryPool,
                                             MockResetQueryPool);
        ASSERT_TRUE(status.success);
    }

    void TearDown() override
    {
        // This is called after each test.
        VkQueue queue = MOCK_QUEUE;
        gpuTime.OnGetDeviceQueue(&queue);
        auto status = gpuTime.OnDestroyDevice(MOCK_DEVICE, MockQueueWaitIdle, MockDestroyQueryPool);
        ASSERT_TRUE(status.success);
    }
};

// Test that a newly created object returns default statistics.
TEST_F(GPUTimeTest, InitialStateReturnsDefaultStats)
{
    auto stats = gpuTime.GetStats();
    EXPECT_EQ(stats.average, 0.0);
    EXPECT_EQ(stats.median, 0.0);
    EXPECT_EQ(stats.max, std::numeric_limits<double>::lowest());
    EXPECT_EQ(stats.min, std::numeric_limits<double>::max());
    EXPECT_EQ(stats.stddev, 0.0);
}

// Test successful allocation and freeing of command buffers.
TEST_F(GPUTimeTest, AllocateAndFreeCommandBuffers)
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.commandPool = MOCK_COMMAND_POOL;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmdBuf = MOCK_COMMAND_BUFFER_1;

    // Allocate
    auto status = gpuTime.OnAllocateCommandBuffers(&allocInfo, &cmdBuf);
    ASSERT_TRUE(status.success);

    // Try to allocate the same one again, should fail
    status = gpuTime.OnAllocateCommandBuffers(&allocInfo, &cmdBuf);
    ASSERT_FALSE(status.success);

    // Free
    status = gpuTime.OnFreeCommandBuffers(1, &cmdBuf);
    ASSERT_TRUE(status.success);

    // Try to free it again, should fail
    status = gpuTime.OnFreeCommandBuffers(1, &cmdBuf);
    ASSERT_FALSE(status.success);
}

// Test that resetting a command pool correctly resets the state of command buffers from that pool.
TEST_F(GPUTimeTest, ResetCommandPool)
{
    // Allocate a command buffer
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.commandPool = MOCK_COMMAND_POOL;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer cmdBuf = MOCK_COMMAND_BUFFER_1;
    gpuTime.OnAllocateCommandBuffers(&allocInfo, &cmdBuf);

    // Set a frame boundary on it
    VkDebugUtilsLabelEXT label = {};
    label.pLabelName = Dive::GPUTime::kVulkanVrFrameDelimiterString;
    gpuTime.OnCmdInsertDebugUtilsLabelEXT(cmdBuf, &label);

    // Reset the pool
    auto status = gpuTime.OnResetCommandPool(MOCK_COMMAND_POOL);
    ASSERT_TRUE(status.success);

    // Check that the command buffer state was reset by trying to mark it as a frame boundary again.
    // This primarily ensures the function runs without errors and doesn't crash on lookup.
    status = gpuTime.OnCmdInsertDebugUtilsLabelEXT(cmdBuf, &label);
    ASSERT_TRUE(status.success);
}

// Test that submitting a single command buffer marked as a frame boundary updates the metrics.
TEST_F(GPUTimeTest, SingleCommandBufferFrame)
{
    // Allocate a command buffer
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.commandPool = MOCK_COMMAND_POOL;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer cmdBuf = MOCK_COMMAND_BUFFER_1;
    auto            status = gpuTime.OnAllocateCommandBuffers(&allocInfo, &cmdBuf);
    ASSERT_TRUE(status.success);

    // Set the frame boundary label
    VkDebugUtilsLabelEXT label = {};
    label.pLabelName = Dive::GPUTime::kVulkanVrFrameDelimiterString;
    status = gpuTime.OnCmdInsertDebugUtilsLabelEXT(cmdBuf, &label);
    ASSERT_TRUE(status.success);

    // Now, submit this command buffer. This should trigger the frame boundary logic.
    VkSubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;

    status = gpuTime.OnQueueSubmit(1,
                                   &submitInfo,
                                   MockDeviceWaitIdle,
                                   MockResetQueryPool,
                                   MockGetQueryPoolResults);
    ASSERT_TRUE(status.success);

    // After a frame boundary submit, metrics should be updated.
    // Our mock provides a 16ms duration for the first command buffer.
    // (1016000000 - 1000000000) * 1.0f (timestamp_period) * 0.000001 = 16.0
    auto stats = gpuTime.GetStats();
    EXPECT_DOUBLE_EQ(stats.average, 16.0);
    EXPECT_DOUBLE_EQ(stats.min, 16.0);
    EXPECT_DOUBLE_EQ(stats.max, 16.0);
}

// Test submitting multiple command buffers in a single frame, where the total time is aggregated.
TEST_F(GPUTimeTest, MultiCommandBufferSingleFrame)
{
    // Allocate two command buffers
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.commandPool = MOCK_COMMAND_POOL;
    allocInfo.commandBufferCount = 2;  // Allocate both at once
    VkCommandBuffer cmdBufs[] = { MOCK_COMMAND_BUFFER_1, MOCK_COMMAND_BUFFER_2 };
    gpuTime.OnAllocateCommandBuffers(&allocInfo, cmdBufs);

    // Mark the second one as the frame boundary
    VkDebugUtilsLabelEXT label = {};
    label.pLabelName = Dive::GPUTime::kVulkanVrFrameDelimiterString;
    gpuTime.OnCmdInsertDebugUtilsLabelEXT(MOCK_COMMAND_BUFFER_2, &label);

    // Submit both in one go
    VkSubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 2;
    submitInfo.pCommandBuffers = cmdBufs;

    auto status = gpuTime.OnQueueSubmit(1,
                                        &submitInfo,
                                        MockDeviceWaitIdle,
                                        MockResetQueryPool,
                                        MockGetQueryPoolResults);
    ASSERT_TRUE(status.success);

    // The total frame time should be the sum of durations from both command buffers.
    // From our mock function: 16ms + 32ms = 48ms.
    auto stats = gpuTime.GetStats();
    EXPECT_DOUBLE_EQ(stats.average, 48.0);
    EXPECT_DOUBLE_EQ(stats.min, 48.0);
    EXPECT_DOUBLE_EQ(stats.max, 48.0);
}

// Test submitting multiple, separate frames to see how statistics accumulate.
TEST_F(GPUTimeTest, MultipleFramesUpdateMetrics)
{
    // Allocate command buffers
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.commandPool = MOCK_COMMAND_POOL;
    allocInfo.commandBufferCount = 2;
    VkCommandBuffer cmdBufs[] = { MOCK_COMMAND_BUFFER_1, MOCK_COMMAND_BUFFER_2 };
    gpuTime.OnAllocateCommandBuffers(&allocInfo, cmdBufs);

    VkDebugUtilsLabelEXT label = {};
    label.pLabelName = Dive::GPUTime::kVulkanVrFrameDelimiterString;
    VkSubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;

    // --- Submit Frame 1 (16ms) ---
    gpuTime.OnCmdInsertDebugUtilsLabelEXT(MOCK_COMMAND_BUFFER_1, &label);
    submitInfo.pCommandBuffers = &MOCK_COMMAND_BUFFER_1;
    auto status = gpuTime.OnQueueSubmit(1,
                                        &submitInfo,
                                        MockDeviceWaitIdle,
                                        MockResetQueryPool,
                                        MockGetQueryPoolResults);
    ASSERT_TRUE(status.success);

    // Check stats after one frame
    auto stats1 = gpuTime.GetStats();
    EXPECT_DOUBLE_EQ(stats1.average, 16.0);
    EXPECT_DOUBLE_EQ(stats1.min, 16.0);
    EXPECT_DOUBLE_EQ(stats1.max, 16.0);

    // --- Submit Frame 2 (32ms) ---
    gpuTime.OnCmdInsertDebugUtilsLabelEXT(MOCK_COMMAND_BUFFER_2, &label);
    submitInfo.pCommandBuffers = &MOCK_COMMAND_BUFFER_2;
    status = gpuTime.OnQueueSubmit(1,
                                   &submitInfo,
                                   MockDeviceWaitIdle,
                                   MockResetQueryPool,
                                   MockGetQueryPoolResults);
    ASSERT_TRUE(status.success);

    // Check stats after two frames (16ms and 32ms)
    auto stats2 = gpuTime.GetStats();
    EXPECT_DOUBLE_EQ(stats2.average, 24.0);  // (16 + 32) / 2
    EXPECT_DOUBLE_EQ(stats2.median, 24.0);
    EXPECT_DOUBLE_EQ(stats2.min, 16.0);
    EXPECT_DOUBLE_EQ(stats2.max, 32.0);
}

}  // namespace Dive