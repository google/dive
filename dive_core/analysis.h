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
#pragma once

#include "dive_core/common/gpudefs.h"
#include "event_state.h"

namespace Dive
{
namespace Analysis
{
constexpr double   kReportThresholdUs = 100.0;
constexpr uint64_t kReportThresholdCycles = static_cast<uint64_t>(kReportThresholdUs * kClockMhz);

void        PrintDuration(std::ostream& ostr, uint64_t cycles);
std::string DurationString(uint64_t cycles);

EventStateInfo::ConstIterator GetStateInfoForEvent(const EventStateInfo& state, uint32_t event_id);
}  // namespace Analysis
}  // namespace Dive
