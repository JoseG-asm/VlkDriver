#include "device.h"
#include "context.h"

VulkanContext::Context *VulkanDispatcher::Device::DeviceDispatchTable::vk_context = nullptr;
VulkanDispatcher::Device::DispatchMap_T<std::string, PFN_vkVoidFunction> VulkanDispatcher::Device::DeviceDispatchTable::device_dispatch_table;

VulkanDispatcher::Device::DeviceDispatchTable::DeviceDispatchTable(
        VulkanContext::Context *context) {
    if (context) {
        vk_context = context;
    } else {
        Log::VLK_LOG(Log::Level::ERROR, Log::LevelType::INSTANCE_DISPATCH_TABLE,
                     "received a invalid context vulkan");
    }
}


/**
 * device functions
 */
VkResult VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_CreateDevice(
        VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo,
        const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_CreateDevice called >>");

    /**
     * instance
     */
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    auto key = reinterpret_cast<uintptr_t>(physicalDevice);
    auto it_pdev = vk_context->physicalDevices.find(key);
    if (it_pdev == vk_context->physicalDevices.end()) {
        throw std::runtime_error("PhysicalDevice not found");
    }

    /**
     * device object
     */
    VkDevice obj;

    if (it_instance != vk_context->instances.end()) {
        const auto func = (PFN_vkCreateDevice) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkCreateDevice");

        VkResult result = func(it_pdev->second->dispatch_handle, pCreateInfo,
                               pAllocator, &obj);

        if (result != VK_SUCCESS) {
            Log::VLK_LOG(Log::Level::ERROR, Log::LevelType::CONTEXT,
                         "Failed to create VkDevice with code %d", result);
            return result;
        }

        /**
         * create a device driver representation of VkDevice
         */
        auto device = std::make_unique<VulkanContext::VkDeviceObject>();
        device->dispatch_handle = obj;
        *pDevice = device->dispatch_handle;

        VulkanContext::VkDeviceObject::device_magic = reinterpret_cast<uint64_t>(device->dispatch_handle);

        Log::VLK_LOG(Log::Level::INFO, Log::LevelType::DEVICE, "magic value = ",
                     VulkanContext::VkDeviceObject::device_magic);

        vk_context->devices[VulkanContext::VkDeviceObject::device_magic] = std::move(device);



        /**
         * Queue handler
         */
        for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; ++i) {
            const VkDeviceQueueCreateInfo &qci = pCreateInfo->pQueueCreateInfos[i];
            for (uint32_t j = 0; j < qci.queueCount; ++j) {
                VulkanContext::VkDeviceObject::QueueKey key{qci.queueFamilyIndex, j};

                VulkanContext::VkDeviceObject::VkQueueObject q;
                q.deviceObject = obj;

                /**
                 * dipatch handle queue->
                 */
                const auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                        it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
                const auto vgdq = (PFN_vkGetDeviceQueue) func(obj, "vkGetDeviceQueue");

                vgdq(obj, qci.queueFamilyIndex, j, &q.dispatch_handle);

                vk_context->devices[VulkanContext::VkDeviceObject::device_magic]->queues[key] = q;
                vk_context->devices[VulkanContext::VkDeviceObject::device_magic]->queue_by_handles[q.dispatch_handle] = q;
            }
        }

        return VK_SUCCESS;
    }

    return VK_ERROR_INITIALIZATION_FAILED;
}


void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_DestroyDevice(
        VkDevice device, const VkAllocationCallbacks *pAllocator) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_DestroyDevice called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));

    /**
    * destroy queues
    */
    it_device->second->queues.clear();

    /**
     * destroy memories
     */
    for (auto &mem: it_device->second->memory_map) {
        free(mem.second->mapped_ptr);
    }
    it_device->second->memory_map.clear();

    /**
     * destroy buffers and images
     */
    it_device->second->buffers.clear();
    it_device->second->chain_images.clear();

    /**
     * destroy images and fences
     */
    it_device->second->chain_images.clear();
    it_device->second->fences.clear();

    /**
     * destroy semaphores and events
     */
    it_device->second->semaphores.clear();
    it_device->second->events.clear();

    /*
     * destroy queries and buffer views
     */
    it_device->second->queries.clear();
    it_device->second->buffer_views.clear();

    vk_context->devices.erase(it_device);
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_GetDeviceQueue(
        VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue *pQueue) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_GetDeviceQueue called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_queue = it_device->second->queues.find({queueFamilyIndex, queueIndex});

    if (it_queue != it_device->second->queues.end()) {
        *pQueue = it_queue->second.dispatch_handle;
    }
}

