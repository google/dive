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

#include <cassert>

#include "command_arguments.h"
#include "command_decoder.h"
#include "command_printer.h"
#include "dive_core/dive_strings.h"

namespace Dive
{

void PrintCommandParametersBrief(std::ostream&                    os,
                                 VKCmdID                          cmdID,
                                 char*                            data,
                                 size_t                           data_size,
                                 const VulkanMetadataBlockHeader& metadata_version)
{
    if (data == nullptr || data_size == 0)
        return;

    CommandDecoder::AllocatorType allocator(data_size);

    CommandDecoder decoder(&allocator, metadata_version);
    os << "(";
    switch (cmdID)
    {
    case VKCmdID::vkQueueSubmit: break;
    case VKCmdID::vkBeginCommandBufferCmdID:
    {
        BeginCommandBufferArgs* args = decoder.DecodeBeginCommandBuffer(data, data_size);
        // If there is more than 1, print "..." to keep it brief
        if ((args->pBeginInfo->flags & (args->pBeginInfo->flags - 1)) != 0)
            os << "flags:...";
        else
            os << "flags:" << GetVkCommandBufferUsageFlagBits(args->pBeginInfo->flags);
        break;
    };
    break;
    case VKCmdID::vkEndCommandBufferCmdID: break;
    case VKCmdID::vkCmdExecuteCommandsCmdID:
    {
        CmdExecuteCommandsArgs* args = decoder.DecodeCmdExecuteCommands(data, data_size);
        os << "cmdBufferCount:" << args->commandBufferCount;
        break;
    }
    case VKCmdID::vkCmdCopyBufferCmdID:
    {
        CmdCopyBufferArgs* args = decoder.DecodeCmdCopyBuffer(data, data_size);
        os << "regionCount:" << args->regionCount;
        break;
    }
    case VKCmdID::vkCmdCopyImageCmdID:
    {
        CmdCopyImageArgs* args = decoder.DecodeCmdCopyImage(data, data_size);
        os << "srcLayout:" << GetVkImageLayoutString(args->srcImageLayout)
           << ",dstLayout:" << GetVkImageLayoutString(args->dstImageLayout)
           << ",regionCount:" << args->regionCount;
        break;
    }
    case VKCmdID::vkCmdBlitImageCmdID:
    {
        CmdBlitImageArgs* args = decoder.DecodeCmdBlitImage(data, data_size);
        os << "srcLayout:" << GetVkImageLayoutString(args->srcImageLayout)
           << ",dstLayout:" << GetVkImageLayoutString(args->dstImageLayout)
           << ",regionCount:" << args->regionCount;
        break;
    }
    case VKCmdID::vkCmdCopyBufferToImageCmdID:
    {
        CmdCopyBufferToImageArgs* args = decoder.DecodeCmdCopyBufferToImage(data, data_size);
        os << "dstLayout:" << GetVkImageLayoutString(args->dstImageLayout)
           << ",regionCount:" << args->regionCount;
        break;
    }
    case VKCmdID::vkCmdCopyImageToBufferCmdID:
    {
        CmdCopyImageToBufferArgs* args = decoder.DecodeCmdCopyImageToBuffer(data, data_size);
        os << "srcLayout:" << GetVkImageLayoutString(args->srcImageLayout)
           << ",regionCount:" << args->regionCount;
        break;
    }
    case VKCmdID::vkCmdUpdateBufferCmdID:
    {
        CmdUpdateBufferArgs* args = decoder.DecodeCmdUpdateBuffer(data, data_size);
        os << "offset:" << args->dstOffset << ",size:" << args->dataSize;
        break;
    }
    case VKCmdID::vkCmdFillBufferCmdID:
    {
        CmdFillBufferArgs* args = decoder.DecodeCmdFillBuffer(data, data_size);
        os << "offset:" << args->dstOffset << ",size:" << args->size;
        break;
    }
    case VKCmdID::vkCmdClearColorImageCmdID:
    {
        CmdClearColorImageArgs* args = decoder.DecodeCmdClearColorImage(data, data_size);
        os << "layout:" << GetVkImageLayoutString(args->imageLayout) << ",color:("
           << args->pColor->float32[0] << "," << args->pColor->float32[1]
           << args->pColor->float32[2] << "," << args->pColor->float32[3] << ")"
           << ",rangeCount:" << args->rangeCount;
        break;
    }
    case VKCmdID::vkCmdClearDepthStencilImageCmdID:
    {
        CmdClearDepthStencilImageArgs* args = decoder.DecodeCmdClearDepthStencilImage(data,
                                                                                      data_size);
        os << "layout:" << GetVkImageLayoutString(args->imageLayout)
           << ",depth:" << args->pDepthStencil->depth << ",stencil:" << args->pDepthStencil->stencil
           << ",rangeCount:" << args->rangeCount;
        break;
    }
    case VKCmdID::vkCmdClearAttachmentsCmdID:
    {
        CmdClearAttachmentsArgs* args = decoder.DecodeCmdClearAttachments(data, data_size);
        os << "attachmentCount:" << args->attachmentCount << ",rectCount:" << args->rectCount;
        break;
    }
    case VKCmdID::vkCmdResolveImageCmdID:
    {
        CmdResolveImageArgs* args = decoder.DecodeCmdResolveImage(data, data_size);
        os << "srcLayout:" << GetVkImageLayoutString(args->srcImageLayout)
           << ",dstLayout:" << GetVkImageLayoutString(args->dstImageLayout)
           << ",regionCount:" << args->regionCount;
        break;
    }
    case VKCmdID::vkCmdBindDescriptorSetsCmdID:
    {
        CmdBindDescriptorSetsArgs* args = decoder.DecodeCmdBindDescriptorSets(data, data_size);
        os << "bindPoint:";
        switch (args->pipelineBindPoint)
        {
        case VK_PIPELINE_BIND_POINT_GRAPHICS: os << "GRAPHICS"; break;
        case VK_PIPELINE_BIND_POINT_COMPUTE: os << "COMPUTE"; break;
        case VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR: os << "RAY_TRACING"; break;
        }
        os << ",firstSet:" << args->firstSet << ",setCount:" << args->descriptorSetCount
           << ",dynamicOffsetCount:" << args->dynamicOffsetCount;
        break;
    }
    case VKCmdID::vkCmdPushConstantsCmdID:
    {
        CmdPushConstantsArgs* args = decoder.DecodeCmdPushConstants(data, data_size);
        // If there is more than 1, print "..." to keep it brief
        if ((args->stageFlags & (args->stageFlags - 1)) != 0)
            os << "stage:...";
        else
            os << "stage:" << GetVkShaderStageBits(args->stageFlags);
        os << ",offset:" << args->offset << ",size:" << args->size;
        break;
    }
    case VKCmdID::vkCmdBindIndexBufferCmdID:
    {
        CmdBindIndexBufferArgs* args = decoder.DecodeCmdBindIndexBuffer(data, data_size);
        os << "offset:" << args->offset << ",indexType:";
        switch (args->indexType)
        {
        case VK_INDEX_TYPE_UINT16: os << "UINT16"; break;
        case VK_INDEX_TYPE_UINT32: os << "UINT32"; break;
        case VK_INDEX_TYPE_NONE_KHR: os << "NONE_KHR"; break;
        case VK_INDEX_TYPE_UINT8_EXT: os << "UINT8_EXT"; break;
        }
        break;
    }
    case VKCmdID::vkCmdBindVertexBuffersCmdID:
    {
        CmdBindVertexBuffersArgs* args = decoder.DecodeCmdBindVertexBuffers(data, data_size);
        os << "first:" << args->firstBinding << ",count:" << args->bindingCount;
        break;
    }
    case VKCmdID::vkCmdDrawCmdID:
    {
        CmdDrawArgs* args = decoder.DecodeCmdDraw(data, data_size);
        os << "vertexCount:" << args->vertexCount << ",instanceCount:" << args->instanceCount
           << ",firstVertex:" << args->firstVertex << ",firstInstance:" << args->firstInstance;
        break;
    }
    case VKCmdID::vkCmdDrawIndexedCmdID:
    {
        CmdDrawIndexedArgs* args = decoder.DecodeCmdDrawIndexed(data, data_size);
        os << "indexCount:" << args->indexCount << ",instanceCount:" << args->instanceCount
           << ",firstIndex:" << args->firstIndex << ",firstInstance:" << args->firstInstance;
        break;
    }
    case VKCmdID::vkCmdDrawIndirectCmdID:
    {
        CmdDrawIndirectArgs* args = decoder.DecodeCmdDrawIndirect(data, data_size);
        os << "offset:" << args->offset << ",drawCount:" << args->drawCount
           << ",stride:" << args->stride;
        break;
    }
    case VKCmdID::vkCmdDrawIndexedIndirectCmdID:
    {
        CmdDrawIndexedIndirectArgs* args = decoder.DecodeCmdDrawIndexedIndirect(data, data_size);
        os << "offset:" << args->offset << ",drawCount:" << args->drawCount
           << ",stride:" << args->stride;
        break;
    }
    case VKCmdID::vkCmdDispatchCmdID:
    {
        CmdDispatchArgs* args = decoder.DecodeCmdDispatch(data, data_size);
        os << "x:" << args->groupCountX << ",y:" << args->groupCountY << ",z:" << args->groupCountZ;
        break;
    }
    case VKCmdID::vkCmdDispatchIndirectCmdID:
    {
        CmdDispatchIndirectArgs* args = decoder.DecodeCmdDispatchIndirect(data, data_size);
        os << "offset:" << args->offset;
        break;
    }
    case VKCmdID::vkCmdBindPipelineCmdID:
    {
        CmdBindPipelineArgs* args = decoder.DecodeCmdBindPipeline(data, data_size);
        os << "bindPoint:";
        switch (args->pipelineBindPoint)
        {
        case VK_PIPELINE_BIND_POINT_GRAPHICS: os << "GRAPHICS"; break;
        case VK_PIPELINE_BIND_POINT_COMPUTE: os << "COMPUTE"; break;
        case VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR: os << "RAY_TRACING"; break;
        }
        break;
    }
    case VKCmdID::vkCmdSetViewportCmdID:
    {
        CmdSetViewportArgs* args = decoder.DecodeCmdSetViewport(data, data_size);
        os << "first:" << args->firstViewport << ",count:" << args->viewportCount;
        break;
    }
    case VKCmdID::vkCmdSetScissorCmdID:
    {
        CmdSetScissorArgs* args = decoder.DecodeCmdSetScissor(data, data_size);
        os << "first:" << args->firstScissor << ",count:" << args->scissorCount;
        break;
    }
    case VKCmdID::vkCmdSetLineWidthCmdID:
    {
        CmdSetLineWidthArgs* args = decoder.DecodeCmdSetLineWidth(data, data_size);
        os << "lineWidth:" << args->lineWidth;
        break;
    }
    case VKCmdID::vkCmdSetDepthBiasCmdID:
    {
        CmdSetDepthBiasArgs* args = decoder.DecodeCmdSetDepthBias(data, data_size);
        os << "constant:" << args->depthBiasConstantFactor << ",clamp:" << args->depthBiasClamp
           << ",slope:" << args->depthBiasSlopeFactor;
        break;
    }
    case VKCmdID::vkCmdSetBlendConstantsCmdID:
    {
        CmdSetBlendConstantsArgs* args = decoder.DecodeCmdSetBlendConstants(data, data_size);
        os << "blendConstants:(" << args->blendConstants[0] << "," << args->blendConstants[1] << ","
           << args->blendConstants[2] << "," << args->blendConstants[3] << ")";
        break;
    }
    case VKCmdID::vkCmdSetDepthBoundsCmdID:
    {
        CmdSetDepthBoundsArgs* args = decoder.DecodeCmdSetDepthBounds(data, data_size);
        os << "min:" << args->minDepthBounds << ",max:" << args->maxDepthBounds;
        break;
    }
    case VKCmdID::vkCmdSetStencilCompareMaskCmdID:
    {
        CmdSetStencilCompareMaskArgs* args = decoder.DecodeCmdSetStencilCompareMask(data,
                                                                                    data_size);
        os << "faceMask:" << GetVkStencilFaceFlags(args->faceMask) << std::hex << ",compareMask:0x"
           << args->compareMask << std::dec;
        break;
    }
    case VKCmdID::vkCmdSetStencilWriteMaskCmdID:
    {
        CmdSetStencilWriteMaskArgs* args = decoder.DecodeCmdSetStencilWriteMask(data, data_size);
        os << "faceMask:" << GetVkStencilFaceFlags(args->faceMask) << std::hex << ",writeMask:0x"
           << args->writeMask << std::dec;
        break;
    }
    case VKCmdID::vkCmdSetStencilReferenceCmdID:
    {
        CmdSetStencilReferenceArgs* args = decoder.DecodeCmdSetStencilReference(data, data_size);
        os << "faceMask:" << GetVkStencilFaceFlags(args->faceMask) << std::hex
           << ",reference:" << args->reference;
        break;
    }
    case VKCmdID::vkCmdBeginQueryCmdID:
    {
        CmdBeginQueryArgs* args = decoder.DecodeCmdBeginQuery(data, data_size);
        os << "query:" << args->query;
        if (args->flags != 0)
            os << ",flags:CONTROL_PRECISE";
        break;
    }
    case VKCmdID::vkCmdEndQueryCmdID:
    {
        CmdEndQueryArgs* args = decoder.DecodeCmdEndQuery(data, data_size);
        os << "query:" << args->query;
        break;
    }
    case VKCmdID::vkCmdResetQueryPoolCmdID:
    {
        CmdResetQueryPoolArgs* args = decoder.DecodeCmdResetQueryPool(data, data_size);
        os << "first:" << args->firstQuery << ",count:" << args->queryCount;
        break;
    }
    case VKCmdID::vkCmdWriteTimestampCmdID:
    {
        CmdWriteTimestampArgs* args = decoder.DecodeCmdWriteTimestamp(data, data_size);
        // If there is more than 1, print "..." to keep it brief
        if ((args->pipelineStage & (args->pipelineStage - 1)) != 0)
            os << "stage:...";
        else
            os << "stage:" << GetVkPipelineStageBits(args->pipelineStage);
        os << ",query:" << args->query;
        break;
    }
    case VKCmdID::vkCmdCopyQueryPoolResultsCmdID:
    {
        CmdCopyQueryPoolResultsArgs* args = decoder.DecodeCmdCopyQueryPoolResults(data, data_size);
        os << "firstQuery:" << args->firstQuery << ",count:" << args->queryCount
           << ",dstOffset:" << args->dstOffset << ",stride:" << args->stride;
        // If there is more than 1, print "..." to keep it brief
        if ((args->flags & (args->flags - 1)) != 0)
            os << ",flags:...";
        else
            os << ",flags:" << GetVkQueryResultFlagBits(args->flags);
        break;
    }
    case VKCmdID::vkCmdBeginRenderPassCmdID:
    {
        CmdBeginRenderPassArgs* args = decoder.DecodeCmdBeginRenderPass(data, data_size);
        os << "contents:";
        if (args->contents == VK_SUBPASS_CONTENTS_INLINE)
            os << "CONTENTS_INLINE";
        else if (args->contents == VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS)
            os << "SECONDARY_COMMAND_BUFFERS";
        break;
    }
    case VKCmdID::vkCmdNextSubpassCmdID:
    {
        CmdNextSubpassArgs* args = decoder.DecodeCmdNextSubpass(data, data_size);
        os << "contents:";
        if (args->contents == VK_SUBPASS_CONTENTS_INLINE)
            os << "CONTENTS_INLINE";
        else if (args->contents == VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS)
            os << "SECONDARY_COMMAND_BUFFERS";
        break;
    }
    case VKCmdID::vkCmdEndRenderPassCmdID: break;
    case VKCmdID::vkCmdSetEventCmdID:
    {
        CmdSetEventArgs* args = decoder.DecodeCmdSetEvent(data, data_size);
        // If there is more than 1, print "..." to keep it brief
        if ((args->stageMask & (args->stageMask - 1)) != 0)
            os << "stageMask:...";
        else
            os << "stageMask:" << GetVkPipelineStageBits(args->stageMask);
        break;
    }
    case VKCmdID::vkCmdResetEventCmdID:
    {
        CmdResetEventArgs* args = decoder.DecodeCmdResetEvent(data, data_size);
        // If there is more than 1, print "..." to keep it brief
        if ((args->stageMask & (args->stageMask - 1)) != 0)
            os << "stageMask:...";
        else
            os << "stageMask:" << GetVkPipelineStageBits(args->stageMask);
        break;
    }
    case VKCmdID::vkCmdWaitEventsCmdID:
    {
        CmdWaitEventsArgs* args = decoder.DecodeCmdWaitEvents(data, data_size);
        os << "eventCount" << args->eventCount;
        // If there is more than 1, print "..." to keep it brief
        if ((args->srcStageMask & (args->srcStageMask - 1)) != 0)
            os << ",srcStageMask:...";
        else
            os << ",srcStageMask:" << GetVkPipelineStageBits(args->srcStageMask);
        // If there is more than 1, print "..." to keep it brief
        if ((args->dstStageMask & (args->dstStageMask - 1)) != 0)
            os << ",dstStageMask:...";
        else
            os << ",dstStageMask:" << GetVkPipelineStageBits(args->dstStageMask);
        if (args->memoryBarrierCount > 0)
            os << ",memoryBarrierCount:" << args->memoryBarrierCount;
        if (args->bufferMemoryBarrierCount > 0)
            os << ",bufferBarrierCount:" << args->bufferMemoryBarrierCount;
        if (args->imageMemoryBarrierCount)
            os << ",imageBarrierCount:" << args->imageMemoryBarrierCount;
        break;
    }
    case VKCmdID::vkCmdPipelineBarrierCmdID:
    {
        CmdPipelineBarrierArgs* args = decoder.DecodeCmdPipelineBarrier(data, data_size);
        // If there is more than 1, print "..." to keep it brief
        if ((args->srcStageMask & (args->srcStageMask - 1)) != 0)
            os << "srcStageMask:...";
        else
            os << "srcStageMask:" << GetVkPipelineStageBits(args->srcStageMask);
        // If there is more than 1, print "..." to keep it brief
        if ((args->dstStageMask & (args->dstStageMask - 1)) != 0)
            os << ",dstStageMask:...";
        else
            os << ",dstStageMask:" << GetVkPipelineStageBits(args->dstStageMask);
        if (args->memoryBarrierCount > 0)
            os << ",memoryBarrierCount:" << args->memoryBarrierCount;
        if (args->bufferMemoryBarrierCount > 0)
            os << ",bufferBarrierCount:" << args->bufferMemoryBarrierCount;
        if (args->imageMemoryBarrierCount)
            os << ",imageBarrierCount:" << args->imageMemoryBarrierCount;
        break;
    }
    case VKCmdID::vkCmdWriteBufferMarkerAMDCmdID:
    {
        CmdWriteBufferMarkerAMDArgs* args = decoder.DecodeCmdWriteBufferMarkerAMD(data, data_size);
        // If there is more than 1, print "..." to keep it brief
        if ((args->pipelineStage & (args->pipelineStage - 1)) != 0)
            os << "pipelineStage:...";
        else
            os << "pipelineStage:" << GetVkPipelineStageBits(args->pipelineStage);
        os << ",dstOffset:" << args->dstOffset << ",marker:0x" << std::hex << args->marker
           << std::dec;
        break;
    }
    case VKCmdID::vkCmdDrawIndirectCountAMDCmdID:
    {
        CmdDrawIndirectCountAMDArgs* args = decoder.DecodeCmdDrawIndirectCountAMD(data, data_size);
        os << "offset:" << args->offset << ",countOffset:" << args->countOffset
           << ",maxDrawCount:" << args->maxDrawCount << ",stride:" << args->stride;
        break;
    }
    case VKCmdID::vkCmdDrawIndexedIndirectCountAMDCmdID:
    {
        CmdDrawIndexedIndirectCountAMDArgs*
        args = decoder.DecodeCmdDrawIndexedIndirectCountAMD(data, data_size);
        os << "offset:" << args->offset << "countOffset:" << args->countOffset
           << ",maxDrawCount:" << args->maxDrawCount << ",stride" << args->stride;
        break;
    }
    case VKCmdID::vkCmdBeginDebugUtilsLabelEXTCmdID: break;
    case VKCmdID::vkCmdEndDebugUtilsLabelEXTCmdID: break;
    case VKCmdID::vkCmdInsertDebugUtilsLabelEXTCmdID: break;
    case VKCmdID::vkCmdSetDeviceMaskKHRCmdID:
    {
        CmdSetDeviceMaskKHRArgs* args = decoder.DecodeCmdSetDeviceMaskKHR(data, data_size);
        os << "deviceMask:0x" << std::hex << args->deviceMask << std::dec;
        break;
    }
    case VKCmdID::vkCmdSetDeviceMaskCmdID:
    {
        CmdSetDeviceMaskArgs* args = decoder.DecodeCmdSetDeviceMask(data, data_size);
        os << "deviceMask:0x" << std::hex << args->deviceMask << std::dec;
        break;
    }
    case VKCmdID::vkCmdDispatchBaseKHRCmdID:
    {
        CmdDispatchBaseKHRArgs* args = decoder.DecodeCmdDispatchBaseKHR(data, data_size);
        os << "baseGroup(" << args->baseGroupX << "," << args->baseGroupY << "," << args->baseGroupZ
           << ")"
           << ",groupCount(" << args->groupCountX << "," << args->groupCountY << ","
           << args->groupCountZ << ")";
        break;
    }
    case VKCmdID::vkCmdDispatchBaseCmdID:
    {
        CmdDispatchBaseArgs* args = decoder.DecodeCmdDispatchBase(data, data_size);
        os << "baseGroup(" << args->baseGroupX << "," << args->baseGroupY << "," << args->baseGroupZ
           << ")"
           << ",groupCount(" << args->groupCountX << "," << args->groupCountY << ","
           << args->groupCountZ << ")";
        break;
    }
    case VKCmdID::vkCmdDrawIndirectCountKHRCmdID:
    {
        CmdDrawIndirectCountKHRArgs* args = decoder.DecodeCmdDrawIndirectCountKHR(data, data_size);
        os << "offset:" << args->offset << ",countBuffer:" << args->countBuffer
           << ",countOffset:" << args->countOffset << ",maxDrawCount:" << args->maxDrawCount
           << ",stride:" << args->stride;
        break;
    }
    case VKCmdID::vkCmdDrawIndexedIndirectCountKHRCmdID:
    {
        CmdDrawIndexedIndirectCountKHRArgs*
        args = decoder.DecodeCmdDrawIndexedIndirectCountKHR(data, data_size);
        os << "offset:" << args->offset << ",countOffset:" << args->countOffset
           << ",maxDrawCount:" << args->maxDrawCount << ",stride:" << args->stride;
        break;
    }
    case VKCmdID::vkCmdBeginConditionalRenderingEXTCmdID:
    {
        auto args = decoder.DecodeCmdBeginConditionalRenderingEXT(data, data_size);
        if (args->pConditinalRenderingBegin)
        {
            os << "offset:" << args->pConditinalRenderingBegin->offset
               << ",flags:" << args->pConditinalRenderingBegin->flags;
        }
        break;
    }
    case VKCmdID::vkCmdEndConditionalRenderingEXTCmdID: break;
    }
    os << ")";
    assert(decoder.DecodedArgsSize() <= data_size);
}

}  // namespace Dive
