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
    EXPECT_EQ(arg.GetVkCmdName(), expected_name);
    EXPECT_EQ(arg.GetVkCmdIndex(), expected_index);
    EXPECT_EQ(arg.GetArgs(), expected_args);
    return true;
}

MATCHER_P2(SubmitInfoEq, expected_name, expected_command_buffer_count, "")
{
    return arg->GetSubmitText() == expected_name &&
           arg->GetCommandBufferCount() == expected_command_buffer_count;
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
    EXPECT_THAT(submits[0]->GetNoneCmdVkCommands(), IsEmpty());
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
    EXPECT_THAT(submits[0]->GetCommandBufferHandles(), testing::ElementsAre(1001));
    EXPECT_THAT(submits[0]->GetNoneCmdVkCommands(), IsEmpty());

    // Verify Submit 2
    EXPECT_THAT(submits[1].get(),
                SubmitInfoEq(submit_data_2.GetFunctionName(), /*expected_command_buffer_count=*/1));
    EXPECT_THAT(submits[1]->GetCommandBufferHandles(), testing::ElementsAre(1002));
    EXPECT_THAT(submits[1]->GetNoneCmdVkCommands(), IsEmpty());

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
}  // namespace
}  // namespace gfxrecon::decode
