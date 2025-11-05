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

namespace Dive
{
namespace StringUtils
{

void Trim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); })
            .base(),
            s.end());
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
}

void RemoveQuotes(std::string& s)
{
    if (s.length() >= 2 && s.front() == '"' && s.back() == '"')
    {
        s = s.substr(1, s.length() - 2);
    }
}

bool GetTrimmedLine(std::ifstream& file, std::string& line)
{
    // Read the first line
    if (!std::getline(file, line))
        return false;

    // Keep reading new lines as long as the total number of
    // un-escaped quotes is odd, which means we are inside a
    // multi-line field.
    int    unescaped_quotes = 0;
    size_t i = 0;
    while (true)
    {
        for (; i < line.length(); ++i)
        {
            if (line[i] == '"')
            {
                // Check for escaped quote ""
                if (i + 1 < line.length() && line[i + 1] == '"')
                {
                    // Skip the escaped quote.
                    i++;
                }
                else
                {
                    unescaped_quotes++;
                }
            }
        }

        // If count is even, we have a complete record. We are done.
        if (unescaped_quotes % 2 == 0)
        {
            break;
        }

        // If count is odd, we are inside a quote. Read the next line.
        std::string next_line;
        if (!std::getline(file, next_line))
        {
            // Reached end of file while inside a quote, which is a file error.
            return false;
        }

        // Append the newline (lost by getline) and the next line's content
        line.append("\n" + next_line);
    }

    Trim(line);
    return true;
}

bool GetTrimmedField(std::stringstream& ss, std::string& field, char delimiter)
{
    field.clear();
    char c;
    bool in_quotes = false;

    // Eat leading whitespace (but not newlines).
    while (ss.peek() != EOF && std::isspace(ss.peek()) && ss.peek() != '\n')
    {
        ss.get();
    }
    if (ss.eof())
    {
        return false;
    }

    // Check if field starts with a quote.
    if (ss.peek() == '"')
    {
        in_quotes = true;
        ss.get(c);
    }

    // Read the rest of the field, char by char.
    while (ss.get(c))
    {
        if (in_quotes)
        {
            if (c == '"')
            {
                if (ss.peek() == '"')
                {
                    // It's an escaped quote (""), add one "
                    field += '"';
                    ss.get();
                }
                else
                {
                    in_quotes = false;
                }
            }
            else
            {
                // Add any character (including \n)
                field += c;
            }
        }
        else
        {
            if (c == delimiter)
            {
                break;
            }
            // Add the character (it's part of an unquoted field)
            field += c;
        }
    }

    Trim(field);
    RemoveQuotes(field);

    return true;
}

}  // namespace StringUtils
}  // namespace Dive
