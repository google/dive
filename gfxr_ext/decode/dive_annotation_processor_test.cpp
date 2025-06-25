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

using ::testing::SizeIs;

namespace gfxrecon::decode
{
namespace
{

class DiveAnnotationProcessorTest : public testing::Test
{
protected:
    // Helper function to compare VulkanCommandInfo
    bool AreVulkanCommandInfoEqual(const DiveAnnotationProcessor::VulkanCommandInfo& actual,
                                   const std::string&                                expected_name,
                                   uint32_t                                          expected_index,
                                   const nlohmann::ordered_json&                     expected_args)
    {
        return actual.GetVkCmdName() == expected_name && actual.GetVkCmdIndex() == expected_index &&
               actual.GetArgs() == expected_args;
    }

    // Helper function to compare SubmitInfo
    bool AreSubmitInfoEqual(const DiveAnnotationProcessor::SubmitInfo* actual,
                            const std::string&                         expected_name,
                            uint32_t expected_command_buffer_count)
    {
        return actual != nullptr && actual->GetSubmitText() == expected_name &&
               actual->GetCommandBufferCount() == expected_command_buffer_count;
    }
};

TEST_F(DiveAnnotationProcessorTest, WriteBlockEnd_SingleSubmit_CreatesOneSubmitWithNoCommands)
{
    DiveAnnotationProcessor processor_;
    processor_.WriteBlockEnd(gfxrecon::util::DiveFunctionData("vkQueueSubmit",
                                                              /*cmd_buffer_index=*/0,
                                                              /*block_index=*/1,
                                                              {}));

    auto submits = processor_.getSubmits();
    ASSERT_THAT(submits, SizeIs(1));
    EXPECT_TRUE(AreSubmitInfoEqual(submits[0].get(), "vkQueueSubmit", 0));
    EXPECT_TRUE(submits[0]->GetVulkanCommands().empty());
}

TEST_F(DiveAnnotationProcessorTest,
       WriteBlockEnd_MultipleSubmitsWithCommands_CreatesSubmitsWithCorrectCommands)
{
    DiveAnnotationProcessor          processor_;
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

    processor_.WriteBlockEnd(cmd_data_1);
    processor_.WriteBlockEnd(cmd_data_2);
    processor_.WriteBlockEnd(cmd_data_3);
    processor_.WriteBlockEnd(cmd_data_4);
    processor_.WriteBlockEnd(submit_data_1);
    processor_.WriteBlockEnd(cmd_data_5);
    processor_.WriteBlockEnd(cmd_data_6);
    processor_.WriteBlockEnd(cmd_data_7);
    processor_.WriteBlockEnd(submit_data_2);

    auto submits = processor_.getSubmits();
    ASSERT_THAT(submits, SizeIs(2));
    EXPECT_TRUE(AreSubmitInfoEqual(submits[0].get(),
                                   submit_data_1.GetFunctionName(),
                                   /*expected_command_buffer_count=*/1));
    ASSERT_THAT(submits[0]->GetVulkanCommands(), SizeIs(4));
    EXPECT_TRUE(AreVulkanCommandInfoEqual(submits[0]->GetVulkanCommands()[1],
                                          cmd_data_2.GetFunctionName(),
                                          /*expected_index=*/1,
                                          nlohmann::ordered_json()));
    EXPECT_TRUE(AreVulkanCommandInfoEqual(submits[0]->GetVulkanCommands()[2],
                                          cmd_data_3.GetFunctionName(),
                                          /*expected_index=*/2,
                                          nlohmann::ordered_json()));

    EXPECT_TRUE(AreSubmitInfoEqual(submits[1].get(), submit_data_2.GetFunctionName(), 1));
    ASSERT_THAT(submits[1]->GetVulkanCommands(), SizeIs(3));
    EXPECT_TRUE(AreVulkanCommandInfoEqual(submits[1]->GetVulkanCommands()[1],
                                          cmd_data_6.GetFunctionName(),
                                          /*expected_index=*/1,
                                          nlohmann::ordered_json()));
}
}  // namespace
}  // namespace gfxrecon::decode
