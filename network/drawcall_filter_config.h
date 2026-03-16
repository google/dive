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

struct DrawcallFilterConfig
{
    // Vertex Count (for vkCmdDraw)
    bool filter_by_vertex_count = false;
    uint32_t target_vertex_count = 0;

    // Index Count (for vkCmdDrawIndexed)
    bool filter_by_index_count = false;
    uint32_t target_index_count = 0;

    // Instance Count (for vkCmdDraw and vkCmdDrawIndexed)
    bool filter_by_instance_count = false;
    uint32_t target_instance_count = 0;

    // Drawcall limit
    bool enable_drawcall_limit = false;
    uint32_t max_drawcalls = 0;
};

}  // namespace Network
