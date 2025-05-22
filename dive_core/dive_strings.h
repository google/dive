/*
 Copyright 2019 Google LLC

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
#include <string>

#include "third_party/Vulkan-Headers/include/vulkan/vulkan.h"

#include "common.h"
#include "dive_core/common/dive_capture_format.h"
#include "dive_core/common/gpudefs.h"

#ifndef _MSC_VER
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wunused-variable"
#endif

// clang-format off
static const char *kShaderStageStrings[(uint32_t)Dive::ShaderStage::kShaderStageCount] =
{   "CS Shader",
    "GS Shader",
    "HS Shader",
    "PS Shader",
    "VS Shader"
};

static const char *kEngineTypeStrings[(uint32_t)Dive::EngineType::kCount] =
{
    "Universal",
    "Compute",
    "Dma",
    "Timer",
    "Other",
    "None"
};

static const char *kQueueTypeStrings[(uint32_t)Dive::QueueType::kCount] =
{
    "Universal",
    "Compute",
    "Dma",
    "Timer",
    "Other",
    "None"
};

static const char *kBufferDataFormatStrings[] =
{
    "INVALID",
    "8",
    "16",
    "8_8",
    "32",
    "16_16",
    "10_11_11",
    "11_11_10",
    "10_10_10_2",
    "2_10_10_10",
    "8_8_8_8",
    "32_32",
    "16_16_16_16",
    "32_32_32",
    "32_32_32_32",
    "RESERVED_15"
};

static const char *kBufferNumFormatStrings[] =
{
    "UNORM",
    "SNORM",
    "USCALED",
    "SSCALED",
    "UINT",
    "SINT",
    "RESERVED_6__GFX09",
    "FLOAT"
};

static const char *kSqSelStrings[] =
{
    "0",
    "1",
    "N_BC_1",
    "RESERVED_1",
    "X",
    "Y",
    "Z",
    "W"
};

static const char *kVgtEventTypeStrings[] =
{
    "Reserved_0x00",
    "SAMPLE_STREAMOUTSTATS1",
    "SAMPLE_STREAMOUTSTATS2",
    "SAMPLE_STREAMOUTSTATS3",
    "CACHE_FLUSH_TS",
    "CONTEXT_DONE",
    "CACHE_FLUSH",
    "CS_PARTIAL_FLUSH",
    "VGT_STREAMOUT_SYNC",
    "SET_FE_ID__GFX09",
    "VGT_STREAMOUT_RESET",
    "END_OF_PIPE_INCR_DE",
    "END_OF_PIPE_IB_END",
    "RST_PIX_CNT",
    "BREAK_BATCH",
    "VS_PARTIAL_FLUSH",
    "PS_PARTIAL_FLUSH",
    "FLUSH_HS_OUTPUT",
    "FLUSH_DFSM",
    "RESET_TO_LOWEST_VGT",
    "CACHE_FLUSH_AND_INV_TS_EVENT",
    "ZPASS_DONE",
    "CACHE_FLUSH_AND_INV_EVENT",
    "PERFCOUNTER_START",
    "PERFCOUNTER_STOP",
    "PIPELINESTAT_START",
    "PIPELINESTAT_STOP",
    "PERFCOUNTER_SAMPLE",
    "Available_0x1c__GFX09",
    "Available_0x1d__GFX09",
    "SAMPLE_PIPELINESTAT",
    "SO_VGTSTREAMOUT_FLUSH",
    "SAMPLE_STREAMOUTSTATS",
    "RESET_VTX_CNT",
    "BLOCK_CONTEXT_DONE",
    "CS_CONTEXT_DONE",
    "VGT_FLUSH",
    "TGID_ROLLOVER",
    "SQ_NON_EVENT",
    "SC_SEND_DB_VPZ",
    "BOTTOM_OF_PIPE_TS",
    "FLUSH_SX_TS",
    "DB_CACHE_FLUSH_AND_INV",
    "FLUSH_AND_INV_DB_DATA_TS",
    "FLUSH_AND_INV_DB_META",
    "FLUSH_AND_INV_CB_DATA_TS",
    "FLUSH_AND_INV_CB_META",
    "CS_DONE",
    "PS_DONE",
    "FLUSH_AND_INV_CB_PIXEL_DATA",
    "SX_CB_RAT_ACK_REQUEST",
    "THREAD_TRACE_START",
    "THREAD_TRACE_STOP",
    "THREAD_TRACE_MARKER",
    "THREAD_TRACE_FLUSH__GFX09",
    "THREAD_TRACE_FINISH",
    "PIXEL_PIPE_STAT_CONTROL",
    "PIXEL_PIPE_STAT_DUMP",
    "PIXEL_PIPE_STAT_RESET",
    "CONTEXT_SUSPEND",
    "OFFCHIP_HS_DEALLOC",
    "ENABLE_NGG_PIPELINE",
    "ENABLE_LEGACY_PIPELINE",
    "Reserved_0x3f__GFX09"
};

#ifndef _MSC_VER
#    pragma GCC diagnostic pop
#endif

const char *GetVkFormatString(uint32_t vk_format);
const char *GetVkColorSpaceKhrString(uint32_t vk_color_space_khr);
const char *GetVkImageLayoutString(uint32_t vk_image_layout);
const char *GetVkStencilFaceFlags(uint32_t vk_stencil_face_mask);
std::string GetVkCommandBufferUsageFlagBits(uint32_t vk_cmd_buffer_usage_flag_bits, const char *separator = "|");
std::string GetVkShaderStageBits(uint32_t vk_shader_stage_flag_bits, const char *separator = "|");
std::string GetVkPipelineStageBits(uint32_t vk_pipeline_stage_flag_bits, const char *separator = "|");
std::string GetVkQueryResultFlagBits(uint32_t vk_query_result_flag_bits, const char *separator = "|");
const char *GetVkPrimitiveTopology(VkPrimitiveTopology vk_primitive_topology);
const char *GetVkPolygonMode(VkPolygonMode vk_polygon_mode);
const char *GetVkCullModeFlags(VkCullModeFlags vk_cull_mode_flags);
const char *GetVkFrontFace(VkFrontFace vk_front_face);
const char *GetVkCompareOp(VkCompareOp vk_compare_op);
const char *GetVkStencilOp(VkStencilOp vk_stencil_op);
const char *GetVkSampleCountFlags(VkSampleCountFlags vk_sample_count_flag);
const char *GetZFormat(Dive::Legacy::ZFormat z_format);
const char *GetZOrder(Dive::Legacy::ZOrder z_order);
const char *GetColorFormat(Dive::Legacy::ColorFormat color_format);
const char *GetVkLogicOp(VkLogicOp vk_logic_op);
const char *GetVkBlendFactor(VkBlendFactor vk_blend_factor);
const char *GetVkBlendOp(VkBlendOp vk_blend_op);