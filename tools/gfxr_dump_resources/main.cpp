#include <iostream>

#include "gfxreconstruct/framework/decode/file_processor.h"
#include "gfxreconstruct/framework/generated/generated_vulkan_consumer.h"
#include "gfxreconstruct/framework/generated/generated_vulkan_decoder.h"

namespace
{

class MyConsumer : public gfxrecon::decode::VulkanConsumer
{
public:
    void Process_vkQueueSubmit(
    const gfxrecon::decode::ApiCallInfo&                                            call_info,
    VkResult                                                                        returnValue,
    gfxrecon::format::HandleId                                                      queue,
    uint32_t                                                                        submitCount,
    gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkSubmitInfo>* pSubmits,
    gfxrecon::format::HandleId                                                      fence) override
    {
        std::cout << "Process_vkQueueSubmit\n";
    }
};

}  // namespace

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: gfxr_dump_resources FILE.GFXR\n";
        return 1;
    }

    const char* input_filename = argv[1];

    gfxrecon::decode::FileProcessor file_processor;
    if (!file_processor.Initialize(input_filename))
    {
        std::cerr << "Failed to open " << input_filename << '\n';
        return 1;
    }

    gfxrecon::decode::VulkanDecoder vulkan_decoder;
    MyConsumer                      my_consumer;
    vulkan_decoder.AddConsumer(&my_consumer);
    file_processor.AddDecoder(&vulkan_decoder);

    file_processor.ProcessAllFrames();

    std::cout << "Hello world\n";
    return 0;
}