#pragma once

/**
 * vulkan core includes
 */
#include <vulkan/vulkan.h>

/**
 * utils
 */
#include <unordered_map>
#include <string>

/**
 * foward declaration
 */
namespace VulkanContext {
    class Context;
}

namespace VulkanDispatcher::Device {

    template<typename K, typename V>
    using DispatchMap_T = std::unordered_map<K, V>;

    class DeviceDispatchTable {
    public:
        explicit DeviceDispatchTable(VulkanContext::Context* context);

        bool factory();

        /**
         * device functions
         */
        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_CreateDevice(
                VkPhysicalDevice                            physicalDevice,
                const VkDeviceCreateInfo*                   pCreateInfo,
                const VkAllocationCallbacks*                pAllocator,
                VkDevice*                                   pDevice);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_DestroyDevice(
                VkDevice                                    device,
                const VkAllocationCallbacks*                pAllocator);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_GetDeviceQueue(
                VkDevice                                    device,
                uint32_t                                    queueFamilyIndex,
                uint32_t                                    queueIndex,
                VkQueue*                                    pQueue);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_QueueSubmit(
                VkQueue                                     queue,
                uint32_t                                    submitCount,
                const VkSubmitInfo*                         pSubmits,
                VkFence                                     fence);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_QueueWaitIdle(
                VkQueue                                     queue);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_DeviceWaitIdle(
                VkDevice                                    device);


        static VKAPI_ATTR VkResult VKAPI_CALL
        vlk_trampoline_call_AllocateMemory(VkDevice device, const VkMemoryAllocateInfo* pAllocateInfo,
                                           const VkAllocationCallbacks* pAllocator, VkDeviceMemory* pMemory);

        static VKAPI_ATTR VkResult VKAPI_CALL
        vlk_trampoline_call_MapMemory(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset,
                                      VkDeviceSize size, VkMemoryMapFlags flags, void** ppData);

        static VKAPI_ATTR void VKAPI_CALL
        vlk_trampoline_call_UnmapMemory(VkDevice device, VkDeviceMemory memory);

        static VKAPI_ATTR void VKAPI_CALL
        vlk_trampoline_call_FreeMemory(VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks* pAllocator);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_FlushMappedMemoryRanges(
                VkDevice                                    device,
                uint32_t                                    memoryRangeCount,
                const VkMappedMemoryRange*                  pMemoryRanges);

