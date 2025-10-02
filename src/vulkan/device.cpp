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
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);



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

    it_device->second->chain_image_views.clear();

    if (it_device == vk_context->devices.end() && it_instance == vk_context->instances.end()) {

        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkDestroyDevice = (PFN_vkDestroyDevice) func(it_device->second->dispatch_handle,
                                                                                  "vkDestroyDevice");


        if(func)
            vkDestroyDevice(it_device->second->dispatch_handle, pAllocator);
    }


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

    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_instance == vk_context->instances.end()) return VK_ERROR_DEVICE_LOST;


    auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
    auto vkCreateBuffer = (PFN_vkCreateBuffer)func(it_device->second->dispatch_handle, "vkCreateBuffer");


    /**
     * create buffer representation object
     */
    auto buffer = std::make_unique<VulkanContext::VkDeviceObject::VkBufferObject>();
    buffer->size = pCreateInfo->size;
    buffer->usage = pCreateInfo->usage;
    buffer->sharingMode = pCreateInfo->sharingMode;

    vkCreateBuffer(it_device->second->dispatch_handle, pCreateInfo, pAllocator, &buffer->dispatch_handle);


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
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()){

        auto it_buffer = it_device->second->buffers.find(buffer);

        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkDestroyBuffer = (PFN_vkDestroyBuffer)func(it_device->second->dispatch_handle, "vkDestroyBuffer");

        vkDestroyBuffer(it_device->second->dispatch_handle, it_buffer->second->dispatch_handle, pAllocator);

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

    buff_view.handle = it_device->second->buffers.find(pCreateInfo->buffer)->second->dispatch_handle;

    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {

        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCreateBufferView = (PFN_vkCreateBufferView) func(it_device->second->dispatch_handle,
                                                          "vkCreateBufferView");

        VkBufferViewCreateInfo mod_info = *pCreateInfo;

        mod_info.buffer = buff_view.handle;
        vkCreateBufferView(it_device->second->dispatch_handle, &mod_info, pAllocator, pView);

    }

    it_device->second->buffer_views[*pView] = buff_view;

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
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {

        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkDestroyBufferView = (PFN_vkDestroyBufferView) func(it_device->second->dispatch_handle,
                                                                "vkDestroyBufferView");


        vkDestroyBufferView(it_device->second->dispatch_handle, bufferView, pAllocator);

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

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkDestroyImage = (PFN_vkDestroyImage) func(
                it_device->second->dispatch_handle,
                "vkDestroyImage");


        if(func) {
            vkDestroyImage(it_device->second->dispatch_handle, it_real_image_handle->second->dispatch_handle, pAllocator);
        }

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

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
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

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
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

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto it_image_real_handle = it_device->second->chain_images.find(image);


        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkGetImageSubresourceLayout = (PFN_vkGetImageSubresourceLayout) func(it_device->second->dispatch_handle,
                                                                                  "vkGetImageSubresourceLayout");


        if(func)
            vkGetImageSubresourceLayout(device, it_image_real_handle->second->dispatch_handle, pSubresource, pLayout);
    }


}

VkResult VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_CreateShaderModule(
        VkDevice device, const VkShaderModuleCreateInfo *pCreateInfo,
        const VkAllocationCallbacks *pAllocator, VkShaderModule *pShaderModule) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_CreateShaderModule called >>");

    /**
      * get device and instance from handle
      */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    VkResult ret{VK_ERROR_DEVICE_LOST};

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {

        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCreateShaderModule = (PFN_vkCreateShaderModule) func(it_device->second->dispatch_handle,
                                                                    "vkCreateShaderModule");


        if(func)
            ret = vkCreateShaderModule(it_device->second->dispatch_handle, pCreateInfo, pAllocator, pShaderModule);
    }

    return ret;
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_DestroyShaderModule(
        VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks *pAllocator) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_DestroyShaderModule called >>");

    /**
      * get device and instance from handle
      */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {

        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkDestroyShaderModule = (PFN_vkDestroyShaderModule) func(it_device->second->dispatch_handle,
                                                                      "vkDestroyShaderModule");


        if(func)
            vkDestroyShaderModule(it_device->second->dispatch_handle, shaderModule, pAllocator);
    }

}

VkResult VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_CreatePipelineCache(
        VkDevice device, const VkPipelineCacheCreateInfo *pCreateInfo,
        const VkAllocationCallbacks *pAllocator, VkPipelineCache *pPipelineCache) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_CreatePipelineCache called >>");

    /**
     * get device and instance from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    VkResult ret{VK_ERROR_DEVICE_LOST};

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {

        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCreatePipelineCache = (PFN_vkCreatePipelineCache) func(it_device->second->dispatch_handle,
                                                                      "vkCreatePipelineCache");


        if(func)
            ret = vkCreatePipelineCache(it_device->second->dispatch_handle, pCreateInfo, pAllocator, pPipelineCache);
    }

    return ret;
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_DestroyPipelineCache(
        VkDevice device, VkPipelineCache pipelineCache, const VkAllocationCallbacks *pAllocator) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_DestroyPipelineCache called >>");

    /**
     * get device and instance from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {

        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkDestroyPipelineCache = (PFN_vkDestroyPipelineCache) func(it_device->second->dispatch_handle,
                                                                      "vkDestroyPipelineCache");


        if(func)
            vkDestroyPipelineCache(it_device->second->dispatch_handle, pipelineCache, pAllocator);
    }

}

VkResult VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_GetPipelineCacheData(
        VkDevice device, VkPipelineCache pipelineCache, size_t *pDataSize, void *pData) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_GetPipelineCacheData called >>");

    /**
     * get device and instance from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    VkResult ret{VK_ERROR_DEVICE_LOST};

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {

        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkGetPipelineCacheData = (PFN_vkGetPipelineCacheData) func(it_device->second->dispatch_handle,
                                                                      "vkGetPipelineCacheData");


        if(func)
            ret = vkGetPipelineCacheData(it_device->second->dispatch_handle, pipelineCache, pDataSize, pData);
    }

    return ret;
}


VkResult VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_MergePipelineCaches(
        VkDevice device, VkPipelineCache dstCache, uint32_t srcCacheCount,
        const VkPipelineCache *pSrcCaches) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_MergePipelineCaches called >>");

    /**
     * get device and instance from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    VkResult ret{VK_ERROR_DEVICE_LOST};

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {

        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkMergePipelineCaches = (PFN_vkMergePipelineCaches) func(it_device->second->dispatch_handle,
                                                                        "vkMergePipelineCaches");


        if(func)
            ret = vkMergePipelineCaches(it_device->second->dispatch_handle, dstCache, srcCacheCount, pSrcCaches);
    }

    return ret;
}

VkResult VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_CreateGraphicsPipelines(
        VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
        const VkGraphicsPipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator,
        VkPipeline *pPipelines) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_CreateGraphicsPipelines called >>");

    /**
     * get device and instance from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    VkResult ret{VK_ERROR_DEVICE_LOST};

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {

        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines) func(it_device->second->dispatch_handle,
                                                                      "vkCreateGraphicsPipelines");


        if(func)
            ret = vkCreateGraphicsPipelines(it_device->second->dispatch_handle, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    }

    return ret;
}

VkResult VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_CreateComputePipelines(
        VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
        const VkComputePipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator,
        VkPipeline *pPipelines) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_CreateComputePipelines called >>");

    /**
     * get device and instance from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    VkResult ret{VK_ERROR_DEVICE_LOST};

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {

        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCreateComputePipelines = (PFN_vkCreateComputePipelines) func(it_device->second->dispatch_handle,
                                                                              "vkCreateComputePipelines");


        if(func)
            ret = vkCreateComputePipelines(it_device->second->dispatch_handle, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    }

    return ret;
}

void
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_DestroyPipeline(VkDevice device,
                                                                                   VkPipeline pipeline,
                                                                                   const VkAllocationCallbacks *pAllocator) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_DestroyPipeline called >>");

    /**
     * get device and instance from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {

        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkDestroyPipeline = (PFN_vkDestroyPipeline) func(it_device->second->dispatch_handle,
                                                                        "vkDestroyPipeline");


        if(func)
            vkDestroyPipeline(it_device->second->dispatch_handle, pipeline, pAllocator);
    }
}

VkResult VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_CreatePipelineLayout(
        VkDevice device, const VkPipelineLayoutCreateInfo *pCreateInfo,
        const VkAllocationCallbacks *pAllocator, VkPipelineLayout *pPipelineLayout) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_CreatePipelineLayout called >>");

    /**
     * get device and instance from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    VkResult ret{VK_ERROR_DEVICE_LOST};

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {

        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout) func(it_device->second->dispatch_handle,
                                                                            "vkCreatePipelineLayout");


        if(func)
            ret = vkCreatePipelineLayout(it_device->second->dispatch_handle, pCreateInfo, pAllocator, pPipelineLayout);
    }

    return ret;
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_DestroyPipelineLayout(
        VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks *pAllocator) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_DestroyPipeline called >>");

    /**
     * get device and instance from handle
     */
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {

        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout) func(it_device->second->dispatch_handle,
                                                              "vkDestroyPipelineLayout");


        if(func)
            vkDestroyPipelineLayout(it_device->second->dispatch_handle, pipelineLayout, pAllocator);
    }
}