VkResult
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_QueueSubmit(VkQueue queue,
                                                                               uint32_t submitCount,
                                                                               const VkSubmitInfo *pSubmits,
                                                                               VkFence fence) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_QueueSubmit called >>");

    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);


    auto it_queue = it_device->second->queue_by_handles.find(queue);

    if(it_queue == it_device->second->queue_by_handles.end()) {
        Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "QueueSubmit: device queue from submission not found");
    }

    // get real submits in driver ->

    std::vector<VulkanContext::VkDeviceObject::VkDeviceSubmitInfoObject> real_submits;
    real_submits.reserve(submitCount);

    for (uint32_t i = 0; i < submitCount; i++) {
        const auto& submitInfo = pSubmits[i];
        VulkanContext::VkDeviceObject::VkDeviceSubmitInfoObject rs{};

        rs.cmd_buffers.resize(submitInfo.commandBufferCount);
        for (uint32_t j = 0; j < rs.cmd_buffers.size(); j++) {
            // for now driver dont manage the cmd buffers only for now obv
        }

        rs.wait_semaphores.resize(submitInfo.waitSemaphoreCount);
        for (uint32_t j = 0; j < rs.wait_semaphores.size(); j++) {
            auto it_semaphore = it_device->second->semaphores.find(submitInfo.pWaitSemaphores[j]);

            if (it_semaphore == it_device->second->semaphores.end()) {
                Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "QueueSubmit: wait semaphore handle from app submit info not found");
            }

            rs.wait_semaphores[j] = it_semaphore->second->dispatch_handle;
        }

        rs.signal_semaphores.resize(submitInfo.signalSemaphoreCount);
        for (uint32_t j = 0; j < rs.signal_semaphores.size(); j++) {
            auto it_semaphore = it_device->second->semaphores.find(submitInfo.pSignalSemaphores[j]);

            if (it_semaphore == it_device->second->semaphores.end()) {
                Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "QueueSubmit: wait semaphore handle from app submit info not found");
            }

            rs.signal_semaphores[j] = it_semaphore->second->dispatch_handle;
        }

        // --- copy wait dst stage masks (safe local copy) ---
        rs.wait_dst_stage_mask.resize(submitInfo.waitSemaphoreCount);
        if (submitInfo.pWaitDstStageMask && submitInfo.waitSemaphoreCount > 0) {
            for (uint32_t j = 0; j < submitInfo.waitSemaphoreCount; ++j) {
                rs.wait_dst_stage_mask[j] = submitInfo.pWaitDstStageMask[j];
            }
        }

        rs.submit_info = VkSubmitInfo{
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .pNext = submitInfo.pNext,
                // real wait semaphores from app submit info
                .waitSemaphoreCount = (uint32_t)rs.wait_semaphores.size(),
                .pWaitSemaphores = rs.wait_semaphores.empty() ? nullptr : rs.wait_semaphores.data(),
                // the same stages as well
                .pWaitDstStageMask = rs.wait_dst_stage_mask.empty() ? nullptr : rs.wait_dst_stage_mask.data(),
                // real cmd buffers from app submit info
                .commandBufferCount = (uint32_t)rs.cmd_buffers.size(),
                .pCommandBuffers = rs.cmd_buffers.empty() ? nullptr : rs.cmd_buffers.data(),
                // real signal semaphores from app submit info
                .signalSemaphoreCount = (uint32_t)rs.signal_semaphores.size(),
                .pSignalSemaphores = rs.signal_semaphores.empty() ? nullptr : rs.signal_semaphores.data(),
        };


        // push back the submit ->
        real_submits.push_back(std::move(rs));
    }

    VkFence real_fence = VK_NULL_HANDLE;
    if (fence != VK_NULL_HANDLE) {
        auto it_fence = it_device->second->fences.find(fence);
        if (it_fence != it_device->second->fences.end()) {
            real_fence = it_fence->second->dispatch_handle;
        } else {
            Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "QueueSubmit: fence not found");
        }
    }

    // real submit ptrs
    std::vector<VkSubmitInfo> submit_ptrs;
    submit_ptrs.reserve(real_submits.size());
    for (auto& rs : real_submits) {
        submit_ptrs.push_back(rs.submit_info);
    }


    /**
      * dipatch handle queue->
      */
    const auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
            it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
    const auto vqs = (PFN_vkQueueSubmit) func(it_device->second->dispatch_handle, "vkQueueSubmit");



    // pass to real vk call with valid fences/semaphores/cmd_buffers for submission
    return vqs(it_queue->second.dispatch_handle, submitCount, submit_ptrs.data(), real_fence);

}


VkResult VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_QueueWaitIdle(
        VkQueue queue) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_QueueWaitIdle called >>");

    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);

    auto &dev = it_device->second->dispatch_handle;
    auto it_queue = it_device->second->queue_by_handles.find(queue);

    if (it_queue != it_device->second->queue_by_handles.end() && it_instance != vk_context->instances.end() && it_device != vk_context->devices.end()) {
        const auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        const auto vqwi = (PFN_vkQueueWaitIdle) func(dev, "vkQueueWaitIdle");

        VkResult result = vqwi(it_queue->second.dispatch_handle);

        return result;
    }

    return VK_ERROR_DEVICE_LOST;
}

VkResult VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_DeviceWaitIdle(
        VkDevice device) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_DeviceWaitIdle called >>");

    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));

    if (it_device == vk_context->devices.end() && it_instance == vk_context->instances.end()) {
        return VK_ERROR_DEVICE_LOST;
    }

    auto &dev = it_device->second;

    const auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
    const auto vdwi = (PFN_vkDeviceWaitIdle) func(dev->dispatch_handle, "vkQueueWaitIdle");

    if (!func) return VK_ERROR_INITIALIZATION_FAILED;

    return vdwi(dev->dispatch_handle);
}

VKAPI_ATTR VkResult VKAPI_CALL
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_AllocateMemory(
        VkDevice device, const VkMemoryAllocateInfo *pAllocateInfo,
        const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMemory) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_AllocateMemory called >>");

    /**
     * get device from HANDLE
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));

    /**
     * chack if device handle is valid
     */
    if (it_device == vk_context->devices.end()) {
        return VK_ERROR_DEVICE_LOST;
    }

    /**
     * memory representation
     */
    auto mem = std::make_shared<VulkanContext::VkDeviceObject::VkDeviceMemoryObject>();

    /*
     * dont use ahb for now
     */
    mem->buffer = nullptr;

    /**
     * size in bytes must be multiple of alignment requirements in general is 256 bytes
     */
    mem->size = pAllocateInfo->allocationSize;

    /**
     * type of memory allocation scope
     */
    mem->memoryTypeIndex = pAllocateInfo->memoryTypeIndex;

    /**
     * size of allocation
     */
    auto *data = malloc(mem->size);

    if (!data) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    /*
     * pointer to mapped mem
     */
    mem->mapped_ptr = data;

    /**
     * mem unique handle to use after
     */
    auto id = it_device->second->memory_handler_id++;
    auto handle = reinterpret_cast<VkDeviceMemory>(id);

    /**
     * store in mapped mem map for more control
     */
    it_device->second->memory_map[handle] = mem;

    /**
     * pass the handle for VkDeviceMemory opaque structure
     */
    *pMemory = handle;

    /**
     * now return success
     */
    return VK_SUCCESS;
}

