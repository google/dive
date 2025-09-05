/*
 Copyright 2025 Google LLC

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

#include "dive_annotation_processor.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::IsEmpty;
using ::testing::SizeIs;

namespace gfxrecon::decode
{
namespace
{

MATCHER_P3(VulkanCommandInfoEqual, expected_name, expected_index, expected_args, "")
{
    EXPECT_EQ(arg.name, expected_name);
    EXPECT_EQ(arg.index, expected_index);
    EXPECT_EQ(arg.args, expected_args);
    return true;
}

MATCHER_P2(SubmitInfoEq, expected_name, expected_command_buffer_count, "")
{
    return arg->name == expected_name &&
           arg->vk_command_buffer_handles.size() == expected_command_buffer_count;
}

gfxrecon::util::DiveFunctionData CreateCommandData(const std::string& name,
                                                   uint64_t           command_buffer_handle,
                                                   uint32_t           cmd_buffer_index,
                                                   uint32_t           block_index)
{
    nlohmann::ordered_json args = { { "commandBuffer", command_buffer_handle } };
    return gfxrecon::util::DiveFunctionData(name, cmd_buffer_index, block_index, args);
}

TEST(WriteBlockEndTest, SingleSubmitCreatesOneSubmitWithNoCommands)
{
    DiveAnnotationProcessor processor;
    processor.WriteBlockEnd(gfxrecon::util::DiveFunctionData("vkQueueSubmit",
                                                             /*cmd_buffer_index=*/0,
                                                             /*block_index=*/1,
                                                             {}));

    auto submits = processor.TakeSubmits();
    ASSERT_THAT(submits, SizeIs(1));
    EXPECT_THAT(submits[0].get(), SubmitInfoEq("vkQueueSubmit", 0));
    EXPECT_THAT(submits[0]->none_cmd_vk_commands, IsEmpty());
}

