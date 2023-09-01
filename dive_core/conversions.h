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

static double ConvertCyclesToMs(uint64_t cycle)
{
    return (cycle / Dive::kClockMhz) / 1000.0;
}

static double ConvertCyclesToUs(uint64_t cycle)
{
    return (cycle / Dive::kClockMhz);
}

static double ConvertCyclesToNs(uint64_t cycle)
{
    return (cycle / Dive::kClockMhz) * 1000.0;
}