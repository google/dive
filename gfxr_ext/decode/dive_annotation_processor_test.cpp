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
    gfxrecon::util::DiveFunctionData cmd_data_3("vkCmdDraw",
                                                /*cmd_buffer_index=*/2,
                                                /*block_index=*/8,
                                                args_cmd_3);
    nlohmann::ordered_json           args_cmd_4 = { { "commandBuffer", 1001 } };
    gfxrecon::util::DiveFunctionData cmd_data_4("vkCmdDispatch",
                                                /*cmd_buffer_index=*/3,
                                                /*block_index=*/9,
                                                args_cmd_4);
    nlohmann::ordered_json           args_cmd_5 = { { "commandBuffer", 1001 } };
    gfxrecon::util::DiveFunctionData cmd_data_5("vkEndCommandBuffer",
                                                /*cmd_buffer_index=*/0,
                                                /*block_index=*/10,
                                                args_cmd_5);
    nlohmann::ordered_json           args_cmd_6 = { { "commandBuffer", 1001 } };
    gfxrecon::util::DiveFunctionData cmd_data_6("vkBeginCommandBuffer",
                                                /*cmd_buffer_index=*/0,
                                                /*block_index=*/11,
                                                args_cmd_6);
    nlohmann::ordered_json           args_cmd_7 = { { "commandBuffer", 1001 } };
    gfxrecon::util::DiveFunctionData cmd_data_7("vkCmdDraw",
                                                /*cmd_buffer_index=*/4,
                                                /*block_index=*/12,
                                                args_cmd_7);
    nlohmann::ordered_json           args_cmd_8 = { { "commandBuffer", 1001 } };
    gfxrecon::util::DiveFunctionData cmd_data_8("vkEndCommandBuffer",
                                                /*cmd_buffer_index=*/0,
                                                /*block_index=*/13,
                                                args_cmd_8);

    // Submit 1 (submitting command buffer 1001)
    nlohmann::ordered_json args_submit_1 = {
        { "submitCount", 1 },
        { "pSubmits", { { { "commandBufferCount", 1 }, { "pCommandBuffers", { 1001 } } } } }
    };
    gfxrecon::util::DiveFunctionData submit_data_1("vkQueueSubmit",
                                                   /*cmd_buffer_index=*/0,
                                                   /*block_index=*/8,
                                                   args_submit_1);

    processor.WriteBlockEnd(cmd_data_1);
    processor.WriteBlockEnd(cmd_data_2);
    processor.WriteBlockEnd(cmd_data_3);
    processor.WriteBlockEnd(cmd_data_4);
    processor.WriteBlockEnd(cmd_data_5);
    processor.WriteBlockEnd(cmd_data_6);
    processor.WriteBlockEnd(cmd_data_7);
    processor.WriteBlockEnd(cmd_data_8);
    processor.WriteBlockEnd(submit_data_1);

    auto draw_calls_map = processor.TakeCommandBufferDrawCallMap();
    ASSERT_THAT(draw_calls_map, SizeIs(1));
    ASSERT_TRUE(draw_calls_map.count(1001));

    EXPECT_THAT(draw_calls_map[1001], testing::ElementsAre(2, 1));
}