TEST(WriteBlockEndTest, MultipleSubmitsWithCommandsCreatesSubmitsWithCorrectCommands)
{
    DiveAnnotationProcessor processor;

    // Command Buffer 1 (handle 1001)
    nlohmann::ordered_json           args_cmd_1 = { { "commandBuffer", 1001 } };
    gfxrecon::util::DiveFunctionData cmd_data_1("vkBeginCommandBuffer",
                                                /*cmd_buffer_index=*/0,
                                                /*block_index=*/6,
                                                args_cmd_1);
    nlohmann::ordered_json           args_cmd_2 = { { "commandBuffer", 1001 } };
    gfxrecon::util::DiveFunctionData cmd_data_2("vkCmdDraw",
                                                /*cmd_buffer_index=*/1,
                                                /*block_index=*/7,
                                                args_cmd_2);
    nlohmann::ordered_json           args_cmd_3 = { { "commandBuffer", 1001 } };
    gfxrecon::util::DiveFunctionData cmd_data_3("vkCmdDispatch",
                                                /*cmd_buffer_index=*/2,
                                                /*block_index=*/9,
                                                args_cmd_3);
    nlohmann::ordered_json           args_cmd_4 = { { "commandBuffer", 1001 } };
    gfxrecon::util::DiveFunctionData cmd_data_4("vkEndCommandBuffer",
                                                /*cmd_buffer_index=*/0,
                                                /*block_index=*/8,
                                                args_cmd_4);

    // Submit 1 (submitting command buffer 1001)
    nlohmann::ordered_json args_submit_1 = {
        { "submitCount", 1 },
        { "pSubmits", { { { "commandBufferCount", 1 }, { "pCommandBuffers", { 1001 } } } } }
    };
    gfxrecon::util::DiveFunctionData submit_data_1("vkQueueSubmit",
                                                   /*cmd_buffer_index=*/0,
                                                   /*block_index=*/8,
                                                   args_submit_1);

    // Command Buffer 2 (handle 1002)
    nlohmann::ordered_json           args_cmd_5 = { { "commandBuffer", 1002 } };
    gfxrecon::util::DiveFunctionData cmd_data_5("vkBeginCommandBuffer",
                                                /*cmd_buffer_index=*/0,
                                                /*block_index=*/6,
                                                args_cmd_5);
    nlohmann::ordered_json           args_cmd_6 = { { "commandBuffer", 1002 } };
    gfxrecon::util::DiveFunctionData cmd_data_6("vkCmdCopyBuffer",
                                                /*cmd_buffer_index=*/1,
                                                /*block_index=*/9,
                                                args_cmd_6);
    nlohmann::ordered_json           args_cmd_7 = { { "commandBuffer", 1002 } };
    gfxrecon::util::DiveFunctionData cmd_data_7("vkEndCommandBuffer",
                                                /*cmd_buffer_index=*/0,
                                                /*block_index=*/6,
                                                args_cmd_7);

    // Submit 2 (submitting command buffer 1002)
    nlohmann::ordered_json args_submit_2 = {
        { "submitCount", 1 },
        { "pSubmits", { { { "commandBufferCount", 1 }, { "pCommandBuffers", { 1002 } } } } }
    };
    gfxrecon::util::DiveFunctionData submit_data_2("vkQueueSubmit",
                                                   /*cmd_buffer_index=*/0,
                                                   /*block_index=*/10,
                                                   args_submit_2);

    processor.WriteBlockEnd(cmd_data_1);
    processor.WriteBlockEnd(cmd_data_2);
    processor.WriteBlockEnd(cmd_data_3);
    processor.WriteBlockEnd(cmd_data_4);
    processor.WriteBlockEnd(submit_data_1);
    processor.WriteBlockEnd(cmd_data_5);
    processor.WriteBlockEnd(cmd_data_6);
    processor.WriteBlockEnd(cmd_data_7);
    processor.WriteBlockEnd(submit_data_2);

    auto submits = processor.TakeSubmits();
    ASSERT_THAT(submits, SizeIs(2));

    // Verify Submit 1
    EXPECT_THAT(submits[0].get(),
                SubmitInfoEq(submit_data_1.GetFunctionName(), /*expected_command_buffer_count=*/1));
    EXPECT_THAT(submits[0]->vk_command_buffer_handles, testing::ElementsAre(1001));
    EXPECT_THAT(submits[0]->none_cmd_vk_commands, IsEmpty());

    // Verify Submit 2
    EXPECT_THAT(submits[1].get(),
                SubmitInfoEq(submit_data_2.GetFunctionName(), /*expected_command_buffer_count=*/1));
    EXPECT_THAT(submits[1]->vk_command_buffer_handles, testing::ElementsAre(1002));
    EXPECT_THAT(submits[1]->none_cmd_vk_commands, IsEmpty());

    // Verify command cache
    auto vk_commands_cache = processor.TakeVkCommandsCache();
    ASSERT_THAT(vk_commands_cache, SizeIs(2));

    // Verify commands for command buffer 1001
    ASSERT_TRUE(vk_commands_cache.count(1001));
    ASSERT_THAT(vk_commands_cache[1001], SizeIs(4));
    EXPECT_THAT(vk_commands_cache[1001][0],
                VulkanCommandInfoEqual(cmd_data_1.GetFunctionName(), 0, args_cmd_1));
    EXPECT_THAT(vk_commands_cache[1001][1],
                VulkanCommandInfoEqual(cmd_data_2.GetFunctionName(), 1, args_cmd_2));
    EXPECT_THAT(vk_commands_cache[1001][2],
                VulkanCommandInfoEqual(cmd_data_3.GetFunctionName(), 2, args_cmd_3));
    EXPECT_THAT(vk_commands_cache[1001][3],
                VulkanCommandInfoEqual(cmd_data_4.GetFunctionName(), 0, args_cmd_4));

    // Verify commands for command buffer 1002
    ASSERT_TRUE(vk_commands_cache.count(1002));
    ASSERT_THAT(vk_commands_cache[1002], SizeIs(3));
    EXPECT_THAT(vk_commands_cache[1002][0],
                VulkanCommandInfoEqual(cmd_data_5.GetFunctionName(), 0, args_cmd_5));
    EXPECT_THAT(vk_commands_cache[1002][1],
                VulkanCommandInfoEqual(cmd_data_6.GetFunctionName(), 1, args_cmd_6));
    EXPECT_THAT(vk_commands_cache[1002][2],
                VulkanCommandInfoEqual(cmd_data_7.GetFunctionName(), 0, args_cmd_7));
}

TEST(WriteBlockEndTest,
     SingleSubmitWithSingleCommandBufferHasCorrectNumberOfDrawCallsPerCommandBufferRecord)
{
    DiveAnnotationProcessor processor;
    uint64_t                handle = 1001;

    processor.WriteBlockEnd(CreateCommandData("vkBeginCommandBuffer", handle, 0, 6));

    // First Render Pass
    processor.WriteBlockEnd(CreateCommandData("vkCmdBeginRenderPass", handle, 0, 7));
    processor.WriteBlockEnd(CreateCommandData("vkCmdDraw", handle, 1, 8));
    processor.WriteBlockEnd(CreateCommandData("vkCmdDraw", handle, 2, 9));
    processor.WriteBlockEnd(CreateCommandData("vkCmdEndRenderPass", handle, 0, 10));

    processor.WriteBlockEnd(CreateCommandData("vkCmdDispatch", handle, 3, 11));

    // Second Render Pass
    processor.WriteBlockEnd(CreateCommandData("vkCmdBeginRenderPass", handle, 0, 12));
    processor.WriteBlockEnd(CreateCommandData("vkCmdDraw", handle, 4, 13));
    processor.WriteBlockEnd(CreateCommandData("vkCmdEndRenderPass", handle, 0, 14));

    processor.WriteBlockEnd(CreateCommandData("vkEndCommandBuffer", handle, 0, 15));

    // Submit 1 (submitting command buffer 1001)
    nlohmann::ordered_json args_submit_1 = {
        { "submitCount", 1 },
        { "pSubmits", { { { "commandBufferCount", 1 }, { "pCommandBuffers", { 1001 } } } } }
    };
    gfxrecon::util::DiveFunctionData submit_data_1("vkQueueSubmit", 0, 16, args_submit_1);
    processor.WriteBlockEnd(submit_data_1);

    auto draw_counts_map = processor.TakeDrawCallMap();
    ASSERT_THAT(draw_counts_map, SizeIs(1));
    ASSERT_TRUE(draw_counts_map.count(handle));

    // Total draw count for command buffer
    EXPECT_THAT(draw_counts_map.at(handle).first, testing::ElementsAre(3));
    // Per-render pass draw counts
    EXPECT_THAT(draw_counts_map.at(handle).second, testing::ElementsAre(2, 1));
}