VkResult VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_CreateSampler(
        VkDevice device,
        const VkSamplerCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkSampler* pSampler)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_CreateSampler called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    VkResult ret{VK_ERROR_DEVICE_LOST};

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCreateSampler = (PFN_vkCreateSampler) func(it_device->second->dispatch_handle,
                                                          "vkCreateSampler");

        if (vkCreateSampler)
            ret = vkCreateSampler(it_device->second->dispatch_handle, pCreateInfo, pAllocator, pSampler);
    }

    return ret;
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_DestroySampler(
        VkDevice device,
        VkSampler sampler,
        const VkAllocationCallbacks* pAllocator)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_DestroySampler called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkDestroySampler = (PFN_vkDestroySampler) func(it_device->second->dispatch_handle,
                                                            "vkDestroySampler");

        if (vkDestroySampler)
            vkDestroySampler(it_device->second->dispatch_handle, sampler, pAllocator);
    }
}

VkResult VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_CreateDescriptorSetLayout(
        VkDevice device,
        const VkDescriptorSetLayoutCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDescriptorSetLayout* pSetLayout)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_CreateDescriptorSetLayout called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    VkResult ret{VK_ERROR_DEVICE_LOST};

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCreateDescriptorSetLayout =
                (PFN_vkCreateDescriptorSetLayout) func(it_device->second->dispatch_handle,
                                                       "vkCreateDescriptorSetLayout");

        if (vkCreateDescriptorSetLayout)
            ret = vkCreateDescriptorSetLayout(it_device->second->dispatch_handle,
                                              pCreateInfo, pAllocator, pSetLayout);
    }

    return ret;
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_DestroyDescriptorSetLayout(
        VkDevice device,
        VkDescriptorSetLayout descriptorSetLayout,
        const VkAllocationCallbacks* pAllocator)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_DestroyDescriptorSetLayout called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkDestroyDescriptorSetLayout =
                (PFN_vkDestroyDescriptorSetLayout) func(it_device->second->dispatch_handle,
                                                        "vkDestroyDescriptorSetLayout");

        if (vkDestroyDescriptorSetLayout)
            vkDestroyDescriptorSetLayout(it_device->second->dispatch_handle, descriptorSetLayout, pAllocator);
    }
}

VkResult VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_CreateDescriptorPool(
        VkDevice device,
        const VkDescriptorPoolCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDescriptorPool* pDescriptorPool)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_CreateDescriptorPool called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    VkResult ret{VK_ERROR_DEVICE_LOST};

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCreateDescriptorPool =
                (PFN_vkCreateDescriptorPool) func(it_device->second->dispatch_handle,
                                                  "vkCreateDescriptorPool");

        if (vkCreateDescriptorPool)
            ret = vkCreateDescriptorPool(it_device->second->dispatch_handle,
                                         pCreateInfo, pAllocator, pDescriptorPool);
    }

    return ret;
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_DestroyDescriptorPool(
        VkDevice device,
        VkDescriptorPool descriptorPool,
        const VkAllocationCallbacks* pAllocator)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_DestroyDescriptorPool called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkDestroyDescriptorPool =
                (PFN_vkDestroyDescriptorPool) func(it_device->second->dispatch_handle,
                                                   "vkDestroyDescriptorPool");

        if (vkDestroyDescriptorPool)
            vkDestroyDescriptorPool(it_device->second->dispatch_handle, descriptorPool, pAllocator);
    }
}

VkResult VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_ResetDescriptorPool(
        VkDevice device,
        VkDescriptorPool descriptorPool,
        VkDescriptorPoolResetFlags flags)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_ResetDescriptorPool called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    VkResult ret{VK_ERROR_DEVICE_LOST};

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkResetDescriptorPool =
                (PFN_vkResetDescriptorPool) func(it_device->second->dispatch_handle,
                                                 "vkResetDescriptorPool");

        if (vkResetDescriptorPool)
            ret = vkResetDescriptorPool(it_device->second->dispatch_handle,
                                        descriptorPool, flags);
    }

    return ret;
}

VkResult VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_AllocateDescriptorSets(
        VkDevice device,
        const VkDescriptorSetAllocateInfo* pAllocateInfo,
        VkDescriptorSet* pDescriptorSets)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_AllocateDescriptorSets called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    VkResult ret{VK_ERROR_DEVICE_LOST};

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkAllocateDescriptorSets =
                (PFN_vkAllocateDescriptorSets) func(it_device->second->dispatch_handle,
                                                    "vkAllocateDescriptorSets");

        if (vkAllocateDescriptorSets)
            ret = vkAllocateDescriptorSets(it_device->second->dispatch_handle,
                                           pAllocateInfo, pDescriptorSets);
    }

    return ret;
}