        static VKAPI_ATTR VkResult VKAPI_CALL
        vlk_trampoline_call_InvalidateMappedMemoryRanges(VkDevice device, uint32_t memoryRangeCount,
                                                         const VkMappedMemoryRange *pMemoryRanges);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_GetDeviceMemoryCommitment(
                VkDevice                                    device,
                VkDeviceMemory                              memory,
                VkDeviceSize*                               pCommittedMemoryInBytes);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_CreateBuffer(
                VkDevice                                    device,
                const VkBufferCreateInfo*                   pCreateInfo,
                const VkAllocationCallbacks*                pAllocator,
                VkBuffer*                                   pBuffer);


        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_DestroyBuffer(
                VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* pAllocator);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_CreateBufferView(
                VkDevice                                    device,
                const VkBufferViewCreateInfo*               pCreateInfo,
                const VkAllocationCallbacks*                pAllocator,
                VkBufferView*                               pView);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_DestroyBufferView(
                VkDevice                                    device,
                VkBufferView                                bufferView,
                const VkAllocationCallbacks*                pAllocator);


        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_BindBufferMemory(
                VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_CreateImage(
                VkDevice                                    device,
                const VkImageCreateInfo*                    pCreateInfo,
                const VkAllocationCallbacks*                pAllocator,
                VkImage*                                    pImage);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_DestroyImage(
                VkDevice                                    device,
                VkImage                                     image,
                const VkAllocationCallbacks*                pAllocator);


        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_BindImageMemory(
                VkDevice device,
                VkImage image,
                VkDeviceMemory memory,
                VkDeviceSize memoryOffset);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_GetBufferMemoryRequirements(
                VkDevice                                    device,
                VkBuffer                                    buffer,
                VkMemoryRequirements*                       pMemoryRequirements);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_GetImageMemoryRequirements(
                VkDevice device,
                VkImage image,
                VkMemoryRequirements* pMemoryRequirements);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_GetImageSparseMemoryRequirements(
                VkDevice device,
                VkImage image,
                uint32_t* pSparseMemoryRequirementCount,
                VkSparseImageMemoryRequirements* pSparseMemoryRequirements);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_QueueBindSparse(
                VkQueue                                     queue,
                uint32_t                                    bindInfoCount,
                const VkBindSparseInfo*                     pBindInfo,
                VkFence                                     fence);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_CreateFence(
                VkDevice                                    device,
                const VkFenceCreateInfo*                    pCreateInfo,
                const VkAllocationCallbacks*                pAllocator,
                VkFence*                                    pFence);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_DestroyFence(
                VkDevice                                    device,
                VkFence                                     fence,
                const VkAllocationCallbacks*                pAllocator);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_ResetFences(
                VkDevice                                    device,
                uint32_t                                    fenceCount,
                const VkFence*                              pFences);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_GetFenceStatus(
                VkDevice                                    device,
                VkFence                                     fence);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_WaitForFences(
                VkDevice                                    device,
                uint32_t                                    fenceCount,
                const VkFence*                              pFences,
                VkBool32                                    waitAll,
                uint64_t                                    timeout);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_CreateSemaphore(
                VkDevice                                    device,
                const VkSemaphoreCreateInfo*                pCreateInfo,
                const VkAllocationCallbacks*                pAllocator,
                VkSemaphore*                                pSemaphore);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_DestroySemaphore(
                VkDevice                                    device,
                VkSemaphore                                 semaphore,
                const VkAllocationCallbacks*                pAllocator);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_CreateEvent(
                VkDevice                                    device,
                const VkEventCreateInfo*                    pCreateInfo,
                const VkAllocationCallbacks*                pAllocator,
                VkEvent*                                    pEvent);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_DestroyEvent(
                VkDevice                                    device,
                VkEvent                                     event,
                const VkAllocationCallbacks*                pAllocator);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_GetEventStatus(
                VkDevice                                    device,
                VkEvent                                     event);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_SetEvent(
                VkDevice                                    device,
                VkEvent                                     event);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_ResetEvent(
                VkDevice                                    device,
                VkEvent                                     event);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_CreateQueryPool(
                VkDevice                                    device,
                const VkQueryPoolCreateInfo*                pCreateInfo,
                const VkAllocationCallbacks*                pAllocator,
                VkQueryPool*                                pQueryPool);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_DestroyQueryPool(
                VkDevice                                    device,
                VkQueryPool                                 queryPool,
                const VkAllocationCallbacks*                pAllocator);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_GetQueryPoolResults(
                VkDevice                                    device,
                VkQueryPool                                 queryPool,
                uint32_t                                    firstQuery,
                uint32_t                                    queryCount,
                size_t                                      dataSize,
                void*                                       pData,
                VkDeviceSize                                stride,
                VkQueryResultFlags                          flags);


        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_GetImageSubresourceLayout(
                VkDevice device,
                VkImage image,
                const VkImageSubresource* pSubresource,
                VkSubresourceLayout* pLayout);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_CreateImageView(
                VkDevice                                    device,
                const VkImageViewCreateInfo*                pCreateInfo,
                const VkAllocationCallbacks*                pAllocator,
                VkImageView*                                pView);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_DestroyImageView(
                VkDevice                                    device,
                VkImageView                                 imageView,
                const VkAllocationCallbacks*                pAllocator);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_CreateShaderModule(
                VkDevice                                    device,
                const VkShaderModuleCreateInfo*             pCreateInfo,
                const VkAllocationCallbacks*                pAllocator,
                VkShaderModule*                             pShaderModule);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_DestroyShaderModule(
                VkDevice                                    device,
                VkShaderModule                              shaderModule,
                const VkAllocationCallbacks*                pAllocator);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_CreatePipelineCache(
                VkDevice                                    device,
                const VkPipelineCacheCreateInfo*            pCreateInfo,
                const VkAllocationCallbacks*                pAllocator,
                VkPipelineCache*                            pPipelineCache);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_DestroyPipelineCache(
                VkDevice                                    device,
                VkPipelineCache                             pipelineCache,
                const VkAllocationCallbacks*                pAllocator);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_GetPipelineCacheData(
                VkDevice                                    device,
                VkPipelineCache                             pipelineCache,
                size_t*                                     pDataSize,
                void*                                       pData);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_MergePipelineCaches(
                VkDevice                                    device,
        VkPipelineCache                             dstCache,
                uint32_t                                    srcCacheCount,
        const VkPipelineCache*                      pSrcCaches);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_CreateGraphicsPipelines(
                VkDevice                                    device,
                VkPipelineCache                             pipelineCache,
                uint32_t                                    createInfoCount,
                const VkGraphicsPipelineCreateInfo*         pCreateInfos,
                const VkAllocationCallbacks*                pAllocator,
                VkPipeline*                                 pPipelines);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_CreateComputePipelines(
                VkDevice                                    device,
                VkPipelineCache                             pipelineCache,
                uint32_t                                    createInfoCount,
                const VkComputePipelineCreateInfo*          pCreateInfos,
                const VkAllocationCallbacks*                pAllocator,
                VkPipeline*                                 pPipelines);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_DestroyPipeline(
                VkDevice                                    device,
                VkPipeline                                  pipeline,
                const VkAllocationCallbacks*                pAllocator);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_CreatePipelineLayout(
                VkDevice                                    device,
                const VkPipelineLayoutCreateInfo*           pCreateInfo,
                const VkAllocationCallbacks*                pAllocator,
                VkPipelineLayout*                           pPipelineLayout);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_DestroyPipelineLayout(
                VkDevice                                    device,
                VkPipelineLayout                            pipelineLayout,
                const VkAllocationCallbacks*                pAllocator);


        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_CreateSampler(
                VkDevice device,
                const VkSamplerCreateInfo* pCreateInfo,
                const VkAllocationCallbacks* pAllocator,
                VkSampler* pSampler);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_DestroySampler(
                VkDevice device,
                VkSampler sampler,
                const VkAllocationCallbacks* pAllocator);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_CreateDescriptorSetLayout(
                VkDevice device,
                const VkDescriptorSetLayoutCreateInfo* pCreateInfo,
                const VkAllocationCallbacks* pAllocator,
                VkDescriptorSetLayout* pSetLayout);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_DestroyDescriptorSetLayout(
                VkDevice device,
                VkDescriptorSetLayout descriptorSetLayout,
                const VkAllocationCallbacks* pAllocator);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_CreateDescriptorPool(
                VkDevice device,
                const VkDescriptorPoolCreateInfo* pCreateInfo,
                const VkAllocationCallbacks* pAllocator,
                VkDescriptorPool* pDescriptorPool);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_DestroyDescriptorPool(
                VkDevice device,
                VkDescriptorPool descriptorPool,
                const VkAllocationCallbacks* pAllocator);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_ResetDescriptorPool(
                VkDevice device,
                VkDescriptorPool descriptorPool,
                VkDescriptorPoolResetFlags flags);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_AllocateDescriptorSets(
                VkDevice device,
                const VkDescriptorSetAllocateInfo* pAllocateInfo,
                VkDescriptorSet* pDescriptorSets);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_FreeDescriptorSets(
                VkDevice device,
                VkDescriptorPool descriptorPool,
                uint32_t descriptorSetCount,
                const VkDescriptorSet* pDescriptorSets);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_UpdateDescriptorSets(
                VkDevice device,
                uint32_t descriptorWriteCount,
                const VkWriteDescriptorSet* pDescriptorWrites,
                uint32_t descriptorCopyCount,
                const VkCopyDescriptorSet* pDescriptorCopies);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_CreateFramebuffer(
                VkDevice device,
                const VkFramebufferCreateInfo* pCreateInfo,
                const VkAllocationCallbacks* pAllocator,
                VkFramebuffer* pFramebuffer);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_DestroyFramebuffer(
                VkDevice device,
                VkFramebuffer framebuffer,
                const VkAllocationCallbacks* pAllocator);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_CreateRenderPass(
                VkDevice device,
                const VkRenderPassCreateInfo* pCreateInfo,
                const VkAllocationCallbacks* pAllocator,
                VkRenderPass* pRenderPass);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_DestroyRenderPass(
                VkDevice device,
                VkRenderPass renderPass,
                const VkAllocationCallbacks* pAllocator);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_GetRenderAreaGranularity(
                VkDevice device,
                VkRenderPass renderPass,
                VkExtent2D* pGranularity);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_CreateCommandPool(
                VkDevice device,
                const VkCommandPoolCreateInfo* pCreateInfo,
                const VkAllocationCallbacks* pAllocator,
                VkCommandPool* pCommandPool);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_DestroyCommandPool(
                VkDevice device,
                VkCommandPool commandPool,
                const VkAllocationCallbacks* pAllocator);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_ResetCommandPool(
                VkDevice device,
                VkCommandPool commandPool,
                VkCommandPoolResetFlags flags);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_AllocateCommandBuffers(
                VkDevice                                    device,
                const VkCommandBufferAllocateInfo*          pAllocateInfo,
                VkCommandBuffer*                            pCommandBuffers);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_FreeCommandBuffers(
                VkDevice                                    device,
                VkCommandPool                               commandPool,
                uint32_t                                    commandBufferCount,
                const VkCommandBuffer*                      pCommandBuffers);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_BeginCommandBuffer(
                VkCommandBuffer                             commandBuffer,
                const VkCommandBufferBeginInfo*             pBeginInfo);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_EndCommandBuffer(
                VkCommandBuffer                             commandBuffer);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_ResetCommandBuffer(
                VkCommandBuffer                             commandBuffer,
                VkCommandBufferResetFlags                   flags);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdBindPipeline(
                VkCommandBuffer commandBuffer,
                VkPipelineBindPoint pipelineBindPoint,
                VkPipeline pipeline);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdSetViewport(
                VkCommandBuffer commandBuffer,
                uint32_t firstViewport,
                uint32_t viewportCount,
                const VkViewport* pViewports);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdSetScissor(
                VkCommandBuffer commandBuffer,
                uint32_t firstScissor,
                uint32_t scissorCount,
                const VkRect2D* pScissors);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdSetLineWidth(
                VkCommandBuffer commandBuffer,
                float lineWidth);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdSetDepthBias(
                VkCommandBuffer commandBuffer,
                float depthBiasConstantFactor,
                float depthBiasClamp,
                float depthBiasSlopeFactor);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdSetBlendConstants(
                VkCommandBuffer commandBuffer,
                const float blendConstants[4]);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdSetDepthBounds(
                VkCommandBuffer commandBuffer,
                float minDepthBounds,
                float maxDepthBounds);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdSetStencilCompareMask(
                VkCommandBuffer commandBuffer,
                VkStencilFaceFlags faceMask,
                uint32_t compareMask);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdSetStencilWriteMask(
                VkCommandBuffer commandBuffer,
                VkStencilFaceFlags faceMask,
                uint32_t writeMask);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdSetStencilReference(
                VkCommandBuffer commandBuffer,
                VkStencilFaceFlags faceMask,
                uint32_t reference);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdBindDescriptorSets(
                VkCommandBuffer commandBuffer,
                VkPipelineBindPoint pipelineBindPoint,
                VkPipelineLayout layout,
                uint32_t firstSet,
                uint32_t descriptorSetCount,
                const VkDescriptorSet* pDescriptorSets,
                uint32_t dynamicOffsetCount,
                const uint32_t* pDynamicOffsets);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdBindIndexBuffer(
                VkCommandBuffer commandBuffer,
                VkBuffer buffer,
                VkDeviceSize offset,
                VkIndexType indexType);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdBindVertexBuffers(
                VkCommandBuffer commandBuffer,
                uint32_t firstBinding,
                uint32_t bindingCount,
                const VkBuffer* pBuffers,
                const VkDeviceSize* pOffsets);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdDraw(
                VkCommandBuffer commandBuffer,
                uint32_t vertexCount,
                uint32_t instanceCount,
                uint32_t firstVertex,
                uint32_t firstInstance);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdDrawIndexed(
                VkCommandBuffer commandBuffer,
                uint32_t indexCount,
                uint32_t instanceCount,
                uint32_t firstIndex,
                int32_t vertexOffset,
                uint32_t firstInstance);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdDrawIndirect(
                VkCommandBuffer commandBuffer,
                VkBuffer buffer,
                VkDeviceSize offset,
                uint32_t drawCount,
                uint32_t stride);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdDrawIndexedIndirect(
                VkCommandBuffer commandBuffer,
                VkBuffer buffer,
                VkDeviceSize offset,
                uint32_t drawCount,
                uint32_t stride);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdDispatch(
                VkCommandBuffer commandBuffer,
                uint32_t groupCountX,
                uint32_t groupCountY,
                uint32_t groupCountZ);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdDispatchIndirect(
                VkCommandBuffer commandBuffer,
                VkBuffer buffer,
                VkDeviceSize offset);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdCopyBuffer(
                VkCommandBuffer commandBuffer,
                VkBuffer srcBuffer,
                VkBuffer dstBuffer,
                uint32_t regionCount,
                const VkBufferCopy* pRegions);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdCopyImage(
                VkCommandBuffer commandBuffer,
                VkImage srcImage,
                VkImageLayout srcImageLayout,
                VkImage dstImage,
                VkImageLayout dstImageLayout,
                uint32_t regionCount,
                const VkImageCopy* pRegions);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdBlitImage(
                VkCommandBuffer commandBuffer,
                VkImage srcImage,
                VkImageLayout srcImageLayout,
                VkImage dstImage,
                VkImageLayout dstImageLayout,
                uint32_t regionCount,
                const VkImageBlit* pRegions,
                VkFilter filter);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdCopyBufferToImage(
                VkCommandBuffer commandBuffer,
                VkBuffer srcBuffer,
                VkImage dstImage,
                VkImageLayout dstImageLayout,
                uint32_t regionCount,
                const VkBufferImageCopy* pRegions);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdCopyImageToBuffer(
                VkCommandBuffer commandBuffer,
                VkImage srcImage,
                VkImageLayout srcImageLayout,
                VkBuffer dstBuffer,
                uint32_t regionCount,
                const VkBufferImageCopy* pRegions);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdUpdateBuffer(
                VkCommandBuffer commandBuffer,
                VkBuffer dstBuffer,
                VkDeviceSize dstOffset,
                VkDeviceSize dataSize,
                const void* pData);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdFillBuffer(
                VkCommandBuffer commandBuffer,
                VkBuffer dstBuffer,
                VkDeviceSize dstOffset,
                VkDeviceSize size,
                uint32_t data);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdClearColorImage(
                VkCommandBuffer commandBuffer,
                VkImage image,
                VkImageLayout imageLayout,
                const VkClearColorValue* pColor,
                uint32_t rangeCount,
                const VkImageSubresourceRange* pRanges);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdClearDepthStencilImage(
                VkCommandBuffer commandBuffer,
                VkImage image,
                VkImageLayout imageLayout,
                const VkClearDepthStencilValue* pDepthStencil,
                uint32_t rangeCount,
                const VkImageSubresourceRange* pRanges);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdClearAttachments(
                VkCommandBuffer commandBuffer,
                uint32_t attachmentCount,
                const VkClearAttachment* pAttachments,
                uint32_t rectCount,
                const VkClearRect* pRects);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdResolveImage(
                VkCommandBuffer commandBuffer,
                VkImage srcImage,
                VkImageLayout srcImageLayout,
                VkImage dstImage,
                VkImageLayout dstImageLayout,
                uint32_t regionCount,
                const VkImageResolve* pRegions);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdSetEvent(
                VkCommandBuffer commandBuffer,
                VkEvent event,
                VkPipelineStageFlags stageMask);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdResetEvent(
                VkCommandBuffer commandBuffer,
                VkEvent event,
                VkPipelineStageFlags stageMask);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdWaitEvents(
                VkCommandBuffer commandBuffer,
                uint32_t eventCount,
                const VkEvent* pEvents,
                VkPipelineStageFlags srcStageMask,
                VkPipelineStageFlags dstStageMask,
                uint32_t memoryBarrierCount,
                const VkMemoryBarrier* pMemoryBarriers,
                uint32_t bufferMemoryBarrierCount,
                const VkBufferMemoryBarrier* pBufferMemoryBarriers,
                uint32_t imageMemoryBarrierCount,
                const VkImageMemoryBarrier* pImageMemoryBarriers);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdPipelineBarrier(
                VkCommandBuffer commandBuffer,
                VkPipelineStageFlags srcStageMask,
                VkPipelineStageFlags dstStageMask,
                VkDependencyFlags dependencyFlags,
                uint32_t memoryBarrierCount,
                const VkMemoryBarrier* pMemoryBarriers,
                uint32_t bufferMemoryBarrierCount,
                const VkBufferMemoryBarrier* pBufferMemoryBarriers,
                uint32_t imageMemoryBarrierCount,
                const VkImageMemoryBarrier* pImageMemoryBarriers);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdBeginQuery(
                VkCommandBuffer commandBuffer,
                VkQueryPool queryPool,
                uint32_t query,
                VkQueryControlFlags flags);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdEndQuery(
                VkCommandBuffer commandBuffer,
                VkQueryPool queryPool,
                uint32_t query);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdResetQueryPool(
                VkCommandBuffer commandBuffer,
                VkQueryPool queryPool,
                uint32_t firstQuery,
                uint32_t queryCount);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdWriteTimestamp(
                VkCommandBuffer commandBuffer,
                VkPipelineStageFlagBits pipelineStage,
                VkQueryPool queryPool,
                uint32_t query);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdCopyQueryPoolResults(
                VkCommandBuffer commandBuffer,
                VkQueryPool queryPool,
                uint32_t firstQuery,
                uint32_t queryCount,
                VkBuffer dstBuffer,
                VkDeviceSize dstOffset,
                VkDeviceSize stride,
                VkQueryResultFlags flags);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdPushConstants(
                VkCommandBuffer commandBuffer,
                VkPipelineLayout layout,
                VkShaderStageFlags stageFlags,
                uint32_t offset,
                uint32_t size,
                const void* pValues);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdBeginRenderPass(
                VkCommandBuffer commandBuffer,
                const VkRenderPassBeginInfo* pRenderPassBegin,
                VkSubpassContents contents);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdNextSubpass(
                VkCommandBuffer commandBuffer,
                VkSubpassContents contents);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdEndRenderPass(
                VkCommandBuffer commandBuffer);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_vkCmdExecuteCommands(
                VkCommandBuffer commandBuffer,
                uint32_t commandBufferCount,
                const VkCommandBuffer* pCommandBuffers);


        static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vlk_trampoline_call_GetDeviceProcAddr(
                VkDevice                                    device,
                const char*                                 pName);

        /**
         * hash map of device functions
         */
        static DispatchMap_T<std::string, PFN_vkVoidFunction> device_dispatch_table;

    private:
        /**
         * static pointer to vulkan context initializated
         */
        static VulkanContext::Context* vk_context;

        /**
         *
         * @tparam T type of function that will be cast
         * @param func the reference of function
         * @return generic pointer for function
         */
        template<typename T>
        inline PFN_vkVoidFunction to(T func) {
            return reinterpret_cast<PFN_vkVoidFunction>(func);
        }

        /**
         * @tparam T type of pointer receiver function
         * @param pName name of function that will be registered
         * @param func the lvalue for a function
         * @return true for sucessfully registration otherwise false
         */
        template<typename T>
        bool registerTrampoline(const std::string& pName, T func) {
            auto state = false;

            device_dispatch_table[pName] = to(func);

            const auto &iterator = device_dispatch_table.find(pName);

            if (iterator != device_dispatch_table.end()) {
                state = true;
            }

            return state;
        }
    };
}
