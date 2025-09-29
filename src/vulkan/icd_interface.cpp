#include "icd_interface.h"

#include "context.h"

/* icd interface communication */
#if defined(__GNUC__) && __GNUC__ >= 4
#define EXPORT __attribute__((visibility("default")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590)
#define EXPORT __attribute__((visibility("default")))
#else
#define EXPORT
#endif

inline VulkanContext::Context vlk_context;


extern "C" {

EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vk_icdGetInstanceProcAddr(VkInstance instance, const char *pName) {
    if (!vlk_context.ready) {
        vlk_context.ready = vlk_context.initialize();
    }

    /**
     * IMPORTANT NOTE =>
     * device functions are returned from the 'vlk_trampoline_call_GetDeviceProcAddr'
     * hook if they are not implemented the driver will try to recover them from host
     * pointers but as the driver structure is different from the internal structure
     * where we use real and non-opaque handles end up getting SIGBUS when some function
     * is not implemented for now I am implementing until it reaches vulkan spec 1.1 so
     * if more functions are required I will have to implement them later so if you get
     * SIGBUS please opens a request to implement the corresponding x function, so thats all!!
     */
    if(vlk_context.ready) {
        return vlk_context.instance_disp->vlk_trampoline_call_GetInstanceProcAddr(instance, pName);
    }
}

EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
vk_icdGetPhysicalDeviceProcAddr(VkInstance instance, const char *pName) {
    if(vlk_context.ready)
    return vlk_context.instance_disp->vlk_trampoline_call_GetInstanceProcAddr(instance, pName);
}

EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vk_icdNegotiateLoaderICDInterfaceVersion(uint32_t *pSupportedVersion) {
    negotiate_loader_icd_interface_called = true;
    loader_interface_version = *pSupportedVersion;
    if (*pSupportedVersion > SUPPORTED_LOADER_ICD_INTERFACE_VERSION) {
        *pSupportedVersion = SUPPORTED_LOADER_ICD_INTERFACE_VERSION;
    }
    return VK_SUCCESS;
}

}