#include "physical_device.h"

#include "context.h"

VulkanContext::Context *VulkanDispatcher::PhysicalDevice::PhysicalDeviceDispatchTable::vk_context = nullptr;
VulkanDispatcher::PhysicalDevice::DispatchMap_T<std::string, PFN_vkVoidFunction> VulkanDispatcher::PhysicalDevice::PhysicalDeviceDispatchTable::physical_device_dispatch_table;

VulkanDispatcher::PhysicalDevice::PhysicalDeviceDispatchTable::PhysicalDeviceDispatchTable(
        VulkanContext::Context *context) {
    if (context) {
        vk_context = context;
    } else {
        Log::VLK_LOG(Log::Level::ERROR, Log::LevelType::INSTANCE_DISPATCH_TABLE,
                     "received a invalid context vulkan");
    }
}


/**
 * physical device functions
 */

VkResult
VulkanDispatcher::PhysicalDevice::PhysicalDeviceDispatchTable::vlk_trampoline_call_EnumeratePhysicalDevices(
        VkInstance instance, uint32_t *pPhysicalDeviceCount, VkPhysicalDevice *pPhysicalDevices) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_EnumeratePhysicalDevices called >>");

    /**
     * get instance from handle
     */
    auto it = vk_context->instances.find(reinterpret_cast<uint64_t>(instance));

    if (it == vk_context->instances.end()) {
        Log::VLK_LOG(Log::Level::INFO, Log::LevelType::INSTANCE,
                     "cannot get instance from magic handle");

        throw std::runtime_error("Cannot get VkInstance obj from magic handle");
    }

    const auto &func = (PFN_vkEnumeratePhysicalDevices) vk_context->GetInstanceProcAddr(
            it->second->dispatch_handle,
            "vkEnumeratePhysicalDevices");

    uint32_t deviceCount = 0;
    func(it->second->dispatch_handle, &deviceCount, nullptr);


    if (deviceCount < 1) {
        *pPhysicalDeviceCount = 0;
        return VK_INCOMPLETE;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    func(it->second->dispatch_handle, &deviceCount, devices.data());


    if (pPhysicalDevices) {
        *pPhysicalDeviceCount = deviceCount;

        for (uint32_t i = 0; i < deviceCount; ++i) {
            pPhysicalDevices[i] = devices[i];

            auto pdev = std::make_unique<VulkanContext::VkPhysicalDeviceObject>();
            pdev->dispatch_handle = devices[i];

            vk_context->physicalDevices[reinterpret_cast<uint64_t>(devices[i])] = std::move(
                    pdev);
        }
    } else {
        *pPhysicalDeviceCount = deviceCount;
    }

    return VK_SUCCESS;
}

void
VulkanDispatcher::PhysicalDevice::PhysicalDeviceDispatchTable::vlk_trampoline_call_GetPhysicalDeviceFeatures(
        VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures *pFeatures) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_GetPhysicalDeviceFeatures called >>");

    VkPhysicalDeviceFeatures real_features;

    /**
     * in this case get pdev from magic handle
     */
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    auto key = reinterpret_cast<uintptr_t>(physicalDevice);
    auto it_pdev = vk_context->physicalDevices.find(key);
    if (it_pdev == vk_context->physicalDevices.end()) {
        throw std::runtime_error("PhysicalDevice not found");
    }

    if (it_instance != vk_context->instances.end()) {
        const auto &func = (PFN_vkGetPhysicalDeviceFeatures) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle,
                "vkGetPhysicalDeviceFeatures");

        if (func) {
            func(it_pdev->second->dispatch_handle, &real_features);
        }

        /**
         * remove support of sparse things for now
         */
        real_features.sparseBinding = VK_FALSE;
        real_features.sparseResidencyImage2D = VK_FALSE;

        memcpy(pFeatures, &real_features, sizeof(VkPhysicalDeviceFeatures));

    }
}

