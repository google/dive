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

#include "dive_core/common/string_utils.h"
#include "gtest/gtest.h"
#include <limits>

namespace Dive
{
namespace StringUtils
{

TEST(StringUtils, SafeConvertFromStringInt)
{
    int val = 0;
    EXPECT_TRUE(SafeConvertFromString("123", val));
    EXPECT_EQ(val, 123);

    EXPECT_TRUE(SafeConvertFromString("-123", val));
    EXPECT_EQ(val, -123);

    EXPECT_TRUE(SafeConvertFromString("0", val));
    EXPECT_EQ(val, 0);

    EXPECT_FALSE(SafeConvertFromString("123a", val));
    EXPECT_FALSE(SafeConvertFromString("abc", val));
    EXPECT_FALSE(SafeConvertFromString("", val));
    EXPECT_FALSE(SafeConvertFromString("123.45", val));

    // Test limits
    EXPECT_TRUE(SafeConvertFromString(std::to_string(std::numeric_limits<int>::max()), val));
    EXPECT_EQ(val, std::numeric_limits<int>::max());
    EXPECT_TRUE(SafeConvertFromString(std::to_string(std::numeric_limits<int>::min()), val));
    EXPECT_EQ(val, std::numeric_limits<int>::min());

    // Out of range
    constexpr long long too_big = static_cast<long long>(std::numeric_limits<int>::max()) + 1;
    EXPECT_FALSE(SafeConvertFromString(std::to_string(too_big), val));
    constexpr long long too_small = static_cast<long long>(std::numeric_limits<int>::min()) - 1;
    EXPECT_FALSE(SafeConvertFromString(std::to_string(too_small), val));
}

TEST(StringUtils, SafeConvertFromStringUnsignedInt)
{
    unsigned int val = 0;
    EXPECT_TRUE(SafeConvertFromString("123", val));
    EXPECT_EQ(val, 123u);

    EXPECT_FALSE(SafeConvertFromString("-123", val));

    EXPECT_TRUE(SafeConvertFromString("0", val));
    EXPECT_EQ(val, 0u);

    EXPECT_FALSE(SafeConvertFromString("123a", val));
    EXPECT_FALSE(SafeConvertFromString("abc", val));
    EXPECT_FALSE(SafeConvertFromString("", val));
    EXPECT_FALSE(SafeConvertFromString("123.45", val));

    // Test limits
    EXPECT_TRUE(
    SafeConvertFromString(std::to_string(std::numeric_limits<unsigned int>::max()), val));
    EXPECT_EQ(val, std::numeric_limits<unsigned int>::max());

    // Out of range
    constexpr unsigned long long too_big = static_cast<unsigned long long>(
                                           std::numeric_limits<unsigned int>::max()) +
                                           1;
    EXPECT_FALSE(SafeConvertFromString(std::to_string(too_big), val));
}

TEST(StringUtils, SafeConvertFromStringFloat)
{
    float val = 0.0f;
    EXPECT_TRUE(SafeConvertFromString("123.45", val));
    EXPECT_FLOAT_EQ(val, 123.45f);

    EXPECT_TRUE(SafeConvertFromString("-123.45", val));
    EXPECT_FLOAT_EQ(val, -123.45f);

    EXPECT_TRUE(SafeConvertFromString("0.0", val));
    EXPECT_FLOAT_EQ(val, 0.0f);

    EXPECT_TRUE(SafeConvertFromString("1.23e2", val));
    EXPECT_FLOAT_EQ(val, 123.0f);

    EXPECT_FALSE(SafeConvertFromString("123.45a", val));
    EXPECT_FALSE(SafeConvertFromString("abc", val));
    EXPECT_FALSE(SafeConvertFromString("", val));
}

TEST(StringUtils, SafeConvertFromStringDouble)
{
    double val = 0.0;
    EXPECT_TRUE(SafeConvertFromString("123.45", val));
    EXPECT_DOUBLE_EQ(val, 123.45);

    EXPECT_TRUE(SafeConvertFromString("-123.45", val));
    EXPECT_DOUBLE_EQ(val, -123.45);

    EXPECT_TRUE(SafeConvertFromString("0.0", val));
    EXPECT_DOUBLE_EQ(val, 0.0);

    EXPECT_TRUE(SafeConvertFromString("1.23e2", val));
    EXPECT_DOUBLE_EQ(val, 123.0);

    EXPECT_FALSE(SafeConvertFromString("123.45a", val));
    EXPECT_FALSE(SafeConvertFromString("abc", val));
    EXPECT_FALSE(SafeConvertFromString("", val));
}

TEST(StringUtils, Trim)
{
    std::string s1 = "  hello  ";
    Trim(s1);
    EXPECT_EQ(s1, "hello");

    std::string s2 = "hello";
    Trim(s2);
    EXPECT_EQ(s2, "hello");

    std::string s3 = "  hello";
    Trim(s3);
    EXPECT_EQ(s3, "hello");

    std::string s4 = "hello  ";
    Trim(s4);
    EXPECT_EQ(s4, "hello");

    std::string s5 = "  ";
    Trim(s5);
    EXPECT_EQ(s5, "");

    std::string s6 = "";
    Trim(s6);
    EXPECT_EQ(s6, "");

    std::string s7 = "  hello world  ";
    Trim(s7);
    EXPECT_EQ(s7, "hello world");
}

TEST(StringUtils, RemoveQuotes)
{
    std::string s1 = "\"hello\"";
    RemoveQuotes(s1);
    EXPECT_EQ(s1, "hello");

    std::string s2 = "\"hello";
    RemoveQuotes(s2);
    EXPECT_EQ(s2, "\"hello");

    std::string s3 = "hello\"";
    RemoveQuotes(s3);
    EXPECT_EQ(s3, "hello\"");

    std::string s4 = "hello";
    RemoveQuotes(s4);
    EXPECT_EQ(s4, "hello");

    std::string s5 = "\"\"";
    RemoveQuotes(s5);
    EXPECT_EQ(s5, "");

    std::string s6 = "\"";
    RemoveQuotes(s6);
    EXPECT_EQ(s6, "\"");

    std::string s7 = "";
    RemoveQuotes(s7);
    EXPECT_EQ(s7, "");
}

}  // namespace StringUtils
}  // namespace Dive