VkResult VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_MapMemory(
        VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size,
        VkMemoryMapFlags flags, void **ppData) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_MapMemory called >>");

    /**
     * get device from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));

    if (it_device == vk_context->devices.end()) return VK_ERROR_DEVICE_LOST;

    /**
     * get structure from handle pMemory
     */
    auto it_memory_handle = it_device->second->memory_map.find(memory);

    if (it_memory_handle == it_device->second->memory_map.end()) return VK_ERROR_MEMORY_MAP_FAILED;

    /**
     * verify if is already mapped
     */
    if (it_memory_handle->second->is_mapped) {
        return VK_ERROR_MEMORY_MAP_FAILED;
    }

    /**
     * offset cannot be grater than memory for example off set 512 -> mem size 256 always offset < size
     */
    if (offset >= it_memory_handle->second->size) return VK_ERROR_MEMORY_MAP_FAILED;

    /**
     * if can map all memory size including offset for example size = 512 for offset 256 whole size equals = 512 - 256 = 256 size;
     */
    if (size == VK_WHOLE_SIZE) size = it_memory_handle->second->size - offset;

    /*
     * off set + size cannot be grater than memory size always (offset + size) <= memory->size
     */
    if (offset + size > it_memory_handle->second->size) return VK_ERROR_MEMORY_MAP_FAILED;

    /**
     * define range of memory including off set and update state to mapped in this case the new pointer starts in base_ptr + offset
     */
    *ppData = static_cast<uint8_t *>(it_memory_handle->second->mapped_ptr) + offset;
    it_memory_handle->second->is_mapped = true;

    /**
     * now returns success
     */
    return VK_SUCCESS;
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_FreeMemory(
        VkDevice device, VkDeviceMemory memory, const VkAllocationCallbacks *pAllocator) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_FreeMemory called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));

    if (it_device != vk_context->devices.end()) {
        auto it_memory_handle = it_device->second->memory_map.find(memory);

        if (it_memory_handle != it_device->second->memory_map.end()) {
            free(it_memory_handle->second->mapped_ptr);
            it_device->second->memory_map.erase(it_memory_handle);
        }
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_UnmapMemory(
        VkDevice device, VkDeviceMemory memory) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_UnmapMemory called >>");


    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));

    if (it_device != vk_context->devices.end()) {
        auto it_memory_handle = it_device->second->memory_map.find(memory);

        if (it_memory_handle != it_device->second->memory_map.end()) {
            it_memory_handle->second->is_mapped = false;
        }
    }
}

VkResult
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_FlushMappedMemoryRanges(
        VkDevice device, uint32_t memoryRangeCount, const VkMappedMemoryRange *pMemoryRanges) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_FlushMappedMemoryRanges called >>");

    /**
     * get device from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    if (it_device == vk_context->devices.end()) {
        return VK_ERROR_DEVICE_LOST;
    }

    for (uint32_t i = 0; i < memoryRangeCount; i++) {
        const VkMappedMemoryRange &range = pMemoryRanges[i];

        auto it_mem = it_device->second->memory_map.find(range.memory);
        if (it_mem == it_device->second->memory_map.end()) {
            return VK_ERROR_MEMORY_MAP_FAILED;
        }

        auto &mem_obj = it_mem->second;

        if (!mem_obj->is_mapped) {
            return VK_ERROR_MEMORY_MAP_FAILED;
        }
        VkDeviceSize flushSize = range.size;
        if (flushSize == VK_WHOLE_SIZE) {
            flushSize = mem_obj->size - range.offset;
        }

        if (range.offset + flushSize > mem_obj->size) {
            return VK_ERROR_MEMORY_MAP_FAILED;
        }

        /**
         * actually flush is a noop so nothing needs for now
         */
    }

    return VK_SUCCESS;
}

VKAPI_ATTR VkResult VKAPI_CALL
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_InvalidateMappedMemoryRanges(
        VkDevice device,
        uint32_t memoryRangeCount,
        const VkMappedMemoryRange *pMemoryRanges) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_InvalidateMappedMemoryRanges called >>");

    /**
     * get device from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    if (it_device == vk_context->devices.end()) {
        return VK_ERROR_DEVICE_LOST;
    }

    /**
     * iterator ranges
     */
    for (uint32_t i = 0; i < memoryRangeCount; ++i) {
        const VkMappedMemoryRange &range = pMemoryRanges[i];

        auto it_mem = it_device->second->memory_map.find(range.memory);
        if (it_mem == it_device->second->memory_map.end()) {
            return VK_ERROR_MEMORY_MAP_FAILED;
        }

        auto &mem_obj = it_mem->second;

        if (!mem_obj->is_mapped) {
            return VK_ERROR_MEMORY_MAP_FAILED;
        }

        VkDeviceSize size = range.size;
        if (size == VK_WHOLE_SIZE) {
            size = mem_obj->size - range.offset;
        }

        if (range.offset + size > mem_obj->size) {
            return VK_ERROR_MEMORY_MAP_FAILED;
        }

        /**
         * noop again
         */
    }

    return VK_SUCCESS;
}

void
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_GetDeviceMemoryCommitment(
        VkDevice device, VkDeviceMemory memory, VkDeviceSize *pCommittedMemoryInBytes) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_GetDeviceMemoryCommitment called >>");

    /**
     * get the usage mem in a single VkDeviceMemory ->>
     */

    /**
     * get device from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));

    if (it_device != vk_context->devices.end()) *pCommittedMemoryInBytes = 0;

    /**
     * valid
     */
    auto it_memory_handle = it_device->second->memory_map.find(memory);

    if (it_memory_handle != it_device->second->memory_map.end()) {
        /**
         * size of VkDeviceMemory
         */
        *pCommittedMemoryInBytes = it_memory_handle->second->size;
    }
}

