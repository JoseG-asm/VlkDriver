#include "context.h"
#include "instance.h"

VulkanContext::Context *VulkanDispatcher::Instance::InstanceDispatchTable::vk_context = nullptr;
VulkanDispatcher::Instance::DispatchMap_T<std::string, PFN_vkVoidFunction> VulkanDispatcher::Instance::InstanceDispatchTable::instance_disp_table;

VulkanDispatcher::Instance::InstanceDispatchTable::InstanceDispatchTable(
        VulkanContext::Context *context) {
    if (context) {
        vk_context = context;
    } else {
        Log::VLK_LOG(Log::Level::ERROR, Log::LevelType::INSTANCE_DISPATCH_TABLE,
                     "received a invalid context vulkan");
    }
}

VkResult VulkanDispatcher::Instance::InstanceDispatchTable::vlk_trampoline_call_CreateInstance(
        const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
        VkInstance *pInstance) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_CreateInstance called >>");

    VkInstance obj;

    /**
     * if you use modified info
     */
    VkInstanceCreateInfo mod_info = *pCreateInfo;

    std::vector<const char*> filteredExtensions;

    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        const char* ext = pCreateInfo->ppEnabledExtensionNames[i];

        if (strcmp(ext, "VK_KHR_xcb_surface") != 0) {
            filteredExtensions.push_back(ext);
        }
    }

    mod_info.enabledExtensionCount = static_cast<uint32_t>(filteredExtensions.size());
    mod_info.ppEnabledExtensionNames = filteredExtensions.data();

    VkResult result = vk_context->CreateInstance(&mod_info, pAllocator, &obj);

    if (result != VK_SUCCESS) {
        Log::VLK_LOG(Log::Level::ERROR, Log::LevelType::CONTEXT,
                     "Failed to create VkInstance with code %d", result);
        return result;
    }

    /**
     * Create a driver object responsible by instance of driver
     */
    auto instance = std::make_unique<VulkanContext::VkInstanceObject>();
    instance->dispatch_handle = obj;
    *pInstance = instance->dispatch_handle;

    /**
     * define magic value ->
     */
    VulkanContext::VkInstanceObject::instance_magic = reinterpret_cast<uint64_t>(instance->dispatch_handle);

    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::INSTANCE, "magic value = ",
                 VulkanContext::VkInstanceObject::instance_magic);

    vk_context->instances[VulkanContext::VkInstanceObject::instance_magic] = std::move(instance);


    return VK_SUCCESS;
}

void
VulkanDispatcher::Instance::InstanceDispatchTable::vlk_trampoline_call_DestroyInstance(
        VkInstance instance,
        const VkAllocationCallbacks *pAllocator) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_DestroyInstance called >>");

    auto it = vk_context->instances.find(VulkanContext::VkInstanceObject::instance_magic);
    if (it == vk_context->instances.end()) {
        Log::VLK_LOG(Log::Level::ERROR, Log::LevelType::CONTEXT,
                     "Invalid instance passed to vlk_trampoline_call_DestroyInstance");
        return;
    }

    auto &instanceObj = it->second;

    const auto &internal_destroyInstance = (PFN_vkDestroyInstance) vk_context->GetInstanceProcAddr(
            instanceObj->dispatch_handle, "vkDestroyInstance");

    if (internal_destroyInstance) {
        internal_destroyInstance(instance, pAllocator);
    }

    vk_context->instances.erase(it);

    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "Instance destroyed and removed from context");
}

VkResult
VulkanDispatcher::Instance::InstanceDispatchTable::vlk_trampoline_call_EnumerateInstanceVersion(
        uint32_t *pApiVersion) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_EnumerateInstanceVersion called >>");

    // WE WILL EXPORT SPEC 1.1 FOR NOW
    uint32_t driver_api_export = VK_VERSION_1_1;

    memcpy(pApiVersion, &driver_api_export, sizeof(driver_api_export));

    return VK_SUCCESS;
}