void
VulkanDispatcher::PhysicalDevice::PhysicalDeviceDispatchTable::vlk_trampoline_call_GetPhysicalDeviceFormatProperties(
        VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties *pFormatProperties) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_GetPhysicalDeviceFormatProperties called pdev magic = >>",
                 reinterpret_cast<uint64_t>(physicalDevice));

    /**
     * instance
     */
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    auto it_pdev = vk_context->physicalDevices.find(reinterpret_cast<uint64_t>(physicalDevice));
    if (it_pdev == vk_context->physicalDevices.end()) {
        throw std::runtime_error("PhysicalDevice not found");
    }

    if (it_instance != vk_context->instances.end()) {
        const auto func = (PFN_vkGetPhysicalDeviceFormatProperties) vk_context->GetInstanceProcAddr(
                it_instance->second->dispatch_handle, "vkGetPhysicalDeviceFormatProperties");
        func(it_pdev->second->dispatch_handle, format, pFormatProperties);
    }
}

VkResult
VulkanDispatcher::PhysicalDevice::PhysicalDeviceDispatchTable::vlk_trampoline_call_GetPhysicalDeviceImageFormatProperties(
        VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling,
        VkImageUsageFlags usage, VkImageCreateFlags flags,
        VkImageFormatProperties *pImageFormatProperties) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_GetPhysicalDeviceImageFormatProperties called >>");
    auto it_instance = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    auto it_pdev = vk_context->physicalDevices.find(reinterpret_cast<uint64_t>(physicalDevice));
    if (it_pdev == vk_context->physicalDevices.end()) {
        throw std::runtime_error("PhysicalDevice not found");
    }
    if (it_instance == vk_context->instances.end()) {
        Log::VLK_LOG(Log::Level::INFO, Log::LevelType::INSTANCE,
                     "cannot get instance from magic handle");
    }

    // check for sparse reqs
    if (flags & (VK_IMAGE_CREATE_SPARSE_BINDING_BIT |
                 VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT |
                 VK_IMAGE_CREATE_SPARSE_ALIASED_BIT)) {
        Log::VLK_LOG(Log::Level::INFO, Log::LevelType::PHYSICAL_DEVICE, Log::LevelType::CONTEXT,
                     "Sparse image flags requested but not supported");
        return VK_ERROR_FORMAT_NOT_SUPPORTED;
    }

    const auto func = (PFN_vkGetPhysicalDeviceImageFormatProperties) vk_context->GetInstanceProcAddr(
            it_instance->second->dispatch_handle, "vkGetPhysicalDeviceImageFormatProperties");

    if (func)
        return func(it_pdev->second->dispatch_handle, format, type, tiling, usage, flags,
                    pImageFormatProperties);
}


void
VulkanDispatcher::PhysicalDevice::PhysicalDeviceDispatchTable::vlk_trampoline_call_GetPhysicalDeviceProperties(
        VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties *pProperties) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_GetPhysicalDeviceProperties called >>");


    VkPhysicalDeviceProperties real_device_props;

    /**
     * instance
     */
    const auto it = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it != vk_context->instances.end()) {
        const auto func = (PFN_vkGetPhysicalDeviceProperties) vk_context->GetInstanceProcAddr(
                it->second->dispatch_handle, "vkGetPhysicalDeviceProperties");

        func(physicalDevice, &real_device_props);

        /**
         * make the device name of the driver
         */
        strcpy(pProperties->deviceName,
               std::string("Vulkron(" + std::string(real_device_props.deviceName) + ")").c_str());
    }
}


void
VulkanDispatcher::PhysicalDevice::PhysicalDeviceDispatchTable::vlk_trampoline_call_GetPhysicalDeviceQueueFamilyProperties(
        VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount,
        VkQueueFamilyProperties *pQueueFamilyProperties) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_GetPhysicalDeviceQueueFamilyProperties called >>");

    /**
     * instance
     */
    const auto it = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it != vk_context->instances.end()) {
        const auto func = (PFN_vkGetPhysicalDeviceQueueFamilyProperties) vk_context->GetInstanceProcAddr(
                it->second->dispatch_handle, "vkGetPhysicalDeviceQueueFamilyProperties");

        func(physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
    }

}