VkResult
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_CreateBuffer(VkDevice device,
                                                                                const VkBufferCreateInfo *pCreateInfo,
                                                                                const VkAllocationCallbacks *pAllocator,
                                                                                VkBuffer *pBuffer) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_CreateBuffer called >>");

    /**
     * get device object from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));

    if (it_device == vk_context->devices.end()) return VK_ERROR_DEVICE_LOST;

    /**
     * create buffer representation object
     */
    auto buffer = std::make_unique<VulkanContext::VkDeviceObject::VkBufferObject>();
    buffer->size = pCreateInfo->size;
    buffer->usage = pCreateInfo->usage;
    buffer->sharingMode = pCreateInfo->sharingMode;

    auto id = it_device->second->buffer_handler_id++;
    VkBuffer handle = reinterpret_cast<VkBuffer>(id);
    it_device->second->buffers[handle] = std::move(buffer);

    *pBuffer = handle;

    return VK_SUCCESS;
}

VKAPI_ATTR void VKAPI_CALL
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_DestroyBuffer(
        VkDevice device, VkBuffer buffer, const VkAllocationCallbacks *pAllocator) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_DestroyBuffer called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    if (it_device != vk_context->devices.end()) {
        it_device->second->buffers.erase(buffer);
    }
}


VkResult VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_CreateBufferView(VkDevice device,
                                                                         const VkBufferViewCreateInfo *pCreateInfo,
                                                                         const VkAllocationCallbacks *pAllocator,
                                                                         VkBufferView *pView) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_CreateBufferView called >>");

    /**
     * get device object from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));

    if (it_device == vk_context->devices.end()) return VK_ERROR_DEVICE_LOST;

    /**
     * find buffer to association memory
     */
    auto it_buffer = it_device->second->buffers.find(pCreateInfo->buffer);

    if(it_buffer == it_device->second->buffers.end()) return VK_ERROR_DEVICE_LOST;

    // verify if offset + RANGE dont pass of buffer->size;

    auto& bufferObject = it_buffer->second;

    if(pCreateInfo->range + pCreateInfo->offset > bufferObject->size) {
        return VK_ERROR_OUT_OF_DEVICE_MEMORY;
    }

    /**
     * create a driver representation of the buff view
     */
    auto buff_view = VulkanContext::VkDeviceObject::VkBufferViewObject();

    buff_view.offset = pCreateInfo->offset;
    buff_view.range_buffer = pCreateInfo->range;
    buff_view.format = pCreateInfo->format;

    buff_view.handle = pCreateInfo->buffer;

    auto id = it_device->second->buffer_views_handler_id++;

    auto handle = reinterpret_cast<VkBufferView>(id);

    it_device->second->buffer_views[handle] = buff_view;

    *pView = handle;

    return VK_SUCCESS;
}


void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_DestroyBufferView(VkDevice device,
                                                                      VkBufferView bufferView,
                                                                      const VkAllocationCallbacks *pAllocator) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_DestroyBufferView called >>");

    /**
     * get device object from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));

    if (it_device != vk_context->devices.end()) {
        it_device->second->buffer_views.erase(bufferView);
    }
}


VKAPI_ATTR VkResult VKAPI_CALL
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_BindBufferMemory(
        VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_BindBufferMemory called >>");

    /**
     * get device from hndl
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    if (it_device == vk_context->devices.end())
        return VK_ERROR_DEVICE_LOST;

    /**
     * get buffer from handle
     */
    auto it_buffer = it_device->second->buffers.find(buffer);
    if (it_buffer == it_device->second->buffers.end())
        return VK_ERROR_DEVICE_LOST;

    /**
     * get memory from handle
     */
    auto it_memory = it_device->second->memory_map.find(memory);
    if (it_memory == it_device->second->memory_map.end())
        return VK_ERROR_DEVICE_LOST;

    /**
     * verify if offset + size is ok from the memory area
     */
    if (memoryOffset + it_buffer->second->size > it_memory->second->size)
        return VK_ERROR_MEMORY_MAP_FAILED;

    /**
     * association
     */
    auto copy = it_memory->second;
    it_buffer->second->bound_memory = copy.get();
    it_buffer->second->bound_offset = memoryOffset;

    /**
     * host visible move from offset
     */
    if (it_memory->second->mapped_ptr) {
        it_buffer->second->mapped_ptr =
                static_cast<uint8_t *>(it_memory->second->mapped_ptr) + memoryOffset;
    }

    return VK_SUCCESS;
}


VkResult
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_CreateImage(VkDevice device,
                                                                               const VkImageCreateInfo *pCreateInfo,
                                                                               const VkAllocationCallbacks *pAllocator,
                                                                               VkImage *pImage) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_CreateImage called >>");

    /**
     * get device from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t >(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    VkImage g_real_handle;

    if (it_device == vk_context->devices.end() && it_instance == vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCreateImage = (PFN_vkCreateImage) func(
                it_device->second->dispatch_handle,
                "vkCreateImage");


        if(func) {
            VkResult ret = vkCreateImage(device, pCreateInfo, pAllocator, &g_real_handle);

            if(ret == VK_SUCCESS) {
                Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                             "CreateImage: succes to create image");
            }
        }

    }

    if (it_device == vk_context->devices.end()) return VK_ERROR_DEVICE_LOST;

    /**
     * representaion on the driver
     */
    auto chain_image_no_copiable = std::make_unique<VulkanContext::VkDeviceObject::VkImageObject>();
    chain_image_no_copiable->usage = pCreateInfo->usage;
    chain_image_no_copiable->format = pCreateInfo->format;
    chain_image_no_copiable->arrayLayers = pCreateInfo->arrayLayers;
    chain_image_no_copiable->mipLevels = pCreateInfo->mipLevels;
    chain_image_no_copiable->extent = pCreateInfo->extent;
    chain_image_no_copiable->info = *pCreateInfo;
    chain_image_no_copiable->dispatch_handle = g_real_handle;

    auto id = it_device->second->image_handler_id++;
    auto handle = reinterpret_cast<VkImage>(id);

    it_device->second->chain_images[handle] = std::move(chain_image_no_copiable);

    *pImage = handle;

    return VK_ERROR_OUT_OF_DEVICE_MEMORY;
}

