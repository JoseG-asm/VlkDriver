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

namespace VulkanDispatcher::PhysicalDevice {
    template<typename K, typename V>
    using DispatchMap_T = std::unordered_map<K, V>;

    class PhysicalDeviceDispatchTable {
    public:
        explicit PhysicalDeviceDispatchTable(VulkanContext::Context* context);

        bool factory();

        /**
        * pdev functions
        */
        static VKAPI_ATTR VkResult VKAPI_CALL
        vlk_trampoline_call_EnumeratePhysicalDevices(
                VkInstance instance,
                uint32_t* pPhysicalDeviceCount,
                VkPhysicalDevice* pPhysicalDevices);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_GetPhysicalDeviceFeatures(
                VkPhysicalDevice                            physicalDevice,
                VkPhysicalDeviceFeatures*                   pFeatures);


        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_GetPhysicalDeviceFormatProperties(
                VkPhysicalDevice                            physicalDevice,
                VkFormat                                    format,
                VkFormatProperties*                         pFormatProperties);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_GetPhysicalDeviceImageFormatProperties(
                VkPhysicalDevice                            physicalDevice,
                VkFormat                                    format,
                VkImageType                                 type,
                VkImageTiling                               tiling,
                VkImageUsageFlags                           usage,
                VkImageCreateFlags                          flags,
                VkImageFormatProperties*                    pImageFormatProperties);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_GetPhysicalDeviceProperties(
                VkPhysicalDevice                            physicalDevice,
                VkPhysicalDeviceProperties*                 pProperties);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_GetPhysicalDeviceQueueFamilyProperties(
                VkPhysicalDevice                            physicalDevice,
                uint32_t*                                   pQueueFamilyPropertyCount,
                VkQueueFamilyProperties*                    pQueueFamilyProperties);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_GetPhysicalDeviceMemoryProperties(
                VkPhysicalDevice                            physicalDevice,
                VkPhysicalDeviceMemoryProperties*           pMemoryProperties);

        static VKAPI_ATTR void VKAPI_CALL vlk_trampoline_call_GetPhysicalDeviceSparseImageFormatProperties(
                VkPhysicalDevice                            physicalDevice,
                VkFormat                                    format,
                VkImageType                                 type,
                VkSampleCountFlagBits                       samples,
                VkImageUsageFlags                           usage,
                VkImageTiling                               tiling,
                uint32_t*                                   pPropertyCount,
                VkSparseImageFormatProperties*              pProperties);

        static VKAPI_ATTR VkResult VKAPI_CALL vlk_trampoline_call_EnumerateDeviceExtensionProperties(
                VkPhysicalDevice                            physicalDevice,
                const char*                                 pLayerName,
                uint32_t*                                   pPropertyCount,
                VkExtensionProperties*                      pProperties);


        static VKAPI_ATTR VkBool32 VKAPI_CALL vlk_trampoline_call_GetPhysicalDeviceXcbPresentationSupportKHR(
                VkPhysicalDevice                            physicalDevice,
                uint32_t                                    queueFamilyIndex,
                xcb_connection_t*                           connection,
                xcb_visualid_t                              visual_id);



        /**
          * hash map of physical device functions
          */
        static DispatchMap_T<std::string, PFN_vkVoidFunction> physical_device_dispatch_table;

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

            physical_device_dispatch_table[pName] = to(func);

            const auto &iterator = physical_device_dispatch_table.find(pName);

            if (iterator != physical_device_dispatch_table.end()) {
                state = true;
            }

            return state;
        }
    };
};