VkResult VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_FreeDescriptorSets(
        VkDevice device,
        VkDescriptorPool descriptorPool,
        uint32_t descriptorSetCount,
        const VkDescriptorSet* pDescriptorSets)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_FreeDescriptorSets called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    VkResult ret{VK_ERROR_DEVICE_LOST};

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkFreeDescriptorSets =
                (PFN_vkFreeDescriptorSets) func(it_device->second->dispatch_handle,
                                                "vkFreeDescriptorSets");

        if (vkFreeDescriptorSets)
            ret = vkFreeDescriptorSets(it_device->second->dispatch_handle,
                                       descriptorPool, descriptorSetCount, pDescriptorSets);
    }

    return ret;
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_UpdateDescriptorSets(
        VkDevice device,
        uint32_t descriptorWriteCount,
        const VkWriteDescriptorSet* pDescriptorWrites,
        uint32_t descriptorCopyCount,
        const VkCopyDescriptorSet* pDescriptorCopies)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_UpdateDescriptorSets called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkUpdateDescriptorSets =
                (PFN_vkUpdateDescriptorSets) func(it_device->second->dispatch_handle,
                                                  "vkUpdateDescriptorSets");

        if (vkUpdateDescriptorSets)
            vkUpdateDescriptorSets(it_device->second->dispatch_handle,
                                   descriptorWriteCount, pDescriptorWrites,
                                   descriptorCopyCount, pDescriptorCopies);
    }
}

VkResult VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_CreateFramebuffer(
        VkDevice device,
        const VkFramebufferCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkFramebuffer* pFramebuffer)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_CreateFramebuffer called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    VkResult ret{VK_ERROR_DEVICE_LOST};

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCreateFramebuffer =
                (PFN_vkCreateFramebuffer) func(it_device->second->dispatch_handle,
                                               "vkCreateFramebuffer");

        if (vkCreateFramebuffer)
            ret = vkCreateFramebuffer(it_device->second->dispatch_handle,
                                      pCreateInfo, pAllocator, pFramebuffer);
    }

    return ret;
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_DestroyFramebuffer(
        VkDevice device,
        VkFramebuffer framebuffer,
        const VkAllocationCallbacks* pAllocator)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_DestroyFramebuffer called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkDestroyFramebuffer =
                (PFN_vkDestroyFramebuffer) func(it_device->second->dispatch_handle,
                                                "vkDestroyFramebuffer");

        if (vkDestroyFramebuffer)
            vkDestroyFramebuffer(it_device->second->dispatch_handle, framebuffer, pAllocator);
    }
}

VkResult VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_CreateRenderPass(
        VkDevice device,
        const VkRenderPassCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkRenderPass* pRenderPass)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_CreateRenderPass called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    VkResult ret{VK_ERROR_DEVICE_LOST};

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCreateRenderPass =
                (PFN_vkCreateRenderPass) func(it_device->second->dispatch_handle,
                                              "vkCreateRenderPass");

        if (vkCreateRenderPass)
            ret = vkCreateRenderPass(it_device->second->dispatch_handle,
                                     pCreateInfo, pAllocator, pRenderPass);
    }

    return ret;
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_DestroyRenderPass(
        VkDevice device,
        VkRenderPass renderPass,
        const VkAllocationCallbacks* pAllocator)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_DestroyRenderPass called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkDestroyRenderPass =
                (PFN_vkDestroyRenderPass) func(it_device->second->dispatch_handle,
                                               "vkDestroyRenderPass");

        if (vkDestroyRenderPass)
            vkDestroyRenderPass(it_device->second->dispatch_handle, renderPass, pAllocator);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_GetRenderAreaGranularity(
        VkDevice device,
        VkRenderPass renderPass,
        VkExtent2D* pGranularity)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_GetRenderAreaGranularity called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkGetRenderAreaGranularity =
                (PFN_vkGetRenderAreaGranularity) func(it_device->second->dispatch_handle,
                                                      "vkGetRenderAreaGranularity");

        if (vkGetRenderAreaGranularity)
            vkGetRenderAreaGranularity(it_device->second->dispatch_handle, renderPass, pGranularity);
    }
}

VkResult VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_CreateCommandPool(
        VkDevice device,
        const VkCommandPoolCreateInfo* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkCommandPool* pCommandPool)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_CreateCommandPool called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    VkResult ret{VK_ERROR_DEVICE_LOST};

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCreateCommandPool =
                (PFN_vkCreateCommandPool) func(it_device->second->dispatch_handle,
                                               "vkCreateCommandPool");

        if (vkCreateCommandPool)
            ret = vkCreateCommandPool(it_device->second->dispatch_handle,
                                      pCreateInfo, pAllocator, pCommandPool);
    }

    return ret;
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_DestroyCommandPool(
        VkDevice device,
        VkCommandPool commandPool,
        const VkAllocationCallbacks* pAllocator)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_DestroyCommandPool called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkDestroyCommandPool =
                (PFN_vkDestroyCommandPool) func(it_device->second->dispatch_handle,
                                                "vkDestroyCommandPool");

        if (vkDestroyCommandPool)
            vkDestroyCommandPool(it_device->second->dispatch_handle, commandPool, pAllocator);
    }
}

VkResult VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_ResetCommandPool(
        VkDevice device,
        VkCommandPool commandPool,
        VkCommandPoolResetFlags flags)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_ResetCommandPool called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    VkResult ret{VK_ERROR_DEVICE_LOST};

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkResetCommandPool =
                (PFN_vkResetCommandPool) func(it_device->second->dispatch_handle,
                                              "vkResetCommandPool");

        if (vkResetCommandPool)
            ret = vkResetCommandPool(it_device->second->dispatch_handle,
                                     commandPool, flags);
    }

    return ret;
}

VKAPI_ATTR VkResult VKAPI_CALL
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_AllocateCommandBuffers(
        VkDevice device,
        const VkCommandBufferAllocateInfo* pAllocateInfo,
        VkCommandBuffer* pCommandBuffers)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_AllocateCommandBuffers called >>");

    VkResult ret{VK_ERROR_DEVICE_LOST};
    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)func(
                it_device->second->dispatch_handle, "vkAllocateCommandBuffers");

        if (vkAllocateCommandBuffers)
            ret = vkAllocateCommandBuffers(
                    it_device->second->dispatch_handle, pAllocateInfo, pCommandBuffers);
    }
    return ret;
}

