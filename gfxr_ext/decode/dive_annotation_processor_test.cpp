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

namespace gfxrecon::decode
{
namespace
{

class DiveAnnotationProcessorTestFixture : public testing::Test
{
protected:
    DiveAnnotationProcessor processor_;

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

TEST_F(DiveAnnotationProcessorTestFixture, WriteBlockEndSingleSubmit)
{
    gfxrecon::util::DiveFunctionData data("vkQueueSubmit", 0, 1, { { "arg1", 1 } });
    processor_.WriteBlockEnd(data);

    auto submits = processor_.getSubmits();
    ASSERT_EQ(submits.size(), 1);
    EXPECT_TRUE(AreSubmitInfoEqual(submits[0].get(), "vkQueueSubmit", 0));
    EXPECT_TRUE(submits[0]->GetVulkanCommands().empty());
}

TEST_F(DiveAnnotationProcessorTestFixture, WriteBlockEndMultipleSubmits)
{
    gfxrecon::util::DiveFunctionData cmd_data_1("vkBeginCommandBuffer", 0, 6, {});
    gfxrecon::util::DiveFunctionData cmd_data_2("vkCmdDraw", 1, 7, {});
    gfxrecon::util::DiveFunctionData cmd_data_3("vkCmdDispatch", 2, 9, {});
    gfxrecon::util::DiveFunctionData cmd_data_4("vkEndCommandBuffer", 0, 8, {});
    gfxrecon::util::DiveFunctionData submit_data_1("vkQueueSubmit", 0, 8, { { "arg1", 1 } });
    gfxrecon::util::DiveFunctionData cmd_data_5("vkBeginCommandBuffer", 0, 6, {});
    gfxrecon::util::DiveFunctionData cmd_data_6("vkCmdCopyBuffer", 1, 9, {});
    gfxrecon::util::DiveFunctionData cmd_data_7("vkEndCommandBuffer", 0, 6, {});
    gfxrecon::util::DiveFunctionData submit_data_2("vkQueueSubmit", 0, 10, { { "arg2", 2 } });

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
    ASSERT_EQ(submits.size(), 2);
    EXPECT_TRUE(AreSubmitInfoEqual(submits[0].get(), "vkQueueSubmit", 1));
    ASSERT_EQ(submits[0]->GetVulkanCommands().size(), 4);
    EXPECT_TRUE(AreVulkanCommandInfoEqual(submits[0]->GetVulkanCommands()[1],
                                          "vkCmdDraw",
                                          1,
                                          nlohmann::ordered_json()));
    EXPECT_TRUE(AreVulkanCommandInfoEqual(submits[0]->GetVulkanCommands()[2],
                                          "vkCmdDispatch",
                                          2,
                                          nlohmann::ordered_json()));

    EXPECT_TRUE(AreSubmitInfoEqual(submits[1].get(), "vkQueueSubmit", 1));
    ASSERT_EQ(submits[1]->GetVulkanCommands().size(), 3);
    EXPECT_TRUE(AreVulkanCommandInfoEqual(submits[1]->GetVulkanCommands()[1],
                                          "vkCmdCopyBuffer",
                                          1,
                                          nlohmann::ordered_json()));
}
}  // namespace
}  // namespace gfxrecon::decode
