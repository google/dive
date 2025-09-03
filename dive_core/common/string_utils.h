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

#pragma once

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <type_traits>

namespace Dive
{
namespace StringUtils
{

void Trim(std::string& s);

void RemoveQuotes(std::string& s);

// Helper functions for safe string to number conversion
template<typename T> bool SafeConvertFromString(const std::string& s, T& out)
{
    if constexpr (std::is_floating_point_v<T>)
    {
        char*       end = nullptr;
        const char* start = s.c_str();
        errno = 0;
        T val;
        if constexpr (std::is_same_v<T, float>)
        {
            val = std::strtof(start, &end);
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            val = std::strtod(start, &end);
        }
        else
        {  // long double
            val = std::strtold(start, &end);
        }

        if (errno == ERANGE || *end != '\0' || start == end)
        {
            return false;
        }
        out = val;
        return true;
    }
    else if constexpr (std::is_integral_v<T>)
    {
        char*       end = nullptr;
        const char* start = s.c_str();
        errno = 0;

        long long val = 0;
        if constexpr (std::is_signed_v<T>)
            val = std::strtoll(start, &end, 10);
        else
            val = std::strtoull(start, &end, 10);

        if (errno == ERANGE || *end != '\0' || start == end)
        {
            return false;
        }

        if (val < std::numeric_limits<T>::min() || val > std::numeric_limits<T>::max())
            return false;

        out = static_cast<T>(val);
        return true;
    }
    return false;
}

bool GetTrimmedLine(std::ifstream& file, std::string& line);

bool GetTrimmedField(std::stringstream& ss, std::string& field, char delimiter = ',');

}  // namespace StringUtils
}  // namespace Dive