void
VulkanDispatcher::PhysicalDevice::PhysicalDeviceDispatchTable::vlk_trampoline_call_GetPhysicalDeviceMemoryProperties(
        VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties *pMemoryProperties) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_GetPhysicalDeviceMemoryProperties called >>");
    /**
     * instance
     */
    const auto it = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it != vk_context->instances.end()) {
        const auto func = (PFN_vkGetPhysicalDeviceMemoryProperties) vk_context->GetInstanceProcAddr(
                it->second->dispatch_handle, "vkGetPhysicalDeviceMemoryProperties");

        func(physicalDevice, pMemoryProperties);
    }
}


VkResult
VulkanDispatcher::PhysicalDevice::PhysicalDeviceDispatchTable::vlk_trampoline_call_EnumerateDeviceExtensionProperties(
        VkPhysicalDevice physicalDevice, const char *pLayerName, uint32_t *pPropertyCount,
        VkExtensionProperties *pProperties) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_EnumerateDeviceExtensionProperties called >>");
    /**
     * instance
     */
    const auto it = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it != vk_context->instances.end()) {
        const auto func = (PFN_vkEnumerateDeviceExtensionProperties) vk_context->GetInstanceProcAddr(
                it->second->dispatch_handle, "vkEnumerateDeviceExtensionProperties");

        return func(physicalDevice, pLayerName, pPropertyCount, pProperties);
    }

    return VK_ERROR_INITIALIZATION_FAILED;
}

void
VulkanDispatcher::PhysicalDevice::PhysicalDeviceDispatchTable::vlk_trampoline_call_GetPhysicalDeviceSparseImageFormatProperties(
        VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type,
        VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling,
        uint32_t *pPropertyCount, VkSparseImageFormatProperties *pProperties) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_GetPhysicalDeviceSparseImageFormatProperties called >>");

    /**
    * instance
    */
    const auto it = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);

    if (it != vk_context->instances.end()) {
        const auto func = (PFN_vkGetPhysicalDeviceSparseImageFormatProperties) vk_context->GetInstanceProcAddr(
                it->second->dispatch_handle, "vkGetPhysicalDeviceSparseImageFormatProperties");

        func(physicalDevice, format, type, samples, usage, tiling, pPropertyCount, pProperties);
    }
}

VkBool32
VulkanDispatcher::PhysicalDevice::PhysicalDeviceDispatchTable::vlk_trampoline_call_GetPhysicalDeviceXcbPresentationSupportKHR(
        VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, xcb_connection_t *connection,
        xcb_visualid_t visual_id) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_GetPhysicalDeviceXcbPresentationSupportKHR called >>");

    // for now return support
    return VK_SUCCESS;
}


bool VulkanDispatcher::PhysicalDevice::PhysicalDeviceDispatchTable::factory() {
    auto state = false;

    /**
    * pdev functions
    */
    state = registerTrampoline("vkEnumeratePhysicalDevices",
                               vlk_trampoline_call_EnumeratePhysicalDevices);

    state = registerTrampoline("vkGetPhysicalDeviceFeatures",
                               vlk_trampoline_call_GetPhysicalDeviceFeatures);

    state = registerTrampoline("vkGetPhysicalDeviceFormatProperties",
                               vlk_trampoline_call_GetPhysicalDeviceFormatProperties);

    state = registerTrampoline("vkGetPhysicalDeviceImageFormatProperties",
                               vlk_trampoline_call_GetPhysicalDeviceImageFormatProperties);

    state = registerTrampoline("vkGetPhysicalDeviceProperties",
                               vlk_trampoline_call_GetPhysicalDeviceProperties);

    state = registerTrampoline("vkGetPhysicalDeviceQueueFamilyProperties",
                               vlk_trampoline_call_GetPhysicalDeviceQueueFamilyProperties);

    state = registerTrampoline("vkGetPhysicalDeviceMemoryProperties",
                               vlk_trampoline_call_GetPhysicalDeviceMemoryProperties);

    state = registerTrampoline("vkEnumerateDeviceExtensionProperties",
                               vlk_trampoline_call_EnumerateDeviceExtensionProperties);

    state = registerTrampoline("vkGetPhysicalDeviceSparseImageFormatProperties",
                               vlk_trampoline_call_GetPhysicalDeviceSparseImageFormatProperties);

    state = registerTrampoline("vkGetPhysicalDeviceXcbPresentationSupportKHR", vlk_trampoline_call_GetPhysicalDeviceXcbPresentationSupportKHR);

    return state;
}

