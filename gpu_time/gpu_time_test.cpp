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
#include <gmock/gmock.h>
#include "gpu_time.h"

namespace Dive
{
namespace
{

// Since the GPUTime class relies on Vulkan types and function pointers,
// we need to create mocks and stubs for them to use in our tests.
// We are not testing Vulkan itself, just how our class interacts with it.

// --- Mock Vulkan Handles and Functions ---

// Use custom non-zero pointer values for handles to simulate real ones.
// Using reinterpret_cast to create opaque handles from integer values.
#define MOCK_HANDLE(type, name, val) \
    const type name = reinterpret_cast<type>(static_cast<uintptr_t>(val));

MOCK_HANDLE(VkDevice, MOCK_DEVICE, 0x1);
MOCK_HANDLE(VkQueue, MOCK_QUEUE, 0x2);
MOCK_HANDLE(VkCommandPool, MOCK_COMMAND_POOL, 0x3);
MOCK_HANDLE(VkCommandBuffer, MOCK_COMMAND_BUFFER_1, 0x10);
MOCK_HANDLE(VkCommandBuffer, MOCK_COMMAND_BUFFER_2, 0x20);
MOCK_HANDLE(VkCommandBuffer, MOCK_COMMAND_BUFFER_3, 0x30);
MOCK_HANDLE(VkQueryPool, MOCK_QUERY_POOL, 0x4);

constexpr float kMockTimestampPeriod = 1.0f;

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

    // Simulate a 10ms duration for the first command buffer (queries 0 and 1)
    // Timestamp values are in nanoseconds. 10ms = 10,000,000 ns
    timestamps[0] = 1000000000;  // Start time for cmd 1
    timestamps[1] = 1;           // Availability
    timestamps[2] = 1010000000;  // End time for cmd 1
    timestamps[3] = 1;           // Availability

    // Simulate a 20ms duration for the second command buffer (queries 2 and 3)
    // 20ms = 20,000,000 ns
    timestamps[4] = 2000000000;  // Start time for cmd 2
    timestamps[5] = 1;           // Availability
    timestamps[6] = 2020000000;  // End time for cmd 2
    timestamps[7] = 1;           // Availability

