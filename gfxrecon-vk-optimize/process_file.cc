#include "process_file.h"

#include <filesystem>

#include "decode/file_processor.h"
#include "generated/generated_vulkan_consumer.h"
#include "generated/generated_vulkan_decoder.h"
#include "util/defines.h"
#include "util/logging.h"

GFXRECON_BEGIN_NAMESPACE(gfxrecon)
GFXRECON_BEGIN_NAMESPACE(decode)

bool ProcessFile(const std::filesystem::path& file, VulkanConsumer& consumer)
{
    FileProcessor file_processor;
    if (!file_processor.Initialize(file))
    {
        GFXRECON_LOG_INFO("Failed to open capture file: %s", file.string().c_str());
        return false;
    }

    VulkanDecoder decoder;
    decoder.AddConsumer(&consumer);
    file_processor.AddDecoder(&decoder);

    if (!file_processor.ProcessAllFrames())
    {
        GFXRECON_LOG_INFO("A failure has occurred during file processing: %s", file.string().c_str());
        return false;
    }

    return true;
}

GFXRECON_END_NAMESPACE(decode)
GFXRECON_END_NAMESPACE(gfxrecon)