VKAPI_ATTR void VKAPI_CALL
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_FreeCommandBuffers(
        VkDevice device,
        VkCommandPool commandPool,
        uint32_t commandBufferCount,
        const VkCommandBuffer* pCommandBuffers)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_FreeCommandBuffers called >>");

    auto it_device = vk_context->devices.find(reinterpret_cast<uint64_t>(device));
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkFreeCommandBuffers = (PFN_vkFreeCommandBuffers)func(
                it_device->second->dispatch_handle, "vkFreeCommandBuffers");

        if (vkFreeCommandBuffers)
            vkFreeCommandBuffers(
                    it_device->second->dispatch_handle, commandPool, commandBufferCount, pCommandBuffers);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_BeginCommandBuffer(
        VkCommandBuffer commandBuffer,
        const VkCommandBufferBeginInfo* pBeginInfo)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_BeginCommandBuffer called >>");

    VkResult ret{VK_ERROR_DEVICE_LOST};
    auto it_device = vk_context->devices.begin();
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)func(
                it_device->second->dispatch_handle, "vkBeginCommandBuffer");

        if (vkBeginCommandBuffer)
            ret = vkBeginCommandBuffer(commandBuffer, pBeginInfo);
    }
    return ret;
}

VKAPI_ATTR VkResult VKAPI_CALL
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_EndCommandBuffer(
        VkCommandBuffer commandBuffer)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_EndCommandBuffer called >>");

    VkResult ret{VK_ERROR_DEVICE_LOST};
    auto it_device = vk_context->devices.begin();
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkEndCommandBuffer = (PFN_vkEndCommandBuffer)func(
                it_device->second->dispatch_handle, "vkEndCommandBuffer");

        if (vkEndCommandBuffer)
            ret = vkEndCommandBuffer(commandBuffer);
    }
    return ret;
}