    // Simulate a 30ms duration for the third command buffer (queries 4 and 5)
    // 30ms = 30,000,000 ns
    timestamps[8] = 3000000000;   // Start time for cmd 3
    timestamps[9] = 1;            // Availability
    timestamps[10] = 3030000000;  // End time for cmd 3
    timestamps[11] = 1;           // Availability

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

void CreateGPUTime(GPUTime& gpuTime, float timestamp_period)
{
    ASSERT_TRUE(gpuTime
                .OnCreateDevice(MOCK_DEVICE,
                                /*allocator=*/nullptr,
                                timestamp_period,
                                MockCreateQueryPool,
                                MockResetQueryPool)
                .success);
}

void DestroyGPUTime(GPUTime& gpuTime)
{
    VkQueue queue = MOCK_QUEUE;
    gpuTime.OnGetDeviceQueue(&queue);
    ASSERT_TRUE(
    gpuTime.OnDestroyDevice(MOCK_DEVICE, MockQueueWaitIdle, MockDestroyQueryPool).success);
}

MATCHER_P(StatsEq, expected, "")
{
    EXPECT_DOUBLE_EQ(arg.average, expected.average);
    EXPECT_DOUBLE_EQ(arg.median, expected.median);
    EXPECT_DOUBLE_EQ(arg.min, expected.min);
    EXPECT_DOUBLE_EQ(arg.max, expected.max);
    EXPECT_DOUBLE_EQ(arg.stddev, expected.stddev);
    return true;
}

// --- GPUTime Tests ---
// Test that a newly created object returns default statistics.
TEST(GPUTimeTest, InitialStateReturnsDefaultStats)
{
    GPUTime gpuTime;
    ASSERT_NO_FATAL_FAILURE(CreateGPUTime(gpuTime, kMockTimestampPeriod));

    auto           stats = gpuTime.GetStats();
    GPUTime::Stats expected_stats;
    expected_stats.average = 0.0;
    expected_stats.median = 0.0;
    expected_stats.min = std::numeric_limits<double>::max();
    expected_stats.max = std::numeric_limits<double>::lowest();
    expected_stats.stddev = 0.0;
    EXPECT_THAT(stats, StatsEq(expected_stats));

    ASSERT_NO_FATAL_FAILURE(DestroyGPUTime(gpuTime));
}

// Test that a command buffer can be successfully allocated.
TEST(GPUTimeTest, AllocateCommandBufferSucceeds)
{
    GPUTime gpuTime;
    ASSERT_NO_FATAL_FAILURE(CreateGPUTime(gpuTime, kMockTimestampPeriod));

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.commandPool = MOCK_COMMAND_POOL;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer cmdBuf = MOCK_COMMAND_BUFFER_1;

    ASSERT_TRUE(gpuTime.OnAllocateCommandBuffers(&allocInfo, &cmdBuf).success);

    ASSERT_NO_FATAL_FAILURE(DestroyGPUTime(gpuTime));
}

// Test that attempting to allocate the same command buffer again fails.
TEST(GPUTimeTest, AllocateSameCommandBufferTwiceFails)
{
    GPUTime gpuTime;
    ASSERT_NO_FATAL_FAILURE(CreateGPUTime(gpuTime, kMockTimestampPeriod));

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.commandPool = MOCK_COMMAND_POOL;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer cmdBuf = MOCK_COMMAND_BUFFER_1;

    ASSERT_TRUE(gpuTime.OnAllocateCommandBuffers(&allocInfo, &cmdBuf).success);

    // Second allocation of the same buffer should fail
    ASSERT_FALSE(gpuTime.OnAllocateCommandBuffers(&allocInfo, &cmdBuf).success);

    ASSERT_NO_FATAL_FAILURE(DestroyGPUTime(gpuTime));
}

// Test that a command buffer can be successfully freed.
TEST(GPUTimeTest, FreeCommandBufferSucceeds)
{
    GPUTime gpuTime;
    ASSERT_NO_FATAL_FAILURE(CreateGPUTime(gpuTime, kMockTimestampPeriod));

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.commandPool = MOCK_COMMAND_POOL;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer cmdBuf = MOCK_COMMAND_BUFFER_1;

    ASSERT_TRUE(gpuTime.OnAllocateCommandBuffers(&allocInfo, &cmdBuf).success);

    ASSERT_TRUE(gpuTime.OnFreeCommandBuffers(1, &cmdBuf).success);

    ASSERT_NO_FATAL_FAILURE(DestroyGPUTime(gpuTime));
}

// Test that attempting to free the same command buffer again fails.
TEST(GPUTimeTest, FreeSameCommandBufferTwiceFails)
{
    GPUTime gpuTime;
    ASSERT_NO_FATAL_FAILURE(CreateGPUTime(gpuTime, kMockTimestampPeriod));

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.commandPool = MOCK_COMMAND_POOL;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer cmdBuf = MOCK_COMMAND_BUFFER_1;

    gpuTime.OnAllocateCommandBuffers(&allocInfo, &cmdBuf);
    gpuTime.OnFreeCommandBuffers(1, &cmdBuf);

    ASSERT_FALSE(gpuTime.OnFreeCommandBuffers(1, &cmdBuf).success);

    ASSERT_NO_FATAL_FAILURE(DestroyGPUTime(gpuTime));
}

// Test that resetting a command pool correctly resets the state of command buffers from that pool.
TEST(GPUTimeTest, ResetCommandPoolAllowsReinsertingFrameDelimiter)
{
    GPUTime gpuTime;
    ASSERT_NO_FATAL_FAILURE(CreateGPUTime(gpuTime, kMockTimestampPeriod));

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.commandPool = MOCK_COMMAND_POOL;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer cmdBuf = MOCK_COMMAND_BUFFER_1;
    gpuTime.OnAllocateCommandBuffers(&allocInfo, &cmdBuf);

    VkDebugUtilsLabelEXT label = {};
    label.pLabelName = GPUTime::kVulkanVrFrameDelimiterString;
    gpuTime.OnCmdInsertDebugUtilsLabelEXT(cmdBuf, &label);

    ASSERT_TRUE(gpuTime.OnResetCommandPool(MOCK_COMMAND_POOL).success);

    // Check that the command buffer state was reset by trying to mark it as a frame boundary again.
    // This primarily ensures the function runs without errors and doesn't crash on lookup.
    ASSERT_TRUE(gpuTime.OnCmdInsertDebugUtilsLabelEXT(cmdBuf, &label).success);

    ASSERT_NO_FATAL_FAILURE(DestroyGPUTime(gpuTime));
}

// Test that submitting a single command buffer marked as a frame boundary updates the metrics.
TEST(GPUTimeTest, SingleCommandBufferFrameUpdatesMetricsCorrectly)
{
    GPUTime gpuTime;
    ASSERT_NO_FATAL_FAILURE(CreateGPUTime(gpuTime, kMockTimestampPeriod));

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.commandPool = MOCK_COMMAND_POOL;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer cmdBuf = MOCK_COMMAND_BUFFER_1;
    ASSERT_TRUE(gpuTime.OnAllocateCommandBuffers(&allocInfo, &cmdBuf).success);

    VkDebugUtilsLabelEXT label = {};
    label.pLabelName = GPUTime::kVulkanVrFrameDelimiterString;
    ASSERT_TRUE(gpuTime.OnCmdInsertDebugUtilsLabelEXT(cmdBuf, &label).success);

    // This should trigger the frame boundary logic.
    VkSubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;

    ASSERT_TRUE(
    gpuTime
    .OnQueueSubmit(1, &submitInfo, MockDeviceWaitIdle, MockResetQueryPool, MockGetQueryPoolResults)
    .success);

    // After a frame boundary submit, metrics should be updated.
    // Our mock provides a 10ms duration for the first command buffer.
    // (1010000000 - 1000000000) * 1.0f (timestamp_period) * 0.000001 = 10.0
    auto           stats = gpuTime.GetStats();
    GPUTime::Stats expected_stats;
    expected_stats.average = 10.0;
    expected_stats.median = 10.0;
    expected_stats.min = 10.0;
    expected_stats.max = 10.0;
    expected_stats.stddev = 0.0;
    EXPECT_THAT(stats, StatsEq(expected_stats));

    ASSERT_NO_FATAL_FAILURE(DestroyGPUTime(gpuTime));
}

// Test submitting multiple command buffers in a single frame, where the total time is aggregated.
TEST(GPUTimeTest, MultiCommandBufferSingleFrameAggregatesTimeCorrectly)
{
    GPUTime gpuTime;
    ASSERT_NO_FATAL_FAILURE(CreateGPUTime(gpuTime, kMockTimestampPeriod));

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.commandPool = MOCK_COMMAND_POOL;
    allocInfo.commandBufferCount = 2;
    VkCommandBuffer cmdBufs[] = { MOCK_COMMAND_BUFFER_1, MOCK_COMMAND_BUFFER_2 };
    gpuTime.OnAllocateCommandBuffers(&allocInfo, cmdBufs);

    VkDebugUtilsLabelEXT label = {};
    label.pLabelName = GPUTime::kVulkanVrFrameDelimiterString;
    gpuTime.OnCmdInsertDebugUtilsLabelEXT(MOCK_COMMAND_BUFFER_2, &label);

    VkSubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 2;
    submitInfo.pCommandBuffers = cmdBufs;

    ASSERT_TRUE(
    gpuTime
    .OnQueueSubmit(1, &submitInfo, MockDeviceWaitIdle, MockResetQueryPool, MockGetQueryPoolResults)
    .success);

    // The total frame time should be the sum of durations from both command buffers.
    // From our mock function: 10ms + 20ms = 30ms.
    auto           stats = gpuTime.GetStats();
    GPUTime::Stats expected_stats;
    expected_stats.average = 30.0;
    expected_stats.median = 30.0;
    expected_stats.min = 30.0;
    expected_stats.max = 30.0;
    expected_stats.stddev = 0.0;
    EXPECT_THAT(stats, StatsEq(expected_stats));

    ASSERT_NO_FATAL_FAILURE(DestroyGPUTime(gpuTime));
}

// Test submitting multiple, separate frames to see how statistics accumulate.
TEST(GPUTimeTest, MultipleFramesUpdateMetricsCorrectly)
{
    GPUTime gpuTime;
    ASSERT_NO_FATAL_FAILURE(CreateGPUTime(gpuTime, kMockTimestampPeriod));

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.commandPool = MOCK_COMMAND_POOL;
    allocInfo.commandBufferCount = 3;
    VkCommandBuffer cmdBufs[] = { MOCK_COMMAND_BUFFER_1,
                                  MOCK_COMMAND_BUFFER_2,
                                  MOCK_COMMAND_BUFFER_3 };
    gpuTime.OnAllocateCommandBuffers(&allocInfo, cmdBufs);

    VkDebugUtilsLabelEXT label = {};
    label.pLabelName = GPUTime::kVulkanVrFrameDelimiterString;
    VkSubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;
    GPUTime::GpuTimeStatus status;

    // --- Submit Frame 1 (10ms) ---
    gpuTime.OnCmdInsertDebugUtilsLabelEXT(MOCK_COMMAND_BUFFER_1, &label);
    submitInfo.pCommandBuffers = &MOCK_COMMAND_BUFFER_1;
    ASSERT_TRUE(
    gpuTime
    .OnQueueSubmit(1, &submitInfo, MockDeviceWaitIdle, MockResetQueryPool, MockGetQueryPoolResults)
    .success);

    // --- Submit Frame 2 (20ms) ---
    gpuTime.OnCmdInsertDebugUtilsLabelEXT(MOCK_COMMAND_BUFFER_2, &label);
    submitInfo.pCommandBuffers = &MOCK_COMMAND_BUFFER_2;
    ASSERT_TRUE(
    gpuTime
    .OnQueueSubmit(1, &submitInfo, MockDeviceWaitIdle, MockResetQueryPool, MockGetQueryPoolResults)
    .success);

    // --- Submit Frame 3 (30ms) ---
    gpuTime.OnCmdInsertDebugUtilsLabelEXT(MOCK_COMMAND_BUFFER_3, &label);
    submitInfo.pCommandBuffers = &MOCK_COMMAND_BUFFER_3;
    ASSERT_TRUE(
    gpuTime
    .OnQueueSubmit(1, &submitInfo, MockDeviceWaitIdle, MockResetQueryPool, MockGetQueryPoolResults)
    .success);

    // Check stats after three frames (10ms, 20ms, and 30ms)
    auto stats = gpuTime.GetStats();
    // Average: (10 + 20 + 30) / 3 = 20.0
    // Median: The middle value of {10, 20, 30} is 20.0
    // Min: 10.0, Max: 30.0
    // Stddev: sqrt(((10-20)^2 + (20-20)^2 + (30-20)^2) / (3-1))
    //       = sqrt((100 + 0 + 100) / 2) = sqrt(100) = 10.0
    GPUTime::Stats expected_stats;
    expected_stats.average = 20.0;
    expected_stats.median = 20.0;
    expected_stats.min = 10.0;
    expected_stats.max = 30.0;
    expected_stats.stddev = 10.0;
    EXPECT_THAT(stats, StatsEq(expected_stats));

    ASSERT_NO_FATAL_FAILURE(DestroyGPUTime(gpuTime));
}

}  // namespace
}  // namespace Dive