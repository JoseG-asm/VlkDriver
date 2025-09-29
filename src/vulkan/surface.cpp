#include "surface.h"
#include "context.h"

VkResult x11::Surface::createSurface(VulkanContext::Context* pContext,const VkXcbSurfaceCreateInfoKHR* pInfo, VkSurfaceKHR* pSurface) {

    auto surface_plataform = std::make_unique<VulkanContext::VkSurfaceObject<VkIcdSurfaceXcb>>();


    surface_plataform->make_surface([&](VkIcdSurfaceXcb *sfc) -> bool {
        sfc->base.platform = VK_ICD_WSI_PLATFORM_XCB;
        sfc->connection = pInfo->connection;
        sfc->window = pInfo->window;
        return true;
    });

    surface_plataform->dispatch_handle = reinterpret_cast<VkSurfaceKHR>(surface_plataform->surface);
    *pSurface = surface_plataform->dispatch_handle;
    m_sfc_handle = surface_plataform->dispatch_handle;

    auto id = pContext->m_surface_handler_id++;
    pContext->surfaces[id] = std::move(surface_plataform);

    return VK_SUCCESS;
}
