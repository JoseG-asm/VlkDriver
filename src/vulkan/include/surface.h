#pragma once

#include "xcb/xcb.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_xcb.h"

namespace VulkanContext {
    class Context;
}

namespace x11 {
    class Surface {
    public:

        // x11 surface ->
        VkResult createSurface(VulkanContext::Context* pContext, const VkXcbSurfaceCreateInfoKHR* pInfo, VkSurfaceKHR* pSurface);
        bool presentSupport();
    private:
        VkSurfaceKHR m_sfc_handle;
    };
};