void
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_DestroyImage(VkDevice device,
                                                                                VkImage image,
                                                                                const VkAllocationCallbacks *pAllocator) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_DestroyImage called >>");

    /**
     * get device from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    auto it_real_image_handle = it_device->second->chain_images.find(image);

    if (it_device == vk_context->devices.end() && it_instance == vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkDestroyImage = (PFN_vkDestroyImage) func(
                it_device->second->dispatch_handle,
                "vkDestroyImage");


        if(func) {
            vkDestroyImage(it_device->second->dispatch_handle, it_real_image_handle->second->dispatch_handle, pAllocator);
        }
    }

    if (it_device != vk_context->devices.end()) {
        it_device->second->chain_images.erase(image);
    }
}

VkResult
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_CreateImageView(VkDevice device,
                                                                                   const VkImageViewCreateInfo *pCreateInfo,
                                                                                   const VkAllocationCallbacks *pAllocator,
                                                                                   VkImageView *pView) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_CreateImageView called >>");

    /**
     * get device from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    auto it_real_image_handle = it_device->second->chain_images.find(pCreateInfo->image);

    if (it_device == vk_context->devices.end() && it_instance == vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCreateImageView = (PFN_vkCreateImageView) func(
                it_device->second->dispatch_handle,
                "vkCreateImageView");

        VkImage vk_image = it_real_image_handle->second->dispatch_handle;

        VkImageViewCreateInfo mod_info = *pCreateInfo;

        mod_info.image = vk_image;

        if(func) {
            // create pView literal
            VkResult ret = vkCreateImageView(it_device->second->dispatch_handle, &mod_info, pAllocator, pView);

            // save in unordered map to get image view atached to the image -> optional
            it_device->second->chain_image_views[pCreateInfo->image] = *pView;

            return ret;
        }
    }
}

void
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_DestroyImageView(VkDevice device,
                                                                                    VkImageView imageView,
                                                                                    const VkAllocationCallbacks *pAllocator) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_DestroyImageView called >>");

    /**
     * get device from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device == vk_context->devices.end() && it_instance == vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkDestroyImageView = (PFN_vkDestroyImageView) func(
                it_device->second->dispatch_handle,
                "vkDestroyImageView");

        vkDestroyImageView(device, imageView, pAllocator);
    }

}


VKAPI_ATTR VkResult VKAPI_CALL
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_BindImageMemory(
        VkDevice device,
        VkImage image,
        VkDeviceMemory memory,
        VkDeviceSize memoryOffset) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_BindImageMemory called >>");

    /**
     * get device from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    if (it_device == vk_context->devices.end()) return VK_ERROR_DEVICE_LOST;

    /**
     * get image from handle
     */
    auto it_image = it_device->second->chain_images.find(image);
    if (it_image == it_device->second->chain_images.end()) return VK_ERROR_DEVICE_LOST;

    /**
     * get memory handle
     */
    auto it_memory = it_device->second->memory_map.find(memory);
    if (it_memory == it_device->second->memory_map.end()) return VK_ERROR_DEVICE_LOST;


    /**
     * offset cannot be grater than size of memory region
     */
    if (memoryOffset >= it_memory->second->size) return VK_ERROR_MEMORY_MAP_FAILED;

    if (!it_image->second->is_binded) {
        /**
         * association
         */
        auto copy = it_memory->second;
        it_image->second->bound_memory = copy.get();
        it_image->second->memory_offset = memoryOffset;


        /**
         * move pointer pos
         */
        it_image->second->mapped_ptr =
                static_cast<uint8_t *>(it_memory->second->mapped_ptr) + memoryOffset;
    }

    return VK_SUCCESS;
}


void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_GetBufferMemoryRequirements(
        VkDevice device, VkBuffer buffer, VkMemoryRequirements *pMemoryRequirements) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_GetBufferMemoryRequirements called >>");

    /**
     * get device from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));

    /**
     * get buffer from handle
     */
    auto it_buffer = it_device->second->buffers.find(buffer);
    if (it_buffer != it_device->second->buffers.end()) {
        pMemoryRequirements->size = it_buffer->second->size;
        pMemoryRequirements->alignment = 256;
        pMemoryRequirements->memoryTypeBits = 0xFFFF;
    }
}


void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_GetImageMemoryRequirements(
        VkDevice device, VkImage image, VkMemoryRequirements *pMemoryRequirements) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_GetImageMemoryRequirements called >>");
    /**
     * device from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    if (it_device != vk_context->devices.end()) {
        auto it_image = it_device->second->chain_images.find(image);
        if (it_image != it_device->second->chain_images.end()) {
            switch (it_image->second->info.format) {
                /**
                 * support to bgra format only for now
                 */
                case VK_FORMAT_B8G8R8A8_UNORM:
                    pMemoryRequirements->size = it_image->second->info.extent.height *
                                                it_image->second->info.extent.width * 4;
                    break;
                default:
                    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                                 "vlk_trampoline_call_GetImageMemoryRequirements type not handled format:",
                                 it_image->second->format);
                    break;
            }
            pMemoryRequirements->alignment = 256;
            pMemoryRequirements->memoryTypeBits = 0xFFFF;
        }
    }
}

void
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_GetImageSparseMemoryRequirements(
        VkDevice device, VkImage image, uint32_t *pSparseMemoryRequirementCount,
        VkSparseImageMemoryRequirements *pSparseMemoryRequirements) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_GetImageSparseMemoryRequirements called >>");


    /**
     * we dont have support to sparse images for high coust of hw
     */
    // dont support that
    *pSparseMemoryRequirementCount = 0;
}


VkResult
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_QueueBindSparse(VkQueue queue,
                                                                                   uint32_t bindInfoCount,
                                                                                   const VkBindSparseInfo *pBindInfo,
                                                                                   VkFence fence) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_QueueBindSparse called >>");


    /**
     * we dont support that for now so just pass that function
     */
    return VK_SUCCESS;
}