VKAPI_ATTR VkResult VKAPI_CALL
VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_ResetCommandBuffer(
        VkCommandBuffer commandBuffer,
        VkCommandBufferResetFlags flags)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_ResetCommandBuffer called >>");

    VkResult ret{VK_ERROR_DEVICE_LOST};
    auto it_device = vk_context->devices.begin();
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkResetCommandBuffer = (PFN_vkResetCommandBuffer)func(
                it_device->second->dispatch_handle, "vkResetCommandBuffer");

        if (vkResetCommandBuffer)
            ret = vkResetCommandBuffer(commandBuffer, flags);
    }
    return ret;
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdBindPipeline(
        VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdBindPipeline called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdBindPipeline = (PFN_vkCmdBindPipeline)func(it_device->second->dispatch_handle, "vkCmdBindPipeline");
        if (vkCmdBindPipeline) vkCmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdSetViewport(
        VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount, const VkViewport* pViewports)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdSetViewport called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdSetViewport = (PFN_vkCmdSetViewport)func(it_device->second->dispatch_handle, "vkCmdSetViewport");
        if (vkCmdSetViewport) vkCmdSetViewport(commandBuffer, firstViewport, viewportCount, pViewports);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdSetScissor(
        VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount, const VkRect2D* pScissors)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdSetScissor called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdSetScissor = (PFN_vkCmdSetScissor)func(it_device->second->dispatch_handle, "vkCmdSetScissor");
        if (vkCmdSetScissor) vkCmdSetScissor(commandBuffer, firstScissor, scissorCount, pScissors);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdSetLineWidth(
        VkCommandBuffer commandBuffer, float lineWidth)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdSetLineWidth called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdSetLineWidth = (PFN_vkCmdSetLineWidth)func(it_device->second->dispatch_handle, "vkCmdSetLineWidth");
        if (vkCmdSetLineWidth) vkCmdSetLineWidth(commandBuffer, lineWidth);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdSetDepthBias(
        VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdSetDepthBias called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdSetDepthBias = (PFN_vkCmdSetDepthBias)func(it_device->second->dispatch_handle, "vkCmdSetDepthBias");
        if (vkCmdSetDepthBias) vkCmdSetDepthBias(commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdSetBlendConstants(
        VkCommandBuffer commandBuffer, const float blendConstants[4])
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdSetBlendConstants called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdSetBlendConstants = (PFN_vkCmdSetBlendConstants)func(it_device->second->dispatch_handle, "vkCmdSetBlendConstants");
        if (vkCmdSetBlendConstants) vkCmdSetBlendConstants(commandBuffer, blendConstants);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdSetDepthBounds(
        VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdSetDepthBounds called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdSetDepthBounds = (PFN_vkCmdSetDepthBounds)func(it_device->second->dispatch_handle, "vkCmdSetDepthBounds");
        if (vkCmdSetDepthBounds) vkCmdSetDepthBounds(commandBuffer, minDepthBounds, maxDepthBounds);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdSetStencilCompareMask(
        VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdSetStencilCompareMask called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdSetStencilCompareMask = (PFN_vkCmdSetStencilCompareMask)func(it_device->second->dispatch_handle, "vkCmdSetStencilCompareMask");
        if (vkCmdSetStencilCompareMask) vkCmdSetStencilCompareMask(commandBuffer, faceMask, compareMask);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdSetStencilWriteMask(
        VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdSetStencilWriteMask called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdSetStencilWriteMask = (PFN_vkCmdSetStencilWriteMask)func(it_device->second->dispatch_handle, "vkCmdSetStencilWriteMask");
        if (vkCmdSetStencilWriteMask) vkCmdSetStencilWriteMask(commandBuffer, faceMask, writeMask);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdSetStencilReference(
        VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdSetStencilReference called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdSetStencilReference = (PFN_vkCmdSetStencilReference)func(it_device->second->dispatch_handle, "vkCmdSetStencilReference");
        if (vkCmdSetStencilReference) vkCmdSetStencilReference(commandBuffer, faceMask, reference);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdBindDescriptorSets(
        VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet,
        uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount,
        const uint32_t* pDynamicOffsets)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdBindDescriptorSets called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)func(it_device->second->dispatch_handle, "vkCmdBindDescriptorSets");
        if (vkCmdBindDescriptorSets) vkCmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
    }
}


void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdBindIndexBuffer(
        VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdBindIndexBuffer called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto it_buffer = it_device->second->buffers.find(buffer);

        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)func(it_device->second->dispatch_handle, "vkCmdBindIndexBuffer");
        if (vkCmdBindIndexBuffer) vkCmdBindIndexBuffer(commandBuffer, it_buffer->second->dispatch_handle, offset, indexType);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdBindVertexBuffers(
        VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount,
        const VkBuffer* pBuffers, const VkDeviceSize* pOffsets)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdBindVertexBuffers called >>");


    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    std::vector<VkBuffer> real_buffers_handle;


    for (int i = 0; i < bindingCount; i++) {
        const auto& buff = pBuffers[i];

        auto it_buffer = it_device->second->buffers.find(buff);

        real_buffers_handle.emplace_back(it_buffer->second->dispatch_handle);
    }

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)func(it_device->second->dispatch_handle, "vkCmdBindVertexBuffers");
        if (vkCmdBindVertexBuffers) vkCmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, real_buffers_handle.data(), pOffsets);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdDraw(
        VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount,
        uint32_t firstVertex, uint32_t firstInstance)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdDraw called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdDraw = (PFN_vkCmdDraw)func(it_device->second->dispatch_handle, "vkCmdDraw");
        if (vkCmdDraw) vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdDrawIndexed(
        VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount,
        uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdDrawIndexed called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)func(it_device->second->dispatch_handle, "vkCmdDrawIndexed");
        if (vkCmdDrawIndexed) vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdDrawIndirect(
        VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdDrawIndirect called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    auto it_buffer = it_device->second->buffers.find(buffer);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdDrawIndirect = (PFN_vkCmdDrawIndirect)func(it_device->second->dispatch_handle, "vkCmdDrawIndirect");
        if (vkCmdDrawIndirect) vkCmdDrawIndirect(commandBuffer, it_buffer->second->dispatch_handle, offset, drawCount, stride);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdDrawIndexedIndirect(
        VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdDrawIndexedIndirect called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    auto it_buffer = it_device->second->buffers.find(buffer);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdDrawIndexedIndirect = (PFN_vkCmdDrawIndexedIndirect)func(it_device->second->dispatch_handle, "vkCmdDrawIndexedIndirect");
        if (vkCmdDrawIndexedIndirect) vkCmdDrawIndexedIndirect(commandBuffer, it_buffer->second->dispatch_handle, offset, drawCount, stride);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdDispatch(
        VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdDispatch called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdDispatch = (PFN_vkCmdDispatch)func(it_device->second->dispatch_handle, "vkCmdDispatch");
        if (vkCmdDispatch) vkCmdDispatch(commandBuffer, groupCountX, groupCountY, groupCountZ);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdDispatchIndirect(
        VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdDispatchIndirect called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto it_buffer = it_device->second->buffers.find(buffer);

        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdDispatchIndirect = (PFN_vkCmdDispatchIndirect)func(it_device->second->dispatch_handle, "vkCmdDispatchIndirect");
        if (vkCmdDispatchIndirect) vkCmdDispatchIndirect(commandBuffer, it_buffer->second->dispatch_handle, offset);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdCopyBuffer(
        VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer,
        uint32_t regionCount, const VkBufferCopy* pRegions)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdCopyBuffer called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto it_buffer_dst = it_device->second->buffers.find(dstBuffer);
        auto it_buffer_src = it_device->second->buffers.find(srcBuffer);

        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdCopyBuffer = (PFN_vkCmdCopyBuffer)func(it_device->second->dispatch_handle, "vkCmdCopyBuffer");
        if (vkCmdCopyBuffer) vkCmdCopyBuffer(commandBuffer, it_buffer_src->second->dispatch_handle, it_buffer_dst->second->dispatch_handle, regionCount, pRegions);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdCopyImage(
        VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
        VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount,
        const VkImageCopy* pRegions)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdCopyImage called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {

        auto it_src_image = it_device->second->chain_images.find(srcImage);
        auto it_dst_image = it_device->second->chain_images.find(dstImage);

        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdCopyImage = (PFN_vkCmdCopyImage)func(it_device->second->dispatch_handle, "vkCmdCopyImage");
        if (vkCmdCopyImage) vkCmdCopyImage(commandBuffer, it_src_image->second->dispatch_handle, srcImageLayout, it_dst_image->second->dispatch_handle, dstImageLayout, regionCount, pRegions);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdBlitImage(
        VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
        VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount,
        const VkImageBlit* pRegions, VkFilter filter)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdBlitImage called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {

        auto it_src_image = it_device->second->chain_images.find(srcImage);
        auto it_dst_image = it_device->second->chain_images.find(dstImage);

        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdBlitImage = (PFN_vkCmdBlitImage)func(it_device->second->dispatch_handle, "vkCmdBlitImage");
        if (vkCmdBlitImage) vkCmdBlitImage(commandBuffer, it_src_image->second->dispatch_handle, srcImageLayout, it_dst_image->second->dispatch_handle, dstImageLayout, regionCount, pRegions, filter);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdCopyBufferToImage(
        VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage,
        VkImageLayout dstImageLayout, uint32_t regionCount,
        const VkBufferImageCopy* pRegions)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdCopyBufferToImage called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {

        auto it_src_buffer = it_device->second->buffers.find(srcBuffer);
        auto it_dst_image = it_device->second->chain_images.find(dstImage);


        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdCopyBufferToImage = (PFN_vkCmdCopyBufferToImage)func(it_device->second->dispatch_handle, "vkCmdCopyBufferToImage");
        if (vkCmdCopyBufferToImage) vkCmdCopyBufferToImage(commandBuffer, it_src_buffer->second->dispatch_handle, it_dst_image->second->dispatch_handle, dstImageLayout, regionCount, pRegions);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdCopyImageToBuffer(
        VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
        VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdCopyImageToBuffer called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {

        auto it_dst_buffer = it_device->second->buffers.find(dstBuffer);
        auto it_src_image = it_device->second->chain_images.find(srcImage);

        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdCopyImageToBuffer = (PFN_vkCmdCopyImageToBuffer)func(it_device->second->dispatch_handle, "vkCmdCopyImageToBuffer");
        if (vkCmdCopyImageToBuffer) vkCmdCopyImageToBuffer(commandBuffer, it_src_image->second->dispatch_handle, srcImageLayout, it_dst_buffer->second->dispatch_handle, regionCount, pRegions);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdUpdateBuffer(
        VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset,
        VkDeviceSize dataSize, const void* pData)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdUpdateBuffer called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {

        auto it_dst_buffer = it_device->second->buffers.find(dstBuffer);

        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdUpdateBuffer = (PFN_vkCmdUpdateBuffer)func(it_device->second->dispatch_handle, "vkCmdUpdateBuffer");
        if (vkCmdUpdateBuffer) vkCmdUpdateBuffer(commandBuffer, it_dst_buffer->second->dispatch_handle, dstOffset, dataSize, pData);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdFillBuffer(
        VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset,
        VkDeviceSize size, uint32_t data)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdFillBuffer called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {

        auto it_dst_buffer = it_device->second->buffers.find(dstBuffer);

        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdFillBuffer = (PFN_vkCmdFillBuffer)func(it_device->second->dispatch_handle, "vkCmdFillBuffer");
        if (vkCmdFillBuffer) vkCmdFillBuffer(commandBuffer, it_dst_buffer->second->dispatch_handle, dstOffset, size, data);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdClearColorImage(
        VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
        const VkClearColorValue* pColor, uint32_t rangeCount,
        const VkImageSubresourceRange* pRanges)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdClearColorImage called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {

        auto it_image = it_device->second->chain_images.find(image);

        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdClearColorImage = (PFN_vkCmdClearColorImage)func(it_device->second->dispatch_handle, "vkCmdClearColorImage");
        if (vkCmdClearColorImage) vkCmdClearColorImage(commandBuffer, it_image->second->dispatch_handle, imageLayout, pColor, rangeCount, pRanges);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdClearDepthStencilImage(
        VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
        const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount,
        const VkImageSubresourceRange* pRanges)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdClearDepthStencilImage called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {

        auto it_image = it_device->second->chain_images.find(image);

        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdClearDepthStencilImage = (PFN_vkCmdClearDepthStencilImage)func(it_device->second->dispatch_handle, "vkCmdClearDepthStencilImage");
        if (vkCmdClearDepthStencilImage) vkCmdClearDepthStencilImage(commandBuffer, it_image->second->dispatch_handle, imageLayout, pDepthStencil, rangeCount, pRanges);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdClearAttachments(
        VkCommandBuffer commandBuffer, uint32_t attachmentCount,
        const VkClearAttachment* pAttachments, uint32_t rectCount,
        const VkClearRect* pRects)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdClearAttachments called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdClearAttachments = (PFN_vkCmdClearAttachments)func(it_device->second->dispatch_handle, "vkCmdClearAttachments");
        if (vkCmdClearAttachments) vkCmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdResolveImage(
        VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
        VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount,
        const VkImageResolve* pRegions)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdResolveImage called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {

        auto it_src_image = it_device->second->chain_images.find(srcImage);
        auto it_dst_image = it_device->second->chain_images.find(dstImage);

        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdResolveImage = (PFN_vkCmdResolveImage)func(it_device->second->dispatch_handle, "vkCmdResolveImage");
        if (vkCmdResolveImage) vkCmdResolveImage(commandBuffer, it_src_image->second->dispatch_handle, srcImageLayout, it_dst_image->second->dispatch_handle, dstImageLayout, regionCount, pRegions);
    }
}


// todo :
void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdSetEvent(
        VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdSetEvent called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdSetEvent = (PFN_vkCmdSetEvent)func(it_device->second->dispatch_handle, "vkCmdSetEvent");
        if (vkCmdSetEvent) vkCmdSetEvent(commandBuffer, event, stageMask);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdResetEvent(
        VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdResetEvent called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdResetEvent = (PFN_vkCmdResetEvent)func(it_device->second->dispatch_handle, "vkCmdResetEvent");
        if (vkCmdResetEvent) vkCmdResetEvent(commandBuffer, event, stageMask);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdWaitEvents(
        VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent* pEvents,
        VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
        uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
        uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers,
        uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdWaitEvents called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdWaitEvents = (PFN_vkCmdWaitEvents)func(it_device->second->dispatch_handle, "vkCmdWaitEvents");
        if (vkCmdWaitEvents) vkCmdWaitEvents(commandBuffer, eventCount, pEvents,
                                             srcStageMask, dstStageMask,
                                             memoryBarrierCount, pMemoryBarriers,
                                             bufferMemoryBarrierCount, pBufferMemoryBarriers,
                                             imageMemoryBarrierCount, pImageMemoryBarriers);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdPipelineBarrier(
        VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask,
        VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
        uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers,
        uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier* pBufferMemoryBarriers,
        uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier* pImageMemoryBarriers)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdPipelineBarrier called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)func(it_device->second->dispatch_handle, "vkCmdPipelineBarrier");
        if (vkCmdPipelineBarrier) vkCmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags,
                                                       memoryBarrierCount, pMemoryBarriers,
                                                       bufferMemoryBarrierCount, pBufferMemoryBarriers,
                                                       imageMemoryBarrierCount, pImageMemoryBarriers);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdBeginQuery(
        VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdBeginQuery called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdBeginQuery = (PFN_vkCmdBeginQuery)func(it_device->second->dispatch_handle, "vkCmdBeginQuery");
        if (vkCmdBeginQuery) vkCmdBeginQuery(commandBuffer, queryPool, query, flags);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdEndQuery(
        VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdEndQuery called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdEndQuery = (PFN_vkCmdEndQuery)func(it_device->second->dispatch_handle, "vkCmdEndQuery");
        if (vkCmdEndQuery) vkCmdEndQuery(commandBuffer, queryPool, query);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdResetQueryPool(
        VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdResetQueryPool called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdResetQueryPool = (PFN_vkCmdResetQueryPool)func(it_device->second->dispatch_handle, "vkCmdResetQueryPool");
        if (vkCmdResetQueryPool) vkCmdResetQueryPool(commandBuffer, queryPool, firstQuery, queryCount);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdWriteTimestamp(
        VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage,
        VkQueryPool queryPool, uint32_t query)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdWriteTimestamp called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdWriteTimestamp = (PFN_vkCmdWriteTimestamp)func(it_device->second->dispatch_handle, "vkCmdWriteTimestamp");
        if (vkCmdWriteTimestamp) vkCmdWriteTimestamp(commandBuffer, pipelineStage, queryPool, query);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdCopyQueryPoolResults(
        VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery,
        uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset,
        VkDeviceSize stride, VkQueryResultFlags flags)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdCopyQueryPoolResults called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdCopyQueryPoolResults = (PFN_vkCmdCopyQueryPoolResults)func(it_device->second->dispatch_handle, "vkCmdCopyQueryPoolResults");
        if (vkCmdCopyQueryPoolResults) vkCmdCopyQueryPoolResults(commandBuffer, queryPool, firstQuery, queryCount, dstBuffer, dstOffset, stride, flags);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdPushConstants(
        VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags,
        uint32_t offset, uint32_t size, const void* pValues)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdPushConstants called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdPushConstants = (PFN_vkCmdPushConstants)func(it_device->second->dispatch_handle, "vkCmdPushConstants");
        if (vkCmdPushConstants) vkCmdPushConstants(commandBuffer, layout, stageFlags, offset, size, pValues);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdBeginRenderPass(
        VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pRenderPassBegin,
        VkSubpassContents contents)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdBeginRenderPass called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)func(it_device->second->dispatch_handle, "vkCmdBeginRenderPass");
        if (vkCmdBeginRenderPass) vkCmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdNextSubpass(
        VkCommandBuffer commandBuffer, VkSubpassContents contents)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdNextSubpass called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdNextSubpass = (PFN_vkCmdNextSubpass)func(it_device->second->dispatch_handle, "vkCmdNextSubpass");
        if (vkCmdNextSubpass) vkCmdNextSubpass(commandBuffer, contents);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdEndRenderPass(
        VkCommandBuffer commandBuffer)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdEndRenderPass called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)func(it_device->second->dispatch_handle, "vkCmdEndRenderPass");
        if (vkCmdEndRenderPass) vkCmdEndRenderPass(commandBuffer);
    }
}

void VulkanDispatcher::Device::DeviceDispatchTable::vlk_trampoline_call_vkCmdExecuteCommands(
        VkCommandBuffer commandBuffer, uint32_t commandBufferCount,
        const VkCommandBuffer* pCommandBuffers)
{
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "vlk_trampoline_call_vkCmdExecuteCommands called >>");
    auto it_device = vk_context->devices.find(VulkanContext::VkDeviceObject::device_magic);
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it_device != vk_context->devices.end() && it_instance != vk_context->instances.end()) {
        auto func = (PFN_vkGetDeviceProcAddr)vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetDeviceProcAddr");
        auto vkCmdExecuteCommands = (PFN_vkCmdExecuteCommands)func(it_device->second->dispatch_handle, "vkCmdExecuteCommands");
        if (vkCmdExecuteCommands) vkCmdExecuteCommands(commandBuffer, commandBufferCount, pCommandBuffers);
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
    state = registerTrampoline("vkDeviceWaitIdle", vlk_trampoline_call_DeviceWaitIdle);
    state = registerTrampoline("vkQueueSubmit", vlk_trampoline_call_QueueSubmit);
    state = registerTrampoline("vkQueueWaitIdle", vlk_trampoline_call_QueueWaitIdle);

    /**
     * memory functions
     */
    state = registerTrampoline("vkAllocateMemory", vlk_trampoline_call_AllocateMemory);
    state = registerTrampoline("vkFreeMemory", vlk_trampoline_call_FreeMemory);
    state = registerTrampoline("vkMapMemory", vlk_trampoline_call_MapMemory);
    state = registerTrampoline("vkUnmapMemory", vlk_trampoline_call_UnmapMemory);
    state = registerTrampoline("vkFlushMappedMemoryRanges", vlk_trampoline_call_FlushMappedMemoryRanges);
    state = registerTrampoline("vkInvalidateMappedMemoryRanges", vlk_trampoline_call_InvalidateMappedMemoryRanges);
    state = registerTrampoline("vkGetDeviceMemoryCommitment", vlk_trampoline_call_GetDeviceMemoryCommitment);

    /**
     * buffer functions
     */
    state = registerTrampoline("vkCreateBuffer", vlk_trampoline_call_CreateBuffer);
    state = registerTrampoline("vkDestroyBuffer", vlk_trampoline_call_DestroyBuffer);
    state = registerTrampoline("vkBindBufferMemory", vlk_trampoline_call_BindBufferMemory);
    state = registerTrampoline("vkGetBufferMemoryRequirements", vlk_trampoline_call_GetBufferMemoryRequirements);
    state = registerTrampoline("vkCreateBufferView", vlk_trampoline_call_CreateBufferView);
    state = registerTrampoline("vkDestroyBufferView", vlk_trampoline_call_DestroyBufferView);

    /**
     * image functions
     */
    state = registerTrampoline("vkCreateImage", vlk_trampoline_call_CreateImage);
    state = registerTrampoline("vkDestroyImage", vlk_trampoline_call_DestroyImage);
    state = registerTrampoline("vkBindImageMemory", vlk_trampoline_call_BindImageMemory);
    state = registerTrampoline("vkGetImageMemoryRequirements", vlk_trampoline_call_GetImageMemoryRequirements);
    state = registerTrampoline("vkGetImageSparseMemoryRequirements", vlk_trampoline_call_GetImageSparseMemoryRequirements);
    state = registerTrampoline("vkGetImageSubresourceLayout", vlk_trampoline_call_GetImageSubresourceLayout);
    state = registerTrampoline("vkCreateImageView", vlk_trampoline_call_CreateImageView);
    state = registerTrampoline("vkDestroyImageView", vlk_trampoline_call_DestroyImageView);

    /**
     * queue / sparse functions
     */
    state = registerTrampoline("vkQueueBindSparse", vlk_trampoline_call_QueueBindSparse);

    /**
     * sync objects (fence, semaphore, event)
     */
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

    /**
     * query pool
     */
    state = registerTrampoline("vkCreateQueryPool", vlk_trampoline_call_CreateQueryPool);
    state = registerTrampoline("vkDestroyQueryPool", vlk_trampoline_call_DestroyQueryPool);
    state = registerTrampoline("vkGetQueryPoolResults", vlk_trampoline_call_GetQueryPoolResults);

    /**
     * shader functions
     */
    state = registerTrampoline("vkCreateShaderModule", vlk_trampoline_call_CreateShaderModule);
    state = registerTrampoline("vkDestroyShaderModule", vlk_trampoline_call_DestroyShaderModule);

    /**
     * pipeline functions
     */
    state = registerTrampoline("vkCreatePipelineCache", vlk_trampoline_call_CreatePipelineCache);
    state = registerTrampoline("vkDestroyPipelineCache", vlk_trampoline_call_DestroyPipelineCache);
    state = registerTrampoline("vkGetPipelineCacheData", vlk_trampoline_call_GetPipelineCacheData);
    state = registerTrampoline("vkMergePipelineCaches", vlk_trampoline_call_MergePipelineCaches);
    state = registerTrampoline("vkCreateGraphicsPipelines", vlk_trampoline_call_CreateGraphicsPipelines);
    state = registerTrampoline("vkCreateComputePipelines", vlk_trampoline_call_CreateComputePipelines);
    state = registerTrampoline("vkDestroyPipeline", vlk_trampoline_call_DestroyPipeline);
    state = registerTrampoline("vkCreatePipelineLayout", vlk_trampoline_call_CreatePipelineLayout);
    state = registerTrampoline("vkDestroyPipelineLayout", vlk_trampoline_call_DestroyPipelineLayout);

    /**
     * sampler
     */
    state = registerTrampoline("vkCreateSampler", vlk_trampoline_call_CreateSampler);
    state = registerTrampoline("vkDestroySampler", vlk_trampoline_call_DestroySampler);

    /**
     * descriptor set/layout functions
     */
    state = registerTrampoline("vkCreateDescriptorSetLayout", vlk_trampoline_call_CreateDescriptorSetLayout);
    state = registerTrampoline("vkDestroyDescriptorSetLayout", vlk_trampoline_call_DestroyDescriptorSetLayout);
    state = registerTrampoline("vkCreateDescriptorPool", vlk_trampoline_call_CreateDescriptorPool);
    state = registerTrampoline("vkDestroyDescriptorPool", vlk_trampoline_call_DestroyDescriptorPool);
    state = registerTrampoline("vkResetDescriptorPool", vlk_trampoline_call_ResetDescriptorPool);
    state = registerTrampoline("vkAllocateDescriptorSets", vlk_trampoline_call_AllocateDescriptorSets);
    state = registerTrampoline("vkFreeDescriptorSets", vlk_trampoline_call_FreeDescriptorSets);
    state = registerTrampoline("vkUpdateDescriptorSets", vlk_trampoline_call_UpdateDescriptorSets);

    /**
     * framebuffer / renderpass
     */
    state = registerTrampoline("vkCreateFramebuffer", vlk_trampoline_call_CreateFramebuffer);
    state = registerTrampoline("vkDestroyFramebuffer", vlk_trampoline_call_DestroyFramebuffer);
    state = registerTrampoline("vkCreateRenderPass", vlk_trampoline_call_CreateRenderPass);
    state = registerTrampoline("vkDestroyRenderPass", vlk_trampoline_call_DestroyRenderPass);
    state = registerTrampoline("vkGetRenderAreaGranularity", vlk_trampoline_call_GetRenderAreaGranularity);

    /**
     * command pool
     */
    state = registerTrampoline("vkCreateCommandPool", vlk_trampoline_call_CreateCommandPool);
    state = registerTrampoline("vkDestroyCommandPool", vlk_trampoline_call_DestroyCommandPool);
    state = registerTrampoline("vkResetCommandPool", vlk_trampoline_call_ResetCommandPool);

    /**
     * command buffers
     */
    state = registerTrampoline("vkAllocateCommandBuffers", vlk_trampoline_call_AllocateCommandBuffers);
    state = registerTrampoline("vkFreeCommandBuffers", vlk_trampoline_call_FreeCommandBuffers);
    state = registerTrampoline("vkBeginCommandBuffer", vlk_trampoline_call_BeginCommandBuffer);
    state = registerTrampoline("vkEndCommandBuffer", vlk_trampoline_call_EndCommandBuffer);
    state = registerTrampoline("vkResetCommandBuffer", vlk_trampoline_call_ResetCommandBuffer);

    /**
     * command buffer recording functions
     */
    state = registerTrampoline("vkCmdBindPipeline", vlk_trampoline_call_vkCmdBindPipeline);
    state = registerTrampoline("vkCmdSetViewport", vlk_trampoline_call_vkCmdSetViewport);
    state = registerTrampoline("vkCmdSetScissor", vlk_trampoline_call_vkCmdSetScissor);
    state = registerTrampoline("vkCmdSetLineWidth", vlk_trampoline_call_vkCmdSetLineWidth);
    state = registerTrampoline("vkCmdSetDepthBias", vlk_trampoline_call_vkCmdSetDepthBias);
    state = registerTrampoline("vkCmdSetBlendConstants", vlk_trampoline_call_vkCmdSetBlendConstants);
    state = registerTrampoline("vkCmdSetDepthBounds", vlk_trampoline_call_vkCmdSetDepthBounds);
    state = registerTrampoline("vkCmdSetStencilCompareMask", vlk_trampoline_call_vkCmdSetStencilCompareMask);
    state = registerTrampoline("vkCmdSetStencilWriteMask", vlk_trampoline_call_vkCmdSetStencilWriteMask);
    state = registerTrampoline("vkCmdSetStencilReference", vlk_trampoline_call_vkCmdSetStencilReference);
    state = registerTrampoline("vkCmdBindDescriptorSets", vlk_trampoline_call_vkCmdBindDescriptorSets);
    state = registerTrampoline("vkCmdBindIndexBuffer", vlk_trampoline_call_vkCmdBindIndexBuffer);
    state = registerTrampoline("vkCmdBindVertexBuffers", vlk_trampoline_call_vkCmdBindVertexBuffers);
    state = registerTrampoline("vkCmdDraw", vlk_trampoline_call_vkCmdDraw);
    state = registerTrampoline("vkCmdDrawIndexed", vlk_trampoline_call_vkCmdDrawIndexed);
    state = registerTrampoline("vkCmdDrawIndirect", vlk_trampoline_call_vkCmdDrawIndirect);
    state = registerTrampoline("vkCmdDrawIndexedIndirect", vlk_trampoline_call_vkCmdDrawIndexedIndirect);
    state = registerTrampoline("vkCmdDispatch", vlk_trampoline_call_vkCmdDispatch);
    state = registerTrampoline("vkCmdDispatchIndirect", vlk_trampoline_call_vkCmdDispatchIndirect);
    state = registerTrampoline("vkCmdCopyBuffer", vlk_trampoline_call_vkCmdCopyBuffer);
    state = registerTrampoline("vkCmdCopyImage", vlk_trampoline_call_vkCmdCopyImage);
    state = registerTrampoline("vkCmdBlitImage", vlk_trampoline_call_vkCmdBlitImage);
    state = registerTrampoline("vkCmdCopyBufferToImage", vlk_trampoline_call_vkCmdCopyBufferToImage);
    state = registerTrampoline("vkCmdCopyImageToBuffer", vlk_trampoline_call_vkCmdCopyImageToBuffer);
    state = registerTrampoline("vkCmdUpdateBuffer", vlk_trampoline_call_vkCmdUpdateBuffer);
    state = registerTrampoline("vkCmdFillBuffer", vlk_trampoline_call_vkCmdFillBuffer);
    state = registerTrampoline("vkCmdClearColorImage", vlk_trampoline_call_vkCmdClearColorImage);
    state = registerTrampoline("vkCmdClearDepthStencilImage", vlk_trampoline_call_vkCmdClearDepthStencilImage);
    state = registerTrampoline("vkCmdClearAttachments", vlk_trampoline_call_vkCmdClearAttachments);
    state = registerTrampoline("vkCmdResolveImage", vlk_trampoline_call_vkCmdResolveImage);
    state = registerTrampoline("vkCmdSetEvent", vlk_trampoline_call_vkCmdSetEvent);
    state = registerTrampoline("vkCmdResetEvent", vlk_trampoline_call_vkCmdResetEvent);
    state = registerTrampoline("vkCmdWaitEvents", vlk_trampoline_call_vkCmdWaitEvents);
    state = registerTrampoline("vkCmdPipelineBarrier", vlk_trampoline_call_vkCmdPipelineBarrier);
    state = registerTrampoline("vkCmdBeginQuery", vlk_trampoline_call_vkCmdBeginQuery);
    state = registerTrampoline("vkCmdEndQuery", vlk_trampoline_call_vkCmdEndQuery);
    state = registerTrampoline("vkCmdResetQueryPool", vlk_trampoline_call_vkCmdResetQueryPool);
    state = registerTrampoline("vkCmdWriteTimestamp", vlk_trampoline_call_vkCmdWriteTimestamp);
    state = registerTrampoline("vkCmdCopyQueryPoolResults", vlk_trampoline_call_vkCmdCopyQueryPoolResults);
    state = registerTrampoline("vkCmdPushConstants", vlk_trampoline_call_vkCmdPushConstants);
    state = registerTrampoline("vkCmdBeginRenderPass", vlk_trampoline_call_vkCmdBeginRenderPass);
    state = registerTrampoline("vkCmdNextSubpass", vlk_trampoline_call_vkCmdNextSubpass);
    state = registerTrampoline("vkCmdEndRenderPass", vlk_trampoline_call_vkCmdEndRenderPass);
    state = registerTrampoline("vkCmdExecuteCommands", vlk_trampoline_call_vkCmdExecuteCommands);


    /**
     * core
     */
    state = registerTrampoline("vkGetDeviceProcAddr", vlk_trampoline_call_GetDeviceProcAddr);


    return state;
}
