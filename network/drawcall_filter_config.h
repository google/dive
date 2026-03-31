/*
Copyright 2026 Google Inc.

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

#include <cstdint>

namespace Network
{

// ==============================================================================================
// DRAWCALL FILTERING LIMITATION
// ==============================================================================================
// The drawcall filtering and limiting logic operates during command buffer recording
// (intercepting vkCmdDraw* calls), not during execution (vkQueueSubmit).
// Because of this, the drawcall limit will NOT apply to "pre-recorded" command buffers
// (buffers that are recorded once during initialization and submitted multiple times).
// It only successfully filters command buffers that are built dynamically every frame.
struct DrawcallFilterConfig
{
    std::string target_render_pass_name;

    uint32_t target_vertex_count = 0;
    uint32_t target_index_count = 0;
    uint32_t target_instance_count = 0;
    uint32_t max_drawcalls = 0;

    bool filter_by_vertex_count = false;
    bool filter_by_index_count = false;
    bool filter_by_instance_count = false;
    bool enable_drawcall_limit = false;
    bool filter_by_alpha_blended = false;
    bool filter_by_render_pass = false;
};

struct PSOInfo
{
    std::string name;
    // Cast VkPipeline to uint64_t for the network
    uint64_t pipeline_handle;
    bool has_alpha_blend;
};

struct RenderPassInfo
{
    std::string name;
    // Cast VkRenderPass to uint64_t for the network
    uint64_t render_pass_handle;
};

}  // namespace Network
