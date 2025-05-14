#include <cstdint>
#include <functional>
#include <iostream>
#include <optional>
#include <unordered_map>

#include "gfxreconstruct/framework/decode/api_decoder.h"
#include "gfxreconstruct/framework/decode/file_processor.h"
#include "gfxreconstruct/framework/decode/struct_pointer_decoder.h"
#include "gfxreconstruct/framework/format/format.h"
#include "gfxreconstruct/framework/generated/generated_vulkan_consumer.h"
#include "gfxreconstruct/framework/generated/generated_vulkan_decoder.h"
#include "gfxreconstruct/framework/generated/generated_vulkan_struct_decoders.h"

namespace
{

// TODO: Process_vkBeginCommandBuffer: commandBuffer=18446744073709551612
// Process_vkQueueSubmit
// emit
// Process_vkBeginCommandBuffer: commandBuffer=18446744073709551612
// Process_vkQueueSubmit
// emit

// Mirrors dump resources JSON schema.
// During MyConsumer, this may be incomplete and missing info.
// When dump_found_callback is run, it should be complete and ready for `--dump-resources`.
struct DumpEntry
{
    uint64_t begin_command_buffer_block_index = 0;
    uint64_t queue_submit_block_index = 0;
};

struct IncompleteDump
{
    DumpEntry                         dump_entry{};
    gfxrecon::decode::VulkanConsumer* state = nullptr;
};

class State
{
public:
    virtual ~State() = default;

    virtual void TransitionTo(State& from_state) { dump_entry_ = from_state.TransitionFrom(); }

private:
    virtual DumpEntry TransitionFrom()
    {
        DumpEntry dump_entry = std::move(dump_entry_).value();
        dump_entry_ = {};
        return std::move(dump_entry);
    }

    std::optional<DumpEntry> dump_entry_;
};

// TODO rename state machine
class MyConsumer : public gfxrecon::decode::VulkanConsumer
{
public:
    MyConsumer(std::function<void(DumpEntry)> dump_found_callback) :
        dump_found_callback_(std::move(dump_found_callback))
    {
    }

    void Process_vkBeginCommandBuffer(
    const gfxrecon::decode::ApiCallInfo& call_info,
    VkResult                             returnValue,
    gfxrecon::format::HandleId           commandBuffer,
    gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkCommandBufferBeginInfo>*
    pBeginInfo) override
    {
        std::cerr << "Process_vkBeginCommandBuffer: commandBuffer=" << commandBuffer << '\n';
        // TODO set state
        IncompleteDump incomplete_dump{ DumpEntry{
        .begin_command_buffer_block_index = call_info.index } };
        auto [it, inserted] = incomplete_dumps_.try_emplace(commandBuffer,
                                                            std::move(incomplete_dump));
        if (!inserted)
        {
            std::cerr << "Command buffer " << commandBuffer
                      << " never submitted! Discarding previous state...\n";
            // TODO why are some made but never submitted?
            it->second = std::move(incomplete_dump);
            return;
        }
    }

    void Process_vkQueueSubmit(
    const gfxrecon::decode::ApiCallInfo&                                            call_info,
    VkResult                                                                        returnValue,
    gfxrecon::format::HandleId                                                      queue,
    uint32_t                                                                        submitCount,
    gfxrecon::decode::StructPointerDecoder<gfxrecon::decode::Decoded_VkSubmitInfo>* pSubmits,
    gfxrecon::format::HandleId                                                      fence) override
    {
        std::cerr << "Process_vkQueueSubmit\n";
        for (int submit_index = 0; submit_index < submitCount; submit_index++)
        {
            const gfxrecon::decode::Decoded_VkSubmitInfo&
            submit = pSubmits->GetMetaStructPointer()[submit_index];
            for (int command_buffer_index = 0;
                 command_buffer_index < pSubmits->GetPointer()->commandBufferCount;
                 command_buffer_index++)
            {
                gfxrecon::format::HandleId command_buffer_id = submit.pCommandBuffers
                                                               .GetPointer()[command_buffer_index];
                if (auto it = incomplete_dumps_.find(command_buffer_id);
                    it != incomplete_dumps_.end())
                {
                    gfxrecon::format::HandleId command_buffer_id = it->first;
                    IncompleteDump&            incomplete_dump = it->second;
                    DumpEntry                  dump_entry = std::move(incomplete_dump.dump_entry);
                    incomplete_dumps_.erase(it);
                    dump_entry.queue_submit_block_index = call_info.index;
                    // TODO assert begin is before submit
                    dump_found_callback_(std::move(dump_entry));
                    std::cerr << "emit\n";
                }
                else
                {
                    std::cerr << "Command buffer " << command_buffer_id << " never started!\n";
                }
            }
        }
    }

private:
    // Function run when a complete dump entry has been formed. This is ready to be written to disk,
    // etc.
    std::function<void(DumpEntry)> dump_found_callback_;
    // Incomplete dumps for each command buffer
    std::unordered_map<gfxrecon::format::HandleId, IncompleteDump> incomplete_dumps_;
    // TODO separate map of open command buffers?
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

    std::vector<DumpEntry> complete_dump_entries;

    gfxrecon::decode::VulkanDecoder vulkan_decoder;
    MyConsumer                      my_consumer([&complete_dump_entries](DumpEntry dump_entry) {
        complete_dump_entries.push_back(std::move(dump_entry));
    });
    vulkan_decoder.AddConsumer(&my_consumer);
    file_processor.AddDecoder(&vulkan_decoder);

    file_processor.ProcessAllFrames();

    std::cout << "Found " << complete_dump_entries.size() << " dumpables\n";
    for (const DumpEntry& dump : complete_dump_entries)
    {
        std::cout << " Dump\n";
        std::cout << "   BeginCommandBuffer=" << dump.begin_command_buffer_block_index << '\n';
        std::cout << "   QueueSubmit=" << dump.queue_submit_block_index << '\n';
    }

    return 0;
}