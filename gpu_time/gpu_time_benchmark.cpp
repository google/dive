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

#include <benchmark/benchmark.h>

#include "gpu_time.h"

namespace Dive
{
namespace
{

#define MOCK_HANDLE(type, name, val) \
    const type name = reinterpret_cast<type>(static_cast<uintptr_t>(val));

MOCK_HANDLE(VkDevice, MOCK_DEVICE, 0x1);
MOCK_HANDLE(VkQueue, MOCK_QUEUE, 0x2);
MOCK_HANDLE(VkCommandPool, MOCK_COMMAND_POOL, 0x3);
MOCK_HANDLE(VkQueryPool, MOCK_QUERY_POOL, 0x4);

VkResult MockCreateQueryPool(VkDevice, const VkQueryPoolCreateInfo*, const VkAllocationCallbacks*,
                             VkQueryPool* pQueryPool)
{
    *pQueryPool = MOCK_QUERY_POOL;
    return VK_SUCCESS;
}

void MockDestroyQueryPool(VkDevice, VkQueryPool, const VkAllocationCallbacks*) {}
void MockResetQueryPool(VkDevice, VkQueryPool, uint32_t, uint32_t) {}
void MockCmdWriteTimestamp(VkCommandBuffer, VkPipelineStageFlagBits, VkQueryPool, uint32_t) {}
VKAPI_ATTR VkResult VKAPI_CALL MockQueueWaitIdle(VkQueue) { return VK_SUCCESS; }

GPUTime g_gpu_time;
std::vector<VkCommandBuffer> g_cmds;
constexpr size_t max_thread_count = 16;

struct GlobalSetup
{
    GlobalSetup()
    {
        g_gpu_time.SetEnable(true);
        g_gpu_time.OnCreateDevice(MOCK_DEVICE, nullptr, 1.0f, MockCreateQueryPool,
                                  MockResetQueryPool, MockDestroyQueryPool);

        g_cmds.resize(max_thread_count);
        VkCommandBufferAllocateInfo alloc_info = {};
        alloc_info.commandPool = MOCK_COMMAND_POOL;
        alloc_info.commandBufferCount = 1;

        for (int i = 0; i < max_thread_count; ++i)
        {
            g_cmds[i] = reinterpret_cast<VkCommandBuffer>(static_cast<uintptr_t>(0x1000 + i));
            g_gpu_time.OnAllocateCommandBuffers(&alloc_info, &g_cmds[i]);
        }
    }

    ~GlobalSetup()
    {
        VkQueue queue = MOCK_QUEUE;
        g_gpu_time.OnGetDeviceQueue(&queue);
        g_gpu_time.OnDestroyDevice(MOCK_DEVICE, MockQueueWaitIdle);
    }
} g_setup_instance;

void BM_RecordCommandBuffers(benchmark::State& state)
{
    VkCommandBuffer cmd = g_cmds[state.thread_index()];

    for (auto _ : state)
    {
        g_gpu_time.OnBeginCommandBuffer(cmd, 0, MockCmdWriteTimestamp);
        g_gpu_time.OnCmdBeginRenderPass(cmd, MockCmdWriteTimestamp);

        // Simulate CPU overhead of doing actual Vulkan command binding/drawing
        benchmark::ClobberMemory();

        g_gpu_time.OnCmdEndRenderPass(cmd, MockCmdWriteTimestamp);
        g_gpu_time.OnEndCommandBuffer(cmd, MockCmdWriteTimestamp);
    }
}

BENCHMARK(BM_RecordCommandBuffers)->ThreadRange(1, max_thread_count)->UseRealTime();

}  // namespace
}  // namespace Dive
