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

    auto submits = processor.getSubmits();
    ASSERT_THAT(submits, SizeIs(1));
    EXPECT_THAT(submits[0].get(), SubmitInfoEq("vkQueueSubmit", 0));
    EXPECT_THAT(submits[0]->GetVulkanCommands(), IsEmpty());
}

TEST(WriteBlockEndTest, MultipleSubmitsWithCommandsCreatesSubmitsWithCorrectCommands)
{
    DiveAnnotationProcessor          processor;
    gfxrecon::util::DiveFunctionData cmd_data_1("vkBeginCommandBuffer",
                                                /*cmd_buffer_index=*/0,
                                                /*block_index=*/6,
                                                {});
    gfxrecon::util::DiveFunctionData cmd_data_2("vkCmdDraw",
                                                /*cmd_buffer_index=*/1,
                                                /*block_index=*/7,
                                                {});
    gfxrecon::util::DiveFunctionData cmd_data_3("vkCmdDispatch",
                                                /*cmd_buffer_index=*/2,
                                                /*block_index=*/9,
                                                {});
    gfxrecon::util::DiveFunctionData cmd_data_4("vkEndCommandBuffer",
                                                /*cmd_buffer_index=*/0,
                                                /*block_index=*/8,
                                                {});
    gfxrecon::util::DiveFunctionData submit_data_1("vkQueueSubmit",
                                                   /*cmd_buffer_index=*/0,
                                                   /*block_index=*/8,
                                                   { { "arg1", 1 } });
    gfxrecon::util::DiveFunctionData cmd_data_5("vkBeginCommandBuffer",
                                                /*cmd_buffer_index=*/0,
                                                /*block_index=*/6,
                                                {});
    gfxrecon::util::DiveFunctionData cmd_data_6("vkCmdCopyBuffer",
                                                /*cmd_buffer_index=*/1,
                                                /*block_index=*/9,
                                                {});
    gfxrecon::util::DiveFunctionData cmd_data_7("vkEndCommandBuffer",
                                                /*cmd_buffer_index=*/0,
                                                /*block_index=*/6,
                                                {});
    gfxrecon::util::DiveFunctionData submit_data_2("vkQueueSubmit",
                                                   /*cmd_buffer_index=*/0,
                                                   /*block_index=*/10,
                                                   { { "arg2", 2 } });

    processor.WriteBlockEnd(cmd_data_1);
    processor.WriteBlockEnd(cmd_data_2);
    processor.WriteBlockEnd(cmd_data_3);
    processor.WriteBlockEnd(cmd_data_4);
    processor.WriteBlockEnd(submit_data_1);
    processor.WriteBlockEnd(cmd_data_5);
    processor.WriteBlockEnd(cmd_data_6);
    processor.WriteBlockEnd(cmd_data_7);
    processor.WriteBlockEnd(submit_data_2);

    auto submits = processor.getSubmits();
    ASSERT_THAT(submits, SizeIs(2));
    EXPECT_THAT(submits[0].get(),
                SubmitInfoEq(submit_data_1.GetFunctionName(), /*expected_command_buffer_count=*/1));
    ASSERT_THAT(submits[0]->GetVulkanCommands(), SizeIs(4));
    EXPECT_THAT(submits[0]->GetVulkanCommands()[1],
                VulkanCommandInfoEqual(cmd_data_2.GetFunctionName(),
                                       /*expected_index=*/1,
                                       nlohmann::ordered_json()));
    EXPECT_THAT(submits[0]->GetVulkanCommands()[2],
                VulkanCommandInfoEqual(cmd_data_3.GetFunctionName(),
                                       /*expected_index=*/2,
                                       nlohmann::ordered_json()));

    EXPECT_THAT(submits[1].get(), SubmitInfoEq(submit_data_2.GetFunctionName(), 1));
    ASSERT_THAT(submits[1]->GetVulkanCommands(), SizeIs(3));
    EXPECT_THAT(submits[1]->GetVulkanCommands()[1],
                VulkanCommandInfoEqual(cmd_data_6.GetFunctionName(),
                                       /*expected_index=*/1,
                                       nlohmann::ordered_json()));
}
}  // namespace
}  // namespace gfxrecon::decode
