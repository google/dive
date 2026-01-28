/*
Copyright 2023 Google Inc.

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

#include "dive/os/command_utils.h"

#include <cctype>
#include <set>
#include <string>
#include <string_view>

#include "absl/base/no_destructor.h"

namespace Dive
{

namespace
{

constexpr int kMaxAscii = 127;
bool IsAscii(int c) { return 0 <= c && c <= kMaxAscii; }
bool IsBashKeyword(std::string_view str)
{
    static absl::NoDestructor<std::set<std::string_view>> lut(std::set<std::string_view>(
        {"if", "then", "elif", "else", "fi", "time", "for", "in", "until", "while", "do", "done",
         "case", "esac", "coproc", "select", "function"}));
    return lut->contains(str);
}

bool IsBashSafeCharacter(char c)
{
    static const auto lut = []() {
        constexpr std::string_view kShellSafeCharacters =
            "+,-./0123456789:@ABCDEFGHIJKLMNOPQRSTUVWXYZ^_abcdefghijklmnopqrstuvwxyz";
        std::array<bool, kMaxAscii + 1> result = {};
        for (char c : kShellSafeCharacters)
        {
            result[c] = true;
        }
        return result;
    }();
    return IsAscii(c) && lut[c];
}

std::string BashEscapeImpl(std::string_view original)
{
    constexpr std::string_view kSingleQuoteReplacement = R"('"'"')";
    std::string escaped;
    escaped.reserve(original.size() + 2);
    escaped.push_back('\'');
    for (char c : original)
    {
        if (c == '\'')
        {
            escaped.append(kSingleQuoteReplacement);
        }
        else
        {
            escaped.push_back(c);
        }
    }
    escaped.push_back('\'');
    return escaped;
}

}  // namespace

std::string BashEscape(std::string_view original, bool force_escape)
{
    if (original.empty())
    {
        return "\"\"";
    }

    if (!force_escape)
    {
        bool require_escape = IsBashKeyword(original);
        for (char c : original)
        {
            if (!IsBashSafeCharacter(c))
            {
                require_escape = true;
            }
        }
        if (!require_escape)
        {
            return std::string(original);
        }
    }

    return BashEscapeImpl(original);
}

// See
// https://learn.microsoft.com/en-us/cpp/cpp/main-function-command-line-args?view=msvc-170#parsing-c-command-line-arguments
std::string WindowsCrtProgramEscape(std::string_view original)
{
    bool require_escape = false;
    for (char c : original)
    {
        switch (c)
        {
            case '<':
            case '>':
            case ':':
            case '"':
            case '|':
            case '?':
            case '*':
                // Invalid character in program name.
                return "\"\"";
            case ' ':
            case '\t':
                // Documentation only says space and tab are seperator, so ignore newline and etc.
                require_escape = true;
                break;
            default:
                break;
        }
    }
    if (!require_escape)
    {
        return std::string(original);
    }
    return "\"" + std::string(original) + "\"";
}

std::string WindowsCrtArgumentEscape(std::string_view original)
{
    if (original.empty())
    {
        return "\"\"";
    }

    std::string escaped;
    escaped.reserve(original.size() + 2);
    escaped.push_back('"');
    bool require_escape = false;
    for (char c : original)
    {
        switch (c)
        {
            case ' ':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
            case '\v':
                require_escape = true;
                escaped.push_back(c);
                break;
            case '\\':
            case '"':
                require_escape = true;
                escaped.push_back('\\');
                escaped.push_back(c);
                break;
            default:
                if (!IsAscii(c))
                {
                    require_escape = true;
                }
                escaped.push_back(c);
                break;
        }
    }
    escaped.push_back('"');
    if (!require_escape)
    {
        return std::string(original);
    }
    return escaped;
}

std::string MakeUnixCommand(std::string_view program, const std::vector<std::string>& args)
{
    std::string command = BashEscape(program);
    for (const auto& arg : args)
    {
        command.push_back(' ');
        command.append(BashEscape(arg));
    }
    return command;
}

std::string MakeWindowsCommand(std::string_view program, const std::vector<std::string>& args)
{
    std::string command = WindowsCrtProgramEscape(program);
    for (const auto& arg : args)
    {
        command.push_back(' ');
        command.append(WindowsCrtArgumentEscape(arg));
    }
    return command;
}

}  // namespace Dive
