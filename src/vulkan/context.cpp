#include "context.h"

uint64_t VulkanContext::VkInstanceObject::instance_magic = 0x0000000000000000;
uint64_t VulkanContext::VkDeviceObject::device_magic = 0x0000000000000000;

bool VulkanContext::Context::initialize() {
    Cb::Handle handle;
    const Cb g_cb{
            .close = [](void *handle) {
                if (handle) {
                    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT, "libvulkan.so closed");
                    dlclose(handle);
                }
            },
            .open = [&handle](const std::string &path) -> bool {
                handle.so = dlopen(path.c_str(), RTLD_LOCAL | RTLD_NOW);

                if (handle.so) {
                    Log::VLK_LOG(Log::Level::INFO, Log::LevelType::CONTEXT,
                                 std::string("successfully opened libvulkan.so"));
                    return true;
                } else {
                    Log::VLK_LOG(Log::Level::ERROR, Log::LevelType::CONTEXT,
                                 "failed to open libvulkan.so");
                    return false;
                }
            },
    };

    bool state{false};
    state = g_cb.open(VULKAN_PATH);

    if (state) {
        /**
         * fill the global vulkan api functions
         */
        GetInstanceProcAddr = GetFuncAddrFromHandle<PFN_vkGetInstanceProcAddr>(handle,
                                                                               "vkGetInstanceProcAddr");

        CreateInstance = GetFuncAddrFromHandle<PFN_vkCreateInstance>(handle, "vkCreateInstance");
        EnumerateInstanceLayerProperties = GetFuncAddrFromHandle<PFN_vkEnumerateInstanceLayerProperties>(
                handle, "vkEnumerateInstanceLayerProperties");
        EnumerateInstanceVersion = GetFuncAddrFromHandle<PFN_vkEnumerateInstanceVersion>(handle,
                                                                                         "vkEnumerateInstanceVersion");
        EnumerateInstanceExtensionProperties = GetFuncAddrFromHandle<PFN_vkEnumerateInstanceExtensionProperties>(
                handle, "vkEnumerateInstanceExtensionProperties");

    }

    /**
     * initilaize of dispatchables tables
     */
    bool ins_state = false, pdev_state = false, dev_state = false;

    /**
     * instance dispatch
     */
    instance_disp = std::make_unique<VulkanDispatcher::Instance::InstanceDispatchTable>(this);
    ins_state = instance_disp->factory();

    /**
     * physical device dispatch
     */
    physical_device_disp = std::make_unique<VulkanDispatcher::PhysicalDevice::PhysicalDeviceDispatchTable>(this);
    pdev_state = physical_device_disp->factory();

    /**
     * device dispatch
     */
    device_disp = std::make_unique<VulkanDispatcher::Device::DeviceDispatchTable>(this);
    dev_state = device_disp->factory();

    /**
     * define state
     */
    if ((pdev_state && ins_state) && dev_state)
    {
        state = true;
    } else
    {
        state = false;
    }

    return state;
}