TEST(WriteBlockEndTest, MultipleCommandBuffersHaveCorrectDrawCallCounts)
{
    DiveAnnotationProcessor processor;
    uint64_t                handle_1 = 1001;
    uint64_t                handle_2 = 1002;

    // Command Buffer 1 (handle 1001)
    processor.WriteBlockEnd(CreateCommandData("vkBeginCommandBuffer", handle_1, 0, 1));
    processor.WriteBlockEnd(CreateCommandData("vkCmdBeginRenderPass", handle_1, 0, 2));
    processor.WriteBlockEnd(CreateCommandData("vkCmdDraw", handle_1, 1, 3));
    processor.WriteBlockEnd(CreateCommandData("vkCmdEndRenderPass", handle_1, 0, 4));
    processor.WriteBlockEnd(CreateCommandData("vkEndCommandBuffer", handle_1, 0, 5));

    // Command Buffer 2 (handle 1002)
    processor.WriteBlockEnd(CreateCommandData("vkBeginCommandBuffer", handle_2, 0, 6));
    processor.WriteBlockEnd(CreateCommandData("vkCmdBeginRenderPass", handle_2, 0, 7));
    processor.WriteBlockEnd(CreateCommandData("vkCmdDrawIndexed", handle_2, 1, 8));
    processor.WriteBlockEnd(CreateCommandData("vkCmdDraw", handle_2, 2, 9));
    processor.WriteBlockEnd(CreateCommandData("vkCmdEndRenderPass", handle_2, 0, 10));
    processor.WriteBlockEnd(CreateCommandData("vkEndCommandBuffer", handle_2, 0, 11));

    // Single submit of both command buffers
    nlohmann::ordered_json args_submit_1 = {
        { "submitCount", 1 },
        { "pSubmits", { { { "commandBufferCount", 2 }, { "pCommandBuffers", { 1001, 1002 } } } } }
    };
    gfxrecon::util::DiveFunctionData submit_data_1("vkQueueSubmit", 0, 12, args_submit_1);
    processor.WriteBlockEnd(submit_data_1);

    auto draw_counts_map = processor.TakeDrawCallMap();
    ASSERT_THAT(draw_counts_map, SizeIs(2));
    ASSERT_TRUE(draw_counts_map.count(handle_1));
    ASSERT_TRUE(draw_counts_map.count(handle_2));

    EXPECT_THAT(draw_counts_map.at(handle_1).first, testing::ElementsAre(1));
    EXPECT_THAT(draw_counts_map.at(handle_1).second, testing::ElementsAre(1));
    EXPECT_THAT(draw_counts_map.at(handle_2).first, testing::ElementsAre(2));
    EXPECT_THAT(draw_counts_map.at(handle_2).second, testing::ElementsAre(2));
}

TEST(WriteBlockEndTest, CommandBufferWithNoDrawCallsHasZeroCount)
{
    DiveAnnotationProcessor processor;
    uint64_t                handle = 1001;

    // Command Buffer 1 (handle 1001) with no draw calls
    processor.WriteBlockEnd(CreateCommandData("vkBeginCommandBuffer", handle, 0, 1));
    processor.WriteBlockEnd(CreateCommandData("vkCmdBeginRenderPass", handle, 0, 2));
    processor.WriteBlockEnd(CreateCommandData("vkCmdCopyBuffer", handle, 1, 3));
    processor.WriteBlockEnd(CreateCommandData("vkCmdEndRenderPass", handle, 0, 4));
    processor.WriteBlockEnd(CreateCommandData("vkEndCommandBuffer", handle, 0, 5));

    // Single submit
    nlohmann::ordered_json args_submit_1 = {
        { "submitCount", 1 },
        { "pSubmits", { { { "commandBufferCount", 1 }, { "pCommandBuffers", { 1001 } } } } }
    };
    gfxrecon::util::DiveFunctionData submit_data_1("vkQueueSubmit", 0, 6, args_submit_1);
    processor.WriteBlockEnd(submit_data_1);

    auto draw_counts_map = processor.TakeDrawCallMap();
    ASSERT_THAT(draw_counts_map, SizeIs(1));
    ASSERT_TRUE(draw_counts_map.count(handle));

    EXPECT_THAT(draw_counts_map.at(handle).first, testing::ElementsAre(0));
    EXPECT_THAT(draw_counts_map.at(handle).second, testing::ElementsAre(0));
}

