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
    if (!std::getline(file, line))
        return false;
    Trim(line);
    return true;
}

bool GetTrimmedField(std::stringstream& ss, std::string& field, char delimiter)
{
    if (!std::getline(ss, field, delimiter))
        return false;
    Trim(field);
    RemoveQuotes(field);
    return true;
}

}  // namespace StringUtils
}  // namespace Dive