VkResult
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_CreateFence(VkDevice device,
                                                                               const VkFenceCreateInfo *pCreateInfo,
                                                                               const VkAllocationCallbacks *pAllocator,
                                                                               VkFence *pFence) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_CreateFence called >>");


    /**
     * get device from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        const auto vk_get_dev_proc_addr = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        const auto vk_create_fence = (PFN_vkCreateFence) vk_get_dev_proc_addr(
                it_device->second->dispatch_handle, "vkCreateFence");

        VkFence host_fence = VK_NULL_HANDLE;
        VkResult result = vk_create_fence(it_device->second->dispatch_handle, pCreateInfo,
                                          pAllocator, &host_fence);
        if (result != VK_SUCCESS) return result;

        // alloc new fence
        auto fence = std::make_unique<VulkanContext::VkDeviceObject::VkFenceObject>();

        // def initial status
        fence->signaled = (pCreateInfo->flags & VK_FENCE_CREATE_SIGNALED_BIT);

        // real handle
        fence->dispatch_handle = host_fence;

        // gen handle
        auto id = it_device->second->fence_handler_id++;
        VkFence handle = reinterpret_cast<VkFence>(id);

        // register fence
        it_device->second->fences[handle] = std::move(fence);

        *pFence = handle;
    }

    return VK_SUCCESS;
}


void
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_DestroyFence(VkDevice device,
                                                                                VkFence fence,
                                                                                const VkAllocationCallbacks *pAllocator) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_DestroyFence called >>");

    /**
     * get device from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));

    if (it_device != vk_context->devices.end()) {
        it_device->second->fences.erase(fence);
    }
}


VkResult
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_ResetFences(VkDevice device,
                                                                               uint32_t fenceCount,
                                                                               const VkFence *pFences) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_ResetFences called >>");

    /**
     * get device from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    if (it_device == vk_context->devices.end()) {
        return VK_ERROR_DEVICE_LOST;
    }

    /**
     * reset fences status
     */
    for (int i = 0; i < fenceCount; i++) {
        auto &handle = pFences[i];

        auto it_fence = it_device->second->fences.find(handle);

        if (it_fence != it_device->second->fences.end()) {
            // now reset fence status
            it_fence->second->signaled = false;
        }
    }

    return VK_SUCCESS;
}

VkResult
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_GetFenceStatus(VkDevice device,
                                                                                  VkFence fence) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_GetFenceStatus called >>");

    /**
     * get device from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));

    if (it_device == vk_context->devices.end()) return VK_ERROR_DEVICE_LOST;

    /**
     * get fence from handle
     */
    auto it_fence = it_device->second->fences.find(fence);

    if (it_fence == it_device->second->fences.end()) return VK_ERROR_DEVICE_LOST;

    // return fence status
    if (it_fence->second->signaled) {
        return VK_SUCCESS;
    } else {
        return VK_NOT_READY;
    }
}

VkResult
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_WaitForFences(VkDevice device,
                                                                                 uint32_t fenceCount,
                                                                                 const VkFence *pFences,
                                                                                 VkBool32 waitAll,
                                                                                 uint64_t timeout) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_WaitForFences called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    if (it_device == vk_context->devices.end()) return VK_ERROR_DEVICE_LOST;

    auto &fence_map = it_device->second->fences;

    // if we wait for all fences
    if (waitAll) {
        for (uint32_t i = 0; i < fenceCount; ++i) {
            auto it_fence = fence_map.find(pFences[i]);
            if (it_fence == fence_map.end()) return VK_ERROR_DEVICE_LOST;
            if (!it_fence->second->signaled) {
                return VK_TIMEOUT;
            }
        }
        return VK_SUCCESS;
    } else {
        // check if any fence is signaled
        for (uint32_t i = 0; i < fenceCount; ++i) {
            auto it_fence = fence_map.find(pFences[i]);
            if (it_fence == fence_map.end()) return VK_ERROR_DEVICE_LOST;
            if (it_fence->second->signaled) {
                return VK_SUCCESS;
            }
        }
        return VK_TIMEOUT;
    }
}


VkResult
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_CreateSemaphore(VkDevice device,
                                                                                   const VkSemaphoreCreateInfo *pCreateInfo,
                                                                                   const VkAllocationCallbacks *pAllocator,
                                                                                   VkSemaphore *pSemaphore) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_CreateSemaphore called >>");

    /**
     * get device and instance from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device == vk_context->devices.end() && it_instance == vk_context->instances.end())
        return VK_ERROR_DEVICE_LOST;

    auto id = it_device->second->semaphore_handler_id++;
    auto handle = reinterpret_cast<VkSemaphore>(id);

    auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
            it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
    auto vkCreateSemaphore = (PFN_vkCreateSemaphore) func(it_device->second->dispatch_handle,
                                                          "vkCreateSemaphore");

    VkSemaphore copy = VK_NULL_HANDLE;
    VkResult result = vkCreateSemaphore(it_device->second->dispatch_handle, pCreateInfo, pAllocator,
                                        &copy);
    if (result != VK_SUCCESS) return result;

    auto sem_obj = std::make_unique<VulkanContext::VkDeviceObject::VkSemaphoreObject>();
    sem_obj->dispatch_handle = copy;
    sem_obj->signaled = false;

    it_device->second->semaphores[handle] = std::move(sem_obj);

    *pSemaphore = handle;
    return VK_SUCCESS;
}

void
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_DestroySemaphore(VkDevice device,
                                                                                    VkSemaphore semaphore,
                                                                                    const VkAllocationCallbacks *pAllocator) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_DestroySemaphore called >>");

    /**
     * get device and instance from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));

    if (it_device != vk_context->devices.end()) {
        it_device->second->semaphores.erase(semaphore);
    }
}


VkResult
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_CreateEvent(VkDevice device,
                                                                               const VkEventCreateInfo *pCreateInfo,
                                                                               const VkAllocationCallbacks *pAllocator,
                                                                               VkEvent *pEvent) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_CreateEvent called >>");

    /**
     * get device from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));

    if (it_device == vk_context->devices.end()) return VK_ERROR_DEVICE_LOST;

    auto id = it_device->second->event_handler_id++;
    VkEvent handle = reinterpret_cast<VkEvent>(id);

    it_device->second->events[handle] = std::move(
            std::make_unique<VulkanContext::VkDeviceObject::VkEventObject>());

    *pEvent = handle;

    return VK_SUCCESS;
}

void
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_DestroyEvent(VkDevice device,
                                                                                VkEvent event,
                                                                                const VkAllocationCallbacks *pAllocator) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_DestroyEvent called >>");


    /**
    * get device from handle
    */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));

    if (it_device != vk_context->devices.end()) {
        it_device->second->events.erase(event);
    }
}

