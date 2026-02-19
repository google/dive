#pragma once

#include <filesystem>

#include "generated/generated_vulkan_consumer.h"
#include "util/defines.h"

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

bool ProcessFile(const std::filesystem::path& file, VulkanConsumer& consumer);

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)
