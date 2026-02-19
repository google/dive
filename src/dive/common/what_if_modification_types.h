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

#include <array>
#include <string_view>
#include <vector>

namespace Dive
{

enum class WhatModificationType
{
    kDrawCallDisabled,
    kImageCreationFlagRemoved,
    kRenderPassLoadStoreOpOverridden,
    kRenderPassScissorOverridden,
    kAnisotropicFilterDisabled,
    kTimestampsDisabled,
};

struct WhatIfModificationTypeInfo
{
    WhatModificationType type;
    std::string_view ui_name;
    std::string_view ui_name_short;
    std::span<const std::string_view> supported_commands;
};

static constexpr std::string_view kDrawCmds[] = {"vkCmdDraw", "vkCmdDrawIndexed",
                                                 "vkCmdDrawIndirect", "vkCmdDrawIndexedIndirect"};
static constexpr std::string_view kImageCmds[] = {"vkCmdCreateImage"};
static constexpr std::string_view kRenderPassCmds[] = {"vkCmdBeginRenderPass",
                                                       "vkCmdBeginRenderPass2"};
static constexpr std::string_view kSamplerCmds[] = {"vkCreateSampler"};
static constexpr std::string_view kTimestampCmds[] = {"vkCmdWriteTimestamp"};

inline constexpr std::array<WhatIfModificationTypeInfo, 6> kWhatIfModificationTypeInfos = {{
    {.type = WhatModificationType::kDrawCallDisabled,
     .ui_name = "Draw calls were disabled",
     .ui_name_short = "Disable Draw Calls",
     .supported_commands = kDrawCmds},
    {.type = WhatModificationType::kImageCreationFlagRemoved,
     .ui_name = "Image creation flags were removed",
     .ui_name_short = "Remove Image Flags",
     .supported_commands = kImageCmds},
    {.type = WhatModificationType::kRenderPassLoadStoreOpOverridden,
     .ui_name = "Load/Store operations were overridden in the render pass",
     .ui_name_short = "Override Load/Store Operations",
     .supported_commands = kRenderPassCmds},
    {.type = WhatModificationType::kRenderPassScissorOverridden,
     .ui_name = "The scissor of a renderpass was set to 1x1",
     .ui_name_short = "Override Scissor",
     .supported_commands = kRenderPassCmds},
    {.type = WhatModificationType::kAnisotropicFilterDisabled,
     .ui_name = "Anisotropic filters were disabled",
     .ui_name_short = "Disable Anisotropic Filters",
     .supported_commands = kSamplerCmds},
    {.type = WhatModificationType::kTimestampsDisabled,
     .ui_name = "Timestamps were disabled",
     .ui_name_short = "Disable Timestamps",
     .supported_commands = kTimestampCmds},
}};

}  // namespace Dive
