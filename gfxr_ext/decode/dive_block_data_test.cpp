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

#include "dive_block_data.h"

#include <gtest/gtest.h>

namespace gfxrecon::decode
{
namespace
{

class DiveBlockDataTestFixture : public testing::Test
{
protected:
    void SetUp() override
    {
        // Set of blocks with valid contiguous offset & size
        o.push_back(std::pair<uint32_t, uint32_t>(100, 10));
        o.push_back(std::pair<uint32_t, uint32_t>(110, 90));
        o.push_back(std::pair<uint32_t, uint32_t>(200, 50));
    }

    void PopulateExampleModifications()
    {
        // Set up some modifications, string length is used to id them
        m.push_back(CreateModifiedBuffer(""));  // This is an invalid modification
        m.push_back(CreateModifiedBuffer("1"));
        m.push_back(CreateModifiedBuffer("12"));
        m.push_back(CreateModifiedBuffer("123"));
        m.push_back(CreateModifiedBuffer("1234"));
        m.push_back(CreateModifiedBuffer("12345"));
    }

    std::shared_ptr<std::vector<char>> CreateModifiedBuffer(const std::string& content)
    {
        auto ptr = std::make_shared<std::vector<char>>();
        for (const char& c : content)
        {
            ptr->push_back(c);
        }
        return ptr;
    }

    // A helper function for populating d with the original blocks in o
    void LockExampleOriginals()
    {
        // Mocking dive_file_processor
        for (size_t i = 0; i < o.size(); i++)
        {
            // Called at the beginning of each block
            d.AddOriginalBlock(i, o[i].first);
        }
        // Fake block that indicates the end of the file
        d.AddOriginalBlock(o.size(), o.back().first + o.back().second);
        d.FinalizeOriginalBlocksMapSizes();
    }

    // Gives the descrip string of o[id] that TestBlockVisitor would produce
    std::string GetExampleString(std::pair<uint32_t, uint32_t> o_element)
    {
        return "original, offset:" + std::to_string(o_element.first) +
               ", size:" + std::to_string(o_element.second);
    }

    // Gives the descrip string of m[id] that TestBlockVisitor would produce
    std::string GetExampleString(std::shared_ptr<std::vector<char>> m_element)
    {
        return "modified, content length:" + std::to_string(m_element->size());
    }