TEST(WriteBlockEndTest, MultipleCommandBuffersHaveCorrectDrawCallCounts)
{
    DiveAnnotationProcessor processor;

    // Command Buffer 1 (handle 1001)
    nlohmann::ordered_json           args_cmd_1 = { { "commandBuffer", 1001 } };
    gfxrecon::util::DiveFunctionData cmd_data_1("vkBeginCommandBuffer",
                                                /*cmd_buffer_index=*/0,
                                                /*block_index=*/1,
                                                args_cmd_1);
    nlohmann::ordered_json           args_cmd_2 = { { "commandBuffer", 1001 } };
    gfxrecon::util::DiveFunctionData cmd_data_2("vkCmdDraw",
                                                /*cmd_buffer_index=*/1,
                                                /*block_index=*/2,
                                                args_cmd_2);
    nlohmann::ordered_json           args_cmd_3 = { { "commandBuffer", 1001 } };
    gfxrecon::util::DiveFunctionData cmd_data_3("vkEndCommandBuffer",
                                                /*cmd_buffer_index=*/0,
                                                /*block_index=*/3,
                                                args_cmd_3);

    // Command Buffer 2 (handle 1002)
    nlohmann::ordered_json           args_cmd_4 = { { "commandBuffer", 1002 } };
    gfxrecon::util::DiveFunctionData cmd_data_4("vkBeginCommandBuffer",
                                                /*cmd_buffer_index=*/0,
                                                /*block_index=*/4,
                                                args_cmd_4);
    nlohmann::ordered_json           args_cmd_5 = { { "commandBuffer", 1002 } };
    gfxrecon::util::DiveFunctionData cmd_data_5("vkCmdDrawIndexed",
                                                /*cmd_buffer_index=*/1,
                                                /*block_index=*/5,
                                                args_cmd_5);
    nlohmann::ordered_json           args_cmd_6 = { { "commandBuffer", 1002 } };
    gfxrecon::util::DiveFunctionData cmd_data_6("vkCmdDraw",
                                                /*cmd_buffer_index=*/2,
                                                /*block_index=*/6,
                                                args_cmd_6);
    nlohmann::ordered_json           args_cmd_7 = { { "commandBuffer", 1002 } };
    gfxrecon::util::DiveFunctionData cmd_data_7("vkEndCommandBuffer",
                                                /*cmd_buffer_index=*/0,
                                                /*block_index=*/7,
                                                args_cmd_7);

    // Single submit of both command buffers
    nlohmann::ordered_json args_submit_1 = {
        { "submitCount", 1 },
        { "pSubmits", { { { "commandBufferCount", 2 }, { "pCommandBuffers", { 1001, 1002 } } } } }
    };
    gfxrecon::util::DiveFunctionData submit_data_1("vkQueueSubmit",
                                                   /*cmd_buffer_index=*/0,
                                                   /*block_index=*/8,
                                                   args_submit_1);

    processor.WriteBlockEnd(cmd_data_1);
    processor.WriteBlockEnd(cmd_data_2);
    processor.WriteBlockEnd(cmd_data_3);
    processor.WriteBlockEnd(cmd_data_4);
    processor.WriteBlockEnd(cmd_data_5);
    processor.WriteBlockEnd(cmd_data_6);
    processor.WriteBlockEnd(cmd_data_7);
    processor.WriteBlockEnd(submit_data_1);

    auto draw_calls_map = processor.TakeCommandBufferDrawCallMap();
    ASSERT_THAT(draw_calls_map, SizeIs(2));
    ASSERT_TRUE(draw_calls_map.count(1001));
    ASSERT_TRUE(draw_calls_map.count(1002));

    EXPECT_THAT(draw_calls_map[1001], testing::ElementsAre(1));
    EXPECT_THAT(draw_calls_map[1002], testing::ElementsAre(2));
}

TEST(WriteBlockEndTest, CommandBufferWithNoDrawCallsHasZeroCount)
{
    DiveAnnotationProcessor processor;

    // Command Buffer 1 (handle 1001) with no draw calls
    nlohmann::ordered_json           args_cmd_1 = { { "commandBuffer", 1001 } };
    gfxrecon::util::DiveFunctionData cmd_data_1("vkBeginCommandBuffer",
                                                /*cmd_buffer_index=*/0,
                                                /*block_index=*/1,
                                                args_cmd_1);
    nlohmann::ordered_json           args_cmd_2 = { { "commandBuffer", 1001 } };
    gfxrecon::util::DiveFunctionData cmd_data_2("vkCmdCopyBuffer",
                                                /*cmd_buffer_index=*/1,
                                                /*block_index=*/2,
                                                args_cmd_2);
    nlohmann::ordered_json           args_cmd_3 = { { "commandBuffer", 1001 } };
    gfxrecon::util::DiveFunctionData cmd_data_3("vkEndCommandBuffer",
                                                /*cmd_buffer_index=*/0,
                                                /*block_index=*/3,
                                                args_cmd_3);

    // Single submit
    nlohmann::ordered_json args_submit_1 = {
        { "submitCount", 1 },
        { "pSubmits", { { { "commandBufferCount", 1 }, { "pCommandBuffers", { 1001 } } } } }
    };
    gfxrecon::util::DiveFunctionData submit_data_1("vkQueueSubmit",
                                                   /*cmd_buffer_index=*/0,
                                                   /*block_index=*/4,
                                                   args_submit_1);

    processor.WriteBlockEnd(cmd_data_1);
    processor.WriteBlockEnd(cmd_data_2);
    processor.WriteBlockEnd(cmd_data_3);
    processor.WriteBlockEnd(submit_data_1);

    auto draw_calls_map = processor.TakeCommandBufferDrawCallMap();
    ASSERT_THAT(draw_calls_map, SizeIs(1));
    ASSERT_TRUE(draw_calls_map.count(1001));

    EXPECT_THAT(draw_calls_map[1001], testing::ElementsAre(0));
}

