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

#include "dive/utils/string_utils.h"

#include <limits>

#include "gtest/gtest.h"

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
    // NOLINTBEGIN
    constexpr long long too_big = static_cast<long long>(std::numeric_limits<int>::max()) + 1;
    EXPECT_FALSE(SafeConvertFromString(std::to_string(too_big), val));
    constexpr long long too_small = static_cast<long long>(std::numeric_limits<int>::min()) - 1;
    EXPECT_FALSE(SafeConvertFromString(std::to_string(too_small), val));
    // NOLINTEND
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
    // NOLINTBEGIN
    constexpr unsigned long long too_big = static_cast<unsigned long long>(
                                           std::numeric_limits<unsigned int>::max()) +
                                           1;
    EXPECT_FALSE(SafeConvertFromString(std::to_string(too_big), val));
    // NOLINTEND
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

TEST(StringUtils, TrimTricky)
{
    std::string s1 = "\t\n\r hello world \v\f\r\n";
    Trim(s1);
    EXPECT_EQ(s1, "hello world");

    std::string s2 = " \t\v\n\r\f ";
    Trim(s2);
    EXPECT_EQ(s2, "");

    std::string s3 = "hello   \t   world";
    Trim(s3);
    EXPECT_EQ(s3, "hello   \t   world");

    std::string s4(5, ' ');
    s4.append("hello");
    s4.push_back('\0');
    s4.append("world");
    s4.append(3, ' ');
    Trim(s4);
    std::string expected = "hello";
    expected.push_back('\0');
    expected.append("world");
    EXPECT_EQ(s4, expected);
    EXPECT_EQ(s4.length(), 11);
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

TEST(StringUtils, RemoveQuotesTricky)
{
    std::string s1 = "\"\"hello\"\"";
    RemoveQuotes(s1);
    EXPECT_EQ(s1, "\"hello\"");

    std::string s2 = "\"\"\"";
    RemoveQuotes(s2);
    EXPECT_EQ(s2, "\"");

    std::string s3 = "\"\"\"\"";
    RemoveQuotes(s3);
    EXPECT_EQ(s3, "\"\"");
}

TEST(StringUtils, GetTrimmedLine_Basic)
{
    std::string       content = "Line 1\nLine 2\nLine 3";
    std::stringstream ss(content);
    std::string       line;

    EXPECT_TRUE(GetTrimmedLine(ss, line));
    EXPECT_EQ(line, "Line 1");

    EXPECT_TRUE(GetTrimmedLine(ss, line));
    EXPECT_EQ(line, "Line 2");

    EXPECT_TRUE(GetTrimmedLine(ss, line));
    EXPECT_EQ(line, "Line 3");

    EXPECT_FALSE(GetTrimmedLine(ss, line));
}

TEST(StringUtils, GetTrimmedLine_MultilineQuotes)
{
    std::string       content = "Header\n\"Start of multiline\nmiddle\nend of multiline\"\nFooter";
    std::stringstream ss(content);
    std::string       line;

    EXPECT_TRUE(GetTrimmedLine(ss, line));
    EXPECT_EQ(line, "Header");

    // Should read all 3 lines at once because of the quotes.
    EXPECT_TRUE(GetTrimmedLine(ss, line));
    // Note: GetTrimmedLine does NOT remove the surrounding quotes, it just reads the logical line.
    EXPECT_EQ(line, "\"Start of multiline\nmiddle\nend of multiline\"");

    EXPECT_TRUE(GetTrimmedLine(ss, line));
    EXPECT_EQ(line, "Footer");
}

TEST(StringUtils, GetTrimmedLine_EscapedQuotesInMultiline)
{
    // "Line 1 says ""hello""\n and continues here"
    std::string       content = "\"Line 1 says \"\"hello\"\""
                                "\n"
                                " and continues here\"";
    std::stringstream ss(content);
    std::string       line;

    EXPECT_TRUE(GetTrimmedLine(ss, line));
    // It should preserve the double double quotes "" as they are technically valid CSV content for
    // a single "
    EXPECT_EQ(line,
              "\"Line 1 says \"\"hello\"\""
              "\n"
              " and continues here\"");
}

TEST(StringUtils, GetTrimmedLine_UnfinishedQuote)
{
    // A quote that opens but never closes before EOF
    std::string       content = "Normal line\n\"This quote never ends...\nuntil EOF";
    std::stringstream ss(content);
    std::string       line;

    EXPECT_TRUE(GetTrimmedLine(ss, line));
    EXPECT_EQ(line, "Normal line");

    // Should return false because it hit EOF while waiting for a closing quote
    EXPECT_FALSE(GetTrimmedLine(ss, line));
}

TEST(StringUtils, GetTrimmedField_BasicCSV)
{
    std::stringstream ss("apple,banana,cherry");
    std::string       field;

    EXPECT_TRUE(GetTrimmedField(ss, field, ','));
    EXPECT_EQ(field, "apple");
    EXPECT_TRUE(GetTrimmedField(ss, field, ','));
    EXPECT_EQ(field, "banana");
    EXPECT_TRUE(GetTrimmedField(ss, field, ','));
    EXPECT_EQ(field, "cherry");
    EXPECT_FALSE(GetTrimmedField(ss, field, ','));
}

TEST(StringUtils, GetTrimmedField_WithSpaces)
{
    // Spaces around delimiters should be trimmed
    std::stringstream ss("  apple  ,  banana  \t,  cherry  ");
    std::string       field;

    EXPECT_TRUE(GetTrimmedField(ss, field, ','));
    EXPECT_EQ(field, "apple");
    EXPECT_TRUE(GetTrimmedField(ss, field, ','));
    EXPECT_EQ(field, "banana");
    EXPECT_TRUE(GetTrimmedField(ss, field, ','));
    EXPECT_EQ(field, "cherry");
}

TEST(StringUtils, GetTrimmedField_EmptyFields)
{
    std::stringstream ss("apple,,cherry,");
    std::string       field;

    EXPECT_TRUE(GetTrimmedField(ss, field, ','));
    EXPECT_EQ(field, "apple");

    // Empty field between commas
    EXPECT_TRUE(GetTrimmedField(ss, field, ','));
    EXPECT_EQ(field, "");

    EXPECT_TRUE(GetTrimmedField(ss, field, ','));
    EXPECT_EQ(field, "cherry");

    // Trailing empty field
    EXPECT_TRUE(GetTrimmedField(ss, field, ','));
    EXPECT_EQ(field, "");

    EXPECT_FALSE(GetTrimmedField(ss, field, ','));
}

TEST(StringUtils, GetTrimmedField_QuotedSimple)
{
    // Quotes should be removed from the final field result
    std::stringstream ss("\"apple\",\"banana\",\"cherry\"");
    std::string       field;

    EXPECT_TRUE(GetTrimmedField(ss, field, ','));
    EXPECT_EQ(field, "apple");
    EXPECT_TRUE(GetTrimmedField(ss, field, ','));
    EXPECT_EQ(field, "banana");
    EXPECT_TRUE(GetTrimmedField(ss, field, ','));
    EXPECT_EQ(field, "cherry");
}

TEST(StringUtils, GetTrimmedField_QuotedWithDelimiters)
{
    // Delimiters inside quotes should be ignored
    std::stringstream ss("  \"Doe, John\"  ,30,\"New York, NY\"");
    std::string       field;

    EXPECT_TRUE(GetTrimmedField(ss, field, ','));
    EXPECT_EQ(field, "Doe, John");

    EXPECT_TRUE(GetTrimmedField(ss, field, ','));
    EXPECT_EQ(field, "30");

    EXPECT_TRUE(GetTrimmedField(ss, field, ','));
    EXPECT_EQ(field, "New York, NY");
}

TEST(StringUtils, GetTrimmedField_EscapedQuotes)
{
    // "" inside a quoted field should become a single "
    // Input: "He said ""Hello""",NextField
    std::stringstream ss("\"He said \"\"Hello\"\"\",NextField");
    std::string       field;

    EXPECT_TRUE(GetTrimmedField(ss, field, ','));
    EXPECT_EQ(field, "He said \"Hello\"");

    EXPECT_TRUE(GetTrimmedField(ss, field, ','));
    EXPECT_EQ(field, "NextField");
}

TEST(StringUtils, GetTrimmedField_MixedQuotesAndSpaces)
{
    // Spaces OUTSIDE the quotes should be ignored.
    std::stringstream ss("  \"apple\"  ,  banana  ");
    std::string       field;

    EXPECT_TRUE(GetTrimmedField(ss, field, ','));
    EXPECT_EQ(field, "apple");

    EXPECT_TRUE(GetTrimmedField(ss, field, ','));
    EXPECT_EQ(field, "banana");
}

TEST(StringUtils, GetTrimmedField_DifferentDelimiter)
{
    // Test with a different delimiter (e.g., pipe |)
    std::stringstream ss("field1|field2|field3");
    std::string       field;

    EXPECT_TRUE(GetTrimmedField(ss, field, '|'));
    EXPECT_EQ(field, "field1");
    EXPECT_TRUE(GetTrimmedField(ss, field, '|'));
    EXPECT_EQ(field, "field2");
}

}  // namespace StringUtils
}  // namespace Dive