    DiveBlockData                                   d = {};
    TestBlockVisitor                                v = {};
    std::vector<std::pair<uint32_t, uint32_t>>      o = {};  // offset & size
    std::vector<std::shared_ptr<std::vector<char>>> m = {};
};

TEST_F(DiveBlockDataTestFixture, AddOriginalBlock_Success)
{
    EXPECT_TRUE(d.AddOriginalBlock(0, 1));
}

TEST_F(DiveBlockDataTestFixture, AddOriginalBlock_LockedMap_Fail)
{
    LockExampleOriginals();
    EXPECT_FALSE(d.AddOriginalBlock(0, 1));
}

TEST_F(DiveBlockDataTestFixture, AddOriginalBlock_WrongIndex_Fail)
{
    EXPECT_FALSE(d.AddOriginalBlock(1, 1));
}

TEST_F(DiveBlockDataTestFixture, FinalizeOriginalBlocksMapSizes_Success)
{
    LockExampleOriginals();
    EXPECT_TRUE(d.IsOriginalBlocksMapLocked());
}

TEST_F(DiveBlockDataTestFixture, FinalizeOriginalBlocksMapSizes_AlreadyLocked_Success)
{
    LockExampleOriginals();
    EXPECT_TRUE(d.FinalizeOriginalBlocksMapSizes());
    EXPECT_TRUE(d.IsOriginalBlocksMapLocked());
}

TEST_F(DiveBlockDataTestFixture, FinalizeOriginalBlocksMapSizes_EmptyOriginals_Fail)
{
    EXPECT_FALSE(d.FinalizeOriginalBlocksMapSizes());
}

TEST_F(DiveBlockDataTestFixture, FinalizeOriginalBlocksMapSizes_NegativeSizeBlock_Fail)
{
    d.AddOriginalBlock(0, 100);
    d.AddOriginalBlock(1, 50);
    EXPECT_FALSE(d.FinalizeOriginalBlocksMapSizes());
}

TEST_F(DiveBlockDataTestFixture, AddModification_Unlocked_Fail)
{
    EXPECT_FALSE(d.AddModification(1, 0, nullptr));
    EXPECT_FALSE(d.ModificationExists(1, 0));
}

TEST_F(DiveBlockDataTestFixture, AddModification_ConflictPosition_Fail)
{
    LockExampleOriginals();
    PopulateExampleModifications();
    EXPECT_TRUE(d.AddModification(1, 2, m[1]));
    EXPECT_FALSE(d.AddModification(1, 2, m[2]));
    EXPECT_TRUE(d.ModificationExists(1, 2));
}

TEST_F(DiveBlockDataTestFixture, AddModification_OOBPrimary_Fail)
{
    LockExampleOriginals();
    PopulateExampleModifications();
    EXPECT_FALSE(d.AddModification(10, 2, m[1]));
}

TEST_F(DiveBlockDataTestFixture, AddModification_InvalidDelete_Fail)
{
    LockExampleOriginals();
    PopulateExampleModifications();
    EXPECT_FALSE(d.AddModification(1, 1, nullptr));
    EXPECT_FALSE(d.ModificationExists(1, 1));
}

// Since the buffer can be modified outside of DiveBlockData, an empty buffer can be added and will
// only generate an error if it's empty at time of traversal
TEST_F(DiveBlockDataTestFixture, AddModification_EmptyBuffer_Success)
{
    LockExampleOriginals();
    PopulateExampleModifications();
    EXPECT_TRUE(d.AddModification(1, 1, m[0]));
    EXPECT_TRUE(d.ModificationExists(1, 1));
}

TEST_F(DiveBlockDataTestFixture, AddModification_Ordinary_Success)
{
    LockExampleOriginals();
    PopulateExampleModifications();
    EXPECT_TRUE(d.AddModification(1, 1, m[1]));
    EXPECT_TRUE(d.ModificationExists(1, 1));
}

TEST_F(DiveBlockDataTestFixture, AddModification_NegativeSecondary_Success)
{
    LockExampleOriginals();
    PopulateExampleModifications();
    EXPECT_TRUE(d.AddModification(0, -1, m[1]));
    EXPECT_TRUE(d.ModificationExists(0, -1));
}

TEST_F(DiveBlockDataTestFixture, AddModification_LastPrimary_Success)
{
    LockExampleOriginals();
    PopulateExampleModifications();
    EXPECT_TRUE(d.AddModification(2, 3, m[1]));
    EXPECT_TRUE(d.ModificationExists(2, 3));
}

TEST_F(DiveBlockDataTestFixture, AddModification_MarkDeleted_Success)
{
    LockExampleOriginals();
    EXPECT_TRUE(d.AddModification(1, 0, nullptr));
    EXPECT_TRUE(d.ModificationExists(1, 0));
}

TEST_F(DiveBlockDataTestFixture, RemoveModification_Success)
{
    LockExampleOriginals();
    PopulateExampleModifications();
    EXPECT_TRUE(d.AddModification(2, 3, m[1]));
    EXPECT_TRUE(d.ModificationExists(2, 3));
    EXPECT_TRUE(d.RemoveModification(2, 3));
    EXPECT_FALSE(d.ModificationExists(2, 3));
}

TEST_F(DiveBlockDataTestFixture, RemoveModification_Fail)
{
    LockExampleOriginals();
    EXPECT_FALSE(d.ModificationExists(2, 3));
    EXPECT_FALSE(d.RemoveModification(2, 3));
    EXPECT_FALSE(d.ModificationExists(2, 3));
}

TEST_F(DiveBlockDataTestFixture, TraverseBlocks_AllOriginal)
{
    LockExampleOriginals();

    // Traverse and check order
    EXPECT_TRUE(d.TraverseBlocks(v));
    std::vector<std::string> traversed_strings = v.GetTraversedPathString();
    EXPECT_EQ(3, traversed_strings.size());
    EXPECT_EQ(GetExampleString(o[0]), traversed_strings[0]);
    EXPECT_EQ(GetExampleString(o[1]), traversed_strings[1]);
    EXPECT_EQ(GetExampleString(o[2]), traversed_strings[2]);
}

TEST_F(DiveBlockDataTestFixture, TraverseBlocks_ReplaceOriginal)
{
    LockExampleOriginals();
    PopulateExampleModifications();

    // Make modifications to replace some original blocks
    EXPECT_TRUE(d.AddModification(0, 0, m[1]));
    EXPECT_TRUE(d.AddModification(2, 0, m[2]));

    // Traverse and check order
    EXPECT_TRUE(d.TraverseBlocks(v));
    std::vector<std::string> traversed_strings = v.GetTraversedPathString();
    EXPECT_EQ(3, traversed_strings.size());
    EXPECT_EQ(GetExampleString(m[1]), traversed_strings[0]);
    EXPECT_EQ(GetExampleString(o[1]), traversed_strings[1]);
    EXPECT_EQ(GetExampleString(m[2]), traversed_strings[2]);
}

TEST_F(DiveBlockDataTestFixture, TraverseBlocks_DeleteOriginal)
{
    LockExampleOriginals();
    PopulateExampleModifications();

    // Make modifications to delete some original blocks
    EXPECT_FALSE(d.ModificationExists(1, 0));  // TODEL
    EXPECT_TRUE(d.AddModification(1, 0, nullptr));
    EXPECT_TRUE(d.ModificationExists(1, 0));  // TODEL

    // Traverse and check order
    EXPECT_TRUE(d.TraverseBlocks(v));
    std::vector<std::string> traversed_strings = v.GetTraversedPathString();
    EXPECT_EQ(2, traversed_strings.size());
    EXPECT_EQ(GetExampleString(o[0]), traversed_strings[0]);
    EXPECT_EQ(GetExampleString(o[2]), traversed_strings[1]);
}

TEST_F(DiveBlockDataTestFixture, TraverseBlocks_OrderSecondary)
{
    LockExampleOriginals();
    PopulateExampleModifications();

    // Make modifications, anchor multiple modifications on the same block
    EXPECT_TRUE(d.AddModification(1, -10, m[2]));
    EXPECT_TRUE(d.AddModification(1, 2, m[3]));
    EXPECT_TRUE(d.AddModification(1, 4, m[4]));
    EXPECT_TRUE(d.AddModification(1, -4, m[5]));

    // Traverse and check order
    EXPECT_TRUE(d.TraverseBlocks(v));
    std::vector<std::string> traversed_strings = v.GetTraversedPathString();
    EXPECT_EQ(7, traversed_strings.size());
    EXPECT_EQ(GetExampleString(o[0]), traversed_strings[0]);
    EXPECT_EQ(GetExampleString(m[2]), traversed_strings[1]);
    EXPECT_EQ(GetExampleString(m[5]), traversed_strings[2]);
    EXPECT_EQ(GetExampleString(o[1]), traversed_strings[3]);
    EXPECT_EQ(GetExampleString(m[3]), traversed_strings[4]);
    EXPECT_EQ(GetExampleString(m[4]), traversed_strings[5]);
    EXPECT_EQ(GetExampleString(o[2]), traversed_strings[6]);
}

}  // namespace
}  // namespace gfxrecon::decode