VkResult
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_GetEventStatus(VkDevice device,
                                                                                  VkEvent event) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_GetEventStatus called >>");


    /**
      * get device from handle
      */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));

    auto it_event = it_device->second->events.find(event);
    if (it_event == it_device->second->events.end()) return VK_ERROR_DEVICE_LOST;

    return it_event->second->signaled ? VK_EVENT_SET : VK_EVENT_RESET;
}

VkResult
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_SetEvent(VkDevice device,
                                                                            VkEvent event) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_SetEvent called >>");

    /**
      * get device from handle
      */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));

    auto it_event = it_device->second->events.find(event);
    if (it_event == it_device->second->events.end()) return VK_ERROR_DEVICE_LOST;

    it_event->second->signaled.store(VK_TRUE);

    return VK_SUCCESS;
}

VkResult
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_ResetEvent(VkDevice device,
                                                                              VkEvent event) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_ResetEvent called >>");

    /**
     * get device from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));

    auto it_event = it_device->second->events.find(event);
    if (it_event == it_device->second->events.end()) return VK_ERROR_DEVICE_LOST;

    it_event->second->signaled.store(VK_FALSE);

    return VK_SUCCESS;
}


VkResult
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_CreateQueryPool(VkDevice device,
                                                                                   const VkQueryPoolCreateInfo *pCreateInfo,
                                                                                   const VkAllocationCallbacks *pAllocator,
                                                                                   VkQueryPool *pQueryPool) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_CreateQueryPool called >>");


    if (!pCreateInfo || pCreateInfo->queryCount == 0) {
        return VK_ERROR_DEVICE_LOST;
    }

    /**
     * get device from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));

    if(it_device == vk_context->devices.end()) return VK_ERROR_INITIALIZATION_FAILED;

    auto pool = std::make_unique<VulkanContext::VkDeviceObject::VkQueryPoolObject>();

    pool->type = pCreateInfo->queryType;
    pool->count = pCreateInfo->queryCount;
    pool->pipelineStats = pCreateInfo->pipelineStatistics;

    pool->results.resize(pCreateInfo->queryCount);

    auto id = it_device->second->queries_handler_id++;

    auto handle = reinterpret_cast<VkQueryPool>(id);

    it_device->second->queries[handle] = std::move(pool);

    *pQueryPool = handle;

    return VK_ERROR_OUT_OF_DEVICE_MEMORY;
}


void
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_DestroyQueryPool(VkDevice device,
                                                                                    VkQueryPool queryPool,
                                                                                    const VkAllocationCallbacks *pAllocator) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_DestroyQueryPool called >>");


    /**
    * get device from handle
    */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));

    if(it_device != vk_context->devices.end()) {
        it_device->second->queries.erase(queryPool);
    }
}

VkResult VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_GetQueryPoolResults(
        VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount,
        size_t dataSize, void *pData, VkDeviceSize stride, VkQueryResultFlags flags) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_GetQueryPoolResults called >>");

    /**
     * get device from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));

    if(it_device == vk_context->devices.end()) return VK_ERROR_DEVICE_LOST;

    /**
     * get query pool obj
     */
    auto it_query = it_device->second->queries.find(queryPool);

    auto* dst = reinterpret_cast<uint8_t*>(pData);

    // for loop
    for(int i = 0; i < queryCount; ++i) {
        const VulkanContext::VkDeviceObject::VkQueryPoolObject::VkQueryResultObject& res = it_query->second->results[firstQuery + i];

        if(!(res.available) && (flags & VK_QUERY_RESULT_WAIT_BIT)) {}

        bool avaible = res.available;

        // size

        if(flags & VK_QUERY_RESULT_64_BIT) {
            uint64_t val = res.available ? res.value : 0;

            memcpy(dst, &val, sizeof(uint64_t));

            if(flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) {
                uint64_t avail = avaible ? 1 : 0;

                memcpy(dst + sizeof(uint64_t), &avail, sizeof(uint64_t));

            }
        } else {
            uint64_t val = res.available ? (uint32_t)res.value : 0;

            memcpy(dst, &val, sizeof(uint32_t));

            if(flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) {
                uint64_t avail = avaible ? 1 : 0;

                memcpy(dst + sizeof(uint32_t), &avail, sizeof(uint32_t));
            }
        }

        dst += stride;
    }


    for(uint32_t i = 0; i < queryCount; ++i) {
        if(!it_query->second->results[firstQuery + i].available && (flags & VK_QUERY_RESULT_WAIT_BIT)) return VK_NOT_READY;
    }

    return VK_SUCCESS;
}

void
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_GetImageSubresourceLayout(
        VkDevice device, VkImage image, const VkImageSubresource *pSubresource,
        VkSubresourceLayout *pLayout) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_GetImageSubresourceLayout called >>");

    /**
    * get device and instance from handle
    */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device == vk_context->devices.end() && it_instance == vk_context->instances.end()) {
        auto it_image_real_handle = it_device->second->chain_images.find(image);


        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkGetImageSubresourceLayout = (PFN_vkGetImageSubresourceLayout) func(it_device->second->dispatch_handle,
                                                                                  "vkGetImageSubresourceLayout");


        if(func)
            vkGetImageSubresourceLayout(device, it_image_real_handle->second->dispatch_handle, pSubresource, pLayout);
    }


}


