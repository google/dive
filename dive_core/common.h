/*
 Copyright 2020 Google LLC

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
#include <cstdint>
#include <vector>

// Include common code in driver-side code so they can be used by host-side code as well
#include "dive_core/common/common.h"

#ifdef _MSC_VER
#    define strncasecmp _strnicmp
#    define strcasecmp _stricmp
#    define strtok_r strtok_s
#    pragma warning(disable : 4996)
#endif

namespace Dive
{
namespace Legacy
{
/***************************************************/
// TODO(wangra): cleanup fixme!
// put following enums here temporarily
// following types are from gfx9_plus_merged_enum.h
// replace with adreno enums
typedef enum ZFormat
{
    Z_INVALID = 0x00000000,
    Z_16 = 0x00000001,
    Z_24 = 0x00000002,
    Z_32_FLOAT = 0x00000003,
} ZFormat;

typedef enum ZOrder
{
    LATE_Z = 0x00000000,
    EARLY_Z_THEN_LATE_Z = 0x00000001,
    RE_Z = 0x00000002,
    EARLY_Z_THEN_RE_Z = 0x00000003,
} ZOrder;

typedef enum ColorFormat
{
    COLOR_INVALID = 0x00000000,
    COLOR_8 = 0x00000001,
    COLOR_16 = 0x00000002,
    COLOR_8_8 = 0x00000003,
    COLOR_32 = 0x00000004,
    COLOR_16_16 = 0x00000005,
    COLOR_10_11_11 = 0x00000006,
    COLOR_11_11_10 = 0x00000007,
    COLOR_10_10_10_2 = 0x00000008,
    COLOR_2_10_10_10 = 0x00000009,
    COLOR_8_8_8_8 = 0x0000000a,
    COLOR_32_32 = 0x0000000b,
    COLOR_16_16_16_16 = 0x0000000c,
    COLOR_RESERVED_13 = 0x0000000d,
    COLOR_32_32_32_32 = 0x0000000e,
    COLOR_RESERVED_15 = 0x0000000f,
    COLOR_5_6_5 = 0x00000010,
    COLOR_1_5_5_5 = 0x00000011,
    COLOR_5_5_5_1 = 0x00000012,
    COLOR_4_4_4_4 = 0x00000013,
    COLOR_8_24 = 0x00000014,
    COLOR_24_8 = 0x00000015,
    COLOR_X24_8_32_FLOAT = 0x00000016,
    COLOR_RESERVED_23 = 0x00000017,
    COLOR_RESERVED_25 = 0x00000019,
    COLOR_RESERVED_26 = 0x0000001a,
    COLOR_RESERVED_27 = 0x0000001b,
    COLOR_RESERVED_28 = 0x0000001c,
    COLOR_RESERVED_29 = 0x0000001d,
    COLOR_2_10_10_10_6E4 = 0x0000001f,
    COLOR_RESERVED_24__GFX09 = 0x00000018,
    COLOR_RESERVED_30__GFX09 = 0x0000001e,
    COLOR_RESERVED_24__GFX101 = 0x00000018,
    COLOR_RESERVED_30__GFX101 = 0x0000001e,
    COLOR_2_10_10_10_7E3__GFX103COREPLUS = 0x0000001e,
    COLOR_5_9_9_9__GFX103PLUS = 0x00000018,
} ColorFormat;

typedef enum BUF_DATA_FORMAT
{
    BUF_DATA_FORMAT_INVALID = 0x00000000,
    BUF_DATA_FORMAT_8 = 0x00000001,
    BUF_DATA_FORMAT_16 = 0x00000002,
    BUF_DATA_FORMAT_8_8 = 0x00000003,
    BUF_DATA_FORMAT_32 = 0x00000004,
    BUF_DATA_FORMAT_16_16 = 0x00000005,
    BUF_DATA_FORMAT_10_11_11 = 0x00000006,
    BUF_DATA_FORMAT_11_11_10 = 0x00000007,
    BUF_DATA_FORMAT_10_10_10_2 = 0x00000008,
    BUF_DATA_FORMAT_2_10_10_10 = 0x00000009,
    BUF_DATA_FORMAT_8_8_8_8 = 0x0000000a,
    BUF_DATA_FORMAT_32_32 = 0x0000000b,
    BUF_DATA_FORMAT_16_16_16_16 = 0x0000000c,
    BUF_DATA_FORMAT_32_32_32 = 0x0000000d,
    BUF_DATA_FORMAT_32_32_32_32 = 0x0000000e,
    BUF_DATA_FORMAT_RESERVED_15 = 0x0000000f,
} BUF_DATA_FORMAT;

typedef enum BUF_NUM_FORMAT
{
    BUF_NUM_FORMAT_UNORM = 0x00000000,
    BUF_NUM_FORMAT_SNORM = 0x00000001,
    BUF_NUM_FORMAT_USCALED = 0x00000002,
    BUF_NUM_FORMAT_SSCALED = 0x00000003,
    BUF_NUM_FORMAT_UINT = 0x00000004,
    BUF_NUM_FORMAT_SINT = 0x00000005,
    BUF_NUM_FORMAT_FLOAT = 0x00000007,
    BUF_NUM_FORMAT_SNORM_NZ__GFX10CORE = 0x00000006,
    BUF_NUM_FORMAT_RESERVED_6__NOTGFX10 = 0x00000006,
} BUF_NUM_FORMAT;

typedef enum SQ_SEL_XYZW01
{
    SQ_SEL_0 = 0x00000000,
    SQ_SEL_1 = 0x00000001,
    SQ_SEL_N_BC_1 = 0x00000002,
    SQ_SEL_RESERVED_1 = 0x00000003,
    SQ_SEL_X = 0x00000004,
    SQ_SEL_Y = 0x00000005,
    SQ_SEL_Z = 0x00000006,
    SQ_SEL_W = 0x00000007,
} SQ_SEL_XYZW01;
}  // namespace Legacy
}  // namespace Dive
   /***************************************************/