TEST(WriteBlockEndTest, MixedCommandsOnlyCountDrawCalls)
{
    DiveAnnotationProcessor processor;

    // Command Buffer 1 (handle 1001) with mixed commands
    nlohmann::ordered_json           args_cmd_1 = { { "commandBuffer", 1001 } };
    gfxrecon::util::DiveFunctionData cmd_data_1("vkBeginCommandBuffer",
                                                /*cmd_buffer_index=*/0,
                                                /*block_index=*/1,
                                                args_cmd_1);
    nlohmann::ordered_json           args_cmd_2 = { { "commandBuffer", 1001 } };
    gfxrecon::util::DiveFunctionData cmd_data_2("vkCmdDraw",
                                                /*cmd_buffer_index=*/1,
                                                /*block_index=*/2,
                                                args_cmd_2);
    nlohmann::ordered_json           args_cmd_3 = { { "commandBuffer", 1001 } };
    gfxrecon::util::DiveFunctionData cmd_data_3("vkCmdDispatch",
                                                /*cmd_buffer_index=*/2,
                                                /*block_index=*/3,
                                                args_cmd_3);
    nlohmann::ordered_json           args_cmd_4 = { { "commandBuffer", 1001 } };
    gfxrecon::util::DiveFunctionData cmd_data_4("vkCmdCopyBuffer",
                                                /*cmd_buffer_index=*/3,
                                                /*block_index=*/4,
                                                args_cmd_4);
    nlohmann::ordered_json           args_cmd_5 = { { "commandBuffer", 1001 } };
    gfxrecon::util::DiveFunctionData cmd_data_5("vkCmdDrawIndirect",
                                                /*cmd_buffer_index=*/4,
                                                /*block_index=*/5,
                                                args_cmd_5);
    nlohmann::ordered_json           args_cmd_6 = { { "commandBuffer", 1001 } };
    gfxrecon::util::DiveFunctionData cmd_data_6("vkEndCommandBuffer",
                                                /*cmd_buffer_index=*/0,
                                                /*block_index=*/6,
                                                args_cmd_6);

    // Single submit
    nlohmann::ordered_json args_submit_1 = {
        { "submitCount", 1 },
        { "pSubmits", { { { "commandBufferCount", 1 }, { "pCommandBuffers", { 1001 } } } } }
    };
    gfxrecon::util::DiveFunctionData submit_data_1("vkQueueSubmit",
                                                   /*cmd_buffer_index=*/0,
                                                   /*block_index=*/7,
                                                   args_submit_1);

    processor.WriteBlockEnd(cmd_data_1);
    processor.WriteBlockEnd(cmd_data_2);
    processor.WriteBlockEnd(cmd_data_3);
    processor.WriteBlockEnd(cmd_data_4);
    processor.WriteBlockEnd(cmd_data_5);
    processor.WriteBlockEnd(cmd_data_6);
    processor.WriteBlockEnd(submit_data_1);

    auto draw_calls_map = processor.TakeCommandBufferDrawCallMap();
    ASSERT_THAT(draw_calls_map, SizeIs(1));
    ASSERT_TRUE(draw_calls_map.count(1001));

    EXPECT_THAT(draw_calls_map[1001], testing::ElementsAre(2));
}

}  // namespace
}  // namespace gfxrecon::decode