PFN_vkVoidFunction
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_GetDeviceProcAddr(
        VkDevice device, const char *pName) {

    std::string str(pName);
    const auto it = device_dispatch_table.find(str);

    if (it != device_dispatch_table.end()) {
        Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                     "vlk_trampoline_call_GetDeviceProcAddr looking impl function: ",
                     pName);
        return it->second;
    }


    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);


    if (it == device_dispatch_table.end()) {
        Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                     "vlk_trampoline_call_GetDeviceProcAddr looking non impl function: ",
                     pName);
        const auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");

        auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
        return func(it_device->second->dispatch_handle, pName);
    }

    /**
     * loader will be redirected with error cannot retrieve function 'pName' in this case loader understand this that not present in the driver
     */
    return nullptr;
}


bool VulkanDispatcher::Device::DeviceDispatchTable::factory() {
    auto state = false;

    /**
     * device functions
     */
    state = registerTrampoline("vkCreateDevice", vlk_trampoline_call_CreateDevice);

    state = registerTrampoline("vkDestroyDevice", vlk_trampoline_call_DestroyDevice);

    state = registerTrampoline("vkGetDeviceQueue", vlk_trampoline_call_GetDeviceQueue);

    state = registerTrampoline("vkQueueSubmit", vlk_trampoline_call_QueueSubmit);

    state = registerTrampoline("vkQueueWaitIdle", vlk_trampoline_call_QueueWaitIdle);

    state = registerTrampoline("vkDeviceWaitIdle", vlk_trampoline_call_DeviceWaitIdle);

    state = registerTrampoline("vkAllocateMemory", vlk_trampoline_call_AllocateMemory);

    state = registerTrampoline("vkMapMemory", vlk_trampoline_call_MapMemory);

    state = registerTrampoline("vkFreeMemory", vlk_trampoline_call_FreeMemory);

    state = registerTrampoline("vkUnmapMemory", vlk_trampoline_call_UnmapMemory);

    state = registerTrampoline("vkFlushMappedMemoryRanges",
                               vlk_trampoline_call_FlushMappedMemoryRanges);

    state = registerTrampoline("vkInvalidateMappedMemoryRanges",
                               vlk_trampoline_call_InvalidateMappedMemoryRanges);

    state = registerTrampoline("vkGetDeviceMemoryCommitment",
                               vlk_trampoline_call_GetDeviceMemoryCommitment);

    state = registerTrampoline("vkCreateBuffer", vlk_trampoline_call_CreateBuffer);

    state = registerTrampoline("vkDestroyBuffer", vlk_trampoline_call_DestroyBuffer);

    state = registerTrampoline("vkBindBufferMemory", vlk_trampoline_call_BindBufferMemory);

    state = registerTrampoline("vkCreateImage", vlk_trampoline_call_CreateImage);

    state = registerTrampoline("vkDestroyImage", vlk_trampoline_call_DestroyImage);

    state = registerTrampoline("vkBindImageMemory", vlk_trampoline_call_BindImageMemory);

    state = registerTrampoline("vkGetBufferMemoryRequirements",
                               vlk_trampoline_call_GetBufferMemoryRequirements);

    state = registerTrampoline("vkGetImageMemoryRequirements",
                               vlk_trampoline_call_GetImageMemoryRequirements);

    state = registerTrampoline("vkGetImageSparseMemoryRequirements",
                               vlk_trampoline_call_GetImageSparseMemoryRequirements);

    state = registerTrampoline("vkQueueBindSparse", vlk_trampoline_call_QueueBindSparse);

    state = registerTrampoline("vkCreateFence", vlk_trampoline_call_CreateFence);

    state = registerTrampoline("vkDestroyFence", vlk_trampoline_call_DestroyFence);

    state = registerTrampoline("vkResetFences", vlk_trampoline_call_ResetFences);

    state = registerTrampoline("vkGetFenceStatus", vlk_trampoline_call_GetFenceStatus);

    state = registerTrampoline("vkWaitForFences", vlk_trampoline_call_WaitForFences);

    state = registerTrampoline("vkCreateSemaphore", vlk_trampoline_call_CreateSemaphore);

    state = registerTrampoline("vkDestroySemaphore", vlk_trampoline_call_DestroySemaphore);

    state = registerTrampoline("vkCreateEvent", vlk_trampoline_call_CreateEvent);

    state = registerTrampoline("vkDestroyEvent", vlk_trampoline_call_DestroyEvent);

    state = registerTrampoline("vkGetEventStatus", vlk_trampoline_call_GetEventStatus);

    state = registerTrampoline("vkSetEvent", vlk_trampoline_call_SetEvent);

    state = registerTrampoline("vkResetEvent", vlk_trampoline_call_ResetEvent);

    state = registerTrampoline("vkCreateQueryPool", vlk_trampoline_call_CreateQueryPool);

    state = registerTrampoline("vkDestroyQueryPool", vlk_trampoline_call_DestroyQueryPool);

    state = registerTrampoline("vkGetQueryPoolResults", vlk_trampoline_call_GetQueryPoolResults);

    state = registerTrampoline("vkCreateBufferView", vlk_trampoline_call_CreateBufferView);

    state = registerTrampoline("vkDestroyBufferView", vlk_trampoline_call_DestroyBufferView);

    state = registerTrampoline("vkGetImageSubresourceLayout", vlk_trampoline_call_GetImageSubresourceLayout);

    state = registerTrampoline("vkCreateImageView", vlk_trampoline_call_CreateImageView);

    state = registerTrampoline("vkDestroyImageView", vlk_trampoline_call_DestroyImageView);

    state = registerTrampoline("vkGetDeviceProcAddr", vlk_trampoline_call_GetDeviceProcAddr);

    return state;
}

