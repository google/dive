/*
 Copyright 2021 Google LLC

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

#include <stdint.h>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>

#include "analysis.h"

namespace Dive
{
namespace Analysis
{
void PrintDuration(std::ostream& ostr, uint64_t cycles)
{
    ostr << std::setprecision(2);
    double us = cycles / kClockMhz;
    if (us > 100)
    {
        double ms = us / 1000.0;
        ostr << ms << "ms";
    }
    else if (us < 0.1)
    {
        double ns = us * 1000.0;
        ostr << ns << "ns";
    }
    else
    {
        ostr << us << "us";
    }
}

std::string DurationString(uint64_t cycles)
{
    std::ostringstream ostr;
    PrintDuration(ostr, cycles);
    return ostr.str();
}

EventStateInfo::ConstIterator GetStateInfoForEvent(const EventStateInfo& state, uint32_t event_id)
{
    return state.find(static_cast<EventStateId>(event_id));
}
}  // namespace Analysis
}  // namespace Dive