TEST(WriteBlockEndTest, MixedCommandsOnlyCountDrawCalls)
{
    DiveAnnotationProcessor processor;
    uint64_t                handle = 1001;

    // Command Buffer 1 (handle 1001) with mixed commands
    processor.WriteBlockEnd(CreateCommandData("vkBeginCommandBuffer", handle, 0, 1));
    processor.WriteBlockEnd(CreateCommandData("vkCmdBeginRenderPass", handle, 0, 2));
    processor.WriteBlockEnd(CreateCommandData("vkCmdDraw", handle, 1, 3));
    processor.WriteBlockEnd(CreateCommandData("vkCmdDispatch", handle, 2, 4));
    processor.WriteBlockEnd(CreateCommandData("vkCmdCopyBuffer", handle, 3, 5));
    processor.WriteBlockEnd(CreateCommandData("vkCmdDrawIndirect", handle, 4, 6));
    processor.WriteBlockEnd(CreateCommandData("vkCmdEndRenderPass", handle, 0, 7));
    processor.WriteBlockEnd(CreateCommandData("vkEndCommandBuffer", handle, 0, 8));

    // Single submit
    nlohmann::ordered_json args_submit_1 = {
        { "submitCount", 1 },
        { "pSubmits", { { { "commandBufferCount", 1 }, { "pCommandBuffers", { 1001 } } } } }
    };
    gfxrecon::util::DiveFunctionData submit_data_1("vkQueueSubmit", 0, 9, args_submit_1);
    processor.WriteBlockEnd(submit_data_1);

    auto draw_counts_map = processor.TakeDrawCallMap();
    ASSERT_THAT(draw_counts_map, SizeIs(1));
    ASSERT_TRUE(draw_counts_map.count(handle));

    EXPECT_THAT(draw_counts_map.at(handle).first, testing::ElementsAre(2));
    EXPECT_THAT(draw_counts_map.at(handle).second, testing::ElementsAre(2));
}

TEST(WriteBlockEndTest, DrawCallsAreCountedBothInsideAndOutsideRenderPass)
{
    DiveAnnotationProcessor processor;
    uint64_t                handle = 1001;

    // Command Buffer 1 (handle 1001)
    processor.WriteBlockEnd(CreateCommandData("vkBeginCommandBuffer", handle, 0, 1));

    // Draw calls outside of a render pass
    processor.WriteBlockEnd(CreateCommandData("vkCmdDraw", handle, 1, 2));

    // Begin render pass
    processor.WriteBlockEnd(CreateCommandData("vkCmdBeginRenderPass", handle, 0, 3));

    // Draw calls inside the render pass
    processor.WriteBlockEnd(CreateCommandData("vkCmdDraw", handle, 2, 4));
    processor.WriteBlockEnd(CreateCommandData("vkCmdDraw", handle, 3, 5));

    // End render pass
    processor.WriteBlockEnd(CreateCommandData("vkCmdEndRenderPass", handle, 0, 6));

    // More draw calls outside of a render pass
    processor.WriteBlockEnd(CreateCommandData("vkCmdDraw", handle, 4, 7));

    processor.WriteBlockEnd(CreateCommandData("vkEndCommandBuffer", handle, 0, 8));

    // Single submit
    nlohmann::ordered_json args_submit_1 = {
        { "submitCount", 1 },
        { "pSubmits", { { { "commandBufferCount", 1 }, { "pCommandBuffers", { 1001 } } } } }
    };
    gfxrecon::util::DiveFunctionData submit_data_1("vkQueueSubmit", 0, 9, args_submit_1);
    processor.WriteBlockEnd(submit_data_1);

    auto draw_counts_map = processor.TakeDrawCallMap();
    ASSERT_THAT(draw_counts_map, SizeIs(1));
    ASSERT_TRUE(draw_counts_map.count(handle));

    // The total draw call count for the command buffer should be 4.
    EXPECT_THAT(draw_counts_map.at(handle).first, testing::ElementsAre(4));

    // The draw call count for the render pass should be 2.
    EXPECT_THAT(draw_counts_map.at(handle).second, testing::ElementsAre(2));
}

}  // namespace
}  // namespace gfxrecon::decode