VkResult
VulkanDispatcher::Instance::InstanceDispatchTable::vlk_trampoline_call_EnumerateInstanceExtensionProperties(
        const char *pLayerName, uint32_t *pPropertyCount, VkExtensionProperties *pProperties) {

    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_EnumerateInstanceExtensionProperties called >>");

    if (!pPropertyCount) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    uint32_t real_ext_count = 0;
    vk_context->EnumerateInstanceExtensionProperties(pLayerName,
                                                     &real_ext_count,
                                                     nullptr);

    std::vector<VkExtensionProperties> pProps;

    if (real_ext_count > 0) {
        std::vector<VkExtensionProperties> real_props(real_ext_count);
        vk_context->EnumerateInstanceExtensionProperties(pLayerName,
                                                         &real_ext_count,
                                                         real_props.data());
        pProps.insert(pProps.end(), real_props.begin(), real_props.end());
    }



    // add xcb ext
    VkExtensionProperties prop_to_copy{
        "VK_KHR_xcb_surface",
        6
    };
    pProps.emplace_back(prop_to_copy);

    uint32_t totalCount = static_cast<uint32_t>(pProps.size());

    if (!pProperties) {
        *pPropertyCount = totalCount;
        return VK_SUCCESS;
    }

    uint32_t countToCopy = std::min(*pPropertyCount, totalCount);
    if (pProperties && countToCopy > 0) {
        memcpy(pProperties, pProps.data(), sizeof(VkExtensionProperties) * countToCopy);
    }

    *pPropertyCount = totalCount;

    // show instance exts ->
    for (auto prop : pProps) {
        Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                     "enabled instance extension: ", prop.extensionName);
    }

    return (countToCopy < totalCount) ? VK_INCOMPLETE : VK_SUCCESS;
}


VkResult
VulkanDispatcher::Instance::InstanceDispatchTable::vlk_trampoline_call_EnumerateInstanceLayerProperties(
        uint32_t *pPropertyCount, VkLayerProperties *pProperties) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_EnumerateInstanceLayerProperties called >>");

    return vk_context->EnumerateInstanceLayerProperties(pPropertyCount, pProperties);
}

PFN_vkVoidFunction
VulkanDispatcher::Instance::InstanceDispatchTable::vlk_trampoline_call_GetInstanceProcAddr(
        VkInstance instance, const char *pName) {
    std::string str(pName);

    const auto it_ins_disp = instance_disp_table.find(str);
    const auto it_pdev_disp = VulkanDispatcher::PhysicalDevice::PhysicalDeviceDispatchTable::physical_device_dispatch_table.find(str);
    const auto it_dev_disp = VulkanDispatcher::Device::DeviceDispatchTable::device_dispatch_table.find(str);

    if (it_ins_disp != instance_disp_table.end()) {
        Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                     "vlk_trampoline_call_GetInstanceProcAddr looking impl function: ",
                     pName);
        return it_ins_disp->second;
    }

    if(it_pdev_disp != VulkanDispatcher::PhysicalDevice::PhysicalDeviceDispatchTable::physical_device_dispatch_table.end()) {
        Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                     "vlk_trampoline_call_GetInstanceProcAddr looking impl function: ",
                     pName);
        return it_pdev_disp->second;
    }

    if(it_dev_disp != VulkanDispatcher::Device::DeviceDispatchTable::device_dispatch_table.end()) {
        Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                     "vlk_trampoline_call_GetInstanceProcAddr looking impl function: ",
                     pName);
        return it_dev_disp->second;
    }

    auto it_instance = vk_context->instances.find(reinterpret_cast<uint64_t>(instance));

    if (it_pdev_disp == VulkanDispatcher::PhysicalDevice::PhysicalDeviceDispatchTable::physical_device_dispatch_table.end() && it_ins_disp == instance_disp_table.end()) {
        Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                     "vlk_trampoline_call_GetInstanceProcAddr looking non impl function: ",
                     pName);

        return (PFN_vkVoidFunction)vk_context->GetInstanceProcAddr(it_instance->second->dispatch_handle, pName);
    }

    /**
     * loader will be redirected with error cannot retrieve function 'pName' in this case loader understand this that not present in the driver
     */
    return nullptr;
}

VkResult VulkanDispatcher::Instance::InstanceDispatchTable::vlk_trampoline_call_CreateXcbSurfaceKHR(
        VkInstance instance, const VkXcbSurfaceCreateInfoKHR *pCreateInfo,
        const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                 "vlk_trampoline_call_CreateXcbSurfaceKHR called >>");

    return vk_context->m_surface_object_creator.createSurface(vk_context, pCreateInfo, pSurface);
}

bool VulkanDispatcher::Instance::InstanceDispatchTable::factory() {
    auto state = false;

    state = registerTrampoline("vkCreateInstance", vlk_trampoline_call_CreateInstance);
    state = registerTrampoline("vkDestroyInstance", vlk_trampoline_call_DestroyInstance);
    state = registerTrampoline("vkEnumerateInstanceVersion",
                               vlk_trampoline_call_EnumerateInstanceVersion);
    state = registerTrampoline("vkEnumerateInstanceExtensionProperties",
                               vlk_trampoline_call_EnumerateInstanceExtensionProperties);
    state = registerTrampoline("vkEnumerateInstanceLayerProperties",
                               vlk_trampoline_call_EnumerateInstanceLayerProperties);

    state = registerTrampoline("vkCreateXcbSurfaceKHR", vlk_trampoline_call_CreateXcbSurfaceKHR);

    state = registerTrampoline("vkGetInstanceProcAddr", vlk_trampoline_call_GetInstanceProcAddr);


    return state;
}

